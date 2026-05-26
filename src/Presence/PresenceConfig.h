#pragma once

#include <string>

namespace F4DRP::Presence {
struct PresenceConfig
{
    std::string fieldName = "{name}";
    std::string fieldLevel = "LVL {level}";
    std::string fieldHp = "{hp}% HP";
    std::string fieldCaps = "{caps} caps";
    std::string fieldSeparator = " \xe2\x80\xa2 ";

    std::string locationSeparatorSimplified = " \xe2\x80\xa2 ";
    std::string locationSeparatorVerbose = " in ";

    std::string largeImage = "fo4-big";
    std::string largeText = "Fallout 4";

    std::string iconExploring;
    std::string iconCombat;
    std::string iconMenu;
    std::string iconMainMenu;
    std::string iconLoading;
};
} // namespace F4DRP::Presence
