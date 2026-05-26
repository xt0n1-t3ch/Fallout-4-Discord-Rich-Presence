#include <chrono>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "Constants.h"
#include "Game/MenuCatalog.h"
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

TEST_CASE("Mapper: PipBoy menu - stats in details, menu in state", "[mapper][format]")
{
    auto g = minimalState();
    F4DRP::Config::Settings s;
    s.showCaps = true;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state == "In Pipboy Menu");
    REQUIRE(p.details.find("Sole Survivor") != std::string::npos);
    REQUIRE(p.details.find("LVL:12") != std::string::npos);
    REQUIRE(p.details.find("HP:85%") != std::string::npos);
    REQUIRE(p.details.find("1234 caps") != std::string::npos);
    REQUIRE(p.largeImageKey == std::string{F4DRP::Constants::kDefaultLargeImage});
}

TEST_CASE("Mapper: combat target - Fighting <enemy> in <loc>", "[mapper][format]")
{
    auto g = minimalState();
    g.menu = F4DRP::Game::MenuKind::None;
    g.inCombat = true;
    g.combatTargetNames = {"Raider"};
    F4DRP::Config::Settings s;
    s.simplifiedStatus = false;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state == "Fighting Raider in Sanctuary Hills");
}

TEST_CASE("Mapper: combat target - simplified swaps in->|", "[mapper][format]")
{
    auto g = minimalState();
    g.menu = F4DRP::Game::MenuKind::None;
    g.inCombat = true;
    g.combatTargetNames = {"Raider"};
    F4DRP::Config::Settings s;
    s.simplifiedStatus = true;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state == "Fighting Raider | Sanctuary Hills");
}

TEST_CASE("Mapper: event override wins, payload appears with location", "[mapper][time][format]")
{
    auto g = minimalState();
    const auto now = std::chrono::steady_clock::now();
    g.menu = F4DRP::Game::MenuKind::None;
    g.lastEvent = F4DRP::Game::EventKind::HackedTerminal;
    g.eventPayload = "Piper's Terminal";
    g.eventExpiresAt = now + std::chrono::seconds{5};
    F4DRP::Config::Settings s;
    s.simplifiedStatus = false;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, now);
    REQUIRE(p.state == "Hacked Piper's Terminal in Sanctuary Hills");
}

TEST_CASE("Mapper: event drops past expiry - back to default state", "[mapper][time]")
{
    auto g = minimalState();
    const auto now = std::chrono::steady_clock::now();
    g.menu = F4DRP::Game::MenuKind::None;
    g.lastEvent = F4DRP::Game::EventKind::HackedTerminal;
    g.eventPayload = "Old Terminal";
    g.eventExpiresAt = now - std::chrono::seconds{1};
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, now);
    REQUIRE(p.state.find("Hacked") == std::string::npos);
}

TEST_CASE("Mapper: customDetails / customState override", "[mapper][format]")
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

TEST_CASE("Mapper: swapLines exchanges stats and verb line", "[mapper][format]")
{
    auto g = minimalState();
    F4DRP::Config::Settings s;
    s.swapLines = true;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.details == "In Pipboy Menu");
    REQUIRE(p.state.find("Sole Survivor") != std::string::npos);
}

TEST_CASE("Mapper: hiding all stat toggles empties details", "[mapper][null-empty]")
{
    auto g = minimalState();
    F4DRP::Config::Settings s;
    s.showName = false;
    s.showLvl = false;
    s.showCaps = false;
    s.showHp = false;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.details.empty());
}

TEST_CASE("Mapper: caps clamp emits + suffix when exceeded", "[mapper][boundary]")
{
    auto g = minimalState();
    g.caps = 9'999'999;
    F4DRP::Config::Settings s;
    s.showCaps = true;
    s.maxCapsToShow = 500;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.details.find("500+ caps") != std::string::npos);
}

TEST_CASE("Mapper: caps at-clamp boundary emits no + suffix", "[mapper][boundary]")
{
    auto g = minimalState();
    g.caps = 500;
    F4DRP::Config::Settings s;
    s.showCaps = true;
    s.maxCapsToShow = 500;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.details.find("500 caps") != std::string::npos);
    REQUIRE(p.details.find("500+ caps") == std::string::npos);
}

TEST_CASE("Mapper: translation override propagates to state line", "[mapper][format]")
{
    auto g = minimalState();
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t = F4DRP::Config::Translation::fromString("[Strings]\ns_T_PipboyMenu=En la Pip-Boy\n");
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state == "En la Pip-Boy");
}

TEST_CASE("Mapper: main menu - details says it, state empty", "[mapper][null-empty]")
{
    F4DRP::Game::GameState g;
    g.inMainMenu = true;
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.details == "In Main menu");
    REQUIRE(p.state.empty());
}

TEST_CASE("Mapper: chargen - details says it, state empty", "[mapper][null-empty]")
{
    F4DRP::Game::GameState g;
    g.inChargen = true;
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.details == "Started a new game");
    REQUIRE(p.state.empty());
}

