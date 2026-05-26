#pragma once

#include <cstdint>

namespace F4DRP::Game {
struct GameState;
}

namespace F4DRP::Discord {
struct PresenceState;
}

namespace F4DRP::Game::Diagnostics {
bool ptrInModule(const void* p) noexcept;

void logAddressMap();

void logHookStatus(const char* which, bool installed, std::uintptr_t callSite, std::uintptr_t target);

void logUiResolution(const void* global, const void* value, const char* classification);

void logCaptureDecision(const F4DRP::Game::GameState& state, const F4DRP::Discord::PresenceState& presence);
} // namespace F4DRP::Game::Diagnostics
