#include "Game/MainThread.h"

#include <RE/Fallout.h>
#include <Windows.h>
#include <excpt.h>

#include "Game/PlayerAccess.h"

namespace F4DRP::Game {
namespace {
    __declspec(noinline) RE::Main* mainSafe() noexcept
    {
        __try
        {
            auto* m = RE::Main::GetSingleton();
            return isUserspacePtr(m) ? m : nullptr;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }
    }

    __declspec(noinline) std::uint32_t readThreadId(RE::Main* a_main) noexcept
    {
        __try
        {
            return a_main->threadID;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }

    __declspec(noinline) bool readInMenuMode(RE::Main* a_main) noexcept
    {
        __try
        {
            return a_main->inMenuMode;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
} // namespace

std::uint32_t mainThreadId() noexcept
{
    auto* m = mainSafe();
    return m == nullptr ? 0U : readThreadId(m);
}

bool isOnMainThread() noexcept
{
    const auto mt = mainThreadId();
    return mt != 0U && ::GetCurrentThreadId() == mt;
}

bool inMenuMode() noexcept
{
    auto* m = mainSafe();
    return m != nullptr && readInMenuMode(m);
}
} // namespace F4DRP::Game
