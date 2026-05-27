# Test suite

Run offline (no game, no Discord needed):

```bat
tools\build-flat-ng-debug.bat
tools\ctest-flat-ng-debug.bat
```

The presentation pipeline (config → templates → composer → Discord payload) is covered end to
end here, so layout and status changes are verified without opening the game. Only the live
engine-read layer (`EngineGameReader`, the `Main::Update` hook, the `g_UI` resolver) needs an
in-game check. The four `[live]` Discord tests skip automatically unless a desktop client is
running.

| Test | Covers |
|:---|:---|
| [unit/test_template.cpp](unit/test_template.cpp) | `{token}` substitution, unknown/empty tokens, `joinNonEmpty` separator + empty-segment collapse |
| [unit/test_composer.cpp](unit/test_composer.cpp) | Every status (lifecycle, menus, combat, event, exploring, generic menu), field toggles, swap, custom overrides, icon selection |
| [unit/test_presenceconfig.cpp](unit/test_presenceconfig.cpp) | INI parse of `[Format]`/`[Images]`/`[Buttons]`, quoted-space round-trip of the generated default |
| [unit/test_mapper.cpp](unit/test_mapper.cpp) | Legacy Mapper formatting (kept until the plugin is wired to the Composer) |
| [unit/test_menucatalog.cpp](unit/test_menucatalog.cpp) | Menu name → kind classification, label lookup, show-location matrix |
| [unit/test_settings.cpp](unit/test_settings.cpp) | `[Main]`/`[Custom]` parse, defaults, clamping, AppID validation |
| [unit/test_translation.cpp](unit/test_translation.cpp) | Side-file string overrides with English fallback |
| [unit/test_protocol.cpp](unit/test_protocol.cpp) | Frame encode/decode, SET_ACTIVITY payload (assets, timestamps, buttons), clamp, hash |
| [unit/test_ratelimiter.cpp](unit/test_ratelimiter.cpp) | 5-per-20s sliding window |
| [unit/test_playtime.cpp](unit/test_playtime.cpp) | Elapsed accumulation across pause/resume |
| [unit/test_hash.cpp](unit/test_hash.cpp) | FNV-1a presence hashing |
| [unit/test_dispatch.cpp](unit/test_dispatch.cpp) | Update coalescing / state-diff dispatch |
| [contract/test_frames.cpp](contract/test_frames.cpp) | Discord IPC frame wire contract |
| [integration/test_pipe.cpp](integration/test_pipe.cpp) | `[live]` end-to-end pipe + handshake against a running Discord client |
