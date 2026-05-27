#include <filesystem>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "Config/Settings.h"
#include "Presence/PresenceConfig.h"

using F4DRP::Presence::loadPresenceConfig;
using F4DRP::Presence::parsePresenceConfig;
using F4DRP::Presence::PresenceConfig;

TEST_CASE("PresenceConfig: empty INI yields Iconic defaults", "[config]")
{
    const auto c = parsePresenceConfig("");
    REQUIRE(c.fieldName == "{name}");
    REQUIRE(c.fieldLevel == "\xe2\xad\x90 Level {level}");
    REQUIRE(c.fieldHp == "\xf0\x9f\x92\x89 {hp}%");
    REQUIRE(c.fieldSeparator == " \xe2\x80\xa2 ");
    REQUIRE(c.largeImage == "fo4-big");
    REQUIRE(c.largeText == "Fallout 4");
    REQUIRE(c.buttons.empty());
}

TEST_CASE("PresenceConfig: quoted values preserve surrounding spaces", "[config]")
{
    const std::string ini = "[Format]\n"
                            "sFieldName={name} the Survivor\n"
                            "sFieldSeparator=\" | \"\n"
                            "sLocationConnector=\" roaming \"\n"
                            "[Images]\n"
                            "sLargeImage=my-logo\n"
                            "sIconCombat=icon_combat\n";
    const auto c = parsePresenceConfig(ini);
    REQUIRE(c.fieldName == "{name} the Survivor");
    REQUIRE(c.fieldSeparator == " | ");
    REQUIRE(c.locationSeparatorVerbose == " roaming ");
    REQUIRE(c.largeImage == "my-logo");
    REQUIRE(c.iconCombat == "icon_combat");
    REQUIRE(c.iconExploring.empty());
}

TEST_CASE("PresenceConfig: buttons require both label and url", "[config]")
{
    const std::string ini = "[Buttons]\n"
                            "sButton1Label=Get the mod\n"
                            "sButton1Url=https://example.com\n"
                            "sButton2Label=No URL\n";
    const auto c = parsePresenceConfig(ini);
    REQUIRE(c.buttons.size() == 1);
    REQUIRE(c.buttons[0].label == "Get the mod");
    REQUIRE(c.buttons[0].url == "https://example.com");
}

TEST_CASE("PresenceConfig: generated default INI round-trips with spaces intact", "[config]")
{
    const auto path = std::filesystem::temp_directory_path() / "f4drp_test_default.ini";
    std::filesystem::remove(path);
    REQUIRE(F4DRP::Config::writeDefault(path));

    const auto c = loadPresenceConfig(path);
    REQUIRE(c.fieldSeparator == " \xe2\x80\xa2 ");
    REQUIRE(c.locationSeparatorVerbose == " in ");
    REQUIRE(c.fieldName == "{name}");
    REQUIRE(c.largeImage == "fo4-big");

    std::filesystem::remove(path);
}
