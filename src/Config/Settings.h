#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace F4DRP::Config {
struct Settings
{
    bool simplifiedStatus = true;
    bool showPlayTime = true;
    bool showName = true;
    bool showLvl = true;
    bool showCaps = true;
    std::int64_t maxCapsToShow = 99'999'999;
    bool showHp = true;
    bool showEventStatuses = true;
    bool allowEventStatusOverride = false;
    float eventStatusDuration = 7.0F;
    float updateInterval = 3.0F;
    bool debugMode = false;
    bool swapLines = false;
    std::string appId;
    std::string customState;
    std::string customDetails;
    std::string customLargeImageText;
};

Settings loadOrCreate(const std::filesystem::path& iniPath);
bool writeDefault(const std::filesystem::path& iniPath);
Settings parseFromString(const std::string& iniContents);
Settings clamp(Settings s);
} // namespace F4DRP::Config
