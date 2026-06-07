# Multi-Runtime Specification

## Purpose

Define the behavior of the ReaForge C++ host extension that loads, initializes, and concurrently hosts the Lua, JSFX, and QuickJS runtimes inside a single REAPER process. This spec covers coexistence, lifecycle, and main-thread discipline only; the bridge surface is defined separately in `runtime-bridge`.

## Requirements

### Requirement: Runtime Coexistence

The host extension MUST load and initialize the Lua 5.4, JSFX, and QuickJS runtimes inside a single binary without symbol collisions, unsatisfied external references, or duplicate static initializers.

#### Scenario: All three runtimes initialize in one host

- GIVEN the host extension is loaded by REAPER
- WHEN the host's entry point runs
- THEN the Lua state is created via `luaL_newstate` and is non-null
- AND the JSFX module is registered with REAPER's FX system
- AND a QuickJS context is created via `JS_NewContext` and is non-null

#### Scenario: No duplicate symbols at link time

- GIVEN the host is compiled with all three runtimes statically linked
- WHEN the linker resolves symbols
- THEN no symbol is reported as defined more than once
- AND the resulting binary loads into REAPER without `dlopen` errors

### Requirement: Main-Thread Discipline

The host MUST execute all calls into the three runtimes from REAPER's main thread. A call from any other thread SHALL be rejected with a structured error and MUST NOT crash REAPER.

#### Scenario: Off-thread call is rejected, not fatal

- GIVEN a worker thread holds references to a Lua state, a JSFX handle, and a QuickJS context
- WHEN the worker thread invokes any runtime operation through the host
- THEN the host returns a structured error to the caller
- AND REAPER's main thread remains responsive
- AND the runtimes' internal state is not corrupted

### Requirement: Runtime Lifecycle Isolation

The host MUST guarantee that a panic, uncaught exception, or runtime error in one runtime does not corrupt the state of the other two runtimes.

#### Scenario: Lua error does not affect QuickJS

- GIVEN Lua and QuickJS are both initialized
- WHEN a Lua script triggers a runtime error
- THEN the QuickJS context remains usable
- AND subsequent QuickJS evaluations return correct results

### Requirement: Stable Throughput

The host MUST complete a no-op invocation in each runtime in under 5 milliseconds at the 95th percentile, measured over 1000 consecutive calls on a reference machine.

#### Scenario: 1000-call benchmark passes

- GIVEN the benchmark harness invokes a no-op function 1000 times in each runtime
- WHEN the benchmark completes
- THEN the p95 latency for each runtime is under 5 ms
- AND the report is written to `spike-results.md`

### Requirement: REAPER 7+ Linux Target

The host MUST build with g++ 11+ on Linux and MUST load into REAPER 7.0 or later running on the same platform. Windows and macOS are out of scope for the spike.
