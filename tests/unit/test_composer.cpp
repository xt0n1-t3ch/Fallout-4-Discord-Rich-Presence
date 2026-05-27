#include <chrono>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "Constants.h"
#include "Game/GameState.h"
#include "Game/MenuCatalog.h"
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
const std::string kStim = "\xf0\x9f\x92\x89";
const std::string kCoin = "\xf0\x9f\xaa\x99";
const std::string kStar = "\xe2\xad\x90";

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

TEST_CASE("Composer: details = name, HP, caps; level moves to the state line", "[composer]")
{
    auto g = base();
    g.menu = MenuKind::PipBoy;
    Settings s;
    s.showCaps = true;
    Translation t;
    PresenceConfig cfg;
    const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());

    REQUIRE(p.details == "Sole Survivor " + kDot + " " + kStim + " 85% " + kDot + " " + kCoin + " 1,234");
    REQUIRE(p.state == kStar + " Level 12 " + kDot + " In Pipboy Menu");
    REQUIRE(p.largeImageKey == "fo4-big");
    REQUIRE(p.startTimestampUnix == 1716595200);
}

TEST_CASE("Composer: field toggles drop detail segments cleanly", "[composer]")
{
    auto g = base();
    g.menu = MenuKind::PipBoy;
    Settings s;
    s.showName = false;
    s.showCaps = false;
    Translation t;
    PresenceConfig cfg;
    const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
    REQUIRE(p.details == kStim + " 85%");
}

TEST_CASE("Composer: combat shows level, enemy and location, icon applied", "[composer]")
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
        REQUIRE(p.state == kStar + " Level 12 " + kDot + " Fighting Raider " + kDot + " Sanctuary Hills");
        REQUIRE(p.smallImageKey == "icon_combat");
    }
    SECTION("verbose uses in")
    {
        Settings s;
        s.simplifiedStatus = false;
        const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
        REQUIRE(p.state == kStar + " Level 12 " + kDot + " Fighting Raider in Sanctuary Hills");
    }
}

TEST_CASE("Composer: exploring fallback with level prefix", "[composer]")
{
    auto g = base();
    Settings s;
    s.simplifiedStatus = true;
    Translation t;
    PresenceConfig cfg;
    const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
    REQUIRE(p.state == kStar + " Level 12 " + kDot + " Exploring " + kDot + " Sanctuary Hills");
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
    REQUIRE(p.state == kStar + " Level 12 " + kDot + " In a menu");
}

TEST_CASE("Composer: level is omitted when showLvl is off", "[composer]")
{
    auto g = base();
    g.menu = MenuKind::PipBoy;
    Settings s;
    s.showLvl = false;
    Translation t;
    PresenceConfig cfg;
    const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
    REQUIRE(p.state == "In Pipboy Menu");
}

TEST_CASE("Composer: lifecycle states use details only, no level prefix", "[composer]")
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
    REQUIRE(p.state == kStar + " Level 12 " + kDot + " Hacked Terminal " + kDot + " Sanctuary Hills");
}

TEST_CASE("Composer: custom overrides win and swap exchanges lines", "[composer]")
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
        const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
        Settings sw;
        sw.swapLines = true;
        const auto q = compose(g, sw, t, cfg, std::chrono::steady_clock::now());
        REQUIRE(q.state == p.details);
        REQUIRE(q.details == p.state);
    }
}

TEST_CASE("Composer: every cataloged menu maps to label, location and level prefix", "[composer]")
{
    Translation t;
    PresenceConfig cfg;
    Settings s;
    s.simplifiedStatus = true;

    for (const auto& menu : F4DRP::Game::kMenuCatalog) {
        auto g = base();
        g.menu = menu.kind;
        g.menuShowsLocation = menu.showsLocation;
        const auto p = compose(g, s, t, cfg, std::chrono::steady_clock::now());
        const std::string label{menu.defaultLabel};
        std::string status = menu.showsLocation ? label + " " + kDot + " Sanctuary Hills" : label;
        const std::string expected = kStar + " Level 12 " + kDot + " " + status;
        INFO("menu=" << label);
        REQUIRE(p.state == expected);
    }
}

TEST_CASE("Composer: HP formatting boundaries", "[composer][boundary]")
{
    Translation t;
    PresenceConfig cfg;
    Settings s;
    s.showName = false;
    s.showCaps = false;
    auto g = base();
    g.menu = MenuKind::PipBoy;

    SECTION("zero")
    {
        g.healthPct = 0.0F;
        REQUIRE(compose(g, s, t, cfg, std::chrono::steady_clock::now()).details == kStim + " 0%");
    }
    SECTION("full")
    {
        g.healthPct = 1.0F;
        REQUIRE(compose(g, s, t, cfg, std::chrono::steady_clock::now()).details == kStim + " 100%");
    }
}

TEST_CASE("Composer: caps are grouped and clamp adds a plus suffix", "[composer][boundary]")
{
    Translation t;
    PresenceConfig cfg;
    Settings s;
    s.showName = false;
    s.showLvl = false;
    s.showHp = false;
    s.showCaps = true;
    auto g = base();
    g.menu = MenuKind::PipBoy;

    SECTION("grouped thousands")
    {
        s.maxCapsToShow = 99999999;
        g.caps = 10000000;
        REQUIRE(compose(g, s, t, cfg, std::chrono::steady_clock::now()).details == kCoin + " 10,000,000");
    }
    SECTION("clamp")
    {
        s.maxCapsToShow = 9999;
        g.caps = 50000;
        REQUIRE(compose(g, s, t, cfg, std::chrono::steady_clock::now()).details == kCoin + " 9,999+");
    }
}
