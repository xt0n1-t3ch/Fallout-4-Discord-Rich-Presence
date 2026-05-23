# Discord Rich Presence F4SE NG (cross-gen)

Single-DLL Discord Rich Presence for **Fallout 4**, designed from the ground up to load on **both** old-gen (1.10.163) **and** the entire next-gen family (1.10.984 / 1.11.169 / 1.11.191) without rebuilding.

Clean rewrite of [TommInfinite's REMAKE (Nexus 77771)](https://www.nexusmods.com/fallout4/mods/77771) — same INI surface, repaired plugin-version metadata so F4SE 0.7.x loads it on NG, modern Discord IPC protocol per [discord-api-docs commit 86d1f28](https://github.com/discord/discord-api-docs/commit/86d1f28) (April 2026).

## Features

- Menu state (Pip-Boy, workshop, terminal, lockpick, level-up, dialogue, VATS, cooking, crafting, barter, sleep, pause, main).
- Player character name, level, HP %, caps (each toggleable).
- Exterior location names (cell + worldspace + encounter zone), interior cell name.
- Combat: top hostile target name.
- Event short-statuses with override + duration controls (`Hacked terminal`, `Built object in workshop`).
- Play-time elapsed.
- Custom AppID + custom assets text override.
- Side-file translation INI — every visible string overrideable; empty key falls back to built-in English.
- Discord-absent / web-client / killed-mid-session: never crashes the game, reconnects automatically.
- Discord rate-limit safe (5-frame/20-second sliding window + state-diff coalescing).

## Requirements

- Fallout 4 on Steam (Windows, x64). One of:
  - 1.10.163 (old-gen anchor)
  - 1.10.984 (NG, May 2024)
  - 1.11.169 (NG hotfix series)
  - 1.11.191 (NG latest as of May 2026)
- [F4SE](https://f4se.silverlock.org/) matching your runtime:
  - 0.6.23 for 1.10.163
  - 0.7.2 for 1.10.984
  - 0.7.6 for 1.11.169
  - 0.7.7 for 1.11.191
- [Address Library for F4SE Plugins (Nexus 47327)](https://www.nexusmods.com/fallout4/mods/47327) — install the **AIO** to cover every runtime in one shot.
- Discord desktop client running (web client does NOT expose RPC).
- Discord setting: User Settings → Activity Privacy → "Share your detected activities with others" **ON**.

## Install

### Vortex / MO2

Install via the FOMOD or drop `dist\` contents into your Fallout 4 game data:

```
<Fallout 4>\Data\F4SE\Plugins\Discord_Presence_F4SE_NG.dll
<Fallout 4>\Data\F4SE\Plugins\Discord_Presence_F4SE_NG.ini
<Fallout 4>\Data\F4SE\Plugins\Discord_Presence_F4SE_NG_Translation.ini
```

### Manual

Unzip the release into your `Fallout 4` folder so the DLL ends up at `Data\F4SE\Plugins\Discord_Presence_F4SE_NG.dll`. On first launch the plugin regenerates the INI files at the same path.

## Conflict with the original Nexus 77771 mod

If you previously installed `Discord_Presence_F4SE_Remake.dll`, **delete it** from `Data\F4SE\Plugins\`. Our plugin detects the conflicting module at load time and refuses to send Discord frames while both DLLs are loaded; a warning is written to the log.

## INI keys (`Discord_Presence_F4SE_NG.ini`)

### `[Main]`

| Key | Default | Notes |
|:---|:---:|:---|
| `bSimplifiedStatus` | 1 | Drops prepositions for shorter location strings |
| `bShowPlayTime` | 1 | Show elapsed session time |
| `bShowName` | 1 | Show character name |
| `bShowLVL` | 1 | Show character level |
| `bShowCaps` | 1 | Show caps |
| `iMaxCapsToShow` | 99999999 | Clamp caps display (0-99,999,999) |
| `bShowHP` | 1 | Show HP percentage |
| `bShowEventStatuses` | 1 | Surface short event lines |
| `bAllowEventStatusOverride` | 0 | Newer event can preempt active one |
| `fEventStatusDuration` | 7 | Seconds (1.0-100.0) |
| `fUpdateInterval` | 5 | Seconds, floor 5 (Discord rate limit) |
| `bDebugMode` | 0 | Verbose logging |
| `bSwapLines` | 0 | Swap details/state lines |
| `AppID` | empty | Custom Discord application ID (17-21 digits) |

### `[Custom]`

| Key | Notes |
|:---|:---|
| `sCustomState` | Override the entire state line |
| `sCustomDetails` | Override the entire details line |
| `sCustomLargeImageText` | Override Discord large-image hover text |

## Translation (`Discord_Presence_F4SE_NG_Translation.ini`)

Every visible string is overrideable via the `[Strings]` section. Empty value = use built-in English default. Translations from Nexus 77771 are NOT compatible (key names changed).

```ini
[Strings]
s_T_PipboyMenu = En la Pip-Boy
s_T_LVL = NIVEL
s_T_HP = SALUD
```

## Logging

The log lives at `Documents\My Games\Fallout4\F4SE\Discord_Presence_F4SE_NG.log`. Enable `bDebugMode=1` for verbose output.

## Building from source

```powershell
git clone https://github.com/xt0n1/Fallout4-Discord-Rich-Presence-NG
cd 'Fallout4-Discord-Rich-Presence-NG'
git clone --depth 1 https://github.com/microsoft/vcpkg.git "$env:USERPROFILE\vcpkg"
& "$env:USERPROFILE\vcpkg\bootstrap-vcpkg.bat" -disableMetrics
$env:VCPKG_ROOT = "$env:USERPROFILE\vcpkg"

cmake --preset flat-ng-release
cmake --build --preset flat-ng-release
```

### Run tests

```powershell
cmake --preset flat-ng-debug
cmake --build --preset flat-ng-debug
ctest --preset flat-ng-debug --output-on-failure
```

### Live Discord integration test (E2E)

Make sure the Discord desktop client is running, then enable the integration target:

```powershell
cmake --preset flat-ng-debug -DF4DRP_LIVE_DISCORD=ON
cmake --build --preset flat-ng-debug
ctest --preset flat-ng-debug --output-on-failure -R integration
```

## Architecture

```
src/
  Plugin/      F4SE entrypoints + version data (compatibleVersions: 1.10.163, 1.10.984, 1.11.169, 1.11.191)
  Discord/     Pipe RAII + frame protocol + state machine + rate limiter (no SDK)
  Game/        CommonLibF4-bound state pollers (MenuTracker, PlayerSnapshot, Location, CombatTracker, EventTracker)
  Config/      INI loader + translation side-file (SimpleIni)
  Mapper/      Pure GameState -> PresenceState transform
  Util/        spdlog logger + FNV-1a hash + monotonic play-time accumulator
  Constants.h  Single canonical sink for every literal in the project
tests/
  unit/        Catch2 — hash, playtime, protocol, ratelimiter, settings, translation, mapper
  contract/    Golden Discord JSON frames vs. nlohmann_json output
  integration/ Live Discord IPC pipe round-trip (gated by F4DRP_LIVE_DISCORD)
```

## License

MIT. See [LICENSE](LICENSE).

## Credits

- TommInfinite — original Discord RP REMAKE (Nexus 77771) for design parity
- Geluxrum, libxse/CommonLibF4, alandtse/CommonLibF4 — F4SE / Address Library ecosystem
- ianpatt, behippo, purplelunchbox — F4SE
- Ryan-rsm-McKenzie — CommonLibF4 upstream
