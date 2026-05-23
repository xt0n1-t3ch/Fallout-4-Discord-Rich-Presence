#include "Game/CombatTracker.h"

#include <RE/Fallout.h>

#include "Constants.h"
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
    auto* player = RE::PlayerCharacter::GetSingleton();
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
    if (const char* n = target->GetDisplayFullName(); n != nullptr && n[0] != '\0') {
        out.emplace_back(n);
    }
    return out;
}

bool CombatTracker::anyHostile()
{
    auto* player = RE::PlayerCharacter::GetSingleton();
    return player != nullptr && player->IsInCombat();
}
} // namespace F4DRP::Game
