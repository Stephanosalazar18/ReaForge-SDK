# Design: ReaForge Multi-Runtime Spike

## Technical Approach

A single C++ binary, built with g++ on Linux, statically links three runtimes (Lua 5.4, QuickJS-ng, JSFX) and exposes a thin read-only bridge to all of them. The host entry point is registered with REAPER via the standard `reaper_plugin_info` mechanism. A separate benchmark binary drives the same host code to measure per-runtime overhead. The spike produces a single artifact: `spike-results.md` with a go/no-go verdict.

## Architecture Decisions

### Decision: Build system

| Option | Tradeoff | Decision |
|---|---|---|
| Meson | SWS / ReaImGui / ReaPack all use it. Familiar to REAPER extension community. | **Chosen** |
| Plain Make | Zero deps, but reinvents what we get for free | Rejected |
| CMake | Common, but not the REAPER C++ convention | Rejected |

### Decision: Linking model

| Option | Tradeoff | Decision |
|---|---|---|
| Static link all three runtimes | One `.so`, zero external deps, deterministic load order | **Chosen** |
| Dynamic link as separate `.so` | Cleaner isolation, but more packaging, more `dlopen` failure modes | Rejected for spike |

### Decision: JSFX compile strategy

| Option | Tradeoff | Decision |
|---|---|---|
| `reaper_jsfx_compile()` at runtime | Cleanest UX, lets users author JSFX as text | **Try first**; fallback to pre-compiled |
| Pre-compile JSFX offline, ship as binary blob | Always works, no public API dependency | Fallback if probe fails |
| Skip JSFX from spike | Simplest, but leaves a capability gap | Rejected |

### Decision: Threading model

| Option | Tradeoff | Decision |
|---|---|---|
| Main-thread-only with explicit guard | Matches REAPER's API contract, simple to reason about | **Chosen** |
| Worker threads with marshaling | More work, more bugs, no spike benefit | Rejected |

### Decision: QuickJS fork

| Option | Tradeoff | Decision |
|---|---|---|
| `quickjs-ng` | Maintained fork, C++20 friendly, active community | **Chosen** |
| Original `bellard/quickjs` | Unmaintained since 2021, breaks on modern toolchains | Rejected |
| Duktape | Embedded, but ES5 only | Rejected |

## Data Flow

```
REAPER process
  │
  └── reaper_reaforge_host.so  (loaded by REAPER)
        │
        ├── plugin_info entry ──► registers with REAPER's main thread
        │
        ├── host.init() ──┬──► lua_runtime.init()      [Lua 5.4 state]
        │                 ├──► jsfx_runtime.init()     [compile or load]
        │                 └──► quickjs_runtime.init()  [JS_NewContext]
        │
        ├── bridge.{h,cpp}
        │     ├── bridge_lua.cpp     (Lua C functions: 5 funcs)
        │     ├── bridge_quickjs.cpp (JS C functions: 5 funcs)
        │     └── bridge_jsfx.cpp    (JSFX param accessors: 5 funcs)
        │
        ├── executor.cpp
        │     ├── parse manifest
        │     ├── decode JSON args
        │     ├── dispatch by runtime
        │     └── capture errors as structured result
        │
        └── benchmark (separate process, same .so loaded)
              ├── 1000 no-op calls per runtime
              ├── std::chrono timing
              └── write spike-results.md
```

## File Changes

| File | Action | Description |
|---|---|---|
| `src/spike/host.h` | Create | Public host API: `init`, `shutdown`, `invoke` |
| `src/spike/host.cpp` | Create | Lifecycle, main-thread guard, error capture |
| `src/spike/runtime/lua_runtime.{h,cpp}` | Create | `luaL_newstate`, `luaL_loadstring`, panic recovery |
| `src/spike/runtime/quickjs_runtime.{h,cpp}` | Create | `JS_NewContext`, `JS_Eval`, exception capture |
| `src/spike/runtime/jsfx_runtime.{h,cpp}` | Create | Probe `reaper_jsfx_compile`; fallback to blob |
| `src/spike/bridge/bridge.h` | Create | 5-function table shared across runtimes |
| `src/spike/bridge/bridge_lua.cpp` | Create | `lua_register` wrappers |
| `src/spike/bridge/bridge_quickjs.cpp` | Create | `JS_SetCFunction` wrappers |
| `src/spike/bridge/bridge_jsfx.cpp` | Create | Param name → value lookup |
| `src/spike/executor.{h,cpp}` | Create | Manifest parse, JSON decode, dispatch, error shape |
| `src/spike/main.cpp` | Create | `reaper_plugin_info` registration, REAPER entry points |
| `src/spike/bench.cpp` | Create | 1000-call benchmark per runtime |
| `src/spike/meson.build` | Create | Build script |
| `third_party/quickjs-ng/` | Vendor (submodule) | Pinned commit |
| `third_party/reaper-sdk/` | Vendor (submodule) | Pinned commit |
| `scripts/run_benchmark.sh` | Create | Convenience wrapper |
| `spike-results.md` | Generated (not committed) | Benchmark output |

## Interfaces / Contracts

```cpp
// host.h
struct InvocationRequest {
    std::string id;          // extension id
    std::string fn;          // function name
    std::string args_json;   // JSON-serialized args
};

struct InvocationResult {
    bool ok;
    std::string result_json; // present when ok
    std::string error_json;  // present when !ok: {runtime, message, trace}
};

class Host {
public:
    void init();                          // load all 3 runtimes
    void shutdown();                      // tear down in reverse order
    InvocationResult invoke(const InvocationRequest&);
    bool on_main_thread() const;          // threading guard
};
```

JSON encoding/decoding uses a header-only library vendored as `third_party/nlohmann/json.hpp` (single file, MIT).

## Testing Strategy

| Layer | What | How |
|---|---|---|
| Unit | Each runtime in isolation | `bench.cpp` reuses a per-runtime no-op function; CI fails if any runtime fails to init |
| Integration | All three runtimes coexist | `bench.cpp` runs in same process; `spike-results.md` records per-runtime p50/p95/p99 |
| Manual | REAPER loads the `.so` | REAPER launches; developer runs `scripts/run_benchmark.sh`; outputs are inspected |
| E2E (later) | Real Lua + real JS + real JSFX execute | Deferred to Phase 1 |

## Migration / Rollout

No migration. The spike lives entirely under `src/spike/` and the change folder. No production code is touched. If the spike returns **no-go**, the path forward is documented in the same `spike-results.md`.

## Open Questions

- [ ] Confirm the exact function name and signature of `reaper_jsfx_compile` in the REAPER C++ SDK. Resolved by spike Task 1.1.
- [ ] Whether `QuickJS-ng` builds cleanly with `g++ 11 -std=c++17`. Resolved by spike Task 1.2.
- [ ] Whether vendored REAPER SDK headers compile under `g++` on Linux (some headers assume MSVC). Resolved by spike Task 1.3.
