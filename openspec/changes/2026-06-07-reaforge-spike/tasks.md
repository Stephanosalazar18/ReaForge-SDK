# Tasks: ReaForge Multi-Runtime Spike

## Review Workload Forecast

| Field | Value |
|-------|-------|
| Estimated changed lines | ~300 (host + 3 runtimes + bridge + benchmark + meson) |
| 400-line budget risk | Low |
| Chained PRs recommended | No |
| Suggested split | Single PR (spike is bounded by design) |
| Delivery strategy | single-pr |

Decision needed before apply: No
Chained PRs recommended: No
Chain strategy: size-exception
400-line budget risk: Low

## Phase 1: Foundation

- [ ] 1.1 Vendor `quickjs-ng` as a git submodule under `third_party/quickjs-ng/` (pinned commit, recorded in `third_party/quickjs-ng.PIN`)
- [ ] 1.2 Vendor `reaper-sdk` headers under `third_party/reaper-sdk/` (pinned commit, recorded in `third_party/reaper-sdk.PIN`)
- [ ] 1.3 Vendor `nlohmann/json.hpp` single-header under `third_party/json/json.hpp`
- [ ] 1.4 Create `src/spike/meson.build` with three targets: `reaper_reaforge_host` (shared lib), `reaforge_bench` (executable), and the static link of Lua/QuickJS/JSFX
- [ ] 1.5 Create `src/spike/host.h` declaring `Host`, `InvocationRequest`, `InvocationResult`
- [ ] 1.6 Verify build with `meson setup build && ninja -C build` — must succeed with all three runtimes linked

## Phase 2: Runtime Embeds

- [ ] 2.1 Create `src/spike/runtime/lua_runtime.{h,cpp}`: `LuaRuntime` with `init`, `eval`, `shutdown`; `eval` returns `InvocationResult`
- [ ] 2.2 Create `src/spike/runtime/quickjs_runtime.{h,cpp}`: `QuickJSRuntime` mirroring the Lua API; uses `JS_Eval` and `JS_GetException`
- [ ] 2.3 Probe REAPER C++ SDK for `reaper_jsfx_compile`; record result in `src/spike/runtime/jsfx_runtime.cpp` header comment
- [ ] 2.4 Create `src/spike/runtime/jsfx_runtime.{h,cpp}`: try `reaper_jsfx_compile` first; on failure, fall back to including a one-line pre-compiled JSFX blob as a placeholder
- [ ] 2.5 Wire all three runtimes in `host.cpp` `init()` and `shutdown()`; reverse-order teardown

## Phase 3: Bridge and Executor

- [ ] 3.1 Create `src/spike/bridge/bridge.h` with the 5-function table (`get_cursor_position`, `get_project_tempo`, `get_track_count`, `get_track_name`, `get_master_track_volume`)
- [ ] 3.2 Create `src/spike/bridge/bridge_lua.cpp` registering the 5 functions via `lua_register`
- [ ] 3.3 Create `src/spike/bridge/bridge_quickjs.cpp` registering via `JS_SetCFunction`; convert return values to `JS_NewFloat64` or `JS_NULL`
- [ ] 3.4 Create `src/spike/bridge/bridge_jsfx.cpp` with stub that returns error "spike does not bridge to JSFX" (JSFX has no general function-call surface)
- [ ] 3.5 Add main-thread guard `Host::on_main_thread()` checked at the top of every bridge call
- [ ] 3.6 Create `src/spike/executor.{h,cpp}`: parse `InvocationRequest`, look up extension id, dispatch to the right runtime, capture errors, return `InvocationResult`

## Phase 4: Entry Point and Benchmark

- [ ] 4.1 Create `src/spike/main.cpp` exporting `reaper_plugin_info` and registering the host with REAPER
- [ ] 4.2 Create `src/spike/bench.cpp` driving 1000 no-op invocations per runtime via `Host::invoke`; record `std::chrono` p50/p95/p99
- [ ] 4.3 Create `scripts/run_benchmark.sh` that runs the bench, captures stderr, and writes `spike-results.md`
- [ ] 4.4 Verify `spike-results.md` records p95 < 5ms for each runtime (pass condition) or fails the spike (no-go)

## Phase 5: Verification

- [ ] 5.1 Manually load `reaper_reaforge_host.so` in REAPER 7 Linux; confirm no crash
- [ ] 5.2 Run each spec scenario from `multi-runtime/spec.md`, `extension-execution/spec.md`, `runtime-bridge/spec.md`; mark pass/fail
- [ ] 5.3 Write `spike-results.md` with: integration findings, ABI notes, throughput table, scenario pass/fail, and **GO** or **NO-GO** verdict
