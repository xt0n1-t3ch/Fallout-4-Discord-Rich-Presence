#include "Presence/PresenceConfig.h"

#include <fstream>
#include <sstream>

#include <SimpleIni.h>

namespace F4DRP::Presence {
namespace {
    using namespace F4DRP::Constants;

    std::string value(const CSimpleIniA& ini, std::string_view section, std::string_view key, std::string_view fallback)
    {
        const char* raw = ini.GetValue(std::string{section}.c_str(), std::string{key}.c_str(), nullptr);
        return raw != nullptr ? std::string{raw} : std::string{fallback};
    }

    void readButton(const CSimpleIniA& ini,
                    std::string_view labelKey,
                    std::string_view urlKey,
                    std::vector<Button>& out)
    {
        const auto section = std::string{IniSection::kButtons};
        const char* label = ini.GetValue(section.c_str(), std::string{labelKey}.c_str(), "");
        const char* url = ini.GetValue(section.c_str(), std::string{urlKey}.c_str(), "");
        if (label != nullptr && url != nullptr && label[0] != '\0' && url[0] != '\0') {
            out.push_back(Button{label, url});
        }
    }
} // namespace

PresenceConfig parsePresenceConfig(const std::string& iniContents)
{
    CSimpleIniA ini;
    ini.SetUnicode(true);
    ini.SetQuotes(true);
    ini.LoadData(iniContents.data(), iniContents.size());

    PresenceConfig c;
    const auto fmt = IniSection::kFormat;
    const auto img = IniSection::kImages;

    c.fieldName = value(ini, fmt, IniKey::kFieldName, PresenceDefaults::kFieldName);
    c.fieldLevel = value(ini, fmt, IniKey::kFieldLevel, PresenceDefaults::kFieldLevel);
    c.fieldHp = value(ini, fmt, IniKey::kFieldHp, PresenceDefaults::kFieldHp);
    c.fieldCaps = value(ini, fmt, IniKey::kFieldCaps, PresenceDefaults::kFieldCaps);
    c.fieldSeparator = value(ini, fmt, IniKey::kFieldSeparator, PresenceDefaults::kFieldSeparator);
    c.locationSeparatorSimplified =
        value(ini, fmt, IniKey::kLocationSeparatorSimplified, PresenceDefaults::kLocationSeparatorSimplified);
    c.locationSeparatorVerbose =
        value(ini, fmt, IniKey::kLocationSeparatorVerbose, PresenceDefaults::kLocationSeparatorVerbose);

    c.largeImage = value(ini, img, IniKey::kLargeImage, PresenceDefaults::kLargeImage);
    c.largeText = value(ini, img, IniKey::kLargeText, PresenceDefaults::kLargeText);
    c.iconExploring = value(ini, img, IniKey::kIconExploring, "");
    c.iconCombat = value(ini, img, IniKey::kIconCombat, "");
    c.iconMenu = value(ini, img, IniKey::kIconMenu, "");
    c.iconMainMenu = value(ini, img, IniKey::kIconMainMenu, "");
    c.iconLoading = value(ini, img, IniKey::kIconLoading, "");

    readButton(ini, IniKey::kButton1Label, IniKey::kButton1Url, c.buttons);
    readButton(ini, IniKey::kButton2Label, IniKey::kButton2Url, c.buttons);
    return c;
}

PresenceConfig loadPresenceConfig(const std::filesystem::path& iniPath)
{
    if (!std::filesystem::exists(iniPath)) {
        return PresenceConfig{};
    }
    std::ifstream f{iniPath};
    std::stringstream ss;
    ss << f.rdbuf();
    return parsePresenceConfig(ss.str());
}
} // namespace F4DRP::Presence
