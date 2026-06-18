# Extension Read API Specification

## Purpose

Define the HTTP endpoints that back the 3 Read tools (`reaforge_get_state`, `reaforge_list_artifacts`, `reaforge_get_api_reference`) and the embedded API reference payload format. Reads are side-effect free. The project-state endpoint touches REAPER's API; the other two are pure filesystem/embedded-payload reads.

## Scope

| HTTP route | Backing tool | Source module |
|---|---|---|
| `GET /v1/state` | `reaforge_get_state` | `project_reader.cpp` |
| `GET /v1/artifacts` | `reaforge_list_artifacts` | `project_reader.cpp` (filesystem part) |
| `GET /v1/api-reference` | `reaforge_get_api_reference` | `project_reader.cpp` (embedded payload) |

All three are **greenfield** endpoints — `src/host/http_server.cpp` does not exist today.

## Requirements

### Requirement: All Read Endpoints are GET

The 3 Read endpoints MUST be HTTP `GET`. No body is required. The state endpoint accepts an optional `?summary=<bool>` query parameter.

#### Scenario: GET with no body is sufficient

- WHEN the bridge calls `GET /v1/artifacts?kind=jsfx`
- THEN the response is `200 OK` with a JSON body
- AND no request body is sent

### Requirement: Compact Project Projection

`GET /v1/state` MUST return a compact projection of the current REAPER project. No audio data. The full schema:

```json
{
  "project_name": "demo.rpp",
  "sample_rate": 48000,
  "bpm": 120.0,
  "tracks": [
    {
      "id": 0,
      "name": "Vocals",
      "selected": true,
      "fx_count": 2,
      "fx_names": ["ReaEQ", "Delay"],
      "selected_item_count": 1
    }
  ],
  "selected_items_count": 1
}
```

#### Scenario: full state returns per-track FX names

- GIVEN a project where "Vocals" has ReaEQ + Delay loaded
- WHEN `GET /v1/state` is called
- THEN the `tracks[Vocals]` entry has `fx_count: 2` and `fx_names: ["ReaEQ", "Delay"]`

#### Scenario: summary=true omits per-FX names

- GIVEN the same project
- WHEN `GET /v1/state?summary=true` is called
- THEN per-track `fx_names` is omitted from every track entry
- AND `fx_count` is still present

### Requirement: PROJECT_NOT_OPEN When No Project

`GET /v1/state` MUST return `409 Conflict` with `{"error":"PROJECT_NOT_OPEN"}` when REAPER has no active project (e.g., the user closed all projects).

#### Scenario: no project returns PROJECT_NOT_OPEN

- GIVEN REAPER has no open project
- WHEN `GET /v1/state` is called
- THEN the response is `409` with `{"error":"PROJECT_NOT_OPEN","message":"..."}`

### Requirement: Artifact Enumeration is Missing-Folder Tolerant

`GET /v1/artifacts` MUST NOT error when one of the three `ReaForge/` subfolders is missing. A missing folder yields an empty list for that `kind`.

| Query | Returns |
|---|---|
| no `kind` | all three folders merged, each entry with a `kind` field |
| `kind=jsfx` | only `Effects/ReaForge/*.jsfx` |
| `kind=lua` | only `Scripts/ReaForge/*.lua` |
| `kind=fx_chain` | only `FXChains/ReaForge/*.RfxChain` |
| `kind=<other>` | `400` with `INVALID_KIND` |

Per-entry shape:

```json
{"path": "<abs path>", "size": 1234, "mtime": 1717000000, "kind": "jsfx"}
```

#### Scenario: missing FXChains folder returns empty list

- GIVEN `<REAPER>/FXChains/ReaForge/` does not exist
- WHEN `GET /v1/artifacts?kind=fx_chain` is called
- THEN the response is `200` with `{"artifacts":[]}` (no error)

#### Scenario: kind omitted returns merged list

