#include <array>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "Game/MenuCatalog.h"

using namespace F4DRP::Game;

TEST_CASE("MenuCatalog: classifyMenuName maps every engine name to its kind", "[menucatalog]")
{
    for (const auto& m : kMenuCatalog) {
        INFO("engineName=" << m.engineName);
        REQUIRE(classifyMenuName(m.engineName) == m.kind);
    }
}

TEST_CASE("MenuCatalog: LoadingMenu and FaderMenu classify as LoadingMenu", "[menucatalog]")
{
    REQUIRE(classifyMenuName("LoadingMenu") == MenuKind::LoadingMenu);
    REQUIRE(classifyMenuName("FaderMenu") == MenuKind::LoadingMenu);
}

TEST_CASE("MenuCatalog: unknown or empty name classifies as None", "[menucatalog][null-empty]")
{
    REQUIRE(classifyMenuName("SomeUnknownMenu") == MenuKind::None);
    REQUIRE(classifyMenuName("") == MenuKind::None);
    REQUIRE(classifyMenuName("pipboymenu") == MenuKind::None);
}

TEST_CASE("MenuCatalog: findMenu round-trips every cataloged kind", "[menucatalog]")
{
    for (const auto& m : kMenuCatalog) {
        const auto* info = findMenu(m.kind);
        REQUIRE(info != nullptr);
        REQUIRE(info->engineName == m.engineName);
        REQUIRE(info->showsLocation == m.showsLocation);
    }
}

TEST_CASE("MenuCatalog: non-cataloged kinds return nullptr", "[menucatalog][null-empty]")
{
    REQUIRE(findMenu(MenuKind::None) == nullptr);
    REQUIRE(findMenu(MenuKind::LoadingMenu) == nullptr);
    REQUIRE(findMenu(MenuKind::MainMenu) == nullptr);
    REQUIRE(findMenu(MenuKind::Crafting) == nullptr);
}

TEST_CASE("MenuCatalog: every entry has non-empty name, string key, default label", "[menucatalog]")
{
    for (const auto& m : kMenuCatalog) {
        INFO("engineName=" << m.engineName);
        REQUIRE_FALSE(m.engineName.empty());
        REQUIRE_FALSE(m.stringKey.empty());
        REQUIRE_FALSE(m.defaultLabel.empty());
    }
}

TEST_CASE("MenuCatalog: showsLocation matches the reference matrix", "[menucatalog]")
{
    REQUIRE(menuShowsLocation(MenuKind::Dialogue));
    REQUIRE(menuShowsLocation(MenuKind::Sleep));
    REQUIRE(menuShowsLocation(MenuKind::Terminal));
    REQUIRE(menuShowsLocation(MenuKind::Barter));
    REQUIRE(menuShowsLocation(MenuKind::Workshop));
    REQUIRE(menuShowsLocation(MenuKind::Cooking));
    REQUIRE_FALSE(menuShowsLocation(MenuKind::PipBoy));
    REQUIRE_FALSE(menuShowsLocation(MenuKind::PauseMenu));
    REQUIRE_FALSE(menuShowsLocation(MenuKind::LevelUp));
    REQUIRE_FALSE(menuShowsLocation(MenuKind::Lockpicking));
    REQUIRE_FALSE(menuShowsLocation(MenuKind::Vats));
    REQUIRE_FALSE(menuShowsLocation(MenuKind::None));
}

TEST_CASE("MenuCatalog: engine names exact-match the Fallout 4 reference set", "[menucatalog]")
{
    static constexpr std::array<std::string_view, 11> kExpected{{"PauseMenu",
                                                                 "PipboyMenu",
                                                                 "LevelUpMenu",
                                                                 "DialogueMenu",
                                                                 "SleepWaitMenu",
                                                                 "LockpickingMenu",
                                                                 "TerminalMenu",
                                                                 "BarterMenu",
                                                                 "WorkshopMenu",
                                                                 "VATSMenu",
                                                                 "CookingMenu"}};
    REQUIRE(kMenuCatalog.size() == kExpected.size());
    for (std::size_t i = 0; i < kExpected.size(); ++i) {
        REQUIRE(kMenuCatalog[i].engineName == kExpected[i]);
    }
}
