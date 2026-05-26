# Lessons learned — v0.2 ship-fail post-mortem (training material)

> Tony shipped v0.2, it was visibly broken on first launch. Every architectural assumption in the plan was wrong. This document is the operating manual for not repeating these mistakes. Each lesson cites the exact engineering rule from `~/.claude/CLAUDE.md §3` it operationalises.

## Lesson 1 — Verify the reference before you plan the rewrite

**What I did wrong**: the v0.2 plan was built on a chain of plausible-sounding assumptions about how TommInfinite's mod works (TaskInterface threading, MenuOpenCloseEvent sinks, `largeImageKey="fallout4"`, ItemPlacedEvent via `RE::Workshop::RegisterForItemPlaced`). None of those assumptions came from reading the actual reference implementation.

**What the reference actually does** (after I finally read its 17 KB `main.cpp` from the Mega source archive): trampoline write_call hook on `BGSAnimSoundStateManager::Update`, polled `g_UI->GetMenuOpen()`, hardcoded `largeImageKey="fo4-big"`, custom `TESPlacementSource` at `REL::ID(1067439)`. Every architectural detail was wrong in the plan.

**Rule cited (§3.2 Think Before Coding — "Trust-then-verify (HARD)")**: when there's an authoritative reference, read its source FIRST. The Nexus 77771 page literally has a "Source code is located [here]" link pointing to a 32 KB Mega archive. That should have been the first artifact pulled in research, not the last.

**Repeat-prevention rule**: any rewrite of an existing working thing starts with reading the working thing's source. No exceptions. If the source link is gated (Mega, login-walled), invoke `/browser` to fetch it. If it doesn't exist at all, state `REFERENCE_SOURCE_MISSING` and downgrade confidence on every architectural claim.

## Lesson 2 — "Verified" means observed under runtime conditions, not "the code compiled and tests passed"

**What I did wrong**: I claimed v0.2 was "verified" because (a) `discord_rich_presence.dll` linked at 806912 bytes, (b) `dumpbin /EXPORTS` showed the two required exports, (c) `ctest --preset flat-ng-debug` returned 39/39 PASS. None of those tests exercise the new threading model — they cover the wire protocol, the rate limiter, the INI loader, the mapper given non-empty input.

**Reality check**: the mapper passing all its tests with synthetic GameState inputs tells me nothing about whether GameState is *actually populated* in-game. There was a runtime-only failure mode (TaskInterface lambda never firing, GameState staying empty, mapper falling back to the empty-input rendering) that no test in the suite could possibly catch.

**Rule cited (§3.4 Verify in Reality — "Real-API default" + "Completion gate (autonomous)")**: nothing is done without green format → lint → typecheck → test PLUS evidence the production path was exercised. For an F4SE plugin, "evidence" means a debug log showing `capture player: valid=true, name='<real>', lvl=N, hp=N.xx, caps=NNNN` repeated every `fUpdateInterval` seconds. I had no such evidence; I should have either shipped with `bDebugMode=1` and asked Tony to capture the log, or invoked `/run` against a real F4 instance (`/run` skill is exactly for this).

**Repeat-prevention rule**: a passing test suite is the floor of "done", not the ceiling. For plugin code that runs inside a host process, "done" requires either (a) Tony confirms the live observable behaviour, or (b) the next session opens with an explicit "needs live verification" todo.

## Lesson 3 — When you can't run the host, write tests that probe the riskiest seam

**What I did wrong**: I shipped v0.2 with the existing test suite (unit tests on Mapper, Protocol, RateLimiter, etc.) and zero tests for the new threading code or the new sink-install path. That was the entire surface I'd just rewritten.

**What I should have done**: even when I can't run F4, I can write contract tests that exercise the seams I just touched:
- A test that constructs a `GameState` with realistic values and confirms `Mapper::mapGameStateToPresence` produces a string matching the *reference's exact format* (`<Name> | LVL:N | HP:N% | NNN caps`), not just "any non-empty string".
- A golden-file test pinning the wire payload byte-for-byte for the main-menu, exploring, combat, menu, event, and chargen states — derived from the reference plugin's behaviour.
- An ABI-level test that calls `F4SE::GetTaskInterface()` against a stubbed F4SE messaging surface and asserts the AddTask wiring is present in the alandtse fork at the runtime version we target.

