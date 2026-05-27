#include "Game/PlayerSnapshot.h"

#include <algorithm>
#include <cstdint>

#include <RE/Fallout.h>
#include <excpt.h>

#include "Constants.h"
#include "Game/PlayerAccess.h"
#include "Util/Logger.h"

namespace F4DRP::Game {
namespace {
    const char* baseNameNoAlloc(RE::PlayerCharacter* player)
    {
        auto* base = player->GetObjectReference();
        if (base == nullptr) {
            return nullptr;
        }
        auto* npc = base->As<RE::TESNPC>();
        return npc != nullptr ? npc->GetFullName() : nullptr;
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
        if (hpMax <= 0.0F)
            return 0.0F;
        if (hpMax - hpNow < 0.50F)
            return 1.0F;
        return std::clamp(hpNow / hpMax, 0.0F, 1.0F);
    }

    constexpr std::uint32_t kMaxInventoryEntries = 8192;
    constexpr int kMaxStacksPerItem = 256;

    std::int64_t capsCount(RE::PlayerCharacter* player)
    {
        auto* inv = player->inventoryList;
        if (!isUserspacePtr(inv)) {
            return 0;
        }
        const auto entries = inv->data.size();
        if (entries == 0 || entries > kMaxInventoryEntries) {
            return 0;
        }
        std::int64_t total = 0;
        for (std::uint32_t i = 0; i < entries; ++i) {
            auto* obj = inv->data[i].object;
            if (!isUserspacePtr(obj) || obj->GetFormID() != Constants::kCapsFormID) {
                continue;
            }
            int stacks = 0;
            for (auto* st = inv->data[i].stackData.get(); isUserspacePtr(st) && stacks < kMaxStacksPerItem;
                 st = st->nextStack.get(), ++stacks)
            {
                total += st->count;
            }
        }
        return total;
    }
} // namespace

PlayerSnapshotResult capturePlayerSnapshot()
{
    PlayerSnapshotResult r;
    F4DRP_LOG_DBG("PS p1: enter capturePlayerSnapshot");

    auto* player = getPlayerSafe();
    F4DRP_LOG_DBG("PS p2: getPlayerSafe -> {}", static_cast<const void*>(player));

    if (player == nullptr) {
        F4DRP_LOG_DBG("PS p3: getPlayerSafe returned null; skip capture");
        return r;
    }

    F4DRP_LOG_DBG("PS p4: about to read parentCell");
    r.inMainMenu = player->parentCell == nullptr;
    F4DRP_LOG_DBG("PS p5: parentCell read OK, inMainMenu={}", r.inMainMenu);
    r.inChargen = player->byCharGenFlag != 0;
    F4DRP_LOG_DBG("PS p6: byCharGenFlag read OK, inChargen={}", r.inChargen);
    if (const char* name = baseNameNoAlloc(player); name != nullptr) {
        r.name = name;
    }
    F4DRP_LOG_DBG("PS p7: base name (no scrap alloc) OK, name='{}'", r.name);
    r.level = player->GetLevel();
    F4DRP_LOG_DBG("PS p8: GetLevel OK, level={}", r.level);
    r.healthPct = healthPct(player);
    F4DRP_LOG_DBG("PS p9: healthPct OK, hp={:.2f}", r.healthPct);
    r.caps = capsCount(player);
    F4DRP_LOG_DBG("PS p10: caps (guarded inventory walk, no GetFormByID) OK, caps={}", r.caps);
    r.valid = true;
    return r;
}
} // namespace F4DRP::Game
