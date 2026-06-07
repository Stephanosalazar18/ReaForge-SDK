# Runtime Bridge Specification

## Purpose

Define the thin C++ layer that exposes a minimal, read-only subset of the REAPER ReaScript API to all three runtimes (Lua, JSFX, QuickJS) through a uniform function signature and a uniform value representation. The bridge is the only sanctioned path for extension code to observe REAPER state.

## Requirements

### Requirement: Minimal Read-Only API Surface

The bridge MUST expose the following functions to every runtime, and MUST NOT expose any other REAPER API in the spike:

| Function | Signature | Returns |
|---|---|---|
| `get_cursor_position` | `()` | `number` (seconds) |
| `get_project_tempo` | `()` | `number` (BPM) |
| `get_track_count` | `()` | `integer` |
| `get_track_name` | `(index: integer)` | `string` or `nil` if out of range |
| `get_master_track_volume` | `()` | `number` (0.0 to 1.0) |

Any function not in this table MUST be unavailable to extension code during the spike.

#### Scenario: All five functions are callable from Lua

- GIVEN the host is loaded and the bridge is registered
- WHEN a Lua script calls each of the five functions
- THEN each call returns a value of the documented type
- AND no call to a non-listed function is reachable

#### Scenario: All five functions are callable from QuickJS

- GIVEN the host is loaded and the bridge is registered
- WHEN a JavaScript program calls each of the five functions
- THEN each call returns a value of the documented type
- AND no call to a non-listed function is reachable

### Requirement: Type-Safe Value Marshalling

The bridge MUST return values using each runtime's native type system: `number` for Lua, `number` for QuickJS, and a fixed-precision format compatible with JSFX. The bridge MUST NOT return a value as a string that the runtime must parse.

#### Scenario: Numeric return is a Lua number

- GIVEN a Lua script calls `bridge.get_cursor_position()`
- WHEN the bridge returns the current cursor position
- THEN the Lua value is of type `number`
- AND `type(result) == "number"` evaluates to `true`

#### Scenario: Numeric return is a JS number

- GIVEN a JS program calls `bridge.get_cursor_position()`
- WHEN the bridge returns the current cursor position
- THEN the JavaScript value is of type `number`
- AND `typeof result === "number"` evaluates to `true`

#### Scenario: Out-of-range index returns nil/null, not an error

- GIVEN a script calls `bridge.get_track_name(99999)` with a project that has 4 tracks
- WHEN the bridge returns
- THEN Lua receives `nil` and JS receives `null`
- AND no structured error is returned for an out-of-range read

### Requirement: Main-Thread Guard

The bridge MUST verify that calls originate from REAPER's main thread. Calls from any other thread MUST be rejected with a structured error and MUST NOT call into REAPER's API.

#### Scenario: Off-thread bridge call is rejected

- GIVEN a worker thread invokes a bridge function
- WHEN the bridge's main-thread guard runs
- THEN the bridge returns a structured error indicating the threading violation
- AND REAPER's main thread is not interrupted

### Requirement: No Write Access

The bridge MUST NOT expose any function that mutates REAPER state. Functions such as `SetCursorPosition`, `SetProjectTempo`, or any track-modifying API are out of scope for the spike.

#### Scenario: Write functions are not reachable

- GIVEN a script attempts to call a write function (e.g., by name, by reflection, or by guessing)
- WHEN the bridge resolves the call
- THEN the function is reported as undefined or rejected
- AND no REAPER state is modified