**Rule cited (§3.3 Edge cases by default)**: five categories — boundary / null-empty / format / concurrency / time. The v0.2 work touched concurrency (thread model rewrite) and format (mapper output). Neither got a new test.

**Repeat-prevention rule**: when a change touches a §3.3 edge-case category, the change ships with at least one test for that category. No exceptions. For threading changes, a "the thing was scheduled and observably executed" assertion. For format changes, a golden-string assertion against the reference's exact output.

## Lesson 4 — Layer-skip blindness: the asset layer is part of the contract

**What I did wrong**: the v0.2 plan handed Tony a one-liner under "Out of scope": *"Discord large image. Code already sends largeImageKey='fallout4'; the icon shows up only after Tony uploads fallout4.png… under the App's Rich Presence → Art Assets at https://discord.com/developers/applications/…"*. That sentence is technically correct but operationally false — it ignores that the user-visible outcome of the entire feature depends on the asset being there. Without the asset, the plugin is functionally indistinguishable from broken even if every line of code is correct.

**Repeat-prevention rule**: when a feature depends on a resource the user has to provision out-of-band (Discord asset upload, OAuth client config, DNS record, certificate), the spec includes (a) the exact provisioning step, (b) the verification probe that proves the resource exists, (c) a defensive fallback if it doesn't. The v0.2 plan did (a) in passing and skipped (b) and (c) entirely.

**Specific to this project**: the default AppID must point at an app with the right asset uploaded. TommInfinite solves this by hardcoding his own App ID (`903933837514518548`) as the default — every user gets the Vault Boy out of the box without configuring anything. Then `AppID = <user-app>` in INI lets users with their own app override. We need the same pattern; we currently default to Tony's empty app which guarantees no icon.

## Lesson 5 — Don't fight a missing piece by adding scaffolding around it

**What I did wrong**: my v0.2 `MenuTracker` carried defensive `VirtualQuery`-based pointer-validity checks because the v0.1 race condition occasionally walked into deallocated UI memory. That code wasn't fixing the race — it was treating the symptom by suppressing the crash. The actual fix is to read the UI from the right thread; the moment you do that, all the `isLikelyValidPtr` paranoia becomes dead code.

The reference plugin has zero pointer-validity checks because it polls from the engine's own frame thread where the pointers are by-construction valid.

**Rule cited (§3.2 Read codebase first + §3.3 Surgical changes)**: when defensive code accumulates around a recurring bug, the bug is structural. Stop adding defenses; fix the structure. "200 lines that should be 50" maps directly to "a thread-safety bug treated as a pointer-validity bug grows scar-tissue until you remove the thread-safety bug".

## Lesson 6 — Toolchain mismatch is a production bug

**What I hit during the v0.2 build**: vcpkg's binary cache had Catch2 built against MSVC 14.50.35717 (VS 2026 Community), my build script forced vcvars64 from MSVC 14.44.35207 (BuildTools 2022). Result: `LNK2019 unresolved external symbol __std_find_last_not_ch_pos_1` because that helper exists in 14.50's STL but not 14.44's. The build script defaulted to whatever I found first; I didn't audit Tony's actual installed toolchains.

**Rule cited (§3.2 Validate effective state, not configured)**: the compiler hash vcpkg detects at install time becomes a hard ABI constraint on every consumer. If you don't pin the toolchain at the level vcpkg detects (the active `cl.exe` on PATH when the configure runs), you get a heisenbug across machines and across re-installs.

**Repeat-prevention rule**: anything that touches MSVC version pinning gets explicit toolchain detection AND explicit toolchain selection. Document the chosen vcvars64 path in the README + CI matrix. Never assume "BuildTools 2022 is the only thing installed".

