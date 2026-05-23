#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace F4DRP::Constants {
inline constexpr std::wstring_view kPipePathFormat = L"\\\\?\\pipe\\discord-ipc-{}";
inline constexpr int kPipeIndexMin = 0;
inline constexpr int kPipeIndexMax = 9;

inline constexpr std::string_view kDefaultAppId = "1507704875939790889";
inline constexpr std::string_view kDefaultLargeImage = "fallout4";
inline constexpr std::string_view kDefaultSmallImage = "";
inline constexpr std::string_view kDefaultLargeImageTxt = "Fallout 4";

inline constexpr std::size_t kDiscordDetailsMax = 128;
inline constexpr std::size_t kDiscordStateMax = 128;
inline constexpr std::size_t kDiscordLargeTextMax = 128;
inline constexpr std::size_t kDiscordSmallTextMax = 128;
inline constexpr std::size_t kDiscordButtonLabelMax = 32;

inline constexpr std::size_t kPayloadHeaderBytes = 8;
inline constexpr std::size_t kPayloadMaxBytes = 16 * 1024;

inline constexpr int kRateLimitMaxPerWindow = 5;
inline constexpr int kRateLimitWindowSec = 20;
inline constexpr int kUpdateIntervalFloorSec = 5;
inline constexpr int kUpdateIntervalDefaultSec = 3;
inline constexpr int kHeartbeatIdleSec = 30;
inline constexpr int kHeartbeatTimeoutSec = 5;

inline constexpr std::array<int, 7> kReconnectBackoffSec = {5, 10, 20, 40, 60, 60, 60};

inline constexpr std::string_view kIniFileName = "discord_rich_presence.ini";
inline constexpr std::string_view kTranslationFileName = "discord_rich_presence_translation.ini";
inline constexpr std::string_view kLogFileName = "discord_rich_presence.log";
inline constexpr std::string_view kPluginName = "Discord Rich Presence";
inline constexpr std::string_view kPluginAuthor = "xt0n1";
inline constexpr std::string_view kConflictPluginDll = "Discord_Presence_F4SE_Remake.dll";
inline constexpr std::uint32_t kPluginVersion = 0x0001'0000U;

inline constexpr std::int64_t kMaxCapsToShowDefault = 99'999'999;
inline constexpr std::int64_t kMaxCapsAbsolute = 99'999'999LL;
inline constexpr std::int64_t kMinCapsAbsolute = 0LL;

inline constexpr float kEventStatusDurationDefault = 7.0F;
inline constexpr float kEventStatusDurationMin = 1.0F;
inline constexpr float kEventStatusDurationMax = 100.0F;

inline constexpr std::size_t kCombatTargetSampleMax = 16;

namespace IniSection {
    inline constexpr std::string_view kMain = "Main";
    inline constexpr std::string_view kCustom = "Custom";
    inline constexpr std::string_view kStrings = "Strings";
} // namespace IniSection

namespace IniKey {
    inline constexpr std::string_view kSimplifiedStatus = "bSimplifiedStatus";
    inline constexpr std::string_view kShowPlayTime = "bShowPlayTime";
    inline constexpr std::string_view kShowName = "bShowName";
    inline constexpr std::string_view kShowLvl = "bShowLVL";
    inline constexpr std::string_view kShowCaps = "bShowCaps";
    inline constexpr std::string_view kMaxCapsToShow = "iMaxCapsToShow";
    inline constexpr std::string_view kShowHp = "bShowHP";
    inline constexpr std::string_view kShowEventStatuses = "bShowEventStatuses";
    inline constexpr std::string_view kAllowEventStatusOverride = "bAllowEventStatusOverride";
    inline constexpr std::string_view kEventStatusDuration = "fEventStatusDuration";
    inline constexpr std::string_view kUpdateInterval = "fUpdateInterval";
    inline constexpr std::string_view kDebugMode = "bDebugMode";
    inline constexpr std::string_view kSwapLines = "bSwapLines";
    inline constexpr std::string_view kAppId = "AppID";
    inline constexpr std::string_view kCustomState = "sCustomState";
    inline constexpr std::string_view kCustomDetails = "sCustomDetails";
    inline constexpr std::string_view kCustomLargeImageText = "sCustomLargeImageText";
} // namespace IniKey

