#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>

#include <F4SE/F4SE.h>
#include <RE/Fallout.h>
#include <ShlObj.h>
#include <Windows.h>

#include "Config/Settings.h"
#include "Config/Translation.h"
#include "Constants.h"
#include "Discord/Client.h"
#include "Discord/Protocol.h"
#include "Game/CombatTracker.h"
#include "Game/Diagnostics.h"
#include "Game/EventTracker.h"
#include "Game/GameState.h"
#include "Game/Location.h"
#include "Game/MainThread.h"
#include "Game/MainUpdateHook.h"
#include "Game/MenuTracker.h"
#include "Game/NgIds.h"
#include "Game/PlayerAccess.h"
#include "Game/PlayerSnapshot.h"
#include "Game/UiResolver.h"
#include "Plugin/ConflictCheck.h"
#include "Presence/Composer.h"
#include "Presence/PresenceConfig.h"
#include "Util/Logger.h"
#include "Util/PlayTime.h"

namespace {
constexpr std::chrono::milliseconds kIdleSleep{500};
constexpr int kMinUpdateIntervalMs = 1000;
constexpr std::int64_t kCaptureThrottleMs = 750;
constexpr std::int64_t kPostLoadCaptureCooldownMs = 1000;

struct Runtime
{
    F4DRP::Config::Settings settings;
    F4DRP::Config::Translation translation;
    F4DRP::Presence::PresenceConfig presenceConfig;
    F4DRP::Discord::Client discord;
    F4DRP::Util::PlayTime playTime;
    std::atomic_bool timerRunning{false};
    std::atomic_bool engineReady{false};
    std::atomic_bool gameLoading{false};
    std::atomic_bool hookInstalled{false};
    std::atomic_bool hookTidLogged{false};
    std::atomic_int64_t loadResumedAtMs{0};
    std::atomic_int64_t lastCaptureMs{0};
    std::atomic_uint64_t hookFires{0};
    std::atomic_uint64_t mainCaptures{0};
    std::mutex snapshotMutex;
    F4DRP::Game::GameState latestSnapshot;
    bool snapshotValid{false};
    std::thread timerThread;
    std::filesystem::path pluginDir;
};

Runtime& runtime()
{
    static Runtime r;
    return r;
}

std::filesystem::path documentsFallout4Path()
{
    PWSTR docsRaw = nullptr;
    if (FAILED(::SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docsRaw))) {
        return {};
    }
    std::filesystem::path docs{docsRaw};
    ::CoTaskMemFree(docsRaw);
    return docs / "My Games" / "Fallout4" / "F4SE";
}

