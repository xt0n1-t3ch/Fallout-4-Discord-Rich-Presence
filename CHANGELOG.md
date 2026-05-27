# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-05-27

Polish release. Adds a dedicated Pip-Boy icon slot, a dedicated VATS slot that wins over the
combat branch, refreshed art across the seven small images and the large image, plus repo
hygiene (sponsor button, llms.txt, art preview cleanup).

### Added

- `[Images] sIconPipboy` key: the Pip-Boy menu now shows its own small image instead of the
  generic menu icon. Falls back to `sIconMenu` when empty.
- `[Images] sIconVats` key: entering VATS shows the VATS small image and `In VATS` label,
  taking priority over the combat branch so the icon actually appears mid-fight.
- Pre-populated `[Images]` defaults (`icon_explore`, `icon_combat`, `icon_menu`, `icon_pipboy`,
  `icon_vats`, `icon_mainmenu`, `icon_loading`) so a fresh INI renders out of the box once the
  user uploads the assets to their own Discord application. An explicit empty key still disables
  that small image.
- `.github/FUNDING.yml` enables the repository sponsor button (GitHub Sponsors, Ko-fi,
  Buy Me a Coffee).
- `llms.txt` at the repository root — spec-compliant index for LLM-driven retrieval.

### Changed

- Refreshed art set: large image is the T-60 power armor helmet; combat is the Vault Boy tommy
  gun; explore is the Red Rocket gas station; menu is the Vault Boy statue; Pip-Boy is the
  Pip-Boy 3000 Mark IV device alone on transparent background; VATS is the Vault Boy VATS
  illustration; main menu is the Vault 111 door; loading is the Pip-Boy loading screen art.
- README adds a sponsor badge and a Support section.

## [1.0.0] - 2026-05-26

First public release: a single F4SE DLL that runs on Fallout 4 old-gen and next-gen, with the
whole presence layout driven from one INI.

### Added

- Next-gen support (1.10.984 / 1.11.169 / 1.11.191) on a single DLL, reverse-engineered from the
  binary after the Address Library renumbered the singleton IDs.
- `{token}` template engine: details and state lines are rendered from per-field templates in the
  INI `[Format]` section, so the layout, separators and labels are editable without a rebuild.
- Per-status small-image icons (`[Images]`) and up to two profile buttons (`[Buttons]`).
- Full status set: lifecycle, all eleven menus, exterior/interior location, combat target, and
  timed event statuses, each label overridable through the translation side-file.
- Centralized `[DIAG]` diagnostics: an address-resolution map and per-push decision line in the log.
- Reproducible offline reverse-engineering scanners under `tools/re`.

### Fixed

- Next-gen load failure: the UI singleton is resolved by an exact `RE::UI` vtable match instead of
  the renumbered ID that misses on 1.11.x.
- Engine crash on capture: player, location and combat names are read without ScrapHeap allocation,
  which had been corrupting the heap and crashing in engine culling.
- Freeze on load: `GetMenuOpen` is called only on the confirmed UI pointer, removing an
  SEH-over-lock deadlock that leaked the menu-map read lock.
- Stuck presence: a per-frame main-thread tick is installed at the `Main::Update` call site, since
  the F4SE task interface is jobified on next-gen.

[1.0.0]: https://github.com/xt0n1-t3ch/Fallout-4-Discord-Rich-Presence/releases/tag/v1.0.0
