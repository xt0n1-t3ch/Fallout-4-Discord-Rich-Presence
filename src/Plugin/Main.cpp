#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
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
#include "Game/EventTracker.h"
#include "Game/GameState.h"
#include "Game/Location.h"
#include "Game/MenuTracker.h"
#include "Game/PlayerSnapshot.h"
#include "Mapper/Mapper.h"
#include "Plugin/ConflictCheck.h"
#include "Util/Logger.h"
#include "Util/PlayTime.h"

namespace {
struct Runtime
{
    F4DRP::Config::Settings settings;
    F4DRP::Config::Translation translation;
    F4DRP::Discord::Client discord;
    F4DRP::Util::PlayTime playTime;
    std::atomic_bool pollerRunning{false};
    std::thread pollerThread;
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

F4DRP::Game::GameState capture(std::chrono::steady_clock::time_point now)
{
    F4DRP::Game::GameState g;
    auto playerInfo = F4DRP::Game::capturePlayerSnapshot();
    auto loc = F4DRP::Game::captureLocation();
    g.menu = F4DRP::Game::MenuTracker::instance().currentMenu();
    g.newGameStarted = F4DRP::Game::MenuTracker::instance().newGameStarted();
    if (playerInfo.valid) {
        g.playerName = std::move(playerInfo.name);
        g.level = playerInfo.level;
        g.healthPct = playerInfo.healthPct;
        g.caps = playerInfo.caps;
    }
    if (loc.valid) {
        g.locationName = std::move(loc.name);
        g.isExterior = loc.isExterior;
    }
    auto& combat = F4DRP::Game::CombatTracker::instance();
    g.combatTargetNames = combat.snapshotTargetNames();
    g.inCombat = !g.combatTargetNames.empty();

    auto& ev = F4DRP::Game::EventTracker::instance();
    g.lastEvent = ev.activeEvent();
    g.eventExpiresAt = ev.expiresAt();
    ev.clearIfExpired(now);

    g.playTimeSeconds = runtime().playTime.accumulatedSeconds(now);
    g.sessionStartUnix = runtime().playTime.sessionStartUnix();
    return g;
}

void pollerLoop()
{
    while (runtime().pollerRunning.load(std::memory_order_acquire)) {
        const auto now = std::chrono::steady_clock::now();
        const auto snapshot = capture(now);
        const auto presence =
            F4DRP::Mapper::mapGameStateToPresence(snapshot, runtime().settings, runtime().translation, now);
        runtime().discord.update(presence, static_cast<int>(runtime().settings.updateInterval));
        std::this_thread::sleep_for(std::chrono::milliseconds{500});
    }
}

void onF4SEMessage(F4SE::MessagingInterface::Message* msg)
{
    if (msg == nullptr) {
        return;
    }
    switch (msg->type) {
    case F4SE::MessagingInterface::kGameDataReady:
    case F4SE::MessagingInterface::kPostLoad:
    case F4SE::MessagingInterface::kPostPostLoad:
    case F4SE::MessagingInterface::kNewGame:
    case F4SE::MessagingInterface::kPostLoadGame:
        runtime().playTime.start(std::chrono::steady_clock::now());
        F4DRP::Game::MenuTracker::instance().install();
        F4DRP::Game::CombatTracker::instance().install();
        if (!runtime().pollerRunning.exchange(true)) {
            runtime().pollerThread = std::thread{pollerLoop};
        }
        break;
    case F4SE::MessagingInterface::kPreLoadGame:
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
    data.PluginVersion(REL::Version{0, 1, 0, 0});
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
    F4DRP::Util::Logger::setDebugMode(rt.settings.debugMode);
    F4DRP_LOG_INFO("settings loaded; updateInterval={}s, debug={}", rt.settings.updateInterval, rt.settings.debugMode);

    const auto appId = rt.settings.appId.empty() ? std::string{F4DRP::Constants::kDefaultAppId} : rt.settings.appId;
    rt.discord.start(appId);

    auto* mi = static_cast<const F4SE::MessagingInterface*>(a_f4se->QueryInterface(F4SE::LoadInterface::kMessaging));
    if (mi != nullptr) {
        mi->RegisterListener(onF4SEMessage);
    }
    else {
        F4DRP_LOG_ERR("F4SE MessagingInterface unavailable");
    }
    return true;
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_DETACH) {
        auto& rt = runtime();
        rt.pollerRunning.store(false);
        if (rt.pollerThread.joinable()) {
            rt.pollerThread.join();
        }
        F4DRP::Game::CombatTracker::instance().uninstall();
        F4DRP::Game::MenuTracker::instance().uninstall();
        rt.discord.stop();
    }
    return TRUE;
}