TEST_CASE("Mapper: exploring - verb prepended, normal uses bare space", "[mapper][format]")
{
    F4DRP::Game::GameState g;
    g.menu = F4DRP::Game::MenuKind::None;
    g.locationName = "Sanctuary Hills";
    g.isExterior = true;
    F4DRP::Config::Settings s;
    s.simplifiedStatus = false;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state == "Exploring Sanctuary Hills");
}

TEST_CASE("Mapper: exploring - simplified uses | separator", "[mapper][format]")
{
    F4DRP::Game::GameState g;
    g.menu = F4DRP::Game::MenuKind::None;
    g.locationName = "Sanctuary Hills";
    g.isExterior = true;
    F4DRP::Config::Settings s;
    s.simplifiedStatus = true;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state == "Exploring | Sanctuary Hills");
}

TEST_CASE("Mapper: menu with bShowLocation true (Workshop) appends location", "[mapper][format]")
{
    F4DRP::Game::GameState g;
    g.menu = F4DRP::Game::MenuKind::Workshop;
    g.menuShowsLocation = true;
    g.locationName = "Sanctuary";
    F4DRP::Config::Settings s;
    s.simplifiedStatus = false;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state == "In Workshop mode in Sanctuary");
}

TEST_CASE("Mapper: menu with bShowLocation false (PipBoy) drops location", "[mapper][format]")
{
    F4DRP::Game::GameState g;
    g.menu = F4DRP::Game::MenuKind::PipBoy;
    g.menuShowsLocation = false;
    g.locationName = "Sanctuary";
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state == "In Pipboy Menu");
}

TEST_CASE("Mapper: terminal menu with location appends via kIn", "[mapper][format]")
{
    F4DRP::Game::GameState g;
    g.menu = F4DRP::Game::MenuKind::Terminal;
    g.menuShowsLocation = true;
    g.locationName = "Diamond City";
    F4DRP::Config::Settings s;
    s.simplifiedStatus = false;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state == "Using terminal in Diamond City");
}

TEST_CASE("Mapper: dialogue menu with location, simplified uses |", "[mapper][format]")
{
    F4DRP::Game::GameState g;
    g.menu = F4DRP::Game::MenuKind::Dialogue;
    g.menuShowsLocation = true;
    g.locationName = "Diamond City";
    F4DRP::Config::Settings s;
    s.simplifiedStatus = true;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state == "Talking | Diamond City");
}

TEST_CASE("Mapper: built workshop object event with payload + location", "[mapper][format][time]")
{
    F4DRP::Game::GameState g;
    const auto now = std::chrono::steady_clock::now();
    g.menu = F4DRP::Game::MenuKind::None;
    g.lastEvent = F4DRP::Game::EventKind::BuiltWorkshopObject;
    g.eventPayload = "Wood Foundation";
    g.eventExpiresAt = now + std::chrono::seconds{5};
    g.locationName = "Sanctuary";
    F4DRP::Config::Settings s;
    s.simplifiedStatus = false;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, now);
    REQUIRE(p.state == "Built Wood Foundation in Sanctuary");
}

TEST_CASE("Mapper: stats line byte-exact reference shape", "[mapper][format][boundary]")
{
    F4DRP::Game::GameState g;
    g.menu = F4DRP::Game::MenuKind::None;
    g.locationName = "Sanctuary Hills";
    g.playerName = "Vore";
    g.level = 79;
    g.healthPct = 1.0F;
    g.caps = 4242;
    F4DRP::Config::Settings s;
    s.showCaps = true;
    s.simplifiedStatus = true;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.details == "Vore | LVL:79 | HP:100% | 4242 caps");
    REQUIRE(p.state == "Exploring | Sanctuary Hills");
}

TEST_CASE("Mapper: stats line without caps matches simplified reference", "[mapper][format]")
{
    F4DRP::Game::GameState g;
    g.menu = F4DRP::Game::MenuKind::None;
    g.locationName = "Sanctuary Hills";
    g.playerName = "Vore";
    g.level = 79;
    g.healthPct = 1.0F;
    g.caps = 4242;
    F4DRP::Config::Settings s;
    s.showCaps = false;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.details == "Vore | LVL:79 | HP:100%");
}

TEST_CASE("Mapper: every cataloged menu renders its default label (no location)", "[mapper][menucatalog][format]")
{
    for (const auto& m : F4DRP::Game::kMenuCatalog) {
        F4DRP::Game::GameState g;
        g.menu = m.kind;
        g.menuShowsLocation = m.showsLocation;
        F4DRP::Config::Settings s;
        F4DRP::Config::Translation t;
        const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
        INFO("engineName=" << m.engineName);
        REQUIRE(p.state == std::string{m.defaultLabel});
    }
}

