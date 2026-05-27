# v0.2 ship-fail diagnosis (disciplined loop)

> Methodology: `/diagnose` skill, phases 1-4. Phase 5-6 (fix + cleanup) happen in `/speckit-sdd`. This document is the falsifiable evidence ledger that the spec must address.

## Phase 1 — Feedback loop

| Tier | Loop | Cost | Determinism | Use |
|:---|:---|:---:|:---:|:---|
| **L0 — static differential** | Read v0.2 source vs reference source side-by-side; flag divergences | seconds | 100% | architectural divergence catalogue (this doc) |
| **L1 — golden-string Mapper test** | Construct realistic `GameState` (Vore / LVL 79 / HP 1.00 / 6234 caps / "Sanctuary Hills" / etc.) and assert `mapGameStateToPresence` returns reference's exact strings | ms | 100% | mapper-shape regression — already buildable today, blocked only by missing test fixture |
| **L2 — debug-log capture** | Ship DLL with `bDebugMode=1`, ask Tony to `f4se_loader.exe` + 30s of gameplay, ship the `discord_rich_presence.log` back | minutes | 100% | proves whether `tickOnMainSafe` fires, whether `capture*` snapshots return valid data, whether AddTask lambda is even invoked |
| **L3 — in-game smoke** | Tony deploys DLL, launches Fallout 4, screenshots Discord profile in 5 states (launching, main menu, exterior, combat, pipboy) | 5 min | 100% | end-to-end acceptance gate |

**The loop I can run autonomously**: L0 + L1. They're sufficient for diagnosing 3 of the 4 failure classes (format, asset-layer, init-sequence). The 4th (state-capture working at runtime) requires L2 at minimum.

**L2 prerequisite**: change `dist/discord_rich_presence.ini` to `bDebugMode = 1` in the next build Tony ships, so the log exists.

## Phase 2 — Reproduction

Already reproduced — Tony's screenshot:

```
[?]  PLAYING A GAME
     Fallout 4
     Exploring
     HP 0% | 0 caps
     0:05 elapsed
```

Symptom decomposition:
- `[?]` icon (generic question mark) instead of a Fallout-themed image
- `Exploring` — verb only, no `<location>` appended
- `HP 0% | 0 caps` — stats present in the bottom line BUT name and level missing, and HP/caps are zero
- `0:05` elapsed — Discord IS receiving a `startTimestamp` field; some presence frame IS being delivered

## Phase 3 — Hypotheses (ranked, falsifiable)

### Failure class A — Icon shows generic question mark (asset / appID)

| # | Hypothesis | Falsifiable prediction | Confidence |
|:---:|:---|:---|:---:|
| A1 | Default app ID `1507704875939790889` is Tony's own app and has 0 assets uploaded, while we send `largeImageKey="fallout4"` → key resolves to nothing → Discord falls back to the generic placeholder | If we either (a) change default AppID to TommInfinite's `903933837514518548` (whose `fo4-big` asset is present), or (b) Tony uploads an asset named `fallout4` to his app `1507704875939790889`, the icon should appear within one update interval | **CONFIRMED via L0 + browser-verified screenshot** ([research/nexus-screens reference](../research/nexus-screens/), `C:\Users\xt0n1\.playwright-mcp\output\discord-app-assets.jpeg` showing 0 of 300 assets) |
| A2 | Discord intentionally hides the icon during pre-handshake / first-frame-empty edge case | Would persist even after assets uploaded | Discarded — A1 explains it fully |

**Verdict A**: A1 is the cause. Two-line fix: change `Constants::kDefaultAppId` and `Constants::kDefaultLargeImage`. Recommend defaulting to TommInfinite's working app (`903933837514518548` + `fo4-big`) so the plugin works out-of-the-box, and document the override path for users wanting custom branding.

### Failure class B — `HP 0% | 0 caps` (GameState empty at Mapper time)

