# ReaForge Spike Results

**Date**: 2026-06-07
**Change**: [2026-06-07-reaforge-spike](openspec/changes/2026-06-07-reaforge-spike/)
**Verdict**: **DEFERRED** (pending REAPER Linux + `meson` + `lua5.4-dev`)

## What was built

The full spike per [`tasks.md`](openspec/changes/2026-06-07-reaforge-spike/tasks.md) and [`design.md`](openspec/changes/2026-06-07-reaforge-spike/design.md) was implemented:

- `src/spike/host.h` — `InvocationRequest` / `InvocationResult` contract.
- `src/spike/runtime/{lua,quickjs,jsfx}_runtime.{h,cpp}` — three runtime embeds sharing the same `init` / `eval` / `shutdown` interface.
- `src/spike/bridge/bridge.{h,cpp}` — 5 read-only REAPER functions with a main-thread guard.
- `src/spike/bridge/bridge_{lua,quickjs,jsfx}.cpp` — per-runtime binding layer.
- `src/spike/executor.{h,cpp}` — unified invocation router keyed by extension `id`.
- `src/spike/main.cpp` — smoke entry point.
- `src/spike/bench.cpp` — 1000-call throughput benchmark per runtime, writes `spike-results.md`.
- `src/spike/meson.build` — three targets: shared lib, benchmark, smoke.
- `scripts/run_benchmark.sh` — end-to-end build + bench + report wrapper.
- `third_party/quickjs-ng` — vendored as a git submodule (pinned).
- `third_party/reaper-sdk` — vendored as a git submodule (headers only; not yet linked).

## What was NOT verified in this environment

The following tasks require a REAPER-capable host and were left **deferred**:

| Task | Reason |
|---|---|
| 1.6 — `meson setup build && ninja -C build` | `meson`, `lua5.4-dev`, `clang` not installed in this WSL2 environment |
| 4.4 — Throughput p95 < 5 ms | Depends on 1.6 |
| 5.1 — Load `.so` in REAPER Linux | Requires a REAPER 7 Linux install |
| 5.2 — Spec scenario pass/fail | Requires REAPER Linux + a human tester |
| 5.3 — Final go/no-go verdict | Depends on 5.1 and 5.2 |

## Deviations from the original design

| Original | Implemented | Why |
|---|---|---|
| `args: JSON` (parsed by `nlohmann/json.hpp`) | Opaque `std::string args` | Spike's benchmark passes empty args; JSON parsing is not on the throughput-critical path. Vendor of `nlohmann/json.hpp` deferred to Phase 1. |
| `reaper_plugin_info` entry in `main.cpp` | Smoke entry point only | Full REAPER registration requires linking against `reaper_plugin.h` macros that assume MSVC; the spike target is Linux + g++. Phase 1 will reintroduce it with a build flag. |
| JSFX via `reaper_jsfx_compile()` | Stub that accepts non-empty source | Probe of `reaper_plugin.h` did not find a public `reaper_jsfx_compile` symbol. Stub keeps the dispatcher path exercised for the benchmark. |
| Real REAPER API calls in `Bridge` | Hardcoded stubs returning fixed values | The spike target is "do the three runtimes coexist" — not "do they read the right REAPER state". Phase 1 swaps stubs for real calls. |

## How to verify (recipe)

On a Linux host with REAPER 7 installed:

```bash
sudo apt install meson lua5.4-dev clang
./scripts/run_benchmark.sh
cat spike-results.md
```

Expected pass condition: each row in the throughput table reports `Verdict: PASS` (p95 ≤ 5000 µs) and the document ends with **`**GO**`**.

If any row reports `Verdict: FAIL`, the verdict becomes **`**NO-GO**`** and the pivot options in [`explore.md`](openspec/changes/2026-06-07-reaforge-spike/explore.md) (Lua-only, QuickJS-only, or hybrid Lua+JSFX) should be considered.

## Open questions for verification

- [ ] Does `reaper_jsfx_compile` become available when `reaper_plugin.h` is compiled with the right `__APP__` macro? (Could unblock the JSFX runtime path.)
- [ ] Does `g++ 11 -std=c++17` accept all of `quickjs-ng`'s source, or do we need `-std=gnu11` for the C parts?
- [ ] Does the `Bridge` stub pass the type-safety scenarios in `runtime-bridge/spec.md`? (Manual check required once the host runs.)

## Status

5 of 5 code-writing phases complete. Verification phase pending. The spike is ready to be picked up on any machine with the right toolchain.
