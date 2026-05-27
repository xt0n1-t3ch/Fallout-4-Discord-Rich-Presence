# Configuration

Everything the plugin shows is driven by one INI created on first launch at
`Data\F4SE\Plugins\discord_rich_presence.ini` (under MO2: `overwrite\F4SE\Plugins\`). Edit it,
save, reload the game — no rebuild. Values with leading or trailing spaces must be wrapped in
quotes (`sFieldSeparator=" | "`) so the spaces survive.

## `[Main]` — behaviour toggles

| Key | Default | Meaning |
|:---|:---|:---|
| `bSimplifiedStatus` | `1` | Use the compact location separator instead of the verbose connector |
| `bShowPlayTime` | `1` | Show elapsed session time on the Discord card |
| `bShowName` | `1` | Include the player character name |
| `bShowLVL` | `1` | Include the level |
| `bShowHP` | `1` | Include the health percent |
| `bShowCaps` | `1` | Include the caps count |
| `iMaxCapsToShow` | `9999` | Clamp caps above this and append `+` |
| `bShowEventStatuses` | `1` | Show short-lived event statuses (hacked terminal, built object) |
| `fEventStatusDuration` | `7` | Seconds an event status stays up |
| `fUpdateInterval` | `3` | Seconds between Discord pushes |
| `bSwapLines` | `0` | Swap the details and state lines |
| `bDebugMode` | `0` | Verbose `[DIAG]` logging |
| `AppID` | empty | Your own Discord application id (needed for custom images) |

## `[Format]` — layout templates

The details line is the enabled fields rendered through their templates and joined by
`sFieldSeparator`. The state line appends the location through the active separator.

| Key | Default | Meaning |
|:---|:---|:---|
| `sFieldName` | `{name}` | Name field template |
| `sFieldLevel` | `LVL {level}` | Level field template |
| `sFieldHP` | `{hp}% HP` | Health field template |
| `sFieldCaps` | `{caps} caps` | Caps field template |
| `sFieldSeparator` | `" • "` | Joins the detail fields |
| `sLocationSeparator` | `" • "` | Location join when `bSimplifiedStatus=1` |
| `sLocationConnector` | `" in "` | Location join when `bSimplifiedStatus=0` |

Tokens usable in any template: `{name}` `{level}` `{hp}` `{caps}` `{location}` `{enemy}`
`{menu}` `{event}`. An empty token collapses and its surrounding separator is dropped.

## `[Images]` — large image and per-status icons

`sLargeImage` (default `fo4-big`) and `sLargeText` (default `Fallout 4`) set the big artwork.
The small-image icon changes per status: `sIconExploring`, `sIconCombat`, `sIconMenu`,
`sIconMainMenu`, `sIconLoading`. Each value is an asset key uploaded to your Discord app
(empty = no icon). See the asset setup notes when you bring your own `AppID`.

## `[Buttons]` — clickable links

Up to two `{label, url}` pairs shown on your profile to other users. A pair is used only when
both its label and url are set.

| Key | Meaning |
|:---|:---|
| `sButton1Label` / `sButton1Url` | First button |
| `sButton2Label` / `sButton2Url` | Second button |

## `[Custom]` — hard overrides

`sCustomDetails`, `sCustomState` and `sCustomLargeImageText` replace the computed values when
set.

## `[Strings]` — translation

Every visible label (status words, menu names, event verbs) is overridable here; an empty value
falls back to built-in English.