| # | Hypothesis | Falsifiable prediction | Confidence |
|:---:|:---|:---|:---:|
| B1 | `F4SE::GetTaskInterface()->AddTask(lambda)` from worker thread never invokes the lambda on the main thread → `captureOnMain()` never runs → GameState stays default-constructed → Mapper gets empty input every tick | Log line `capture player: valid=…` would be **missing entirely** from the debug log. L2 capture would confirm by absence. | **HIGH** — the reference impl ([research/reference-source/main.cpp](../research/reference-source/main.cpp) lines 633-650) explicitly uses a trampoline hook (`BGSAnimSoundStateManager::Update` at `REL::ID(1426208)+0x31`) and **not** TaskInterface, which suggests the author hit the same problem and routed around it. Without L2 we can't 100% prove it's *this* failure mode vs B2, but every adjacent piece of evidence points here |
| B2 | TaskInterface lambda DOES fire but `RE::PlayerCharacter::GetSingleton()` returns null when invoked, causing `capturePlayerSnapshot()` to early-return at line 44-45 of [src/Game/PlayerSnapshot.cpp](../src/Game/PlayerSnapshot.cpp) with `valid=false` → Mapper sees no name, level=0, etc. | Log line `capture player: valid=false, name='', lvl=0, hp=0.00, caps=0` repeated every tick. L2 capture would distinguish B1 vs B2. | MEDIUM — possible but unlikely after `kGameDataReady` has fired, because `GetSingleton()` should be stable thereafter |
| B3 | Lambda fires, GetSingleton succeeds, `GetDisplayFullName()` returns null/empty (the reference uses `GetReferenceName()` instead — line 261 of reference main.cpp) → name string stays empty → upstream concat skips the name | Log line `capture player: valid=true, name='', lvl=79, …` — name empty but other fields populated | LOW — but it's a one-line API divergence worth noting |
| B4 | Lambda fires, captures correctly, but `engineReady` atomic never flips because the install-sinks `kInputLoaded` task itself was the one that crashed (SEH-caught silently) → `timerLoop` never submits tick tasks | Log line `engine ready — sinks installed on main thread` would be MISSING; `installSinksOnMain caught SEH` would be present | LOW — even if sinks fail, `timerRunning` independently starts at `kGameDataReady`. Wait — actually reading [src/Plugin/Main.cpp](../src/Plugin/Main.cpp) lines 166-167: the `timerLoop` while-body gates on `engineReady && !gameLoading`. If sinks fail, `engineReady` stays false, and the timer thread just sleeps in `kIdleSleep` forever. So B4 is actually MEDIUM-HIGH |

**Verdict B**: B1 most likely, B4 next most likely, both addressed by switching to the trampoline-hook pattern (which obviates `engineReady` because the hook is installed before kGameDataReady and runs whether sinks are alive or not). L2 capture would confirm.

**Critical secondary finding inside class B**: even if capture worked, the Mapper itself produces wrong output:
- [src/Mapper/Mapper.cpp](../src/Mapper/Mapper.cpp) line 64-71 `buildLocationLabel`: when `locationName` is non-empty AND `simplifiedStatus=true`, returns only the location, NOT `Exploring <location>` (drops the "Exploring " prefix entirely). The reference always prepends the verb.
- Same function never emits `Exploring | <location>` — the simplified separator only applies in the menu/combat paths, not the exploration path.
- Mapper line 143-144 assigns `p.details = detailsLine` (which holds the verb/event/combat/menu line) and `p.state = stateLine` (which holds Name|LVL|HP|caps). The reference assigns the opposite by default (`bSwapLines=false`): `discordPresence.state = GetPlayerState()` (verb), `discordPresence.details = GetPlayerInfo()` (stats). **Our default IS the reference's swapped mode.** This means even when GameState is populated, the lines appear in the wrong slots on Discord's card.

These three Mapper bugs are NOT diagnosed away by fixing B1 — they're independent and require explicit fixes.

### Failure class C — No "In Main Menu" / no "Launching game" on game launch

| # | Hypothesis | Falsifiable prediction | Confidence |
|:---:|:---|:---|:---:|
| C1 | `Discord::Client::start()` is called at `F4SEPlugin_Load` but never `update(...)` is called until the timer thread fires its first tick (which requires `engineReady` AND `!gameLoading` AND a populated `kGameDataReady`) → for the multi-second window between F4 launch and first valid presence, Discord shows nothing | An L2 log would show `discord ready: {"cmd":"DISPATCH",…}` (handshake succeeded) but NO `discord set_activity sent hash=…` line until kGameDataReady arrives. After that point, if B1/B4 are also active, the set_activity may never fire. | **HIGH** — confirmed by reading [src/Plugin/Main.cpp](../src/Plugin/Main.cpp): the only call site of `runtime().discord.update(...)` is inside `tickOnMainImpl` (line 113), which only runs if the lambda fires |
| C2 | Main-menu state IS detected but the `MenuTracker` reports `MenuKind::None` because the BSTEventSink<MenuOpenCloseEvent> RegisterSink call failed silently | Even if B1 fixes the capture path, opening F4 would still show empty fields rather than `In Main menu` | MEDIUM — but only if `MenuTracker::install` actually fails. The reference plugin avoids this whole class by detecting main menu via `parentCell == nullptr` rather than menu name |

