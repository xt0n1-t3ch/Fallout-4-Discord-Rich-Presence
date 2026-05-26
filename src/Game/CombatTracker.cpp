#include "Game/CombatTracker.h"

#include <RE/Fallout.h>

#include "Constants.h"
#include "Game/PlayerAccess.h"
#include "Util/Logger.h"

namespace F4DRP::Game {
CombatTracker& CombatTracker::instance()
{
    static CombatTracker s;
    return s;
}

void CombatTracker::install()
{
    if (m_installed)
        return;
    m_installed = true;
    F4DRP_LOG_INFO("CombatTracker installed (poll mode: IsInCombat + currentCombatTarget)");
}

void CombatTracker::uninstall()
{
    m_installed = false;
}

void CombatTracker::onCombatChange(std::uint32_t, std::string, bool) {}

std::vector<std::string> CombatTracker::snapshotTargetNames()
{
    std::vector<std::string> out;
    auto* player = getPlayerSafe();
    if (player == nullptr) {
        return out;
    }
    if (!player->IsInCombat()) {
        return out;
    }
    auto targetSmart = player->currentCombatTarget.get();
    auto* target = targetSmart.get();
    if (target == nullptr) {
        return out;
    }
    auto* base = target->GetObjectReference();
    auto* npc = base != nullptr ? base->As<RE::TESNPC>() : nullptr;
    if (const char* n = npc != nullptr ? npc->GetFullName() : nullptr; n != nullptr && n[0] != '\0') {
        out.emplace_back(n);
    }
    return out;
}

bool CombatTracker::anyHostile()
{
    auto* player = getPlayerSafe();
    return player != nullptr && player->IsInCombat();
}
} // namespace F4DRP::Game
