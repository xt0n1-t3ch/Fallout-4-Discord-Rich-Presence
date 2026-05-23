#pragma once

#include "Game/GameState.h"

namespace F4DRP::Game {
struct PlayerSnapshotResult
{
    std::string name;
    std::uint32_t level = 0;
    float healthPct = 0.0F;
    std::int64_t caps = 0;
    bool valid = false;
};

PlayerSnapshotResult capturePlayerSnapshot();
} // namespace F4DRP::Game
