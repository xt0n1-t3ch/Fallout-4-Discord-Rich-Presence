# Custom Discord app and icons

Out of the box the plugin uses the author's Discord application and its `fo4-big` large image.
To show your **own** large image or **per-status small icons**, create your own Discord
application, upload the art as assets, and point the plugin at it with `AppID`.

## 1. Create the application

1. Go to the [Discord Developer Portal](https://discord.com/developers/applications) → **New
   Application**. Name it whatever should appear as the game title (for example `Fallout 4`).
2. Copy the **Application ID** (Settings → General Information). Put it in the INI:

   ```ini
   [Main]
   AppID = your-application-id
   ```

## 2. Upload the art assets

Under **Rich Presence → Art Assets**, add each image and name the key exactly as you reference it
in the INI. Discord requires raster images (PNG, at least 512×512).

| INI key (`[Images]`) | Suggested asset key | Shown when |
|:---|:---|:---|
| `sLargeImage` | `fo4-big` | Always (the big artwork) |
| `sIconExploring` | `icon_explore` | Roaming the world |
| `sIconCombat` | `icon_combat` | In combat |
| `sIconMenu` | `icon_menu` | Any menu / Pip-Boy |
| `sIconMainMenu` | `icon_mainmenu` | At the main menu |
| `sIconLoading` | `icon_loading` | Launching / loading |

```ini
[Images]
sLargeImage   = fo4-big
sIconCombat   = icon_combat
sIconExploring = icon_explore
```

Leave an icon key empty to omit its small image. Asset names take a few minutes to propagate after
upload.

## 3. Buttons (optional)

```ini
[Buttons]
sButton1Label = Get the mod
sButton1Url   = https://www.nexusmods.com/fallout4/mods/77771
```

Buttons appear to other people viewing your profile, not in your own client. A button is used only
when both its label and url are set; at most two are sent.
