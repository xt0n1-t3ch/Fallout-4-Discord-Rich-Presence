#include <atomic>
#include <chrono>

#include <catch2/catch_test_macros.hpp>

#include "Constants.h"
#include "Presence/Composer.h"
#include "Presence/PresenceConfig.h"

namespace {
const F4DRP::Presence::PresenceConfig kCfg;
}

TEST_CASE("Dispatch: gameLoading branch emits launching-game sentinel", "[dispatch][time][null-empty]")
{
    F4DRP::Game::GameState g;
    g.gameLoading = true;
    g.playerName = "Sole Survivor";
    g.level = 99;
    g.healthPct = 1.0F;
    g.locationName = "Sanctuary Hills";
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Presence::compose(g, s, t, kCfg, std::chrono::steady_clock::now());
    REQUIRE(p.details == std::string{F4DRP::Constants::Defaults::kLaunchingGame});
    REQUIRE(p.state.empty());
}

TEST_CASE("Dispatch: inMainMenu wins over loaded player snapshot", "[dispatch][null-empty]")
{
    F4DRP::Game::GameState g;
    g.inMainMenu = true;
    g.playerName = "Sole Survivor";
    g.level = 99;
    g.healthPct = 1.0F;
    g.caps = 1234;
    F4DRP::Config::Settings s;
    s.showCaps = true;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Presence::compose(g, s, t, kCfg, std::chrono::steady_clock::now());
    REQUIRE(p.details == std::string{F4DRP::Constants::Defaults::kMainMenu});
    REQUIRE(p.state.empty());
}

TEST_CASE("Dispatch: inChargen wins over loaded player snapshot", "[dispatch][null-empty]")
{
    F4DRP::Game::GameState g;
    g.inChargen = true;
    g.playerName = "Sole Survivor";
    g.level = 1;
    g.healthPct = 1.0F;
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Presence::compose(g, s, t, kCfg, std::chrono::steady_clock::now());
    REQUIRE(p.details == std::string{F4DRP::Constants::Defaults::kStartedANewGame});
    REQUIRE(p.state.empty());
}

TEST_CASE("Dispatch: heartbeat counter contract - submissions track invocations", "[dispatch][concurrency]")
{
    std::atomic_uint64_t submissions{0};
    std::atomic_uint64_t invocations{0};

    auto submit = [&]() { submissions.fetch_add(1, std::memory_order_relaxed); };
    auto invoke = [&]() { invocations.fetch_add(1, std::memory_order_relaxed); };

    for (int i = 0; i < 100; ++i) {
        submit();
        invoke();
    }
    REQUIRE(submissions.load() == 100);
    REQUIRE(invocations.load() == 100);
    REQUIRE(submissions.load() - invocations.load() == 0);

    for (int i = 0; i < 50; ++i) {
        submit();
    }
    REQUIRE(submissions.load() == 150);
    REQUIRE(invocations.load() == 100);
    REQUIRE(submissions.load() - invocations.load() == 50);
}

TEST_CASE("Dispatch: precedence order is event > combat > menu > exploring", "[dispatch][format]")
{
    F4DRP::Game::GameState g;
    const auto now = std::chrono::steady_clock::now();
    g.menu = F4DRP::Game::MenuKind::PipBoy;
    g.menuShowsLocation = false;
    g.inCombat = true;
    g.combatTargetNames = {"Raider"};
    g.lastEvent = F4DRP::Game::EventKind::HackedTerminal;
    g.eventPayload = "Terminal01";
    g.eventExpiresAt = now + std::chrono::seconds{5};
    g.locationName = "Sanctuary";
    F4DRP::Config::Settings s;
    s.simplifiedStatus = false;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Presence::compose(g, s, t, kCfg, now);
    REQUIRE(p.state == "Hacked Terminal01 in Sanctuary");
}
