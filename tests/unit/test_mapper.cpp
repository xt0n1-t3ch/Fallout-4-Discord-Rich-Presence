#include <chrono>

#include <catch2/catch_test_macros.hpp>

#include "Constants.h"
#include "Mapper/Mapper.h"

namespace {
F4DRP::Game::GameState minimalState()
{
    F4DRP::Game::GameState g;
    g.menu = F4DRP::Game::MenuKind::PipBoy;
    g.playerName = "Sole Survivor";
    g.level = 12;
    g.healthPct = 0.85F;
    g.caps = 1234;
    g.locationName = "Sanctuary Hills";
    g.isExterior = true;
    g.sessionStartUnix = 1716595200;
    return g;
}
} // namespace

TEST_CASE("Mapper produces PipBoy details when in Pipboy menu", "[mapper][format]")
{
    auto g = minimalState();
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.details == "In Pipboy Menu");
    REQUIRE(p.state.find("Sole Survivor") != std::string::npos);
    REQUIRE(p.state.find("LVL 12") != std::string::npos);
    REQUIRE(p.state.find("HP 85%") != std::string::npos);
    REQUIRE(p.state.find("1234 caps") != std::string::npos);
    REQUIRE(p.largeImageKey == "fallout4");
}

TEST_CASE("Mapper surfaces combat target", "[mapper][format]")
{
    auto g = minimalState();
    g.menu = F4DRP::Game::MenuKind::None;
    g.inCombat = true;
    g.combatTargetNames = {"Raider"};
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.details == "Fighting with Raider");
}

TEST_CASE("Mapper event override wins when active", "[mapper][time]")
{
    auto g = minimalState();
    const auto now = std::chrono::steady_clock::now();
    g.menu = F4DRP::Game::MenuKind::None;
    g.lastEvent = F4DRP::Game::EventKind::HackedTerminal;
    g.eventExpiresAt = now + std::chrono::seconds{5};
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, now);
    REQUIRE(p.details.find("Hacked") != std::string::npos);
}

TEST_CASE("Mapper event expires past expiry", "[mapper][time]")
{
    auto g = minimalState();
    const auto now = std::chrono::steady_clock::now();
    g.menu = F4DRP::Game::MenuKind::None;
    g.lastEvent = F4DRP::Game::EventKind::HackedTerminal;
    g.eventExpiresAt = now - std::chrono::seconds{1};
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, now);
    REQUIRE(p.details.find("Hacked") == std::string::npos);
}

TEST_CASE("Mapper customState and customDetails override", "[mapper][format]")
{
    auto g = minimalState();
    F4DRP::Config::Settings s;
    s.customDetails = "Coding F4DRP";
    s.customState = "in Sanctuary";
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.details == "Coding F4DRP");
    REQUIRE(p.state == "in Sanctuary");
}

TEST_CASE("Mapper swapLines exchanges details and state", "[mapper][format]")
{
    auto g = minimalState();
    F4DRP::Config::Settings s;
    s.swapLines = true;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.details.find("Sole Survivor") != std::string::npos);
    REQUIRE(p.state == "In Pipboy Menu");
}

TEST_CASE("Mapper hides fields per INI toggles", "[mapper][null-empty]")
{
    auto g = minimalState();
    F4DRP::Config::Settings s;
    s.showName = false;
    s.showLvl = false;
    s.showCaps = false;
    s.showHp = false;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state.empty());
}

TEST_CASE("Mapper caps clamp respects maxCapsToShow", "[mapper][boundary]")
{
    auto g = minimalState();
    g.caps = 9'999'999;
    F4DRP::Config::Settings s;
    s.maxCapsToShow = 500;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state.find("500 caps") != std::string::npos);
}

TEST_CASE("Mapper translation override propagates to details", "[mapper][format]")
{
    auto g = minimalState();
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t = F4DRP::Config::Translation::fromString("[Strings]\ns_T_PipboyMenu=En la Pip-Boy\n");
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.details == "En la Pip-Boy");
}
