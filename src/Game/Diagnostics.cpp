#include "Game/Diagnostics.h"

#include <array>

#include <RE/Fallout.h>
#include <REL/Relocation.h>
#include <Windows.h>

#include "Discord/Protocol.h"
#include "Game/GameState.h"
#include "Game/NgIds.h"
#include "Util/Logger.h"

namespace F4DRP::Game::Diagnostics {
namespace {
    struct NamedId
    {
        const char* name;
        std::uint64_t id;
    };

    constexpr std::array<NamedId, 5> kResolvableIds{{
        {"Main::Update", NgIds::kMainUpdate},
        {"Main::GetSingleton", NgIds::kMainGetSingleton},
        {"GetPlayerHandle", NgIds::kGetPlayerHandle},
        {"UI::GetMenuMapRWLock", NgIds::kMenuMapRWLock},
        {"g_UI", NgIds::kUISingleton},
    }};

    std::uintptr_t moduleEnd(std::uintptr_t base)
    {
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        return base + nt->OptionalHeader.SizeOfImage;
    }
} // namespace

bool ptrInModule(const void* p) noexcept
{
    const auto base = REL::Module::get().base();
    const auto addr = reinterpret_cast<std::uintptr_t>(p);
    return addr >= base && addr < moduleEnd(base);
}

void logAddressMap()
{
    const auto base = REL::Module::get().base();
    const auto end = moduleEnd(base);
    F4DRP_LOG_INFO("[DIAG][ADDR] module base=0x{:x} end=0x{:x} size=0x{:x}", base, end, end - base);

    for (const auto& entry : kResolvableIds) {
        const auto addr = REL::ID(entry.id).address();
        F4DRP_LOG_INFO("[DIAG][ADDR] {} id={} rva=0x{:x} addr=0x{:x}", entry.name, entry.id, addr - base, addr);
    }

    const auto uiGlobal = REL::ID(NgIds::kUISingleton).address();
    const auto* value = *reinterpret_cast<void* const*>(uiGlobal);
    const auto valueAddr = reinterpret_cast<std::uintptr_t>(value);
    const char* classification = "null";
    if (value != nullptr) {
        classification = (valueAddr >= base && valueAddr < end) ? "in-module(SUSPECT)" : "heap(OK)";
    }
    logUiResolution(reinterpret_cast<const void*>(uiGlobal), value, classification);
}

void logHookStatus(const char* which, bool installed, std::uintptr_t callSite, std::uintptr_t target)
{
    F4DRP_LOG_INFO("[DIAG][HOOK] {} installed={} callSite=0x{:x} target=0x{:x}", which, installed, callSite, target);
}

void logUiResolution(const void* global, const void* value, const char* classification)
{
    F4DRP_LOG_INFO("[DIAG][ADDR] g_UI global=0x{:x} *global=0x{:x} class={}",
                   reinterpret_cast<std::uintptr_t>(global),
                   reinterpret_cast<std::uintptr_t>(value),
                   classification);
}

void logCaptureDecision(const F4DRP::Game::GameState& state, const F4DRP::Discord::PresenceState& presence)
{
    F4DRP_LOG_DBG("[DIAG][STATE] menu={} menuMode={} combat={} targets={} loc='{}' mainMenu={} chargen={} loading={} "
                  "| details='{}' state='{}'",
                  static_cast<int>(state.menu),
                  state.menuModeActive,
                  state.inCombat,
                  state.combatTargetNames.size(),
                  state.locationName,
                  state.inMainMenu,
                  state.inChargen,
                  state.gameLoading,
                  presence.details,
                  presence.state);
}
} // namespace F4DRP::Game::Diagnostics
