# Refresh Protocol Specification

## Purpose

Define the contract for `POST /v1/refresh` (backing `reaforge_refresh`). After the agent writes one or more artifacts, the user-facing REAPER UI (FX browser, Action List) does not see them until REAPER rescans its caches. This spec captures the REAPER API calls the extension MUST make, the idempotency contract, and the per-write-burst guidance for the agent.

## Scope

| Item | Value |
|---|---|
| HTTP route | `POST /v1/refresh` |
| Backing tool | `reaforge_refresh` |
| Source module | `src/host/refresh.{h,cpp}` (greenfield) |
| Lane | `runtime-reaper` |
| PR | 6 |

The HTTP route itself is registered in PR 4 (as a `501 Not Implemented` stub); the full REAPER-API implementation lands in PR 6.

## Requirements

### Requirement: Triggers REAPER Cache Rescan

`POST /v1/refresh` MUST trigger REAPER to rescan its FX list and Action List. The extension MUST call the following sequence, in order, on REAPER's main thread:

```
1. reaper.RefreshFXList()      // rescans Effects/ReaForge/ → FX browser
2. reaper.RefreshActionList()  // rescans Scripts/ → Action List (if exposed; else no-op + warn)
```

If `reaper.RefreshFXList()` is not exported by the REAPER build the extension is loaded into, the extension MUST log a warning and continue. The Action List rescan call is best-effort: if no such symbol exists, the call is skipped silently with a debug log.

#### Scenario: REAPER sees the new JSFX

- GIVEN `<REAPER>/Effects/ReaForge/tape_saturation.jsfx` was just written
- WHEN `POST /v1/refresh` is called
- THEN `reaper.RefreshFXList()` was called
- AND the user can find "tape_saturation" in the FX browser

#### Scenario: REAPER sees the newly registered Lua action

- GIVEN `<REAPER>/Scripts/ReaForge/double_velocity.lua` was just written and `register_action=true` was used
- WHEN `POST /v1/refresh` is called
- THEN the Action List contains "Script: ReaForge/double_velocity"

### Requirement: Idempotent

The refresh MUST be safe to call any number of times. Calling it twice in a row MUST NOT crash REAPER, MUST NOT duplicate FX entries, and MUST NOT duplicate Action List entries.

#### Scenario: repeated refresh does not duplicate entries

- GIVEN a single `<REAPER>/Effects/ReaForge/tape_saturation.jsfx` on disk
- WHEN `POST /v1/refresh` is called three times in succession
- THEN the FX browser still contains exactly one entry named "tape_saturation"

#### Scenario: refresh with no pending changes is harmless

- GIVEN no writes have happened since the last refresh
- WHEN `POST /v1/refresh` is called
- THEN the response is `200 OK` with a fresh `refreshed_at` timestamp
- AND REAPER's UI remains responsive

### Requirement: Main-Thread Discipline

The refresh calls MUST execute on REAPER's main thread. If the HTTP handler is invoked from a worker thread, the extension MUST marshal the call back to the main thread (e.g., via a `reaper.defer` callback or by posting a message) before invoking `RefreshFXList`/`RefreshActionList`.

#### Scenario: refresh from a worker thread is marshaled

- GIVEN the HTTP server is running on a worker thread
- WHEN `POST /v1/refresh` is called
- THEN the actual `RefreshFXList` call happens on REAPER's main thread
- AND the HTTP response waits for the main-thread call to complete before returning

### Requirement: Timestamped Response

The response MUST include an ISO 8601 UTC timestamp marking when the refresh completed:

```json
{"refreshed_at": "2026-06-07T21:34:12Z"}
```

The timestamp is informational; the agent does not need to act on it. It is useful for the agent to log the moment REAPER saw the artifact.

#### Scenario: response includes refreshed_at

- WHEN `POST /v1/refresh` is called
- THEN the response is `200 OK` with `{"refreshed_at": "<ISO8601>"}`
- AND the timestamp is after the time of the most recent write

### Requirement: Per-Write-Burst Guidance

The agent SHOULD call `reaforge_refresh` once per write burst (typically: once after all 3 Write tools in a single user prompt have been called), not after every single write. Repeated refresh calls have no benefit and add latency. The extension's contract allows it; the bridge MAY add a "burst cooldown" hint in a future iteration, but for MVP the agent decides.

#### Scenario: agent refreshes once after 3 writes

- GIVEN the user asks the agent to "create a JSFX, a Lua, and a chain"
- WHEN the agent calls `save_jsfx`, `save_lua` (no register), and `save_fx_chain` in sequence
- THEN it calls `refresh` exactly once after all three writes
- AND REAPER sees all three new artifacts

#### Scenario: agent refreshes after every write

- GIVEN the same scenario
- WHEN the agent calls `refresh` after each write
- THEN the final state is still correct (3 new artifacts visible)
- AND the user-perceived latency is higher (acceptable but suboptimal)

## API Contract

### `POST /v1/refresh`

| Field | Value |
|---|---|
| Request body | none |
| Success 200 | `{"refreshed_at": "<ISO8601 UTC>"}` |
| Errors | 500 `EXTENSION_UNREACHABLE` (transport-level — bridge returns this when the extension is down) |
| Effect | calls `reaper.RefreshFXList()` and `reaper.RefreshActionList()` on the main thread |
| Idempotency | safe to call repeatedly |
| Lane | `runtime-reaper` |
| PR | 6 (route stub in PR 4 returns 501) |

## Cross-References

- `bridge-tools-v2/spec.md` — the `reaforge_refresh` tool contract (no parameters, returns timestamp).
- `extension-write-api/spec.md` — registers the `POST /v1/refresh` route in PR 4 (501 stub) before PR 6 fills it in.
- `extension-read-api/spec.md` — the 3 Read endpoints; refresh is unrelated to them.

## Affected PRs

| PR | Lane | What this spec ships |
|---|---|---|
| 4 | `cpp-unit` | Registers the `POST /v1/refresh` route in `http_server.cpp` returning `501 NOT_IMPLEMENTED` |
| 6 | `runtime-reaper` | `refresh.{h,cpp}` greenfield. Implements the two REAPER API calls, main-thread marshaling, timestamped response, and idempotent semantics. Wire into `src/host/meson.build`. |
| 7 | `runtime-reaper` (manual) | End-to-end verify on REAPER Windows: write a JSFX + Lua + chain, call refresh once, confirm all three show up in REAPER's UI. |