- GIVEN 2 `.jsfx`, 1 `.lua`, 0 `.RfxChain`
- WHEN `GET /v1/artifacts` is called with no `kind`
- THEN `artifacts.length == 3` and each entry has its `kind` field set correctly

### Requirement: Embedded API Reference

`GET /v1/api-reference` MUST return one of three markdown payloads bundled with the extension. The payloads are static files inside the extension resources directory:

| `target` | Source file |
|---|---|
| `jsfx` | `docs/api_reference/jsfx.md` (relative to extension resources root) |
| `reascript_lua` | `docs/api_reference/reascript_lua.md` |
| `fx_chain_format` | `docs/api_reference/fx_chain_format.md` |

The response body is:

```json
{"target": "jsfx", "reference": "<markdown content as a string>"}
```

The payloads are **offline-first** — no network fetch, no URL. The extension reads the file at startup and caches it in memory; `GET /v1/api-reference` returns the cached copy.

#### Scenario: target=jsfx returns the JSFX cheatsheet

- GIVEN the extension is built with `docs/api_reference/jsfx.md` as an embedded resource
- WHEN `GET /v1/api-reference?target=jsfx` is called
- THEN `reference` contains the contents of that file
- AND no network request is made (offline)

#### Scenario: invalid target returns 400

- WHEN `GET /v1/api-reference?target=python` is called
- THEN the response is `400` with `{"error":"INVALID_TARGET","target":"python"}`

### Requirement: Cross-Platform Path Emission

All `path` fields in responses MUST use forward slashes (REAPER accepts them on all platforms). This is what the agent and the LLM see; internal C++ code may use the platform separator but the JSON response normalizes to `/`.

#### Scenario: Windows path uses forward slashes

- GIVEN REAPER is running on Windows and the resource path is `C:\Users\u\AppData\Roaming\REAPER`
- WHEN `GET /v1/artifacts?kind=jsfx` is called
- THEN each `path` starts with `C:/Users/u/AppData/Roaming/REAPER/Effects/ReaForge/...`

## API Contract

### `GET /v1/state`

| Field | Value |
|---|---|
| Query | `summary?: bool` |
| Success 200 | `{"project_name":string, "sample_rate":int, "bpm":number, "tracks":[{...}], "selected_items_count":int}` |
| Errors | 409 `PROJECT_NOT_OPEN`, 500 `EXTENSION_UNREACHABLE` (when extension is down — bridge-side) |
| Lane | `runtime-reaper` |
| PR | 5 |

### `GET /v1/artifacts`

| Field | Value |
|---|---|
| Query | `kind?: "jsfx"|"lua"|"fx_chain"` |
| Success 200 | `{"artifacts": [{"path":string, "size":int, "mtime":int, "kind":string}]}` |
| Errors | 400 `INVALID_KIND` |
| Lane | `cpp-unit` (filesystem only) |
| PR | 5 |

### `GET /v1/api-reference`

| Field | Value |
|---|---|
| Query | `target: "jsfx"|"reascript_lua"|"fx_chain_format"` (required) |
| Success 200 | `{"target":string, "reference":string}` |
| Errors | 400 `INVALID_TARGET`, 400 `MISSING_TARGET` |
| Lane | `cpp-unit` (filesystem only — embedded payload) |
| PR | 5 |

## Cross-References

- `bridge-tools-v2/spec.md` — the agent-facing tool contracts that this HTTP API backs.
- `extension-write-api/spec.md` — the 3 Write endpoints + refresh route.
- `artifact-folder-convention/spec.md` — folder rules that `GET /v1/artifacts` enumerates.

## Affected PRs

| PR | Lane | What this spec ships |
|---|---|---|
| 5 | `cpp-unit` + `runtime-reaper` | All 3 Read endpoints. `project_reader.{h,cpp}` is greenfield. Wire into `src/host/meson.build`. The `docs/api_reference/*.md` payloads are added as extension resources. |
