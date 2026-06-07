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

- [x] 1.1 Vendor `quickjs-ng` as a git submodule under `third_party/quickjs-ng/`
- [x] 1.2 Vendor `reaper-sdk` headers under `third_party/reaper-sdk/`
- [ ] 1.3 Vendor `nlohmann/json.hpp` single-header under `third_party/json/json.hpp` *(deferred to Phase 2: spike uses opaque `std::string` args; JSON is not needed for the throughput check)*
- [x] 1.4 Create `src/spike/meson.build` with three targets: `reaper_reaforge_host` (shared lib), `reaforge_bench` (executable), `reaforge_host_main` (smoke)
- [x] 1.5 Create `src/spike/host.h` declaring `InvocationRequest` and `InvocationResult`
- [ ] 1.6 Verify build with `meson setup build && ninja -C build` — *deferred: requires `meson`, `lua5.4-dev`, `clang` apt-installed on a REAPER-capable host*

## Phase 2: Runtime Embeds

- [x] 2.1 Create `src/spike/runtime/lua_runtime.{h,cpp}`: `LuaRuntime` with `init`, `eval`, `shutdown`
- [x] 2.2 Create `src/spike/runtime/quickjs_runtime.{h,cpp}`: `QuickJSRuntime` with `init`, `eval`, `shutdown`
- [x] 2.3 Probe REAPER C++ SDK for `reaper_jsfx_compile` *(result: not exposed in `reaper_plugin.h`; stub strategy used)*
- [x] 2.4 Create `src/spike/runtime/jsfx_runtime.{h,cpp}` with stub strategy (compile accepts non-empty source, marks `compiled_ = true`)
- [x] 2.5 Wire all three runtimes in `executor.cpp` `init()` and `shutdown()`; reverse-order teardown

## Phase 3: Bridge and Executor

- [x] 3.1 Create `src/spike/bridge/bridge.h` and `bridge.cpp` with the 5 read-only functions
- [x] 3.2 Create `src/spike/bridge/bridge_lua.cpp` registering the 5 functions via `lua_pushcfunction`
- [x] 3.3 Create `src/spike/bridge/bridge_quickjs.cpp` registering via `JS_NewCFunction`; converts return values to `JS_NewFloat64` / `JS_NewString` / `JS_NULL`
- [x] 3.4 Create `src/spike/bridge/bridge_jsfx.cpp` with the documented "not implemented" error
- [x] 3.5 Main-thread guard via `Bridge::install_main_thread_id()` and per-call check; off-thread returns `BridgeStatus::NotOnMainThread`
- [x] 3.6 Create `src/spike/executor.{h,cpp}`: routes by `id` to `lua-demo`, `js-demo`, `jsfx-demo`; returns `InvocationResult`

## Phase 4: Entry Point and Benchmark

- [x] 4.1 Create `src/spike/main.cpp` smoke entry point (full `reaper_plugin_info` registration is Phase 1 work; spike only validates init)
- [x] 4.2 Create `src/spike/bench.cpp` driving 1000 invocations per runtime; records p50/p95/p99
- [x] 4.3 Create `scripts/run_benchmark.sh` that builds, runs the smoke, runs the bench, and writes `spike-results.md`
- [ ] 4.4 Verify `spike-results.md` records p95 < 5ms for each runtime — *deferred: needs `meson` and `lua5.4-dev` on a REAPER-capable host*

## Phase 5: Verification

- [ ] 5.1 Manually load `reaper_reaforge_host.so` in REAPER 7 Linux — *deferred: requires REAPER Linux install*
- [ ] 5.2 Run each spec scenario from the 3 capability specs — *deferred: requires REAPER Linux + a human tester*
- [ ] 5.3 Write final `spike-results.md` with go/no-go verdict — *deferred: depends on 5.1 and 5.2*
