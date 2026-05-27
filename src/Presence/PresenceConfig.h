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

    std::string iconExploring{Constants::PresenceDefaults::kIconExploring};
    std::string iconCombat{Constants::PresenceDefaults::kIconCombat};
    std::string iconMenu{Constants::PresenceDefaults::kIconMenu};
    std::string iconPipboy{Constants::PresenceDefaults::kIconPipboy};
    std::string iconMainMenu{Constants::PresenceDefaults::kIconMainMenu};
    std::string iconLoading{Constants::PresenceDefaults::kIconLoading};

    std::vector<Button> buttons;
};

[[nodiscard]] PresenceConfig parsePresenceConfig(const std::string& iniContents);
[[nodiscard]] PresenceConfig loadPresenceConfig(const std::filesystem::path& iniPath);
} // namespace F4DRP::Presence
