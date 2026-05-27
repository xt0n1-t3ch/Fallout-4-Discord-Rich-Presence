# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
