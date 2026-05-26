#include "Game/UiResolver.h"

#include <atomic>
#include <cstdint>

#include <RE/Fallout.h>
#include <excpt.h>

#include "Game/NgIds.h"
#include "Game/PlayerAccess.h"
#include "Util/Logger.h"

namespace F4DRP::Game {
namespace {
    std::atomic<std::uintptr_t> s_confirmedUi{0};
    std::atomic_bool s_probeLogged{false};

    __declspec(noinline) bool isUiObjectSafe(RE::UI* ui) noexcept
    {
        __try
        {
            if (!isUserspacePtr(ui)) {
                return false;
            }
            const auto vtable = *reinterpret_cast<const std::uintptr_t*>(ui);
            const auto expected = REL::Module::get().base() + NgIds::kUiVtableRva;
            return vtable == expected;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    __declspec(noinline) RE::UI* derefGlobal(std::uintptr_t addr) noexcept
    {
        __try
        {
            return *reinterpret_cast<RE::UI* const*>(addr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }
    }
} // namespace

RE::UI* resolveUi() noexcept
{
    const auto confirmed = s_confirmedUi.load(std::memory_order_acquire);
    if (confirmed != 0) {
        auto* ui = reinterpret_cast<RE::UI*>(confirmed);
        if (isUiObjectSafe(ui)) {
            return ui;
        }
        s_confirmedUi.store(0, std::memory_order_release);
    }

    for (const auto id : NgIds::kUISingletonCandidates) {
        auto* ui = derefGlobal(REL::ID(id).address());
        if (isUiObjectSafe(ui)) {
            s_confirmedUi.store(reinterpret_cast<std::uintptr_t>(ui), std::memory_order_release);
            F4DRP_LOG_INFO("[DIAG][UI] CONFIRMED g_UI id={} ui=0x{:x} (vtable matches RE::UI)",
                           id,
                           reinterpret_cast<std::uintptr_t>(ui));
            return ui;
        }
    }

    if (!s_probeLogged.exchange(true, std::memory_order_acq_rel)) {
        F4DRP_LOG_WARN(
            "[DIAG][UI] no g_UI candidate matched the RE::UI vtable — menu detail disabled, generic label only");
    }
    return nullptr;
}

void resetUiResolver() noexcept
{
    s_confirmedUi.store(0, std::memory_order_release);
}
} // namespace F4DRP::Game
