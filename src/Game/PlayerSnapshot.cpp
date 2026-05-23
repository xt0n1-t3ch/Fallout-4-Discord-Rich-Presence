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
        auto* boundCaps = caps->As<RE::TESBoundObject>();
        if (boundCaps == nullptr)
            return 0;
        return static_cast<std::int64_t>(player->GetItemCount(boundCaps));
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
    const float hpMax = player->GetPermanentActorValue(RE::ActorValue::kHealth);
    const float hpNow = player->GetActorValue(RE::ActorValue::kHealth);
    r.healthPct = hpMax > 0.0F ? std::clamp(hpNow / hpMax, 0.0F, 1.0F) : 0.0F;
    r.caps = countCaps(player);
    r.valid = true;
    return r;
}
} // namespace F4DRP::Game
