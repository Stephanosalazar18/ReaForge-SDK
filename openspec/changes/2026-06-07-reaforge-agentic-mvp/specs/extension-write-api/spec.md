# Extension Write API Specification

## Purpose

Define the HTTP endpoints exposed by the greenfield C++ extension (`src/host/http_server.cpp` + `src/host/artifact_writer.cpp`) that back the 3 Write tools and the refresh endpoint. The extension writes REAPER-native files into the `ReaForge/` subfolders under `<REAPER resource>/` and is the **only** sanctioned path for the agent to persist artifacts in this MVP. No direct REAPER state mutation.

## Scope

| HTTP route | Backing tool | Source module |
|---|---|---|
| `POST /v1/jsfx` | `reaforge_save_jsfx` | `artifact_writer.cpp` |
| `POST /v1/lua` | `reaforge_save_lua` | `artifact_writer.cpp` + `refresh.cpp` (on `register_action=true`) |
| `POST /v1/fxchain` | `reaforge_save_fx_chain` | `artifact_writer.cpp` |
| `POST /v1/refresh` | `reaforge_refresh` | `refresh.cpp` (see also `refresh-protocol`) |

The HTTP server itself (`http_server.cpp`) is a routing shell with JSON request/response. Per-tool filesystem and REAPER-API logic lives in `artifact_writer.cpp` and `refresh.cpp`. Both source files are **greenfield** — `src/host/` has no HTTP code today.

## Resource Path Resolution

The extension MUST derive `<REAPER resource>/` from REAPER's API at extension start:

```
resource_root = reaper.GetResourcePath()   // e.g. "C:\Users\<u>\AppData\Roaming\REAPER"
```

All write paths are computed as:

```
effects_dir  = <resource_root>/Effects/ReaForge
scripts_dir  = <resource_root>/Scripts/ReaForge
fxchains_dir = <resource_root>/FXChains/ReaForge
```

If `Effects/ReaForge/`, `Scripts/ReaForge/`, or `FXChains/ReaForge/` does not exist, the writer MUST `mkdir -p` it before writing. No error is raised for the mkdir itself.

The folder convention itself is a cross-cutting rule; see `artifact-folder-convention/spec.md`.

## Requirements

### Requirement: All Write Endpoints Return JSON

The 3 Write endpoints MUST accept `Content-Type: application/json`, parse the body, and return `Content-Type: application/json`. Successful responses return HTTP 200. Errors use the codes in the table below.

#### Scenario: jsfx write returns 200 with path and bytes_written

- GIVEN `<REAPER>/Effects/ReaForge/` exists
- WHEN `POST /v1/jsfx` is called with `{"name":"tape_saturation","code":"desc:..."}`
- THEN the response is `200 OK` with body `{"path":"<abs path>","bytes_written":42}`

#### Scenario: invalid JSON returns 400

- WHEN `POST /v1/jsfx` is called with body `not-json`
- THEN the response is `400 Bad Request` with `{"error":"INVALID_JSON","message":"..."}`

### Requirement: Overwrite Gating is Enforced Server-Side

The extension MUST refuse to overwrite an existing file when the request body does not contain `"overwrite": true`. The bridge cannot bypass this — `overwrite` is a per-request contract enforced inside `artifact_writer.cpp`.

