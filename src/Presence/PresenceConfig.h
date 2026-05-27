#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Constants.h"

namespace F4DRP::Presence {
struct Button
{
    std::string label;
    std::string url;
};

struct PresenceConfig
{
    std::string fieldName{Constants::PresenceDefaults::kFieldName};
    std::string fieldLevel{Constants::PresenceDefaults::kFieldLevel};
    std::string fieldHp{Constants::PresenceDefaults::kFieldHp};
    std::string fieldCaps{Constants::PresenceDefaults::kFieldCaps};
    std::string fieldSeparator{Constants::PresenceDefaults::kFieldSeparator};

    std::string locationSeparatorSimplified{Constants::PresenceDefaults::kLocationSeparatorSimplified};
    std::string locationSeparatorVerbose{Constants::PresenceDefaults::kLocationSeparatorVerbose};

    std::string largeImage{Constants::PresenceDefaults::kLargeImage};
    std::string largeText{Constants::PresenceDefaults::kLargeText};

    std::string iconExploring;
    std::string iconCombat;
    std::string iconMenu;
    std::string iconMainMenu;
    std::string iconLoading;

    std::vector<Button> buttons;
};

[[nodiscard]] PresenceConfig parsePresenceConfig(const std::string& iniContents);
[[nodiscard]] PresenceConfig loadPresenceConfig(const std::filesystem::path& iniPath);
} // namespace F4DRP::Presence