TEST_CASE("Mapper: location-bearing menus append ' in <loc>' (normal mode)", "[mapper][menucatalog][format]")
{
    for (const auto& m : F4DRP::Game::kMenuCatalog) {
        if (!m.showsLocation) {
            continue;
        }
        F4DRP::Game::GameState g;
        g.menu = m.kind;
        g.menuShowsLocation = true;
        g.locationName = "Diamond City";
        F4DRP::Config::Settings s;
        s.simplifiedStatus = false;
        F4DRP::Config::Translation t;
        const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
        INFO("engineName=" << m.engineName);
        REQUIRE(p.state == std::string{m.defaultLabel} + " in Diamond City");
    }
}

TEST_CASE("Mapper: non-location menus never leak the location string", "[mapper][menucatalog][null-empty]")
{
    for (const auto& m : F4DRP::Game::kMenuCatalog) {
        if (m.showsLocation) {
            continue;
        }
        F4DRP::Game::GameState g;
        g.menu = m.kind;
        g.menuShowsLocation = false;
        g.locationName = "Diamond City";
        F4DRP::Config::Settings s;
        F4DRP::Config::Translation t;
        const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
        INFO("engineName=" << m.engineName);
        REQUIRE(p.state == std::string{m.defaultLabel});
        REQUIRE(p.state.find("Diamond City") == std::string::npos);
    }
}

TEST_CASE("Mapper: gameLoading wins before in-world states", "[mapper][format]")
{
    F4DRP::Game::GameState g;
    g.gameLoading = true;
    g.playerName = "Lexie";
    g.menu = F4DRP::Game::MenuKind::PipBoy;
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.details == "Launching game");
    REQUIRE(p.state.empty());
}

TEST_CASE("Mapper: combat outranks an open menu", "[mapper][format]")
{
    auto g = minimalState();
    g.menu = F4DRP::Game::MenuKind::PipBoy;
    g.inCombat = true;
    g.combatTargetNames = {"Deathclaw"};
    F4DRP::Config::Settings s;
    s.simplifiedStatus = true;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state.find("Fighting Deathclaw") != std::string::npos);
    REQUIRE(p.state.find("Pipboy") == std::string::npos);
}

TEST_CASE("Mapper: showPlayTime gates the session start timestamp", "[mapper][time]")
{
    auto g = minimalState();
    g.sessionStartUnix = 1716595200;
    F4DRP::Config::Translation t;

    F4DRP::Config::Settings on;
    on.showPlayTime = true;
    REQUIRE(F4DRP::Mapper::mapGameStateToPresence(g, on, t, std::chrono::steady_clock::now()).startTimestampUnix ==
            1716595200);

    F4DRP::Config::Settings off;
    off.showPlayTime = false;
    REQUIRE(F4DRP::Mapper::mapGameStateToPresence(g, off, t, std::chrono::steady_clock::now()).startTimestampUnix == 0);
}

TEST_CASE("Mapper: customLargeImageText overrides the default large-image text", "[mapper]")
{
    auto g = minimalState();
    F4DRP::Config::Settings s;
    s.customLargeImageText = "Vault 111 Resident";
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.largeImageText == "Vault 111 Resident");
}

TEST_CASE("Mapper: menuModeActive without a specific menu shows generic 'In a menu'", "[mapper][menu]")
{
    F4DRP::Game::GameState g;
    g.menu = F4DRP::Game::MenuKind::None;
    g.menuModeActive = true;
    g.locationName = "Vault 114";
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state == "In a menu");
}

TEST_CASE("Mapper: a specific polled menu outranks the generic menuModeActive fallback", "[mapper][menu]")
{
    F4DRP::Game::GameState g;
    g.menu = F4DRP::Game::MenuKind::PipBoy;
    g.menuModeActive = true;
    F4DRP::Config::Settings s;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state == "In Pipboy Menu");
}

TEST_CASE("Mapper: no menu and menuModeActive false falls through to exploring", "[mapper][menu]")
{
    F4DRP::Game::GameState g;
    g.menu = F4DRP::Game::MenuKind::None;
    g.menuModeActive = false;
    g.locationName = "Vault 114";
    F4DRP::Config::Settings s;
    s.simplifiedStatus = true;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state == "Exploring | Vault 114");
}

TEST_CASE("Mapper: combat still outranks generic menuModeActive", "[mapper][menu]")
{
    F4DRP::Game::GameState g;
    g.menu = F4DRP::Game::MenuKind::None;
    g.menuModeActive = true;
    g.inCombat = true;
    g.combatTargetNames = {"Raider"};
    F4DRP::Config::Settings s;
    s.simplifiedStatus = true;
    F4DRP::Config::Translation t;
    const auto p = F4DRP::Mapper::mapGameStateToPresence(g, s, t, std::chrono::steady_clock::now());
    REQUIRE(p.state.find("Fighting Raider") != std::string::npos);
    REQUIRE(p.state.find("menu") == std::string::npos);
}
