#include "Config/Settings.h"

#include <algorithm>
#include <cctype>
#include <fstream>

#include <SimpleIni.h>

#include "Constants.h"
#include "Util/Logger.h"

namespace F4DRP::Config {
namespace {
    std::string toLower(std::string s)
    {
        std::transform(
            s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    bool parseBool(const char* raw, bool fallback)
    {
        if (raw == nullptr) {
            return fallback;
        }
        const auto v = toLower(raw);
        if (v == "1" || v == "true" || v == "yes" || v == "on")
            return true;
        if (v == "0" || v == "false" || v == "no" || v == "off")
            return false;
        return fallback;
    }

    bool validAppId(std::string_view s)
    {
        if (s.empty())
            return false;
        if (s.size() < 17 || s.size() > 21)
            return false;
        return std::all_of(s.begin(), s.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
    }
} // namespace

bool writeDefault(const std::filesystem::path& iniPath)
{
    CSimpleIniA ini;
    ini.SetUnicode(true);
    ini.SetQuotes(true);

    const auto main = std::string{Constants::IniSection::kMain};
    ini.SetBoolValue(main.c_str(), Constants::IniKey::kSimplifiedStatus.data(), true);
    ini.SetBoolValue(main.c_str(), Constants::IniKey::kShowPlayTime.data(), true);
    ini.SetBoolValue(main.c_str(), Constants::IniKey::kShowName.data(), true);
    ini.SetBoolValue(main.c_str(), Constants::IniKey::kShowLvl.data(), true);
    ini.SetBoolValue(main.c_str(), Constants::IniKey::kShowCaps.data(), true);
    ini.SetLongValue(main.c_str(), Constants::IniKey::kMaxCapsToShow.data(), Constants::kMaxCapsToShowDefault);
    ini.SetBoolValue(main.c_str(), Constants::IniKey::kShowHp.data(), true);
    ini.SetBoolValue(main.c_str(), Constants::IniKey::kShowEventStatuses.data(), true);
    ini.SetBoolValue(main.c_str(), Constants::IniKey::kAllowEventStatusOverride.data(), false);
    ini.SetDoubleValue(
        main.c_str(), Constants::IniKey::kEventStatusDuration.data(), Constants::kEventStatusDurationDefault);
    ini.SetDoubleValue(main.c_str(),
                       Constants::IniKey::kUpdateInterval.data(),
                       static_cast<double>(Constants::kUpdateIntervalDefaultSec));
    ini.SetBoolValue(main.c_str(), Constants::IniKey::kDebugMode.data(), false);
    ini.SetBoolValue(main.c_str(), Constants::IniKey::kSwapLines.data(), false);
    ini.SetValue(main.c_str(), Constants::IniKey::kAppId.data(), "");

    const auto custom = std::string{Constants::IniSection::kCustom};
    ini.SetValue(custom.c_str(), Constants::IniKey::kCustomState.data(), "");
    ini.SetValue(custom.c_str(), Constants::IniKey::kCustomDetails.data(), "");
    ini.SetValue(custom.c_str(), Constants::IniKey::kCustomLargeImageText.data(), "");

    const auto fmt = std::string{Constants::IniSection::kFormat};
    ini.SetValue(fmt.c_str(), Constants::IniKey::kFieldName.data(), Constants::PresenceDefaults::kFieldName.data());
    ini.SetValue(fmt.c_str(), Constants::IniKey::kFieldLevel.data(), Constants::PresenceDefaults::kFieldLevel.data());
    ini.SetValue(fmt.c_str(), Constants::IniKey::kFieldHp.data(), Constants::PresenceDefaults::kFieldHp.data());
    ini.SetValue(fmt.c_str(), Constants::IniKey::kFieldCaps.data(), Constants::PresenceDefaults::kFieldCaps.data());
    ini.SetValue(
        fmt.c_str(), Constants::IniKey::kFieldSeparator.data(), Constants::PresenceDefaults::kFieldSeparator.data());
    ini.SetValue(fmt.c_str(),
                 Constants::IniKey::kLocationSeparatorSimplified.data(),
                 Constants::PresenceDefaults::kLocationSeparatorSimplified.data());
    ini.SetValue(fmt.c_str(),
                 Constants::IniKey::kLocationSeparatorVerbose.data(),
                 Constants::PresenceDefaults::kLocationSeparatorVerbose.data());

    const auto img = std::string{Constants::IniSection::kImages};
    ini.SetValue(img.c_str(), Constants::IniKey::kLargeImage.data(), Constants::PresenceDefaults::kLargeImage.data());
    ini.SetValue(img.c_str(), Constants::IniKey::kLargeText.data(), Constants::PresenceDefaults::kLargeText.data());
    ini.SetValue(
        img.c_str(), Constants::IniKey::kIconExploring.data(), Constants::PresenceDefaults::kIconExploring.data());
    ini.SetValue(img.c_str(), Constants::IniKey::kIconCombat.data(), Constants::PresenceDefaults::kIconCombat.data());
    ini.SetValue(img.c_str(), Constants::IniKey::kIconMenu.data(), Constants::PresenceDefaults::kIconMenu.data());
    ini.SetValue(img.c_str(), Constants::IniKey::kIconPipboy.data(), Constants::PresenceDefaults::kIconPipboy.data());
    ini.SetValue(img.c_str(), Constants::IniKey::kIconVats.data(), Constants::PresenceDefaults::kIconVats.data());
    ini.SetValue(
        img.c_str(), Constants::IniKey::kIconMainMenu.data(), Constants::PresenceDefaults::kIconMainMenu.data());
    ini.SetValue(img.c_str(), Constants::IniKey::kIconLoading.data(), Constants::PresenceDefaults::kIconLoading.data());

    const auto buttons = std::string{Constants::IniSection::kButtons};
    ini.SetValue(buttons.c_str(), Constants::IniKey::kButton1Label.data(), "");
    ini.SetValue(buttons.c_str(), Constants::IniKey::kButton1Url.data(), "");
    ini.SetValue(buttons.c_str(), Constants::IniKey::kButton2Label.data(), "");
    ini.SetValue(buttons.c_str(), Constants::IniKey::kButton2Url.data(), "");

    std::error_code ec;
    std::filesystem::create_directories(iniPath.parent_path(), ec);
    return ini.SaveFile(iniPath.string().c_str(), false) == SI_OK;
}

Settings parseFromString(const std::string& iniContents)
{
    CSimpleIniA ini;
    ini.SetUnicode(true);
    ini.LoadData(iniContents.data(), iniContents.size());

    Settings s;
    const auto main = std::string{Constants::IniSection::kMain};
    const auto custom = std::string{Constants::IniSection::kCustom};

    s.simplifiedStatus =
        parseBool(ini.GetValue(main.c_str(), Constants::IniKey::kSimplifiedStatus.data()), s.simplifiedStatus);
    s.showPlayTime = parseBool(ini.GetValue(main.c_str(), Constants::IniKey::kShowPlayTime.data()), s.showPlayTime);
    s.showName = parseBool(ini.GetValue(main.c_str(), Constants::IniKey::kShowName.data()), s.showName);
    s.showLvl = parseBool(ini.GetValue(main.c_str(), Constants::IniKey::kShowLvl.data()), s.showLvl);
    s.showCaps = parseBool(ini.GetValue(main.c_str(), Constants::IniKey::kShowCaps.data()), s.showCaps);
    s.maxCapsToShow =
        ini.GetLongValue(main.c_str(), Constants::IniKey::kMaxCapsToShow.data(), static_cast<long>(s.maxCapsToShow));
    s.showHp = parseBool(ini.GetValue(main.c_str(), Constants::IniKey::kShowHp.data()), s.showHp);
    s.showEventStatuses =
        parseBool(ini.GetValue(main.c_str(), Constants::IniKey::kShowEventStatuses.data()), s.showEventStatuses);
    s.allowEventStatusOverride = parseBool(
        ini.GetValue(main.c_str(), Constants::IniKey::kAllowEventStatusOverride.data()), s.allowEventStatusOverride);
    s.eventStatusDuration = static_cast<float>(
        ini.GetDoubleValue(main.c_str(), Constants::IniKey::kEventStatusDuration.data(), s.eventStatusDuration));
    s.updateInterval = static_cast<float>(
        ini.GetDoubleValue(main.c_str(), Constants::IniKey::kUpdateInterval.data(), s.updateInterval));
    s.debugMode = parseBool(ini.GetValue(main.c_str(), Constants::IniKey::kDebugMode.data()), s.debugMode);
    s.swapLines = parseBool(ini.GetValue(main.c_str(), Constants::IniKey::kSwapLines.data()), s.swapLines);

    if (const char* raw = ini.GetValue(main.c_str(), Constants::IniKey::kAppId.data(), "")) {
        s.appId = raw;
    }
    if (const char* raw = ini.GetValue(custom.c_str(), Constants::IniKey::kCustomState.data(), "")) {
        s.customState = raw;
    }
    if (const char* raw = ini.GetValue(custom.c_str(), Constants::IniKey::kCustomDetails.data(), "")) {
        s.customDetails = raw;
    }
    if (const char* raw = ini.GetValue(custom.c_str(), Constants::IniKey::kCustomLargeImageText.data(), "")) {
        s.customLargeImageText = raw;
    }
    return clamp(std::move(s));
}

Settings clamp(Settings s)
{
    s.maxCapsToShow =
        std::clamp<std::int64_t>(s.maxCapsToShow, Constants::kMinCapsAbsolute, Constants::kMaxCapsAbsolute);
    s.eventStatusDuration =
        std::clamp(s.eventStatusDuration, Constants::kEventStatusDurationMin, Constants::kEventStatusDurationMax);
    s.updateInterval = std::max(s.updateInterval, static_cast<float>(Constants::kUpdateIntervalFloorSec));
    if (!s.appId.empty() && !validAppId(s.appId)) {
        F4DRP_LOG_WARN("ignoring invalid AppID '{}' (must be 17-21 digits)", s.appId);
        s.appId.clear();
    }
    return s;
}

Settings loadOrCreate(const std::filesystem::path& iniPath)
{
    if (!std::filesystem::exists(iniPath)) {
        F4DRP_LOG_INFO("INI absent at {}, regenerating default", iniPath.string());
        writeDefault(iniPath);
    }
    std::ifstream f{iniPath};
    std::stringstream ss;
    ss << f.rdbuf();
    return parseFromString(ss.str());
}
} // namespace F4DRP::Config