namespace StringKey {
    inline constexpr std::string_view kLaunchingGame = "s_T_LaunchingGame";
    inline constexpr std::string_view kMainMenu = "s_T_MainMenu";
    inline constexpr std::string_view kStartedANewGame = "s_T_StartedANewGame";
    inline constexpr std::string_view kPauseMenu = "s_T_PauseMenu";
    inline constexpr std::string_view kSleepWaitMenu = "s_T_SleepWaitMenu";
    inline constexpr std::string_view kPipboyMenu = "s_T_PipboyMenu";
    inline constexpr std::string_view kLevelUpMenu = "s_T_LevelUpMenu";
    inline constexpr std::string_view kDialogueMenu = "s_T_DialogueMenu";
    inline constexpr std::string_view kLockpickingMenu = "s_T_LockpickingMenu";
    inline constexpr std::string_view kTerminalMenu = "s_T_TerminalMenu";
    inline constexpr std::string_view kBarterMenu = "s_T_BarterMenu";
    inline constexpr std::string_view kCrafting = "s_T_Crafting";
    inline constexpr std::string_view kVatsMenu = "s_T_VATSMenu";
    inline constexpr std::string_view kCookingMenu = "s_T_CookingMenu";
    inline constexpr std::string_view kWorkshopMenu = "s_T_WorkshopMenu";
    inline constexpr std::string_view kFighting = "s_T_Fighting";
    inline constexpr std::string_view kExploring = "s_T_Exploring";
    inline constexpr std::string_view kIn = "s_T_In";
    inline constexpr std::string_view kWith = "s_T_With";
    inline constexpr std::string_view kLvl = "s_T_LVL";
    inline constexpr std::string_view kHp = "s_T_HP";
    inline constexpr std::string_view kCaps = "s_T_Caps";
    inline constexpr std::string_view kEventHackedTerminal = "s_Ev_HackedTerminal";
    inline constexpr std::string_view kEventBuiltObject = "s_Ev_BuiltObjectInWorkshop";
} // namespace StringKey

namespace Defaults {
    inline constexpr std::string_view kLaunchingGame = "Launching game";
    inline constexpr std::string_view kMainMenu = "In Main menu";
    inline constexpr std::string_view kStartedANewGame = "Started a new game";
    inline constexpr std::string_view kPauseMenu = "In Pause Menu";
    inline constexpr std::string_view kSleepWaitMenu = "Sleeping";
    inline constexpr std::string_view kPipboyMenu = "In Pipboy Menu";
    inline constexpr std::string_view kLevelUpMenu = "Leveling up";
    inline constexpr std::string_view kDialogueMenu = "Talking";
    inline constexpr std::string_view kLockpickingMenu = "Lockpicking";
    inline constexpr std::string_view kTerminalMenu = "Using terminal";
    inline constexpr std::string_view kBarterMenu = "Trading in";
    inline constexpr std::string_view kCrafting = "Crafting";
    inline constexpr std::string_view kVatsMenu = "In VATS";
    inline constexpr std::string_view kCookingMenu = "Cooking";
    inline constexpr std::string_view kWorkshopMenu = "In Workshop mode";
    inline constexpr std::string_view kFighting = "Fighting";
    inline constexpr std::string_view kExploring = "Exploring";
    inline constexpr std::string_view kIn = "in";
    inline constexpr std::string_view kWith = "with";
    inline constexpr std::string_view kLvl = "LVL";
    inline constexpr std::string_view kHp = "HP";
    inline constexpr std::string_view kCaps = "caps";
    inline constexpr std::string_view kEventHackedTerminal = "Hacked";
    inline constexpr std::string_view kEventBuiltObject = "Built";
} // namespace Defaults
} // namespace F4DRP::Constants