## Lesson 7 — Acknowledge runtime ground-truth has higher trust than design-time docs

**What I had**: TommInfinite's Nexus 77771 changelog states *"Code runs in the main game thread ensuring more stability."* The plan correctly identified this as the canonical pattern. But the plan **assumed** "main thread" meant `F4SE::GetTaskInterface()->AddTask`, when the source shows it's a trampoline write_call hook. Both are technically "main thread", but the engineering implications are wildly different (queuing latency, dependency on TaskInterface being live, multiple-pending-task pile-up under update bursts).

**Rule cited (§3.2 Cognitive vs mechanical enforcement)**: design-time language ("runs on main thread") is informational; runtime evidence (the actual source code, the actual trampoline install pattern, the actual REL::ID) is authoritative. Don't elevate informational claims to design constraints without runtime corroboration.

## Lesson 8 — When a session hands off, document the assumptions you couldn't verify

**What I did wrong**: my v0.2 engram save at the end of the implementation phase claimed "TaskInterface mandatory for engine reads" as a confirmed architectural decision. It wasn't confirmed — it was an assumption that hadn't been tested under runtime conditions. Future-me (or next-session-me) would read that engram and proceed as if it were settled fact.

**Repeat-prevention rule**: in every `mem_save` (and every handoff document), label claims explicitly: `CONFIRMED` (have evidence and ran the test), `INFERRED` (deduced from documentation, no runtime check), `ASSUMED` (not verified, needs follow-up). Don't let assumptions wear the cloak of confirmed knowledge.

## Lesson 9 — Frustration signals from Tony are diagnostic, not just emotional

**What Tony's reply contained**: *"Nope. No sirve. 1. Implementa tests reales. Para evitar este back and forth. 2. Debe soportar todo lo que te dije que hay aca de features… 3. NO sale el logo real de Fallout 4. 4. NO sirve apenas se abre el juego ya debería salir todo tipo 'In Main Menu'…"*. Each numbered point is a different failure class:
1. Test discipline failure (process)
2. Feature parity failure (scope)
3. Asset layer failure (provisioning)
4. State-detection failure (algorithm)

Four different audit lanes, signposted in one paragraph. The right response is to take each one as a separate diagnostic question, not to apologise and patch the visible one.

**Rule cited (§3.5 Skill-first reflex)**: Tony routed the response by typing `/research /diagnose /speckit-sdd`. That's a tooling prescription, not a vague gesture. Take it literally.

## Lesson 10 — The CoT killswitch applies to engineering self-doubt too

**What I almost did**: after the v0.2 failure, started drafting an apologetic preamble before getting back to the technical work. The §2 identity layer says no — substance first, then voice. Same rule applies to engineering: when something I shipped is broken, the response is to find the root cause and fix it, not to wear the failure as a personality trait.

**Repeat-prevention rule**: when a ship-fail happens, the next message opens with the first concrete diagnostic step or evidence finding. Acknowledgement that the previous attempt failed lives in one short sentence at the top, not paragraphs of meta.

---

## Operating principles distilled

1. **Read the working reference before planning a rewrite.** Always.
2. **"Tests pass" ≠ "code works at runtime".** For host-process code, ship with observability and verify under host conditions.
3. **For every §3.3 edge category touched, add a test.** Threading change ⇒ scheduling test. Format change ⇒ golden-string test.
4. **Asset / provisioning failures are spec-layer failures, not "out of scope".** Default to a working configuration; require the user to opt INTO complexity, not out of brokenness.
5. **Defensive scaffolding around a recurring bug means the bug is structural.** Find the structure; delete the scaffolding.
6. **Pin the toolchain explicitly at the level vcpkg/build-system detects it.** Compiler hash drift is a heisenbug.
7. **Tag every engram / handoff claim with CONFIRMED / INFERRED / ASSUMED.** Don't promote assumptions to facts.
8. **Frustration signals are typed diagnostic lanes.** Take them literally.
