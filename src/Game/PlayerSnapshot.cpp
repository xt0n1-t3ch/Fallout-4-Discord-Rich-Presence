#include "Game/PlayerSnapshot.h"

#include <algorithm>

#include <RE/Fallout.h>

#include "Util/Logger.h"

namespace F4DRP::Game {
namespace {
    constexpr std::uint32_t kCapsFormID = 0x0000000F;

    std::int64_t countCaps(RE::PlayerCharacter* player)
    {
        if (player == nullptr)
            return 0;
        auto* caps = RE::TESForm::GetFormByID(kCapsFormID);
        if (caps == nullptr)
            return 0;
        std::uint32_t count = 0;
        if (player->GetItemCount(count, caps, false)) {
            return static_cast<std::int64_t>(count);
        }
        return 0;
    }

    float healthPct(RE::PlayerCharacter* player)
    {
        if (player == nullptr)
            return 0.0F;
        auto* avs = RE::ActorValue::GetSingleton();
        if (avs == nullptr || avs->health == nullptr)
            return 0.0F;
        const float hpMax = player->GetPermanentActorValue(*avs->health);
        const float hpNow = player->GetActorValue(*avs->health);
        return hpMax > 0.0F ? std::clamp(hpNow / hpMax, 0.0F, 1.0F) : 0.0F;
    }
} // namespace

PlayerSnapshotResult capturePlayerSnapshot()
{
    PlayerSnapshotResult r;
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (player == nullptr) {
        return r;
    }
    if (const char* name = player->GetDisplayFullName(); name != nullptr) {
        r.name = name;
    }
    r.level = player->GetLevel();
    r.healthPct = healthPct(player);
    r.caps = countCaps(player);
    r.valid = true;
    return r;
}
} // namespace F4DRP::Game
