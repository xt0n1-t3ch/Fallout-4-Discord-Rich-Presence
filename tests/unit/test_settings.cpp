#include <catch2/catch_test_macros.hpp>

#include "Config/Settings.h"
#include "Constants.h"

TEST_CASE("Settings parses INI with all parity keys", "[settings][format]")
{
    const std::string ini = R"([Main]
bSimplifiedStatus=0
bShowPlayTime=1
bShowName=0
bShowLVL=1
bShowCaps=0
iMaxCapsToShow=5000
bShowHP=1
bShowEventStatuses=1
bAllowEventStatusOverride=1
fEventStatusDuration=10.5
fUpdateInterval=8
bDebugMode=1
bSwapLines=1
AppID=12345678901234567

[Custom]
sCustomState=custom-state
sCustomDetails=custom-details
sCustomLargeImageText=hover-text
)";
    auto s = F4DRP::Config::parseFromString(ini);
    REQUIRE(s.simplifiedStatus == false);
    REQUIRE(s.showPlayTime == true);
    REQUIRE(s.showName == false);
    REQUIRE(s.maxCapsToShow == 5000);
    REQUIRE(s.eventStatusDuration == 10.5F);
    REQUIRE(s.updateInterval == 8.0F);
    REQUIRE(s.debugMode);
    REQUIRE(s.swapLines);
    REQUIRE(s.appId == "12345678901234567");
    REQUIRE(s.customState == "custom-state");
    REQUIRE(s.customDetails == "custom-details");
    REQUIRE(s.customLargeImageText == "hover-text");
}

TEST_CASE("Settings clamps updateInterval below floor", "[settings][boundary]")
{
    const std::string ini = "[Main]\nfUpdateInterval=1\n";
    auto s = F4DRP::Config::parseFromString(ini);
    REQUIRE(s.updateInterval == static_cast<float>(F4DRP::Constants::kUpdateIntervalFloorSec));
}

TEST_CASE("Settings clamps eventStatusDuration to range", "[settings][boundary]")
{
    auto s = F4DRP::Config::parseFromString("[Main]\nfEventStatusDuration=999\n");
    REQUIRE(s.eventStatusDuration == F4DRP::Constants::kEventStatusDurationMax);
    auto s2 = F4DRP::Config::parseFromString("[Main]\nfEventStatusDuration=0\n");
    REQUIRE(s2.eventStatusDuration == F4DRP::Constants::kEventStatusDurationMin);
}

TEST_CASE("Settings rejects non-digit AppID", "[settings][format][null-empty]")
{
    auto s = F4DRP::Config::parseFromString("[Main]\nAppID=abcdefghijklmnopq\n");
    REQUIRE(s.appId.empty());
}

TEST_CASE("Settings rejects too-short AppID", "[settings][format][boundary]")
{
    auto s = F4DRP::Config::parseFromString("[Main]\nAppID=123\n");
    REQUIRE(s.appId.empty());
}

TEST_CASE("Settings empty INI falls to defaults", "[settings][null-empty]")
{
    auto s = F4DRP::Config::parseFromString("");
    REQUIRE(s.simplifiedStatus);
    REQUIRE(s.showName);
    REQUIRE(s.eventStatusDuration == F4DRP::Constants::kEventStatusDurationDefault);
}
