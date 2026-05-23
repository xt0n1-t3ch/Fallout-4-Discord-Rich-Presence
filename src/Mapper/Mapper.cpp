#include "Mapper/Mapper.h"

#include <fmt/format.h>

#include "Constants.h"

namespace F4DRP::Mapper {
namespace {
    std::string_view menuLabel(Game::MenuKind k, const Config::Translation& t)
    {
        using namespace F4DRP::Constants;
        switch (k) {
        case Game::MenuKind::MainMenu:
            return t.get(StringKey::kMainMenu, Defaults::kMainMenu);
        case Game::MenuKind::PauseMenu:
            return t.get(StringKey::kPauseMenu, Defaults::kPauseMenu);
        case Game::MenuKind::PipBoy:
            return t.get(StringKey::kPipboyMenu, Defaults::kPipboyMenu);
        case Game::MenuKind::Sleep:
            return t.get(StringKey::kSleepWaitMenu, Defaults::kSleepWaitMenu);
        case Game::MenuKind::LevelUp:
            return t.get(StringKey::kLevelUpMenu, Defaults::kLevelUpMenu);
        case Game::MenuKind::Dialogue:
            return t.get(StringKey::kDialogueMenu, Defaults::kDialogueMenu);
        case Game::MenuKind::Lockpicking:
            return t.get(StringKey::kLockpickingMenu, Defaults::kLockpickingMenu);
        case Game::MenuKind::Terminal:
            return t.get(StringKey::kTerminalMenu, Defaults::kTerminalMenu);
        case Game::MenuKind::Barter:
            return t.get(StringKey::kBarterMenu, Defaults::kBarterMenu);
        case Game::MenuKind::Crafting:
            return t.get(StringKey::kCrafting, Defaults::kCrafting);
        case Game::MenuKind::Vats:
            return t.get(StringKey::kVatsMenu, Defaults::kVatsMenu);
        case Game::MenuKind::Cooking:
            return t.get(StringKey::kCookingMenu, Defaults::kCookingMenu);
        case Game::MenuKind::Workshop:
            return t.get(StringKey::kWorkshopMenu, Defaults::kWorkshopMenu);
        case Game::MenuKind::LoadingMenu:
            return t.get(StringKey::kLaunchingGame, Defaults::kLaunchingGame);
        default:
            return {};
        }
    }

    std::string eventLabel(Game::EventKind k, const Config::Translation& t)
    {
        using namespace F4DRP::Constants;
        switch (k) {
        case Game::EventKind::HackedTerminal:
            return std::string{t.get(StringKey::kEventHackedTerminal, Defaults::kEventHackedTerminal)} + " terminal";
        case Game::EventKind::BuiltWorkshopObject:
            return std::string{t.get(StringKey::kEventBuiltObject, Defaults::kEventBuiltObject)} +
                   " object in workshop";
        default:
            return {};
        }
    }

    std::string buildLocationLabel(const Game::GameState& g, const Config::Settings& s, const Config::Translation& t)
    {
        using namespace F4DRP::Constants;
        const auto inWord = t.get(StringKey::kIn, Defaults::kIn);
        if (g.locationName.empty()) {
            return std::string{t.get(StringKey::kExploring, Defaults::kExploring)};
        }
        if (s.simplifiedStatus) {
            return g.locationName;
        }
        return fmt::format("{} {}", std::string{inWord}, g.locationName);
    }

    std::string buildStatusLine(const Game::GameState& g, const Config::Settings& s, const Config::Translation& t)
    {
        using namespace F4DRP::Constants;
        std::string out;
        if (s.showName && !g.playerName.empty()) {
            out += g.playerName;
        }
        if (s.showLvl && g.level > 0) {
            if (!out.empty())
                out += " | ";
            out += fmt::format("{} {}", std::string{t.get(StringKey::kLvl, Defaults::kLvl)}, g.level);
        }
        if (s.showHp) {
            if (!out.empty())
                out += " | ";
            out += fmt::format("{} {:.0f}%", std::string{t.get(StringKey::kHp, Defaults::kHp)}, g.healthPct * 100.0F);
        }
        if (s.showCaps) {
            const auto capsToShow = g.caps > s.maxCapsToShow ? s.maxCapsToShow : g.caps;
            if (!out.empty())
                out += " | ";
            out += fmt::format("{} {}", capsToShow, std::string{t.get(StringKey::kCaps, Defaults::kCaps)});
        }
        return out;
    }
} // namespace

Discord::PresenceState mapGameStateToPresence(const Game::GameState& g,
                                              const Config::Settings& s,
                                              const Config::Translation& t,
                                              std::chrono::steady_clock::time_point now)
{
    using namespace F4DRP::Constants;
    Discord::PresenceState p;
    p.largeImageKey = std::string{Constants::kDefaultLargeImage};
    p.largeImageText =
        s.customLargeImageText.empty() ? std::string{Constants::kDefaultLargeImageTxt} : s.customLargeImageText;
    p.startTimestampUnix = g.sessionStartUnix;

    std::string detailsLine;
    if (g.lastEvent != Game::EventKind::None && now < g.eventExpiresAt && s.showEventStatuses) {
        detailsLine = eventLabel(g.lastEvent, t);
    }
    else if (g.inCombat && !g.combatTargetNames.empty()) {
        const auto& target = g.combatTargetNames.front();
        const auto fighting = t.get(StringKey::kFighting, Defaults::kFighting);
        const auto withWord = t.get(StringKey::kWith, Defaults::kWith);
        detailsLine = fmt::format("{} {} {}", std::string{fighting}, std::string{withWord}, target);
    }
    else if (g.menu != Game::MenuKind::None && g.menu != Game::MenuKind::LoadingMenu) {
        detailsLine = std::string{menuLabel(g.menu, t)};
    }
    else if (g.menu == Game::MenuKind::LoadingMenu) {
        detailsLine = std::string{t.get(StringKey::kLaunchingGame, Defaults::kLaunchingGame)};
    }
    else {
        detailsLine = buildLocationLabel(g, s, t);
    }

    std::string stateLine = buildStatusLine(g, s, t);

    if (!s.customDetails.empty())
        detailsLine = s.customDetails;
    if (!s.customState.empty())
        stateLine = s.customState;

    if (s.swapLines) {
        std::swap(detailsLine, stateLine);
    }

    p.details = std::move(detailsLine);
    p.state = std::move(stateLine);
    if (!s.showPlayTime) {
        p.startTimestampUnix = 0;
    }
    return Discord::clampToDiscordLimits(std::move(p));
}
} // namespace F4DRP::Mapper