bool tryLogAddressMap() noexcept
{
    __try
    {
        F4DRP::Game::Diagnostics::logAddressMap();
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

void logAddressMapSafe()
{
    if (!tryLogAddressMap()) {
        F4DRP_LOG_ERR("[DIAG][ADDR] logAddressMap faulted — an NG id is likely MISS in this Address Library");
    }
}

F4DRP::Game::GameState captureOnMain(std::chrono::steady_clock::time_point now)
{
    F4DRP::Game::GameState g;

    F4DRP_LOG_DBG("CAP c1: before capturePlayerSnapshot");
    const auto playerInfo = F4DRP::Game::capturePlayerSnapshot();
    F4DRP_LOG_DBG("CAP c2: after capturePlayerSnapshot valid={}, name='{}', lvl={}, hp={:.2f}, caps={}",
                  playerInfo.valid,
                  playerInfo.name,
                  playerInfo.level,
                  playerInfo.healthPct,
                  playerInfo.caps);

    F4DRP_LOG_DBG("CAP c3: before captureLocation");
    const auto loc = F4DRP::Game::captureLocation();
    F4DRP_LOG_DBG("CAP c4: after captureLocation valid={}, name='{}', ext={}", loc.valid, loc.name, loc.isExterior);

    F4DRP_LOG_DBG("CAP c5: before CombatTracker::snapshotTargetNames");
    g.combatTargetNames = F4DRP::Game::CombatTracker::instance().snapshotTargetNames();
    g.inCombat = F4DRP::Game::CombatTracker::instance().anyHostile();
    F4DRP_LOG_DBG("CAP c6: after combat inCombat={}, targets={}", g.inCombat, g.combatTargetNames.size());

    F4DRP_LOG_DBG("CAP c7: before resolveUi + MenuTracker::poll");
    auto* ui = F4DRP::Game::resolveUi();
    F4DRP::Game::MenuTracker::instance().poll(ui);
    g.menu = F4DRP::Game::MenuTracker::instance().currentMenu();
    g.menuShowsLocation = F4DRP::Game::MenuTracker::instance().showsLocation();
    g.menuModeActive = F4DRP::Game::inMenuMode();
    F4DRP_LOG_DBG("CAP c8/c9: UI={} polledMenu={} menuModeActive={}",
                  static_cast<const void*>(ui),
                  static_cast<int>(g.menu),
                  g.menuModeActive);

    if (playerInfo.valid) {
        g.playerName = playerInfo.name;
        g.level = playerInfo.level;
        g.healthPct = playerInfo.healthPct;
        g.caps = playerInfo.caps;
    }
    g.inMainMenu = playerInfo.inMainMenu;
    g.inChargen = playerInfo.inChargen;
    g.gameLoading = runtime().gameLoading.load(std::memory_order_acquire);
    if (loc.valid) {
        g.locationName = loc.name;
        g.isExterior = loc.isExterior;
    }

    auto& ev = F4DRP::Game::EventTracker::instance();
    g.lastEvent = ev.activeEvent();
    g.eventPayload = ev.activePayload();
    g.eventExpiresAt = ev.expiresAt();
    ev.clearIfExpired(now);

    g.playTimeSeconds = runtime().playTime.accumulatedSeconds(now);
    g.sessionStartUnix = runtime().playTime.sessionStartUnix();
    return g;
}

std::int64_t steadyNowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

__declspec(noinline) void captureIntoSnapshotImpl(std::chrono::steady_clock::time_point now)
{
    auto snapshot = captureOnMain(now);
    auto& rt = runtime();
    {
        std::lock_guard<std::mutex> lk(rt.snapshotMutex);
        rt.latestSnapshot = std::move(snapshot);
        rt.snapshotValid = true;
    }
    rt.mainCaptures.fetch_add(1, std::memory_order_relaxed);
}

__declspec(noinline) bool captureIntoSnapshotSafe(std::chrono::steady_clock::time_point now) noexcept
{
    __try
    {
        captureIntoSnapshotImpl(now);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

void onMainThreadTick()
{
    auto& rt = runtime();
    const auto fires = rt.hookFires.fetch_add(1, std::memory_order_relaxed) + 1;

    if (!rt.hookTidLogged.exchange(true, std::memory_order_acq_rel)) {
        const auto tid = ::GetCurrentThreadId();
        const auto mainTid = F4DRP::Game::mainThreadId();
        F4DRP_LOG_INFO("[DEBUG-tid] Main::Update hook FIRST fire: tid={} mainTid={} onMainThread={} fires={}",
                       tid,
                       mainTid,
                       tid == mainTid,
                       fires);
    }

    if (!F4DRP::Game::isOnMainThread()) {
        return;
    }
    if (!rt.engineReady.load(std::memory_order_acquire) || rt.gameLoading.load(std::memory_order_acquire)) {
        return;
    }

    const auto nowMs = steadyNowMs();
    const auto resumedMs = rt.loadResumedAtMs.load(std::memory_order_acquire);
    if (resumedMs > 0 && (nowMs - resumedMs) < kPostLoadCaptureCooldownMs) {
        return;
    }
    const auto lastMs = rt.lastCaptureMs.load(std::memory_order_acquire);
    if (lastMs != 0 && (nowMs - lastMs) < kCaptureThrottleMs) {
        return;
    }
    rt.lastCaptureMs.store(nowMs, std::memory_order_release);

    if (!captureIntoSnapshotSafe(std::chrono::steady_clock::now())) {
        F4DRP_LOG_WARN("main-thread capture caught SEH (engine returned partially-deconstructed memory)");
    }
}

__declspec(noinline) void installSinksOnMainImpl()
{
    runtime().engineReady.store(true, std::memory_order_release);
    F4DRP_LOG_INFO("engineReady=true (set before sink installs — stats path is poll-only, no sink dependency)");
    F4DRP::Game::MenuTracker::instance().install();
    F4DRP::Game::CombatTracker::instance().install();
    F4DRP_LOG_WARN(
        "skipping EventTracker::install on NG runtime — RE::TerminalHacked::GetEventSource resolves "
        "REL::ID(425579) and RE::Workshop::RegisterForItemPlaced resolves REL::ID(849008), both verified "
        "MISS on NG by tools/verify-rel-ids.ps1. Terminal/workshop events disabled in v0.3; pivot to NG IDs in v0.4.");
}

__declspec(noinline) bool installSinksOnMainSafe() noexcept
{
    __try
    {
        installSinksOnMainImpl();
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

void pushLatestSnapshot(std::chrono::steady_clock::time_point now)
{
    auto& rt = runtime();
    F4DRP::Game::GameState snap;
    bool have = false;
    {
        std::lock_guard<std::mutex> lk(rt.snapshotMutex);
        if (rt.snapshotValid) {
            snap = rt.latestSnapshot;
            have = true;
        }
    }
    if (!have) {
        return;
    }
    const auto presence = F4DRP::Presence::compose(snap, rt.settings, rt.translation, rt.presenceConfig, now);
    F4DRP::Game::Diagnostics::logCaptureDecision(snap, presence);
    rt.discord.update(presence, static_cast<int>(rt.settings.updateInterval));
}

void timerLoop()
{
    F4DRP_LOG_INFO("timer thread started");
    auto lastHeartbeat = std::chrono::steady_clock::now();
    while (runtime().timerRunning.load(std::memory_order_acquire)) {
        const auto now = std::chrono::steady_clock::now();
        const bool ready = runtime().engineReady.load(std::memory_order_acquire);
        const bool loading = runtime().gameLoading.load(std::memory_order_acquire);

        if (now - lastHeartbeat >= std::chrono::seconds{15}) {
            F4DRP_LOG_INFO("worker heartbeat: ready={} loading={} hook={} fires={} captures={} discord_ready={}",
                           ready,
                           loading,
                           runtime().hookInstalled.load(std::memory_order_acquire),
                           runtime().hookFires.load(std::memory_order_relaxed),
                           runtime().mainCaptures.load(std::memory_order_relaxed),
                           runtime().discord.isReady());
            lastHeartbeat = now;
        }

        if (!ready || loading) {
            std::this_thread::sleep_for(kIdleSleep);
            continue;
        }

        pushLatestSnapshot(now);

        const auto intervalMs =
            std::max(kMinUpdateIntervalMs, static_cast<int>(runtime().settings.updateInterval * 1000.0F));
        std::this_thread::sleep_for(std::chrono::milliseconds{intervalMs});
    }
    F4DRP_LOG_INFO("timer thread exiting");
}

void onF4SEMessage(F4SE::MessagingInterface::Message* msg)
{
    if (msg == nullptr) {
        return;
    }
    switch (msg->type) {
    case F4SE::MessagingInterface::kPostLoad:
        F4DRP_LOG_INFO("[DEBUG-tid] kPostLoad — Discord IPC handshake in flight (msgThread={})",
                       ::GetCurrentThreadId());
        break;

    case F4SE::MessagingInterface::kPostPostLoad: {
        F4DRP::Discord::PresenceState launching;
        launching.details = std::string{F4DRP::Constants::Defaults::kLaunchingGame};
        launching.largeImageKey = std::string{F4DRP::Constants::kDefaultLargeImage};
        launching.largeImageText = std::string{F4DRP::Constants::kDefaultLargeImageTxt};
        runtime().discord.update(launching, F4DRP::Constants::kUpdateIntervalDefaultSec);
        F4DRP_LOG_INFO("kPostPostLoad — Launching game frame queued (discord_ready={})", runtime().discord.isReady());
        break;
    }

    case F4SE::MessagingInterface::kInputLoaded:
        F4DRP_LOG_INFO("kInputLoaded — observed (sink install deferred to kGameDataReady)");
        break;

    case F4SE::MessagingInterface::kGameDataReady: {
        const auto interval = runtime().settings.updateInterval;
        F4DRP_LOG_INFO("kGameDataReady — installing sinks inline");
        if (installSinksOnMainSafe()) {
            F4DRP_LOG_INFO("kGameDataReady — sinks installed; engineReady=true");
        }
        else {
            F4DRP_LOG_ERR(
                "kGameDataReady — inline sink install caught SEH (engineReady was set first, timer still ticks)");
        }

        F4DRP_LOG_INFO("kGameDataReady — starting timer thread (interval={}s; first tick deferred to timer SEH-wrap)",
                       interval);
        runtime().playTime.start(std::chrono::steady_clock::now());
        if (!runtime().timerRunning.exchange(true)) {
            runtime().timerThread = std::thread{timerLoop};
        }
        break;
    }

    case F4SE::MessagingInterface::kNewGame:
    case F4SE::MessagingInterface::kPostLoadGame:
        F4DRP_LOG_INFO("kPostLoadGame/kNewGame — capture resumes on next Main::Update tick (post-load settle)");
        runtime().gameLoading.store(false, std::memory_order_release);
        runtime().lastCaptureMs.store(0, std::memory_order_release);
        runtime().loadResumedAtMs.store(steadyNowMs(), std::memory_order_release);
        runtime().playTime.start(std::chrono::steady_clock::now());
        break;

    case F4SE::MessagingInterface::kPreLoadGame:
        F4DRP_LOG_INFO("kPreLoadGame — pausing capture (save transition)");
        runtime().gameLoading.store(true, std::memory_order_release);
        F4DRP::Game::invalidatePlayerCache();
        F4DRP::Game::resetUiResolver();
        {
            std::lock_guard<std::mutex> lk(runtime().snapshotMutex);
            runtime().snapshotValid = false;
        }
        runtime().playTime.pause(std::chrono::steady_clock::now());
        break;

    default:
        break;
    }
}

std::filesystem::path computePluginDir()
{
    HMODULE thisModule = nullptr;
    ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         reinterpret_cast<LPCWSTR>(&runtime),
                         &thisModule);
    wchar_t buf[MAX_PATH]{};
    ::GetModuleFileNameW(thisModule, buf, MAX_PATH);
    return std::filesystem::path{buf}.parent_path();
}
} // namespace

extern "C" __declspec(dllexport) constinit auto F4SEPlugin_Version = []() noexcept {
    F4SE::PluginVersionData data{};
    data.PluginVersion(REL::Version{0, 3, 0, 0});
    data.PluginName(std::string{F4DRP::Constants::kPluginName});
    data.AuthorName(std::string{F4DRP::Constants::kPluginAuthor});
    data.UsesAddressLibrary(true);
    data.IsLayoutDependent(false);
    data.CompatibleVersions({REL::Version{1, 10, 163, 0},
                             REL::Version{1, 10, 984, 0},
                             REL::Version{1, 11, 169, 0},
                             REL::Version{1, 11, 191, 0}});
    return data;
}();

extern "C" __declspec(dllexport) bool F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
    F4SE::Init(a_f4se);

    auto& rt = runtime();
    rt.pluginDir = computePluginDir();

    const auto docs = documentsFallout4Path();
    F4DRP::Util::Logger::install(docs / std::string{F4DRP::Constants::kLogFileName}, false);

    F4DRP_LOG_INFO("=== {} v{:#x} loading ===", F4DRP::Constants::kPluginName, F4DRP::Constants::kPluginVersion);

    if (F4DRP::Plugin::conflictingPluginLoaded()) {
        F4DRP_LOG_WARN("conflicting plugin '{}' detected; refusing to send Discord frames. Uninstall the old mod.",
                       F4DRP::Constants::kConflictPluginDll);
        return true;
    }

    const auto iniPath = rt.pluginDir / std::string{F4DRP::Constants::kIniFileName};
    const auto txnPath = rt.pluginDir / std::string{F4DRP::Constants::kTranslationFileName};
    rt.settings = F4DRP::Config::loadOrCreate(iniPath);
    rt.translation = F4DRP::Config::Translation::loadOrEmpty(txnPath);
    rt.presenceConfig = F4DRP::Presence::loadPresenceConfig(iniPath);
    F4DRP::Util::Logger::setDebugMode(rt.settings.debugMode);
    F4DRP_LOG_INFO("settings loaded; updateInterval={}s, debug={}", rt.settings.updateInterval, rt.settings.debugMode);

    logAddressMapSafe();

    const auto appId = rt.settings.appId.empty() ? std::string{F4DRP::Constants::kDefaultAppId} : rt.settings.appId;
    rt.discord.start(appId);

    auto* mi = static_cast<const F4SE::MessagingInterface*>(a_f4se->QueryInterface(F4SE::LoadInterface::kMessaging));
    if (mi != nullptr) {
        mi->RegisterListener(onF4SEMessage);
    }
    else {
        F4DRP_LOG_ERR("F4SE MessagingInterface unavailable");
    }

    const bool hookOk = F4DRP::Game::installMainUpdateHook(&onMainThreadTick);
    rt.hookInstalled.store(hookOk, std::memory_order_release);
    F4DRP::Game::Diagnostics::logHookStatus("Main::Update",
                                            hookOk,
                                            REL::Module::get().base() + F4DRP::Game::NgIds::kMainUpdateCallSiteRVA,
                                            hookOk ? REL::ID(F4DRP::Game::NgIds::kMainUpdate).address() : 0);
    if (!hookOk) {
        F4DRP_LOG_WARN("Main::Update hook not installed — presence will not update (no per-frame main-thread tick). "
                       "If the game is unfocused, set bAlwaysActive=1 in Fallout4.ini [General].");
    }
    return true;
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_DETACH) {
        auto& rt = runtime();
        rt.timerRunning.store(false);
        if (rt.timerThread.joinable()) {
            rt.timerThread.join();
        }
        F4DRP::Game::uninstallMainUpdateHook();
        F4DRP::Game::uninstallEventTrackerSinks();
        F4DRP::Game::CombatTracker::instance().uninstall();
        F4DRP::Game::MenuTracker::instance().uninstall();
        rt.discord.stop();
    }
    return TRUE;
}