| HTTP status | Error code | Trigger |
|---|---|---|
| 200 | — | file created or `overwrite=true` and file replaced |
| 400 | `INVALID_NAME` | `name` is empty, contains `/` or `\`, or starts with `.` |
| 400 | `INVALID_JSON` | body is not valid JSON |
| 409 | `FILE_EXISTS` | target file exists and `overwrite` is missing or `false` |
| 500 | `WRITE_FAILED` | filesystem write failed (permission, disk full) |
| 500 | `REGISTER_FAILED` | `AddRemoveReaScript` rejected the registration |

#### Scenario: extension refuses overwrite

- GIVEN `<REAPER>/Effects/ReaForge/foo.jsfx` exists
- WHEN `POST /v1/jsfx` is called with `{"name":"foo","code":"x"}` (no `overwrite`)
- THEN the response is `409 Conflict` with `{"error":"FILE_EXISTS","path":"<abs path>"}`
- AND the existing file's content is unchanged

### Requirement: Name Validation

`name` MUST match the regex `^[A-Za-z0-9_\-]{1,64}$`. No path separators, no leading dot, no spaces. The extension rejects before any filesystem touch.

#### Scenario: name with slash is rejected

- WHEN `POST /v1/jsfx` is called with `{"name":"../escape","code":"x"}`
- THEN the response is `400 Bad Request` with `{"error":"INVALID_NAME","message":"name must match ^[A-Za-z0-9_\\-]{1,64}$"}`

#### Scenario: empty name is rejected

- WHEN `POST /v1/jsfx` is called with `{"name":"","code":"x"}`
- THEN the response is `400` with `INVALID_NAME`

### Requirement: Write Order on Disk

`artifact_writer.cpp` MUST follow this sequence for any write:

1. Validate `name` (regex).
2. Resolve the absolute path (resolve `~` and env vars, refuse if outside the three `ReaForge/` subfolders).
3. If `overwrite != true` and the file already exists, return `409 FILE_EXISTS`.
4. `mkdir -p` the parent directory.
5. Write the file atomically (write to `<path>.tmp` then rename).
6. Return `{path, bytes_written}`.

#### Scenario: parent folder is created on demand

- GIVEN `<REAPER>/Scripts/ReaForge/` does not exist
- WHEN `POST /v1/lua` is called with `{"name":"double_velocity","code":"-- ..."}`
- THEN the extension creates the directory
- AND the file is written
- AND the response is `200`

#### Scenario: atomic write leaves no partial file

- GIVEN a write is in progress
- WHEN the process is killed mid-write
- THEN the original file is intact or absent (never a half-written file at the final path)
- AND no `<name>.jsfx.tmp` lingers

### Requirement: register_action Flow for save_lua

When `POST /v1/lua` is called with `"register_action": true`, the extension MUST perform the registration step after a successful write. See the `refresh-protocol` for the related `RefreshActionList` step.

```
1. write the .lua file (same as above)
2. call reaper.AddRemoveReaScript(0, 0)            // unregister any prior copy under same path
3. call reaper.AddRemoveReaScript(1, <abs_path>, true)  // register
4. capture the action id (reaper.NamedCommandLookup or REAPER's returned handle)
5. return {path, action_id}
```

`register_action` MUST default to `false` in the request schema.

#### Scenario: register_action=true returns action_id

- GIVEN the extension is loaded inside REAPER
- WHEN `POST /v1/lua` is called with `{"name":"double_velocity","code":"-- ...","register_action":true}`
- THEN the file is written
- AND the response is `200` with `{"path":"<abs>","action_id":<int>}`
- AND REAPER's action list gains "Script: ReaForge/double_velocity"

#### Scenario: register_action=false omits action_id

- WHEN `POST /v1/lua` is called with `register_action` absent
- THEN the file is written
- AND the response is `200` with `{"path":"<abs>"}` (no `action_id` field)
- AND REAPER's action list is unchanged

#### Scenario: register_action=true but AddRemoveReaScript fails

- GIVEN REAPER rejects the registration (corrupt state, quota, etc.)
- WHEN `POST /v1/lua` is called with `register_action=true`
- THEN the file is still written
- AND the response is `500 Internal Server Error` with `{"error":"REGISTER_FAILED","message":"..."}`
- AND the file on disk is left in place (registration is a best-effort post-step)

### Requirement: Refresh Endpoint Stub

`POST /v1/refresh` MUST exist as a route in the HTTP server from PR 4 (so the bridge can call it without 404 during the Write PR's smoke). The actual REAPER-API implementation lands in PR 6 (`refresh.cpp`); in PR 4 the route returns a placeholder `501 Not Implemented` for the refresh logic, with the 3 Write endpoints fully working. The full contract is in `refresh-protocol/spec.md`.

#### Scenario: refresh returns 501 in PR 4

- GIVEN only PR 4 has landed (no `refresh.cpp` yet)
- WHEN `POST /v1/refresh` is called
- THEN the response is `501 Not Implemented` with `{"error":"NOT_IMPLEMENTED","message":"see refresh-protocol"}`

## API Contract

### `POST /v1/jsfx`

| Field | Value |
|---|---|
| Request | `{"name": string, "code": string, "overwrite"?: bool = false}` |
| Success 200 | `{"path": string, "bytes_written": int}` |
| Errors | 400 `INVALID_NAME`, 400 `INVALID_JSON`, 409 `FILE_EXISTS`, 500 `WRITE_FAILED` |
| Effect | writes `<REAPER>/Effects/ReaForge/<name>.jsfx` |
| Lane | `cpp-unit` |
| PR | 4 |

### `POST /v1/lua`

| Field | Value |
|---|---|
| Request | `{"name": string, "code": string, "register_action"?: bool = false, "overwrite"?: bool = false}` |
| Success 200 (no register) | `{"path": string}` |
| Success 200 (register=true) | `{"path": string, "action_id": int}` |
| Errors | 400 `INVALID_NAME`, 400 `INVALID_JSON`, 409 `FILE_EXISTS`, 500 `WRITE_FAILED`, 500 `REGISTER_FAILED` |
| Effect | writes `<REAPER>/Scripts/ReaForge/<name>.lua`; optionally registers via `reaper.AddRemoveReaScript` |
| Lane | `cpp-unit` (write) + `runtime-reaper` (register) |
| PR | 4 |

### `POST /v1/fxchain`

| Field | Value |
|---|---|
| Request | `{"name": string, "content": string, "overwrite"?: bool = false}` |
| Success 200 | `{"path": string, "bytes_written": int}` |
| Errors | 400 `INVALID_NAME`, 400 `INVALID_JSON`, 409 `FILE_EXISTS`, 500 `WRITE_FAILED` |
| Effect | writes `<REAPER>/FXChains/ReaForge/<name>.RfxChain` |
| Lane | `cpp-unit` |
| PR | 4 |

### `POST /v1/refresh`

See `refresh-protocol/spec.md` for the full contract. Stubbed in PR 4 (returns 501), fully implemented in PR 6.

## Cross-References

- `bridge-tools-v2/spec.md` — the agent-facing tool contracts that this HTTP API backs.
- `extension-read-api/spec.md` — the 3 Read endpoints (`/v1/state`, `/v1/artifacts`, `/v1/api-reference`).
- `refresh-protocol/spec.md` — full `POST /v1/refresh` semantics, idempotency, REAPER API calls.
- `artifact-folder-convention/spec.md` — the `<REAPER>/{Effects,Scripts,FXChains}/ReaForge/` rule.

## Affected PRs

| PR | Lane | What this spec ships |
|---|---|---|
| 4 | `cpp-unit` + `runtime-reaper` | All 3 Write endpoints + `POST /v1/refresh` route stub. `http_server.{h,cpp}` and `artifact_writer.{h,cpp}` are greenfield. Wire into `src/host/meson.build`. |
| 6 | `runtime-reaper` | Full `POST /v1/refresh` implementation in `refresh.cpp`. |
