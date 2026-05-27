#include "Presence/Composer.h"

#include <algorithm>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "Constants.h"
#include "Game/MenuCatalog.h"
#include "Presence/Template.h"

namespace F4DRP::Presence {
namespace {
    using namespace F4DRP::Constants;

    std::string_view menuLabel(Game::MenuKind kind, const Config::Translation& t)
    {
        if (const auto* info = Game::findMenu(kind); info != nullptr) {
            return t.get(info->stringKey, info->defaultLabel);
        }
        return {};
    }

    std::string groupThousands(std::int64_t value)
    {
        const auto digits = fmt::format("{}", value);
        std::string out;
        int count = 0;
        for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
            if (count != 0 && count % 3 == 0) {
                out.push_back(',');
            }
            out.push_back(*it);
            ++count;
        }
        std::reverse(out.begin(), out.end());
        return out;
    }

    std::string formatCaps(std::int64_t caps, std::int64_t maxToShow)
    {
        const bool clamped = caps > maxToShow;
        const auto shown = clamped ? maxToShow : caps;
        return groupThousands(shown) + (clamped ? "+" : "");
    }

    FieldMap buildFields(const Game::GameState& g, const Config::Settings& s, const Config::Translation& t)
    {
        FieldMap f;
        f.set(Token::kName, g.playerName);
        f.set(Token::kLevel, fmt::format("{}", g.level));
        f.set(Token::kHp, fmt::format("{:.0f}", g.healthPct * 100.0F));
        f.set(Token::kCaps, formatCaps(g.caps, s.maxCapsToShow));
        f.set(Token::kLocation, g.locationName);
        if (g.inCombat && !g.combatTargetNames.empty()) {
            f.set(Token::kEnemy, g.combatTargetNames.front());
        }
        f.set(Token::kMenu, std::string{menuLabel(g.menu, t)});
        return f;
    }

    std::string detailsLine(const Game::GameState& g,
                            const Config::Settings& s,
                            const PresenceConfig& cfg,
                            const FieldMap& fields)
    {
        std::vector<std::string> parts;
        if (s.showName && !g.playerName.empty()) {
            parts.push_back(render(cfg.fieldName, fields));
        }
        if (s.showHp) {
            parts.push_back(render(cfg.fieldHp, fields));
        }
        if (s.showCaps) {
            parts.push_back(render(cfg.fieldCaps, fields));
        }
        return joinNonEmpty(parts, cfg.fieldSeparator);
    }

    std::string withLocation(std::string base,
                             const Game::GameState& g,
                             const Config::Settings& s,
                             const PresenceConfig& cfg)
    {
        const auto& sep = s.simplifiedStatus ? cfg.locationSeparatorSimplified : cfg.locationSeparatorVerbose;
        return joinNonEmpty({std::move(base), g.locationName}, sep);
    }

    std::string eventState(const Game::GameState& g, const Config::Translation& t)
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
        return std::string{verb} + g.eventPayload;
    }

    struct StateResult
    {
        std::string text;
        const std::string* icon = nullptr;
    };

    StateResult stateLine(const Game::GameState& g,
                          const Config::Settings& s,
                          const Config::Translation& t,
                          const PresenceConfig& cfg,
                          std::chrono::steady_clock::time_point now)
    {
        if (g.lastEvent != Game::EventKind::None && now < g.eventExpiresAt && s.showEventStatuses) {
            return {withLocation(eventState(g, t), g, s, cfg), &cfg.iconMenu};
        }
        if (g.inCombat && !g.combatTargetNames.empty()) {
            std::string base =
                std::string{t.get(StringKey::kFighting, Defaults::kFighting)} + " " + g.combatTargetNames.front();
            return {withLocation(std::move(base), g, s, cfg), &cfg.iconCombat};
        }
        if (g.menu != Game::MenuKind::None && g.menu != Game::MenuKind::LoadingMenu) {
            std::string base{menuLabel(g.menu, t)};
            if (g.menuShowsLocation) {
                base = withLocation(std::move(base), g, s, cfg);
            }
            return {std::move(base), &cfg.iconMenu};
        }
        if (g.menu == Game::MenuKind::None && g.menuModeActive) {
            return {std::string{t.get(StringKey::kInMenu, Defaults::kInMenu)}, &cfg.iconMenu};
        }
        std::string base{t.get(StringKey::kExploring, Defaults::kExploring)};
        return {withLocation(std::move(base), g, s, cfg), &cfg.iconExploring};
    }

    void applyIcon(Discord::PresenceState& p, const std::string* icon)
    {
        if (icon != nullptr && !icon->empty()) {
            p.smallImageKey = *icon;
        }
    }

    Discord::PresenceState finalize(Discord::PresenceState p,
                                    const Config::Settings& s,
                                    const PresenceConfig& cfg,
                                    std::int64_t startUnix)
    {
        p.largeImageKey = cfg.largeImage;
        p.largeImageText = s.customLargeImageText.empty() ? cfg.largeText : s.customLargeImageText;
        for (const auto& button : cfg.buttons) {
            p.buttons.emplace_back(button.label, button.url);
        }
        if (s.showPlayTime) {
            p.startTimestampUnix = startUnix;
        }
        return Discord::clampToDiscordLimits(std::move(p));
    }
} // namespace

Discord::PresenceState compose(const Game::GameState& g,
                               const Config::Settings& s,
                               const Config::Translation& t,
                               const PresenceConfig& cfg,
                               std::chrono::steady_clock::time_point now)
{
    if (g.inMainMenu || g.inChargen || g.gameLoading) {
        Discord::PresenceState p;
        const auto key = g.inMainMenu  ? StringKey::kMainMenu
                         : g.inChargen ? StringKey::kStartedANewGame
                                       : StringKey::kLaunchingGame;
        const auto def = g.inMainMenu  ? Defaults::kMainMenu
                         : g.inChargen ? Defaults::kStartedANewGame
                                       : Defaults::kLaunchingGame;
        p.details = std::string{t.get(key, def)};
        applyIcon(p, g.inMainMenu ? &cfg.iconMainMenu : &cfg.iconLoading);
        return finalize(std::move(p), s, cfg, g.sessionStartUnix);
    }

    const auto fields = buildFields(g, s, t);
    std::string details = detailsLine(g, s, cfg, fields);
    auto state = stateLine(g, s, t, cfg, now);
    std::string stateText = std::move(state.text);
    if (s.showLvl && g.level > 0) {
        stateText = joinNonEmpty({render(cfg.fieldLevel, fields), std::move(stateText)}, cfg.fieldSeparator);
    }

    if (!s.customDetails.empty()) {
        details = s.customDetails;
    }
    if (!s.customState.empty()) {
        stateText = s.customState;
    }
    if (s.swapLines) {
        std::swap(details, stateText);
    }

    Discord::PresenceState p;
    p.details = std::move(details);
    p.state = std::move(stateText);
    applyIcon(p, state.icon);
    return finalize(std::move(p), s, cfg, g.sessionStartUnix);
}
} // namespace F4DRP::Presence
