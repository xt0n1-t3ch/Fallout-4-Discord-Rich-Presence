#include "Game/MainUpdateHook.h"

#include <atomic>
#include <cstdint>
#include <cstring>

#include <F4SE/F4SE.h>
#include <REL/Relocation.h>
#include <Windows.h>

#include "Game/NgIds.h"
#include "Util/Logger.h"

namespace F4DRP::Game {
namespace {
    constexpr std::uint32_t kSupportedVersion = REL::Version(1, 11, 191, 0).pack();

    using MainUpdateFn = void (*)(void*);

    MainUpdateFn s_realUpdate = nullptr;
    MainThreadTickFn s_onTick = nullptr;
    std::uintptr_t s_callSite = 0;
    std::uint8_t s_origBytes[5]{};
    std::atomic_bool s_installed{false};

    void mainUpdateThunk(void* a_arg)
    {
        const auto cb = s_onTick;
        if (cb != nullptr) {
            cb();
        }
        if (s_realUpdate != nullptr) {
            s_realUpdate(a_arg);
        }
    }
} // namespace

bool installMainUpdateHook(MainThreadTickFn a_onTick) noexcept
{
    bool expected = false;
    if (!s_installed.compare_exchange_strong(expected, true)) {
        return true;
    }

    if (REL::Module::get().version().pack() != kSupportedVersion) {
        F4DRP_LOG_WARN(
            "MainUpdateHook: runtime is not 1.11.191; call-site hook disabled (no per-frame main-thread tick)");
        s_installed.store(false);
        return false;
    }

    const auto base = REL::Module::get().base();
    const auto mainUpdateAddr = REL::ID(NgIds::kMainUpdate).address();
    const auto callSite = base + NgIds::kMainUpdateCallSiteRVA;

    const auto* bytes = reinterpret_cast<const std::uint8_t*>(callSite);
    if (bytes[0] != 0xE8) {
        F4DRP_LOG_ERR("MainUpdateHook abort: call site 0x{:x} is not a CALL (0x{:02x})", callSite, bytes[0]);
        s_installed.store(false);
        return false;
    }
    const auto rel = *reinterpret_cast<const std::int32_t*>(callSite + 1);
    const auto existingTarget = callSite + 5 + static_cast<std::int64_t>(rel);
    if (existingTarget != mainUpdateAddr) {
        F4DRP_LOG_ERR("MainUpdateHook abort: call site target 0x{:x} != Main::Update 0x{:x} (layout mismatch)",
                      existingTarget,
                      mainUpdateAddr);
        s_installed.store(false);
        return false;
    }

    std::memcpy(s_origBytes, bytes, sizeof(s_origBytes));
    s_callSite = callSite;
    s_onTick = a_onTick;

    F4SE::AllocTrampoline(64);
    s_realUpdate = reinterpret_cast<MainUpdateFn>(
        F4SE::GetTrampoline().write_call<5>(callSite, reinterpret_cast<std::uintptr_t>(&mainUpdateThunk)));

    F4DRP_LOG_INFO("MainUpdateHook installed: callSite=0x{:x} Main::Update=0x{:x} (REL::ID {})",
                   callSite,
                   mainUpdateAddr,
                   NgIds::kMainUpdate);
    return true;
}

void uninstallMainUpdateHook() noexcept
{
    if (!s_installed.load() || s_callSite == 0) {
        return;
    }
    DWORD oldProtect = 0;
    auto* p = reinterpret_cast<void*>(s_callSite);
    if (::VirtualProtect(p, sizeof(s_origBytes), PAGE_EXECUTE_READWRITE, &oldProtect) != 0) {
        std::memcpy(p, s_origBytes, sizeof(s_origBytes));
        ::VirtualProtect(p, sizeof(s_origBytes), oldProtect, &oldProtect);
    }
    s_installed.store(false);
}
} // namespace F4DRP::Game
