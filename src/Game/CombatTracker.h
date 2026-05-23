#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include "Game/GameState.h"

namespace F4DRP::Game {
class CombatTracker
{
public:
    static CombatTracker& instance();

    void install();
    void uninstall();

    std::vector<std::string> snapshotTargetNames();
    bool anyHostile();

    void onCombatChange(std::uint32_t targetFormID, std::string name, bool entering);

private:
    CombatTracker() = default;

    std::mutex m_mtx;
    std::unordered_map<std::uint32_t, std::string> m_hostiles;
    bool m_installed = false;
};
} // namespace F4DRP::Game