**Verdict C**: C1 is structural — we have no equivalent of the reference's `kPostPostLoad → SetDiscordPresence("", "Launching game")` early-frame. C2 is secondary; the reference's `parentCell == nullptr → "In Main menu"` is more robust than any sink-based menu detection.

### Failure class D — Missing feature parity vs the reference

| # | Missing feature | Evidence | Fix lane |
|:---|:---|:---|:---|
| D1 | "Launching game" pre-load frame | absent in our flow | Send `update(PresenceState{state:"", details:"Launching game"}, …)` at `F4SE::Plugin_Load` completion or in `Discord::Client::start` once handshake succeeds |
| D2 | "In Main menu" via parentCell-nullptr check | absent in our flow | Add `parentCell == nullptr` branch to the capture-on-main lambda |
| D3 | "Started a new game" via `byCharGenFlag` | absent | Add `IsPlayerInChargenInternal()` equivalent check |
| D4 | Combat target name in event line ("Fighting <enemy> in <location>") | partially present — our CombatTracker captures `currentCombatTarget` name, but our Mapper doesn't combine it with location in the reference's format | Rewrite Mapper combat branch |
| D5 | Workshop item-placed event includes the placed object's display name ("Built Switch in Sanctuary Hills") | absent — our `EventTracker::fireEvent` takes only an enum, no payload string | Extend `EventKind` events to carry a `payload` field; resolve the placed object's `GetReferenceName()` inside the sink |
| D6 | Workshop sink uses custom `TESPlacementSource` at `REL::ID(1067439)` | we use `RE::Workshop::RegisterForItemPlaced` which may not exist / not work in alandtse fork | Define our own `TESPlacementSource` matching the reference's REL::Relocation pattern |
| D7 | Terminal-hacked event includes the terminal's `GetReferenceName()` | absent — our `TerminalSink::ProcessEvent` ignores `evn.terminal` | Read `evn.terminal.get().get()->GetReferenceName()` and pass into the event payload |
| D8 | Simplified status uses ` | ` between verb and location for combat AND exploration | partially present (combat branch) and absent (exploration branch) | Unify in Mapper |
| D9 | INI defaults match reference (bShowCaps=0, iMaxCapsToShow=9999, bAllowEventStatusOverride=1, fUpdateInterval=3) | our defaults diverge ([dist/discord_rich_presence.ini](../dist/discord_rich_presence.ini), [src/Constants.h](../src/Constants.h) line 30, 44) | Patch ini + Constants |
| D10 | HP "99% fix" — if `(max-current < 0.50f) return 100` | absent — our `healthPct` does straightforward division | Add the epsilon clamp in [src/Game/PlayerSnapshot.cpp](../src/Game/PlayerSnapshot.cpp) `healthPct` |
| D11 | Caps clamp with "+" suffix when exceeded (`9999+`) | partially present (we clamp the value but don't append "+") | Patch Mapper caps emission |
| D12 | Update interval default 3s, floor 1s | our floor is 5s ([src/Constants.h](../src/Constants.h) line 29 `kUpdateIntervalFloorSec = 5`) — even if user sets 3s in INI, we clamp to 5s | Lower floor to 1s |
| D13 | Translation strings match reference byte-for-byte (especially `s_T_In = " in "` and `s_T_With = " with "` with leading + trailing spaces) | our Defaults strip the spaces ([src/Constants.h](../src/Constants.h) lines 125-126: `"in"`, `"with"` without spaces) — concat in Mapper has to manually add them | Match reference exactly to keep concat trivial |

### Failure class E — No real tests (process)

| # | Gap | Fix lane |
|:---|:---|:---|
| E1 | No test for the threading model — neither v0.1 worker-poll nor v0.2 TaskInterface lambda nor the proposed v0.3 trampoline hook has a unit-level assertion that "scheduled work runs" | Add a `tests/integration/test_main_thread_dispatch.cpp` that calls into a stub F4SE TaskInterface (or a stub trampoline target) and asserts the callback fires |
| E2 | No golden-string tests for the Mapper output shape against the reference's exact format | Add `tests/golden/` with text fixtures (`main_menu.expected`, `exploring_default.expected`, `exploring_simplified.expected`, `combat.expected`, `pipboy.expected`, etc.) and a `test_mapper_golden.cpp` that builds the GameState, runs Mapper, asserts byte-for-byte match |
| E3 | No contract test that the default AppID has the default `largeImageKey` available (asset-layer invariant) | Either a smoke test that pings the Discord application endpoint (live), or simpler: a documentation test that the default AppID + default image-key combo Tony ships matches a known-good public asset |
| E4 | No test for the `kPostPostLoad → "Launching game"` ordering — the right behaviour is observable only at integration level | A pytest-style harness around `f4drp_integration.exe` that records the sequence of SetActivity payloads sent during plugin startup |
| E5 | The existing `f4drp_integration` target ([tests/CMakeLists.txt](../tests/CMakeLists.txt) line 48-71) gates behind `F4DRP_LIVE_DISCORD=ON` and was never enabled in CI — even if implemented, it never ran | Default `F4DRP_LIVE_DISCORD=ON` for local dev builds; gate only the actual Discord-connection step behind an env var that opts INTO live network |

## Phase 4 — Instrument list

For the next round (v0.3 shipped with `bDebugMode=1`), the log lines that distinguish hypotheses:

| Log line | Tells you |
|:---|:---|
| `discord ready: …` | handshake succeeded |
| `kPostPostLoad — sending Launching game frame` (NEW) | confirms init-order fix |
| `engine ready — main-thread hook installed` (NEW) | confirms trampoline install if we adopt the reference's pattern |
| `tick on main thread: parentCell=<null|addr>, location='<name>', menu='<name>', combat=<bool>` (EXTENDED) | confirms which state branch fired |
| `discord set_activity sent hash=<n> details='<…>' state='<…>'` (already present in v0.2 [src/Discord/Client.cpp](../src/Discord/Client.cpp) lines 150-165) | confirms what Discord actually received |
| `capture player: valid=<bool>, name='<…>', lvl=N, hp=<pct>, caps=<n>` (already present) | confirms snapshot correctness |
| `capture loc: valid=<bool>, name='<…>', ext=<bool>` (already present) | confirms location resolution |

Their absence/presence directly maps to the hypotheses above.

## Recommended v0.3 fix order (priority by impact × blast radius)

1. **Asset / AppID** (A1). Two-line change, immediately resolves the icon. Blast radius: trivial.
2. **Init-sequence frames** (C1, D1-D3). Send "Launching game" at message handler completion, detect main-menu via `parentCell == nullptr`, detect chargen via `byCharGenFlag`. Blast radius: localised to Main.cpp + GameState additions.
3. **Threading model swap** (B1, B4). Replace timer-thread + TaskInterface lambda with trampoline write_call hook on `BGSAnimSoundStateManager::Update` at `REL::ID(1426208) + 0x31`. Blast radius: rewrites Main.cpp threading; deletes MenuTracker sink path (replaced by polling); removes `engineReady` atomic; preserves `EventTracker` sink path (those still install at the right point on the now-frame-driven flow). Risk: REL::ID may differ between runtime 1.10.984, 1.11.169, 1.11.191 — needs Address Library lookup per runtime.
4. **Mapper rewrite to reference format** (D4, D8, D11, plus the details↔state inversion). Pure-function change, fully testable via golden strings (E2).
5. **Event payload extension** (D5-D7). Extend EventTracker to carry a `payload` string. Add custom `TESPlacementSource` at `REL::ID(1067439)`.
6. **INI defaults parity** (D9, D10, D12, D13). Patch [src/Constants.h](../src/Constants.h) + [dist/discord_rich_presence.ini](../dist/discord_rich_presence.ini).
7. **Test scaffolding** (E1-E5). Golden tests cover #4 cheaply; integration test covers #2 + #3.

## Phase 5-6 — Deferred

Fix + cleanup happens in `/speckit-sdd` next. This document is the input to that spec.

---

## Open questions for /speckit-sdd to settle

1. **AppID default**: ship with TommInfinite's `903933837514518548` (works out-of-the-box with Vault Boy), or keep Tony's `1507704875939790889` and require him to upload an asset? `/speckit-sdd` should pick one and document the override path either way.
2. **Address Library IDs for NG runtimes 1.10.984 / 1.11.169 / 1.11.191**: TommInfinite's source was built against 1.10.162. We need to verify `REL::ID(1426208)` and `REL::ID(1067439)` map to the same engine functions on each NG version, or have per-runtime IDs ready. Reference: alandtse NG Address Library bin files.
3. **`F4DRP_LIVE_DISCORD` default**: leave OFF (current) or flip to ON for dev builds. Affects whether `f4drp_integration` can act as the L2 feedback loop. Recommend ON locally, gated via env var for CI.
