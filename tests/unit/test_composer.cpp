#include <chrono>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "Constants.h"
#include "Game/GameState.h"
#include "Presence/Composer.h"
#include "Presence/PresenceConfig.h"

using F4DRP::Config::Settings;
using F4DRP::Config::Translation;
using F4DRP::Game::GameState;
using F4DRP::Game::MenuKind;
using F4DRP::Presence::compose;
using F4DRP::Presence::PresenceConfig;

namespace {
const std::string kDot = "\xe2\x80\xa2";

GameState base()
{
    GameState g;
    g.playerName = "Sole Survivor";
    g.level = 12;
    g.healthPct = 0.85F;
    g.caps = 1234;
    g.locationName = "Sanctuary Hills";
    g.sessionStartUnix = 1716595200;
    return g;
}
} // namespace

TEST_CASE("Composer: Iconic details layout with bullet separator", "[composer]")
{
    auto g = base();
    g.menu = MenuKind::PipBoy;
    Settings s;
    s.showCaps = true;
    Translation t;
    PresenceConfig cfg;
    const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());

    REQUIRE(p.details == "Sole Survivor " + kDot + " LVL 12 " + kDot + " 85% HP " + kDot + " 1234 caps");
    REQUIRE(p.state == "In Pipboy Menu");
    REQUIRE(p.largeImageKey == "fo4-big");
    REQUIRE(p.startTimestampUnix == 1716595200);
}

TEST_CASE("Composer: field toggles drop segments cleanly", "[composer]")
{
    auto g = base();
    Settings s;
    s.showName = false;
    s.showCaps = false;
    Translation t;
    PresenceConfig cfg;
    const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
    REQUIRE(p.details == "LVL 12 " + kDot + " 85% HP");
}

TEST_CASE("Composer: combat shows enemy and location, icon applied", "[composer]")
{
    auto g = base();
    g.inCombat = true;
    g.combatTargetNames = {"Raider"};
    Translation t;
    PresenceConfig cfg;
    cfg.iconCombat = "icon_combat";

    SECTION("simplified uses bullet")
    {
        Settings s;
        s.simplifiedStatus = true;
        const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
        REQUIRE(p.state == "Fighting Raider " + kDot + " Sanctuary Hills");
        REQUIRE(p.smallImageKey == "icon_combat");
    }
    SECTION("verbose uses in")
    {
        Settings s;
        s.simplifiedStatus = false;
        const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
        REQUIRE(p.state == "Fighting Raider in Sanctuary Hills");
    }
}

TEST_CASE("Composer: exploring fallback", "[composer]")
{
    auto g = base();
    Settings s;
    s.simplifiedStatus = true;
    Translation t;
    PresenceConfig cfg;
    const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
    REQUIRE(p.state == "Exploring " + kDot + " Sanctuary Hills");
}

TEST_CASE("Composer: generic in-menu when menuMode but no specific menu", "[composer]")
{
    auto g = base();
    g.menu = MenuKind::None;
    g.menuModeActive = true;
    Settings s;
    Translation t;
    PresenceConfig cfg;
    const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
    REQUIRE(p.state == "In a menu");
}

TEST_CASE("Composer: lifecycle states use details only", "[composer]")
{
    Translation t;
    PresenceConfig cfg;
    Settings s;

    SECTION("main menu")
    {
        GameState g;
        g.inMainMenu = true;
        const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
        REQUIRE(p.details == "In Main menu");
        REQUIRE(p.state.empty());
    }
    SECTION("chargen")
    {
        GameState g;
        g.inChargen = true;
        const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
        REQUIRE(p.details == "Started a new game");
    }
    SECTION("loading")
    {
        GameState g;
        g.gameLoading = true;
        const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
        REQUIRE(p.details == "Launching game");
    }
}

TEST_CASE("Composer: event status overrides combat for its window", "[composer]")
{
    auto g = base();
    const auto now = std::chrono::steady_clock::now();
    g.inCombat = true;
    g.combatTargetNames = {"Raider"};
    g.lastEvent = F4DRP::Game::EventKind::HackedTerminal;
    g.eventPayload = "Terminal";
    g.eventExpiresAt = now + std::chrono::seconds(5);
    Settings s;
    s.simplifiedStatus = true;
    s.showEventStatuses = true;
    Translation t;
    PresenceConfig cfg;
    const auto p = compose(g, s, t, cfg, now);
    REQUIRE(p.state == "Hacked Terminal " + kDot + " Sanctuary Hills");
}

TEST_CASE("Composer: custom overrides and swap", "[composer]")
{
    auto g = base();
    g.menu = MenuKind::PipBoy;
    Translation t;
    PresenceConfig cfg;

    SECTION("custom details/state win")
    {
        Settings s;
        s.customDetails = "Custom D";
        s.customState = "Custom S";
        const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
        REQUIRE(p.details == "Custom D");
        REQUIRE(p.state == "Custom S");
    }
    SECTION("swap lines")
    {
        Settings s;
        s.swapLines = true;
        const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
        REQUIRE(p.state == "Sole Survivor " + kDot + " LVL 12 " + kDot + " 85% HP " + kDot + " 1234 caps");
        REQUIRE(p.details == "In Pipboy Menu");
    }
}
