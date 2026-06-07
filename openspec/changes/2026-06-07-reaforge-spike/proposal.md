# Proposal: ReaForge Multi-Runtime Spike

## Intent

Before committing to a multi-runtime architecture for ReaForge (Lua + JSFX + QuickJS-hosted TypeScript all running inside a single REAPER C++ extension), we need empirical proof that the three runtimes can coexist without symbol collisions, thread conflicts, or unacceptable performance overhead. This change delivers a focused spike that produces a go/no-go verdict and baseline metrics.

## Scope

### In Scope

- A single REAPER C++ extension (`reaper_reaforge_host`) that loads:
  - One Lua script via the embedded Lua 5.4 interpreter.
  - One JSFX source compiled at load time (or pre-compiled if `reaper_jsfx_compile()` is not public).
  - One JavaScript expression evaluated via an embedded QuickJS-ng context.
- A CLI benchmark script that calls each runtime 1000 times and reports p50/p95/p99 latency.
- `spike-results.md` documenting: integration findings, overhead numbers, ABI notes, and a final **go / no-go** verdict.
- Task list with explicit pass/fail criteria for the spike.

### Out of Scope

- ReaImGui-based UI (Phase 1).
- TS-authored extension SDK (Phase 2).
- ReaPack distribution and registry (Phase 3).
- opencode-bridge agent (Phase 5).
- Windows MSVC build (Linux first; Windows port is Phase 1).
- Production-grade error handling, logging, or packaging.

## Capabilities

### New Capabilities

- `multi-runtime`: a single C++ binary hosts Lua, JSFX, and JS/TS runtimes without conflict.
- `extension-execution`: the host can invoke an extension written in any of the supported runtimes using a unified contract (function name + JSON-like args).
- `runtime-bridge`: a thin C++ layer exposes a minimal subset of the ReaScript API (~10 functions, read-only) to all three runtimes in a uniform way.

### Modified Capabilities

None — this is the first change in the repo.

## Approach

- **C++ host** built with g++ on Linux (Ubuntu 22.04 / WSL2). Header-only references to REAPER's `reaper-sdk` (from `justinfrankel/reaper-sdk` GitHub). Compile with `-DREAPER_API_MIN_VERSION=700` for REAPER 7+.
- **Lua**: link against `liblua5.4-dev` (apt), call `luaL_newstate`, `luaL_loadstring`, `lua_pcall`. No bridge to ReaScript yet — Lua just prints a string.
- **JSFX**: probe `reaper_jsfx_compile()` and `reaper_jsfx_run()` in the C++ SDK. If unavailable, fall back to including a pre-compiled `.jsfx` blob and registering it via `TrackFX_AddByName`.
- **QuickJS-ng**: vendor as a git submodule (commit pinned), embed via `quickjs.h` C API. Run a JS expression that returns a number.
- **Benchmark**: a bash script + a small C++ harness that times N=1000 calls to a no-op function in each runtime, reports percentiles.

## Affected Areas

| Area | Impact | Description |
|------|--------|-------------|
| `src/spike/` | New | C++ host source (~200 LOC) |
| `src/spike/third_party/quickjs-ng/` | New (submodule) | Vendored JS engine |
| `openspec/changes/2026-06-07-reaforge-spike/` | New | This change's artifacts |
| `spike-results.md` | New | Empirical output of the spike |

## Risks

| Risk | Likelihood | Mitigation |
|---|---|---|
| `reaper_jsfx_compile()` is not exposed in REAPER's C++ SDK | Med | Probe in Task 1.1; fallback to pre-compiled JSFX bundle |
| MSVC ABI blocks g++ build artifacts from loading in REAPER Windows | High (Windows only) | Spike targets Linux; Windows is Phase 1 |
| QuickJS-ng embed pulls in unexpected deps or symbols | Low | Vendor exact commit, link statically, `--exclude-libs=ALL` |
| Threading conflict with REAPER's main-thread-only API | Med | Spike runs all three runtimes only from the main thread; document constraint |
| Spike over-scope: building a mini-UI, a registry, etc. | Med | Hard limit: no UI, no registry, no SDK; only the three runtimes + benchmark |

## Rollback Plan

The spike lives entirely in `src/spike/` and `openspec/changes/2026-06-07-reaforge-spike/`. If it returns **no-go**:

1. Archive the change to `openspec/changes/archive/2026-06-07-reaforge-spike-no-go/`.
2. Delete `src/spike/`.
3. Open a new change for the pivot (Lua-only, QuickJS-only, or hybrid Lua+JSFX).
4. No production code is affected; Phase 1 has not started.

## Dependencies

- **REAPER 7+** installed (Linux build available via Wine is acceptable for spike).
- **g++ 11+**, **meson** (for build script), **lua5.4-dev** (apt).
- **quickjs-ng** source (vendored, pinned commit).
- **REAPER Extension SDK** headers from `justinfrankel/reaper-sdk` (vendored, pinned commit).
- Linux runtime environment capable of loading REAPER native plugins (or REAPER via Wine on the WSL host).

## Success Criteria

- [ ] The C++ host compiles cleanly with g++ on Linux.
- [ ] The host loads inside REAPER (Linux or Wine) without crashing.
- [ ] The Lua script executes and produces expected output.
- [ ] The JSFX loads (compile or pre-compile path) and is registered as a usable FX.
- [ ] The QuickJS expression evaluates and returns the expected number.
- [ ] Benchmark report shows per-runtime overhead p95 < 5ms.
- [ ] `spike-results.md` exists with metrics and a clear go/no-go verdict.
- [ ] If **go**: the spike's C++ structure is documented as the seed for Phase 1.
- [ ] If **no-go**: at least one pivot path is identified in the results doc.
