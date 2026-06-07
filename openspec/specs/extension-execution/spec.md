# Extension Execution Specification

## Purpose

Define the unified contract by which the ReaForge host invokes an extension regardless of the runtime in which it is written. The contract is runtime-agnostic at the host level and delegates language-specific behavior to the `multi-runtime` and `runtime-bridge` capabilities.

## Requirements

### Requirement: Unified Invocation Contract

The host MUST accept an extension invocation as a structured payload containing: an extension id, a target function name, and a JSON-serializable argument object. The host MUST return a JSON-serializable result or a structured error.

#### Scenario: Lua extension invoked via unified contract

- GIVEN a Lua extension `humanize` is registered with id `humanize` and runtime `lua`
- WHEN the host invokes `{ id: "humanize", fn: "run", args: { timing_ms: 12 } }`
- THEN the host routes the call to the Lua runtime
- AND the Lua function `run` receives `args` as a decoded table
- AND the host returns the function's return value as JSON

#### Scenario: QuickJS extension invoked via unified contract

- GIVEN a JS extension `render-csv` is registered with id `render-csv` and runtime `quickjs`
- WHEN the host invokes `{ id: "render-csv", fn: "export", args: { path: "/tmp/out.csv" } }`
- THEN the host routes the call to the QuickJS context
- AND the JS function `export` receives `args` as a decoded object
- AND the host returns the function's return value as JSON

#### Scenario: JSFX extension invoked via unified contract

- GIVEN a JSFX effect `saturator` is registered with id `saturator` and runtime `jsfx`
- WHEN the host invokes `{ id: "saturator", fn: "process", args: { track_index: 0 } }`
- THEN the host routes the call to the JSFX runtime
- AND the JSFX module processes the named track
- AND the host returns a JSON result indicating success or a structured error

### Requirement: Runtime Routing

The host MUST determine the target runtime by reading the extension's manifest at load time. The host MUST NOT inspect the extension source to decide the runtime.

#### Scenario: Manifest declares runtime

- GIVEN a manifest `extensions/foo/manifest.lua` declaring `runtime = "quickjs"`
- WHEN the host loads the extension
- THEN all invocations for id `foo` are routed to QuickJS
- AND no fallback routing is attempted

### Requirement: Structured Error Capture

The host MUST capture any runtime error (Lua panic, JS exception, JSFX compile error) and return it as a structured error object with at least: `runtime`, `message`, and `stack_or_trace`. The host MUST NOT propagate the error as an uncaught exception that crashes REAPER.

#### Scenario: Lua runtime error is captured

- GIVEN a Lua extension throws an error during execution
- WHEN the host invokes the extension
- THEN the host returns `{ ok: false, error: { runtime: "lua", message: "...", trace: "..." } }`
- AND REAPER continues running

#### Scenario: QuickJS exception is captured

- GIVEN a JS extension throws an uncaught exception
- WHEN the host invokes the extension
- THEN the host returns `{ ok: false, error: { runtime: "quickjs", message: "...", stack: "..." } }`
- AND the QuickJS context is recoverable for the next invocation

### Requirement: Idempotent Invocation Records

The host MUST support invoking the same extension id multiple times in sequence. State from a previous invocation MUST NOT leak into the next unless the extension explicitly persists it.

#### Scenario: Repeated invocation does not leak state

- GIVEN an extension is invoked twice in a row with identical args
- WHEN the host completes both invocations
- THEN both return the same logical result
- AND no global state from the first call affects the second
