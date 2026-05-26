# Fallout 4 Discord Rich Presence — project memory

F4SE plugin (alandtse CommonLibF4, NG runtime 1.11.191). Discord Rich Presence showing live game state. Build: `tools\build-flat-ng-release.bat` from repo root via `cmd /c`, MSVC 14.50.35717. Tests: `ctest --preset flat-ng-debug` (57/57 green offline). Attribution: `xt0n1 <xt0n1@black-watch.net>`, no AI footers.

## Handoffs

## Handoff f4drp-addtask-gridcell-crash-20260526-063000 — 2026-05-26 06:30 UTC

**Full handoff:** `C:\Users\xt0n1\AppData\Local\Temp\claude-handoff-f4drp-addtask-gridcell-crash-20260526-063000\HANDOFF.md`
**Project:** Fallout 4 Discord Rich Presence
**Focus next:** Resolve the GridCellArray CTD that fires after our AddTask ticks read game state on NG 1.11.191. Thread-id diagnostic build DEPLOYED, NOT yet tested.

### TL;DR
- ROOT CAUSE of stuck presence: Address Library reorganized NG IDs at 1.11.137. `UI::GetSingleton`(2689028) + `PlayerCharacter::GetSingleton`(2690919) are MISS on 1.11.191 (OK on 1.10.98x). NO public CommonLibF4 fork updated. `GetPlayerHandle`(2698072) + `GetMenuMapRWLock`(2707105) survived → player access works via handle; UI/menus disabled.
- UNSOLVED CTD: game crashes ~0.5–13s after our AddTask ticks read game state. Presence updates correctly first (`Lexie|LVL:14|HP:100%|Vault 114` confirmed), then CTD in `Fallout4.exe+1688xxx` GridCellArray on `JobListManager::ServingThread`. Our DLL NOT in stack, our reads never AV. RCX=0x29500000000 (low 32 bits zeroed = torn-pointer = data race).
- ELIMINATION (in-game confirmed): no DLL→ok; DLL+no-ticks→ok; ticks→CRASH; getUISafe-stub(no GetSingleton call)→still crash; 15s cooldown→still crash; cached-player(no refcount)→still crash. So the trigger = reading live game objects (parentCell/IsInterior/currentLocation/GetFullName/GetDisplayFullName) via AddTask.

### Critical decisions (≤3)
- Player access via `GetPlayerHandle`+raw-pointer cache (only NG-valid path); menus disabled on 1.11.x (no UI ID).
- Diagnose threading before any more fixes — deployed `[DEBUG-tid]` build logging whether the lambda runs on the main thread.

