#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace F4DRP::Game::NgIds {

inline constexpr std::uint64_t kMainUpdate = 2228917;
inline constexpr std::size_t kMainUpdateCallSiteRVA = 0x00c3109b;

inline constexpr std::uint64_t kMainGetSingleton = 2698043;
inline constexpr std::uint64_t kGetPlayerHandle = 2698072;
inline constexpr std::uint64_t kMenuMapRWLock = 2707105;

inline constexpr std::uint64_t kUISingleton = 4796377;

inline constexpr std::array<std::uint64_t, 6> kUISingletonCandidates{
    4796377, 4796314, 4799238, 2698073, 4803789, 4796420};

inline constexpr std::size_t kUiVtableRva = 0x027159d8;

inline constexpr std::size_t kUiMenuMapOffset = 0x1A8;
inline constexpr std::size_t kUiMenuModeOffset = 0x1E0;
inline constexpr std::size_t kMainThreadIdOffset = 0x40;
inline constexpr std::size_t kMainInMenuModeOffset = 0x1D0;
} // namespace F4DRP::Game::NgIds
