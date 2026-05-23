#pragma once

#include <chrono>

#include "Config/Settings.h"
#include "Config/Translation.h"
#include "Discord/Protocol.h"
#include "Game/GameState.h"

namespace F4DRP::Mapper {
Discord::PresenceState mapGameStateToPresence(const Game::GameState& g,
                                              const Config::Settings& s,
                                              const Config::Translation& t,
                                              std::chrono::steady_clock::time_point now);
}
