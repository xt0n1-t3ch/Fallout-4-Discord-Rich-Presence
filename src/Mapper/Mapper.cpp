#include "Mapper/Mapper.h"

#include <string>
#include <vector>

#include <fmt/format.h>

#include "Constants.h"
#include "Game/MenuCatalog.h"

namespace F4DRP::Mapper {
namespace {
    using namespace F4DRP::Constants;

    constexpr std::string_view kSimplifiedSeparator = " | ";

    std::string_view menuLabel(Game::MenuKind k, const Config::Translation& t)
    {
        if (const auto* info = Game::findMenu(k); info != nullptr) {
            return t.get(info->stringKey, info->defaultLabel);
        }
        return {};
    }

    std::string locationConnector(const Config::Settings& s, const Config::Translation& t)
    {
        if (s.simplifiedStatus) {
            return std::string{kSimplifiedSeparator};
        }
        return std::string{t.get(StringKey::kIn, Defaults::kIn)};
    }

    std::string appendLocation(std::string base,
                               const Game::GameState& g,
                               const Config::Settings& s,
                               const Config::Translation& t)
    {
        if (g.locationName.empty()) {
            return base;
        }
        base += locationConnector(s, t);
        base += g.locationName;
        return base;
    }

    std::string formatCapsCell(std::int64_t caps, std::int64_t maxCapsToShow, std::string_view capsWord)
    {
        const bool clamped = caps > maxCapsToShow;
        const auto shown = clamped ? maxCapsToShow : caps;
        const std::string_view suffix = clamped ? "+" : "";
        return fmt::format("{}{} {}", shown, suffix, capsWord);
    }

    std::string buildStatsLine(const Game::GameState& g, const Config::Settings& s, const Config::Translation& t)
    {
        std::vector<std::string> parts;
        if (s.showName && !g.playerName.empty()) {
            parts.emplace_back(g.playerName);
        }
        if (s.showLvl && g.level > 0) {
            parts.emplace_back(fmt::format("{}:{}", t.get(StringKey::kLvl, Defaults::kLvl), g.level));
        }
        if (s.showHp) {
            parts.emplace_back(fmt::format("{}:{:.0f}%", t.get(StringKey::kHp, Defaults::kHp), g.healthPct * 100.0F));
        }
        if (s.showCaps) {
            parts.emplace_back(formatCapsCell(g.caps, s.maxCapsToShow, t.get(StringKey::kCaps, Defaults::kCaps)));
        }
        return fmt::format("{}", fmt::join(parts, kSimplifiedSeparator));
    }

    std::string eventState(const Game::GameState& g, const Config::Settings& s, const Config::Translation& t)
    {
        std::string_view verb;
        switch (g.lastEvent) {
        case Game::EventKind::HackedTerminal:
            verb = t.get(StringKey::kEventHackedTerminal, Defaults::kEventHackedTerminal);
            break;
        case Game::EventKind::BuiltWorkshopObject:
            verb = t.get(StringKey::kEventBuiltObject, Defaults::kEventBuiltObject);
            break;
        default:
            return {};
        }
        std::string out{verb};
        out += g.eventPayload;
        return appendLocation(std::move(out), g, s, t);
    }

    std::string combatState(const Game::GameState& g, const Config::Settings& s, const Config::Translation& t)
    {
        std::string out{t.get(StringKey::kFighting, Defaults::kFighting)};
        out += " ";
        out += g.combatTargetNames.front();
        return appendLocation(std::move(out), g, s, t);
    }

    std::string menuState(const Game::GameState& g, const Config::Settings& s, const Config::Translation& t)
    {
        std::string out{menuLabel(g.menu, t)};
        if (g.menuShowsLocation) {
            return appendLocation(std::move(out), g, s, t);
        }
        return out;
    }

    std::string exploringState(const Game::GameState& g, const Config::Settings& s, const Config::Translation& t)
    {
        std::string out{t.get(StringKey::kExploring, Defaults::kExploring)};
        if (g.locationName.empty()) {
            return out;
        }
        if (s.simplifiedStatus) {
            out += kSimplifiedSeparator;
        }
        else {
            out += " ";
        }
        out += g.locationName;
        return out;
    }

    std::string buildStateLine(const Game::GameState& g,
                               const Config::Settings& s,
                               const Config::Translation& t,
                               std::chrono::steady_clock::time_point now)
    {
        if (g.lastEvent != Game::EventKind::None && now < g.eventExpiresAt && s.showEventStatuses) {
            return eventState(g, s, t);
        }
        if (g.inCombat && !g.combatTargetNames.empty()) {
            return combatState(g, s, t);
        }
        if (g.menu != Game::MenuKind::None && g.menu != Game::MenuKind::LoadingMenu) {
            return menuState(g, s, t);
        }
        if (g.menu == Game::MenuKind::None && g.menuModeActive) {
            return std::string{t.get(StringKey::kInMenu, Defaults::kInMenu)};
        }
        return exploringState(g, s, t);
    }

    Discord::PresenceState withDefaults(Discord::PresenceState p, const Config::Settings& s, std::int64_t startUnix)
    {
        p.largeImageKey = std::string{kDefaultLargeImage};
        p.largeImageText = s.customLargeImageText.empty() ? std::string{kDefaultLargeImageTxt} : s.customLargeImageText;
        if (s.showPlayTime) {
            p.startTimestampUnix = startUnix;
        }
        return Discord::clampToDiscordLimits(std::move(p));
    }
} // namespace

Discord::PresenceState mapGameStateToPresence(const Game::GameState& g,
                                              const Config::Settings& s,
                                              const Config::Translation& t,
                                              std::chrono::steady_clock::time_point now)
{
    if (g.inMainMenu) {
        Discord::PresenceState p;
        p.details = std::string{t.get(StringKey::kMainMenu, Defaults::kMainMenu)};
        return withDefaults(std::move(p), s, g.sessionStartUnix);
    }
    if (g.inChargen) {
        Discord::PresenceState p;
        p.details = std::string{t.get(StringKey::kStartedANewGame, Defaults::kStartedANewGame)};
        return withDefaults(std::move(p), s, g.sessionStartUnix);
    }
    if (g.gameLoading) {
        Discord::PresenceState p;
        p.details = std::string{t.get(StringKey::kLaunchingGame, Defaults::kLaunchingGame)};
        return withDefaults(std::move(p), s, g.sessionStartUnix);
    }

    std::string details = buildStatsLine(g, s, t);
    std::string state = buildStateLine(g, s, t, now);

    if (!s.customDetails.empty()) {
        details = s.customDetails;
    }
    if (!s.customState.empty()) {
        state = s.customState;
    }
    if (s.swapLines) {
        std::swap(details, state);
    }

    Discord::PresenceState p;
    p.details = std::move(details);
    p.state = std::move(state);
    return withDefaults(std::move(p), s, g.sessionStartUnix);
}
} // namespace F4DRP::Mapper
