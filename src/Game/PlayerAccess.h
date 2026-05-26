#pragma once

#include <cstdint>

#include <RE/Fallout.h>

namespace F4DRP::Game {
inline bool isUserspacePtr(const void* p) noexcept
{
    const auto v = reinterpret_cast<std::uintptr_t>(p);
    return v >= 0x0000'0000'0001'0000ULL && v < 0x0000'8000'0000'0000ULL;
}

RE::PlayerCharacter* getPlayerSafe() noexcept;
void invalidatePlayerCache() noexcept;
} // namespace F4DRP::Game
