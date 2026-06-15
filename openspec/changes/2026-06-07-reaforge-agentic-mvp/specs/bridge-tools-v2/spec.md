# Bridge Tools v2 Specification

## Purpose

Define the contract for the 7 MCP tools exposed by `tools/opencode_bridge.py` to opencode Desktop. Tools split into 3 Read (context for the LLM), 3 Write (the agent's only side effect: persist REAPER-native files), and 1 Refresh (tell REAPER to rescan its caches). This spec is the agent-facing contract; HTTP wire details live in `extension-write-api` and `extension-read-api`.

## Scope

| Group | Count | Tools |
|---|---|---|
| Read | 3 | `reaforge_get_state`, `reaforge_list_artifacts`, `reaforge_get_api_reference` |
| Write | 3 | `reaforge_save_jsfx`, `reaforge_save_lua`, `reaforge_save_fx_chain` |
| Refresh | 1 | `reaforge_refresh` |

Naming prefix is `reaforge_*` (user-confirmed; not `rf_*`).

## Requirements

### Requirement: Tool Surface is Exactly 7

The bridge MUST expose exactly 7 tools named `reaforge_*`. The pre-pivot tools `reaforge_list_tracks`, `reaforge_list_extensions`, `reaforge_run_extension`, and `reaforge_health` MUST be removed. The 5→7 swap is a clean replacement, not a 5+2 hybrid.

#### Scenario: List returns the 7 tool names

- GIVEN the bridge is started
- WHEN opencode calls `tools/list`
- THEN the response contains exactly: `reaforge_get_state`, `reaforge_list_artifacts`, `reaforge_get_api_reference`, `reaforge_save_jsfx`, `reaforge_save_lua`, `reaforge_save_fx_chain`, `reaforge_refresh`
- AND none of the 5 pre-pivot tool names appear

#### Scenario: Smoke covers all 7 tools

- GIVEN `tools/run_bridge_smoke.sh` runs against the stub
- WHEN the smoke finishes
- THEN it asserts 7 calls, one per tool, and exits 0
- AND no assertion exists for a removed pre-pivot tool

### Requirement: Read Tools are Side-Effect Free

The 3 Read tools MUST NOT mutate REAPER state, write files, or change any cache. They return data only.

#### Scenario: Read tools do not touch the filesystem

- GIVEN a snapshot of `<REAPER>/Effects/ReaForge/`, `Scripts/ReaForge/`, `FXChains/ReaForge/` is taken
- WHEN `reaforge_get_state`, `reaforge_list_artifacts`, and `reaforge_get_api_reference` are called in sequence
- THEN the snapshot's mtimes are unchanged

### Requirement: Write Tools Refuse Without Explicit Overwrite

The 3 Write tools MUST refuse to overwrite an existing file unless the caller passes `overwrite=true`. The flag exists for safety; the LLM MUST default to not overwriting.

#### Scenario: save refuses when file exists and overwrite is false

- GIVEN `<REAPER>/Effects/ReaForge/tape_saturation.jsfx` already exists
- WHEN `reaforge_save_jsfx` is called with `name="tape_saturation"`, `code="..."`, `overwrite=false`
- THEN the tool returns a structured error `{"error": "FILE_EXISTS", "path": "..."}`
- AND the existing file is unchanged

#### Scenario: save overwrites when overwrite is true

- GIVEN `<REAPER>/Effects/ReaForge/tape_saturation.jsfx` already exists
- WHEN `reaforge_save_jsfx` is called with `name="tape_saturation"`, `code="<new>"`, `overwrite=true`
- THEN the file is replaced
- AND the response includes `{"path": "...", "bytes_written": N}`

### Requirement: save_lua Registers Action Only When Opted In

`reaforge_save_lua` MUST default `register_action` to `false`. When the caller passes `register_action=true`, the bridge MUST tell the extension to register via `reaper.AddRemoveReaScript()`. When false, no registration happens.

#### Scenario: save_lua default does not register

- GIVEN a fresh `<REAPER>/Scripts/ReaForge/` folder
- WHEN `reaforge_save_lua` is called with `name="double_velocity"`, `code="..."` (no `register_action`)
- THEN the file is written
- AND the response `action_id` field is `null` / absent
- AND REAPER's action list does not gain a new entry

#### Scenario: save_lua with register_action=true registers

- GIVEN the extension is running inside REAPER
- WHEN `reaforge_save_lua` is called with `name="double_velocity"`, `code="..."`, `register_action=true`
- THEN the file is written
- AND `reaper.AddRemoveReaScript(0, 0)` is called first to clear, then `reaper.AddRemoveReaScript(1, path, true)` to register
- AND the response includes a non-null `action_id`

### Requirement: Refresh is Idempotent

`reaforge_refresh` MUST be safe to call multiple times. Each call MUST trigger REAPER's FX list and Action List rescan; calling it twice MUST NOT crash REAPER or duplicate entries.

#### Scenario: Repeated refresh is safe

- GIVEN a refresh has just been called
- WHEN `reaforge_refresh` is called again immediately
- THEN REAPER's FX list still contains the new artifact exactly once
- AND REAPER's action list still contains the registered script exactly once
- AND the tool returns `{"refreshed_at": "<ISO8601>"}`

### Requirement: Structured Errors

Every tool MUST return a structured JSON error object (never raise an uncaught exception to the LLM) on failure. Error shape:

```json
{"error": "<CODE>", "message": "<human-readable>", "path": "<optional>"}
```

| Code | Meaning |
|---|---|
| `EXTENSION_UNREACHABLE` | HTTP call to the extension failed (timeout, connection refused) |
| `PROJECT_NOT_OPEN` | `reaforge_get_state` called with no active REAPER project |
| `INVALID_TARGET` | `reaforge_get_api_reference` got a `target` outside the enum |
| `INVALID_NAME` | `name` is empty, contains `/` or `\`, or starts with `.` |
| `FILE_EXISTS` | target file exists and `overwrite` is not `true` |
| `WRITE_FAILED` | filesystem write failed (permission, disk full, etc.) |
| `REGISTER_FAILED` | `AddRemoveReaScript` rejected the registration |
| `FOLDER_NOT_FOUND` | `reaforge_list_artifacts` for a `kind` whose folder does not exist — MUST return `{artifacts: []}` instead of erroring |

#### Scenario: bridge never leaks stack traces

- GIVEN the extension is stopped
- WHEN any tool is called
- THEN the bridge returns `{"error": "EXTENSION_UNREACHABLE", "message": "..."}` (not a Python traceback)

## API Contract — Read Tools

### `reaforge_get_state(summary: bool = false)`

| Field | Value |
|---|---|
| Lane | `runtime-reaper` |
| PR | 5 |
| Endpoint | `GET /v1/state?summary=<bool>` |
| Returns | `{project_name, sample_rate, bpm, tracks: [{id, name, selected, fx_count, selected_item_count}], selected_items_count, ...}` |
| Errors | `EXTENSION_UNREACHABLE`, `PROJECT_NOT_OPEN` |

#### Scenario: full state returns per-track FX name list

- GIVEN a project with track "Vocals" that has ReaEQ and a delay loaded
- WHEN `reaforge_get_state` is called with no `summary`
- THEN the returned `tracks[]` entry for "Vocals" includes `fx_count: 2` and an `fx_names: ["ReaEQ", "Delay"]` list

#### Scenario: summary flag truncates

- GIVEN the same project
- WHEN `reaforge_get_state` is called with `summary=true`
- THEN per-track `fx_names` is omitted
- AND only `fx_count` and `selected_item_count` are returned

### `reaforge_list_artifacts(kind?: "jsfx" | "lua" | "fx_chain")`

| Field | Value |
|---|---|
| Lane | `cpp-unit` (filesystem only) |
| PR | 5 |
| Endpoint | `GET /v1/artifacts?kind=<kind>` |
| Returns | `{artifacts: [{path, size, mtime, kind}]}` |
| Errors | none (missing folder ⇒ empty list) |

#### Scenario: kind omitted returns all three folders

- GIVEN the user has 2 `.jsfx`, 1 `.lua`, 0 `.RfxChain` files
- WHEN `reaforge_list_artifacts` is called with no `kind`
- THEN `artifacts.length == 3`
- AND every entry has a `kind` field

#### Scenario: kind=jsfx lists only Effects/ReaForge

- GIVEN 2 `.jsfx` and 1 `.lua`
- WHEN `reaforge_list_artifacts` is called with `kind="jsfx"`
- THEN `artifacts.length == 2` and all entries have `kind: "jsfx"`

#### Scenario: missing folder is not an error

- GIVEN `<REAPER>/FXChains/ReaForge/` does not exist
- WHEN `reaforge_list_artifacts` is called with `kind="fx_chain"`
- THEN the tool returns `{artifacts: []}` (no error)

### `reaforge_get_api_reference(target: "jsfx" | "reascript_lua" | "fx_chain_format")`

| Field | Value |
|---|---|
| Lane | `cpp-unit` (filesystem only) |
| PR | 5 |
| Endpoint | `GET /v1/api-reference?target=<target>` |
| Returns | `{target, reference: "<markdown string>"}` |
| Errors | `INVALID_TARGET` |

#### Scenario: target=jsfx returns JSFX cheatsheet

- GIVEN `docs/api_reference/jsfx.md` is bundled with the extension
- WHEN `reaforge_get_api_reference` is called with `target="jsfx"`
- THEN `reference` contains the contents of that file

#### Scenario: bad target returns INVALID_TARGET

- WHEN `reaforge_get_api_reference` is called with `target="python"`
- THEN the response is `{"error": "INVALID_TARGET", "message": "...", "target": "python"}`

## API Contract — Write Tools

### `reaforge_save_jsfx(name, code, overwrite=false)`

| Field | Value |
|---|---|
| Lane | `cpp-unit` (path validation + write + mkdir) |
| PR | 4 |
| Endpoint | `POST /v1/jsfx` with `{name, code, overwrite?}` |
| Effect | writes `<REAPER>/Effects/ReaForge/<name>.jsfx` |
| Returns | `{path, bytes_written}` |
| Errors | `INVALID_NAME`, `WRITE_FAILED`, `FILE_EXISTS` |

#### Scenario: writes the file and creates the subfolder

- GIVEN `<REAPER>/Effects/ReaForge/` does not exist
- WHEN `reaforge_save_jsfx` is called with `name="tape_saturation"`, `code="desc:..."`
- THEN the folder is created
- AND `tape_saturation.jsfx` exists at the expected path
- AND the response includes `bytes_written` matching the input length

### `reaforge_save_lua(name, code, register_action=false, overwrite=false)`

| Field | Value |
|---|---|
| Lane | `cpp-unit` (write) + `runtime-reaper` (`AddRemoveReaScript`) |
| PR | 4 |
| Endpoint | `POST /v1/lua` with `{name, code, register_action?, overwrite?}` |
| Effect | writes `<REAPER>/Scripts/ReaForge/<name>.lua`; if `register_action=true`, calls `reaper.AddRemoveReaScript` |
| Returns | `{path, action_id?}` |
| Errors | `INVALID_NAME`, `WRITE_FAILED`, `FILE_EXISTS`, `REGISTER_FAILED` |

#### Scenario: opt-in registration succeeds

- GIVEN the extension is loaded inside REAPER
- WHEN `reaforge_save_lua` is called with `name="double_velocity"`, `code="..."`, `register_action=true`
- THEN the file is written
- AND `action_id` is a positive integer
- AND the action appears in REAPER's action list as "Script: ReaForge/double_velocity"

### `reaforge_save_fx_chain(name, content, overwrite=false)`

| Field | Value |
|---|---|
| Lane | `cpp-unit` |
| PR | 4 |
| Endpoint | `POST /v1/fxchain` with `{name, content, overwrite?}` |
| Effect | writes `<REAPER>/FXChains/ReaForge/<name>.RfxChain` |
| Returns | `{path, bytes_written}` |
| Errors | `INVALID_NAME`, `WRITE_FAILED`, `FILE_EXISTS` |

#### Scenario: writes a chain file

- GIVEN the FXChains/ReaForge/ folder is empty
- WHEN `reaforge_save_fx_chain` is called with `name="vocal_slap"`, `content="<RfxChain XML>"`
- THEN `vocal_slap.RfxChain` exists
- AND the file is valid RfxChain XML

## API Contract — Refresh Tool

### `reaforge_refresh()`

| Field | Value |
|---|---|
| Lane | `runtime-reaper` |
| PR | 6 |
| Endpoint | `POST /v1/refresh` (no body) |
| Effect | triggers REAPER's FX list rescan and Action List rescan |
| Returns | `{refreshed_at: <ISO8601>}` |
| Errors | `EXTENSION_UNREACHABLE` |

See `refresh-protocol` for the underlying REAPER API calls and idempotency proof.

## Lane Assignment Summary

| Tool | Lane | PR | HTTP endpoint |
|---|---|---|---|
| `reaforge_get_state` | `runtime-reaper` | 5 | `GET /v1/state` |
| `reaforge_list_artifacts` | `cpp-unit` | 5 | `GET /v1/artifacts` |
| `reaforge_get_api_reference` | `cpp-unit` | 5 | `GET /v1/api-reference` |
| `reaforge_save_jsfx` | `cpp-unit` | 4 | `POST /v1/jsfx` |
| `reaforge_save_lua` (write) | `cpp-unit` | 4 | `POST /v1/lua` |
| `reaforge_save_lua` (register) | `runtime-reaper` | 4 | `POST /v1/lua` |
| `reaforge_save_fx_chain` | `cpp-unit` | 4 | `POST /v1/fxchain` |
| `reaforge_refresh` | `runtime-reaper` | 6 | `POST /v1/refresh` |

## Out of Scope

- Tools that mutate live REAPER state (rejected by user).
- Confirmation UI inside REAPER (opencode's tool approval is the gate).
- Templates, versioning, git integration.
- Authentication, multi-user, cloud sync.

## Affected PRs

| PR | Lane | This spec applies |
|---|---|---|
| 2 | `python-tdd` | Tool schemas, error shape, `overwrite`/`register_action` flags |
| 3 | `python-tdd` | Smoke covers all 7 |
| 4 | `cpp-unit` + `runtime-reaper` | Wire tools to write endpoints |
| 5 | `cpp-unit` + `runtime-reaper` | Wire tools to read endpoints |
| 6 | `runtime-reaper` | Wire `reaforge_refresh` |
