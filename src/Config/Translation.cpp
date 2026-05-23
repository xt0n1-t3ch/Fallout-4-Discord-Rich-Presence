#include "Config/Translation.h"

#include <fstream>
#include <sstream>

#include <SimpleIni.h>

#include "Constants.h"

namespace F4DRP::Config {
Translation Translation::fromString(const std::string& iniContents)
{
    CSimpleIniA ini;
    ini.SetUnicode(true);
    ini.LoadData(iniContents.data(), iniContents.size());

    Translation t;
    const auto section = std::string{Constants::IniSection::kStrings};

    CSimpleIniA::TNamesDepend keys;
    ini.GetAllKeys(section.c_str(), keys);
    for (const auto& key : keys) {
        const char* value = ini.GetValue(section.c_str(), key.pItem, "");
        if (value != nullptr && value[0] != '\0') {
            t.m_entries.emplace(key.pItem, value);
        }
    }
    return t;
}

Translation Translation::loadOrEmpty(const std::filesystem::path& iniPath)
{
    if (!std::filesystem::exists(iniPath)) {
        writeDefault(iniPath);
        return Translation{};
    }
    std::ifstream f{iniPath};
    if (!f.is_open()) {
        return Translation{};
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return fromString(ss.str());
}

bool Translation::writeDefault(const std::filesystem::path& iniPath)
{
    CSimpleIniA ini;
    ini.SetUnicode(true);

    const auto section = std::string{Constants::IniSection::kStrings};
    const auto put = [&](std::string_view k) { ini.SetValue(section.c_str(), std::string{k}.c_str(), ""); };
    put(Constants::StringKey::kLaunchingGame);
    put(Constants::StringKey::kMainMenu);
    put(Constants::StringKey::kStartedANewGame);
    put(Constants::StringKey::kPauseMenu);
    put(Constants::StringKey::kSleepWaitMenu);
    put(Constants::StringKey::kPipboyMenu);
    put(Constants::StringKey::kLevelUpMenu);
    put(Constants::StringKey::kDialogueMenu);
    put(Constants::StringKey::kLockpickingMenu);
    put(Constants::StringKey::kTerminalMenu);
    put(Constants::StringKey::kBarterMenu);
    put(Constants::StringKey::kCrafting);
    put(Constants::StringKey::kVatsMenu);
    put(Constants::StringKey::kCookingMenu);
    put(Constants::StringKey::kWorkshopMenu);
    put(Constants::StringKey::kFighting);
    put(Constants::StringKey::kExploring);
    put(Constants::StringKey::kIn);
    put(Constants::StringKey::kWith);
    put(Constants::StringKey::kLvl);
    put(Constants::StringKey::kHp);
    put(Constants::StringKey::kCaps);
    put(Constants::StringKey::kEventHackedTerminal);
    put(Constants::StringKey::kEventBuiltObject);

    std::error_code ec;
    std::filesystem::create_directories(iniPath.parent_path(), ec);
    return ini.SaveFile(iniPath.string().c_str(), false) == SI_OK;
}

std::string_view Translation::get(std::string_view key, std::string_view fallback) const
{
    auto it = m_entries.find(std::string{key});
    if (it == m_entries.end() || it->second.empty()) {
        return fallback;
    }
    return it->second;
}
} // namespace F4DRP::Config
