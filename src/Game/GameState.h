#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace F4DRP::Game {
enum class MenuKind : std::uint8_t
{
    None,
    MainMenu,
    PauseMenu,
    PipBoy,
    Sleep,
    LevelUp,
    Dialogue,
    Lockpicking,
    Terminal,
    Barter,
    Crafting,
    Vats,
    Cooking,
    Workshop,
    LoadingMenu
};

enum class EventKind : std::uint8_t
{
    None,
    HackedTerminal,
    BuiltWorkshopObject
};

struct GameState
{
    MenuKind menu = MenuKind::None;
    std::string playerName;
    std::uint32_t level = 0;
    float healthPct = 0.0F;
    std::int64_t caps = 0;
    std::string locationName;
    bool isExterior = true;
    std::vector<std::string> combatTargetNames;
    EventKind lastEvent = EventKind::None;
    std::chrono::steady_clock::time_point eventExpiresAt{};
    std::uint64_t playTimeSeconds = 0;
    std::int64_t sessionStartUnix = 0;
    bool inCombat = false;
    bool newGameStarted = false;
};
} // namespace F4DRP::Game