### Open thread (≤3)
- Does AddTask run our lambda on the main thread or a JobListManager worker? (diagnostic awaiting Tony's test → reads `[DEBUG-tid] ... onMainThread=` in plugin log)
- If not main thread → stop using AddTask; read state in F4SE message callbacks or a real per-frame main-thread hook. If main thread → binary-search which read corrupts the grid (suspect virtual GetFullName/GetDisplayFullName/IsInterior).
- DEPLOYED DLL = diagnostic build (812544 B, still crashes by design). Restore `discord_rich_presence.dll.v02bak` (791552 B) for a playable game meanwhile.

### Resume hint
1. Read `C:\Users\xt0n1\AppData\Local\Temp\claude-handoff-f4drp-addtask-gridcell-crash-20260526-063000\HANDOFF.md`.
2. `ctx_search(queries: ["handoff f4drp-addtask-gridcell-crash"])`.
3. Get Tony to load the save once, read the `[DEBUG-tid] onMainThread=` line + newest `crash-*.log`, then branch the fix on that boolean.

## Handoff f4drp-v03-load-crash-getsingleton-20260525-091543 — 2026-05-25 09:15 UTC

**Full handoff:** `C:\Users\xt0n1\AppData\Local\Temp\claude-handoff-f4drp-v03-load-crash-getsingleton-20260525-091543\HANDOFF.md`
**Project:** Fallout 4 Discord Rich Presence
**Focus next:** Presence stuck on "Launching game" + game CTDs on Load Save

### TL;DR
- ROOT CAUSE LOCKED: `RE::PlayerCharacter::GetSingleton()` is BROKEN on NG 1.11.191 in alandtse fork — NG RelocationID(303410, **2690919**) returns junk (`0x800000008` incrementing 0x100000000/frame, an engine counter). Dereferencing it → AV → every capture tick SEHs → presence never updates.
- `GetPlayerHandle().get()` is the WORKING NG path (returns valid `0x2259b75b180`; all player fields + virtuals read fine — name='Lexie' lvl=14 hp=100%). `capturePlayerSnapshot` already fixed to use it.
- Round-6 proved the crash MOVED to `captureLocation` (Location.cpp:12) which STILL calls broken GetSingleton. CombatTracker.cpp:33+53 are the next dominoes.
- CTD on Load Save = secondary: AddTask lambdas run on F4SE's engine job/serving thread; SEH-catching the junk-deref AV there desyncs the engine job dispatcher → engine null-derefs in GridCellArray during cell-load (Buffout4 crash log `crash-2026-05-25-09-09-58.log`, our DLL NOT in faulting stack).
- MITIGATION: F4 install reverted to v0.2 backup (`discord_rich_presence.dll.v02bak`) so the game loads during the pause.

### Critical decisions
- Standardize player resolution on `GetPlayerHandle().get()` everywhere via a centralized `getPlayerSafe()`; never call PlayerCharacter::GetSingleton on NG.
- SEH-catch is the WRONG model on F4SE's serving thread — validate pointers before deref (primary), SEH is a thin backstop only.
- EventTracker Terminal/Workshop sinks stay disabled on NG (pre-NG-only REL::IDs 425579/849008); deferred to v0.4.

### Open thread
- Confirm Load-Save CTD fully clears once junk-derefs are gone (round-7), or whether a Bethesda GridCellArray instability remains.
- Re-check `gameLoading` INSIDE the AddTask lambda body (queued tasks fire during load) + add in-flight guard (round-6 saw subs=5 invs=0 then 7-in-1ms burst).

### Resume hint
1. Read the full handoff (path above).
2. `ctx_search(queries: ["handoff f4drp-v03-load-crash-getsingleton"])`.
3. Round-7: centralize `getPlayerSafe()` (GetPlayerHandle + userspace-ptr validation already in PlayerSnapshot.cpp ~L16); replace 3 broken GetSingleton calls (Location.cpp:12, CombatTracker.cpp:33+53); add lambda-body load-gate + in-flight guard; rebuild; redeploy (overwrites the v0.2 revert); HITL verify stats render + no crash-*.log.

### Round-7 resolution — 2026-05-25 (shipped, awaiting HITL)

Player resolution centralized in new `src/Game/PlayerAccess.{h,cpp}`. `getPlayerSafe()` resolves the player via `GetPlayerHandle().get()` and returns nullptr unless the result is a userspace pointer; the smart-pointer deref lives in a separate `noinline derefPlayerHandle()` so its destructor stays out of the `__try` frame (MSVC unwind rule). Callers routed:
- `PlayerSnapshot.cpp` — dropped local `isUserspacePtr`/`derefPlayerHandle`/`tryPlayerHandle`, now calls `getPlayerSafe()` (kept its own SEH-wrapped `tryGetCapsForm`/`tryGetItemCount`).
- `Location.cpp` `captureLocation` — replaced broken `PlayerCharacter::GetSingleton()`.
- `CombatTracker.cpp` `snapshotTargetNames` + `anyHostile` — replaced both broken `GetSingleton()` calls.

Load-crash hardening in `Main.cpp`:
- Added `tickInFlight` atomic + CAS guard in `submitTickToMain()` — never queues a second tick while one is outstanding (kills the subs-ahead-of-invs burst).
- AddTask lambda re-checks `gameLoading` on the engine thread and bails before touching player/cell state mid-load (the deref that desynced the job dispatcher).
- `tickInFlight` reset on kPostLoadGame/kNewGame so a task dropped across a load cannot wedge the timer.

Build `tools\build-flat-ng-release.bat` → 23/23 clean under /W4 /WX (sole non-compiler warning is the boost vcpkg CMake dev-warning). Deployed to `…\Fallout 4\Data\F4SE\Plugins\discord_rich_presence.dll` (sha256 `3B7615B8…C4DF`, 811520 B); v0.2 retained at `.v02bak`. Source edits uncommitted pending in-game confirmation.

Open (HITL): launch F4, confirm presence shows live stats (name/level/hp/location/combat) AND Load Save no longer CTDs (no new `crash-*.log` in Documents\My Games\Fallout4\F4SE). If a GridCellArray crash still appears with our DLL absent from the faulting stack, that residual is Bethesda-side, not ours.
