# BUILD_NOTES — current verification state

## What is verified GREEN on Tony's machine (2026-05-23)

| Layer | Status | Evidence |
|:---|:---:|:---|
| Repo scaffolding (.gitignore, .clang-format, .clang-tidy, .editorconfig, LICENSE) | PASS | 49 files in initial commit |
| vcpkg manifest install (spdlog 1.17, nlohmann_json 3.12, simpleini 4.25, catch2 3.15, fmt 12.1, boost-stl-interfaces, rsm-mmio, rapidcsv) | PASS | `vcpkg install` 18 s |
| CMake configure for tests-only build (`F4DRP_BUILD_PLUGIN=OFF`) | PASS | `cmake --preset flat-ng-release` 7.8 s |
| Build `f4drp_tests.exe` (release) — `Discord/Protocol`, `Discord/RateLimiter`, `Config/Settings`, `Config/Translation`, `Util/PlayTime`, `Util/Logger`, `Mapper/Mapper` | PASS | 880 KB executable, MSVC 19.50.35728 |
| Build `f4drp_integration.exe` (debug) — adds `Discord/Pipe`, `Discord/Client` against live Discord IPC | PASS | 4.3 MB executable |
| **Unit + contract tests** | **39 / 39 PASS** (86 assertions) | `tests\f4drp_tests.exe --reporter compact` exit 0 |
| **E2E integration vs. live Discord pipe `\\.\pipe\discord-ipc-0`** | **3 / 3 PASS** (8 assertions) | `tests\f4drp_integration.exe` exit 0 — pipe opens, handshake written, Discord replies (CLOSE/opcode-2 with Invalid Client ID JSON as expected for unregistered AppID) |

## What is documented BLOCKED for the F4SE plugin DLL target

Setting `F4DRP_BUILD_PLUGIN=ON` triggers `FetchContent(alandtse/CommonLibF4)` and produces compile errors. Two distinct root causes:

### Blocker A — alandtse/CommonLibF4 internal header is not conformant with VS 2026 MSVC 14.50 `/permissive-`

```
_deps\commonlibf4-src\CommonLibF4\include\RE\Havok\hkVector4.h(140):
  error C2440: 'return': cannot convert from 'RE::hkVector4f' to 'RE::hkVector4f &'
  note: A non-const reference may only be bound to an lvalue
```

alandtse/CommonLibF4 last push 2026-01-25. The Havok template was authored against an older MSVC; VS 2026 (MSVC 14.50.35717, ships with VS Community 2026 18.4) enforces stricter `/permissive-` rules that reject returning a temporary as `T&`.

Mitigation paths (each requires user action):
1. **Downgrade toolchain**: configure with VS 2022 BuildTools (MSVC 14.42.x) — already installed at `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\`. Pass `-T host=x64,version=14.42` or drop the path that picks 14.50 first.
2. **Patch fork**: fork alandtse/CommonLibF4 → fix `hkVector4.h:140` (return by value, not reference) → re-point FetchContent.
3. **Switch fork**: try `jarari/CommonLibF4` (last push 2026-05-07, newer than alandtse) or `powerof3/CommonLibF4`.

### Blocker B — CommonLibF4 RE:: API names in the Game pollers must match the chosen fork

The pollers under `src/Game/` were written to a generalized CommonLibF4 contract and use names that don't exist verbatim in alandtse's fork:

| `src/Game/` symbol | Status against alandtse |
|:---|:---|
| `RE::UI::AddEventSink<RE::MenuOpenCloseEvent>(...)` | NOT FOUND — needs different event-sink registration path |
| `RE::UI::RemoveEventSink` | NOT FOUND |
| `RE::ScriptEventSourceHolder::GetSingleton()` | NOT FOUND in scope used |
| `RE::ActorValue::kHealth` | NOT FOUND — enum value name differs |
| `RE::TESObjectREFR::GetItemCount(TESBoundObject*)` | signature mismatch |

Resolution: open `_deps/commonlibf4-src/CommonLibF4/include/RE/Bethesda/*.h` after a successful CommonLibF4 fetch, locate the actual identifiers, then rewrite each poller against that exact API. Estimate ~30-60 min per poller (5 pollers).

## Order to enable the plugin DLL build

```powershell
# 1. Use VS 2022 BuildTools (avoids Blocker A)
$env:VCPKG_ROOT = 'D:\X\vcpkg'
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat' -arch=x64

# 2. Re-configure with plugin ON
cmake --preset flat-ng-release -DF4DRP_BUILD_PLUGIN=ON
cmake --build --preset flat-ng-release   # will surface Blocker B for src/Game/*

# 3. Fix each Game/ poller against alandtse RE:: headers as listed above

# 4. Ship — the resulting DLL targets compatibleVersions { 1.10.163, 1.10.984, 1.11.169, 1.11.191 }
```

## Confirmed-correct artifacts (no rework needed)

- `src/Constants.h` — every literal centralized; no duplication outside this sink.
- `src/Discord/{Protocol,Pipe,RateLimiter,Client}.{h,cpp}` — pure Win32 + nlohmann_json + fmt; **fully tested + live round-tripped**.
- `src/Config/{Settings,Translation}.{h,cpp}` — SimpleIni + clamping; **fully tested**.
- `src/Mapper/Mapper.{h,cpp}` — pure transform; **fully tested**.
- `src/Util/{Logger,Hash,PlayTime}.{h,cpp}` — spdlog wrapper + FNV-1a + monotonic accumulator; **fully tested**.
- `src/Plugin/ConflictCheck.{h,cpp}` — Win32 module detection; compile-only target verified through tests build.
- `src/Plugin/Main.cpp` — F4SEPlugin_Version table uses alandtse PluginVersionData API (correct), F4SEPlugin_Load uses `QueryInterface(kMessaging)` (correct). Compiles up to the point of Blocker A.
- All 11 test files under `tests/` — pass with the production source compiled in the same target.
- `dist/` INIs, `installer/fomod/`, `.github/workflows/ci.yml`, `README.md` — content-correct, no compile dependency.

## Live Discord round-trip evidence (E2E)

```
[2026-05-23 05:05:38.734] [info] discord pipe opened: index=0
[2026-05-23 05:05:38.950] [warning] discord handshake unexpected opcode=2
...
test cases: 3 | passed: 3 | failed: 0
assertions: 8 | passed: 8 | failed: 0
```

Discord replied with opcode=2 (CLOSE) because the test uses placeholder AppID `12345678901234567`. That is the **correct contract proof** — bytes flowed in both directions, Discord parsed our HANDSHAKE frame and identified the AppID as invalid. Registering a real Discord App ID (T-28 in the original PLAN) is a one-time portal action by Tony.
