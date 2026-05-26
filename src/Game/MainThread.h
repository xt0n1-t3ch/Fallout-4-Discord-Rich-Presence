#pragma once

#include <cstdint>

namespace F4DRP::Game {
std::uint32_t mainThreadId() noexcept;
bool isOnMainThread() noexcept;
bool inMenuMode() noexcept;
} // namespace F4DRP::Game
