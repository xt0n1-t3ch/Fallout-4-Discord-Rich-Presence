#pragma once

#include <array>
#include <string_view>

#include "Constants.h"
#include "Game/GameState.h"

namespace F4DRP::Game {
struct MenuInfo
{
    std::string_view engineName;
    MenuKind kind;
    std::string_view stringKey;
    std::string_view defaultLabel;
    bool showsLocation;
};

inline constexpr std::array<MenuInfo, 11> kMenuCatalog{{
    {"PauseMenu", MenuKind::PauseMenu, Constants::StringKey::kPauseMenu, Constants::Defaults::kPauseMenu, false},
    {"PipboyMenu", MenuKind::PipBoy, Constants::StringKey::kPipboyMenu, Constants::Defaults::kPipboyMenu, false},
    {"LevelUpMenu", MenuKind::LevelUp, Constants::StringKey::kLevelUpMenu, Constants::Defaults::kLevelUpMenu, false},
    {"DialogueMenu", MenuKind::Dialogue, Constants::StringKey::kDialogueMenu, Constants::Defaults::kDialogueMenu, true},
    {"SleepWaitMenu", MenuKind::Sleep, Constants::StringKey::kSleepWaitMenu, Constants::Defaults::kSleepWaitMenu, true},
    {"LockpickingMenu",
     MenuKind::Lockpicking,
     Constants::StringKey::kLockpickingMenu,
     Constants::Defaults::kLockpickingMenu,
     false},
    {"TerminalMenu", MenuKind::Terminal, Constants::StringKey::kTerminalMenu, Constants::Defaults::kTerminalMenu, true},
    {"BarterMenu", MenuKind::Barter, Constants::StringKey::kBarterMenu, Constants::Defaults::kBarterMenu, true},
    {"WorkshopMenu", MenuKind::Workshop, Constants::StringKey::kWorkshopMenu, Constants::Defaults::kWorkshopMenu, true},
    {"VATSMenu", MenuKind::Vats, Constants::StringKey::kVatsMenu, Constants::Defaults::kVatsMenu, false},
    {"CookingMenu", MenuKind::Cooking, Constants::StringKey::kCookingMenu, Constants::Defaults::kCookingMenu, true},
}};

inline constexpr std::string_view kLoadingMenuName = "LoadingMenu";
inline constexpr std::string_view kFaderMenuName = "FaderMenu";

[[nodiscard]] inline constexpr const MenuInfo* findMenu(MenuKind a_kind) noexcept
{
    for (const auto& m : kMenuCatalog) {
        if (m.kind == a_kind) {
            return &m;
        }
    }
    return nullptr;
}

[[nodiscard]] inline constexpr MenuKind classifyMenuName(std::string_view a_engineName) noexcept
{
    for (const auto& m : kMenuCatalog) {
        if (m.engineName == a_engineName) {
            return m.kind;
        }
    }
    if (a_engineName == kLoadingMenuName || a_engineName == kFaderMenuName) {
        return MenuKind::LoadingMenu;
    }
    return MenuKind::None;
}

[[nodiscard]] inline constexpr bool menuShowsLocation(MenuKind a_kind) noexcept
{
    const auto* m = findMenu(a_kind);
    return m != nullptr && m->showsLocation;
}
} // namespace F4DRP::Game
