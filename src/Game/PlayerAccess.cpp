#include "Game/PlayerAccess.h"

#include <excpt.h>

namespace F4DRP::Game {
namespace {
    RE::PlayerCharacter* s_cachedPlayer = nullptr;

    __declspec(noinline) RE::PlayerCharacter* derefPlayerHandle()
    {
        auto handle = RE::PlayerCharacter::GetPlayerHandle();
        auto smart = handle.get();
        return static_cast<RE::PlayerCharacter*>(smart.get());
    }
} // namespace

RE::PlayerCharacter* getPlayerSafe() noexcept
{
    if (s_cachedPlayer != nullptr)
        return s_cachedPlayer;
    __try {
        auto* player = derefPlayerHandle();
        if (isUserspacePtr(player)) {
            s_cachedPlayer = player;
            return player;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return nullptr;
}

void invalidatePlayerCache() noexcept
{
    s_cachedPlayer = nullptr;
}
} // namespace F4DRP::Game
