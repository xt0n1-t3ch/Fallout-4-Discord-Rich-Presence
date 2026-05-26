#pragma once

#include <chrono>

#include "Config/Settings.h"
#include "Config/Translation.h"
#include "Discord/Protocol.h"
#include "Game/GameState.h"
#include "Presence/PresenceConfig.h"

namespace F4DRP::Presence {
[[nodiscard]] Discord::PresenceState compose(const Game::GameState& state,
                                             const Config::Settings& settings,
                                             const Config::Translation& translation,
                                             const PresenceConfig& config,
                                             std::chrono::steady_clock::time_point now);
}
