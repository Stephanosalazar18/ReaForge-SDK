# Artifact Folder Convention Specification

## Purpose

Define the cross-cutting rule for where ReaForge-generated artifacts live on disk. The convention keeps every artifact under a `ReaForge/` subfolder within REAPER's native folders, derived from `reaper.GetResourcePath()` at extension start. Applies to all 3 Write tools, the refresh hook (which rescans these folders), and the artifact enumeration endpoint.

## Scope

| Concern | Value |
|---|---|
| Resource root | `reaper.GetResourcePath()` |
| Subfolders | `Effects/ReaForge/`, `Scripts/ReaForge/`, `FXChains/ReaForge/` |
| Applies to | `save_jsfx`, `save_lua`, `save_fx_chain`, `list_artifacts`, `refresh` |
| Lane | `cpp-unit` (filesystem) + `runtime-reaper` (path resolution) |
| PRs | 4 (writer), 5 (reader/listing), 6 (refresh) |

## Requirements

### Requirement: Resource Root is Derived from REAPER

`<REAPER resource>` MUST be the value returned by `reaper.GetResourcePath()` at extension start. The extension MUST NOT hardcode any path, user name, or platform-specific location. The result is cached for the lifetime of the extension.

```
resource_root = reaper.GetResourcePath()   // e.g.:
                                            //   Windows: "C:\\Users\\u\\AppData\\Roaming\\REAPER"
                                            //   macOS:   "/Users/u/Library/Application Support/REAPER"
                                            //   Linux:   "/home/u/.config/REAPER"
```

#### Scenario: resource path comes from REAPER

- GIVEN REAPER is running and the extension is loaded
- WHEN the extension's start hook runs
- THEN `resource_root` matches what `reaper.GetResourcePath()` returns at that moment
- AND the value is cached, not re-queried per request

#### Scenario: changing the resource path mid-session is not supported

- GIVEN the extension is loaded with `resource_root = "/old/path"`
- WHEN the user changes REAPER's resource path and triggers a refresh
- THEN the extension continues to use the cached `resource_root` (changing resource path requires an extension reload)

### Requirement: Three ReaForge Subfolders

All artifacts MUST live under one of these three subfolders, derived from `resource_root`:

| Artifact kind | Absolute path |
|---|---|
| JSFX | `<resource_root>/Effects/ReaForge/<name>.jsfx` |
| Lua | `<resource_root>/Scripts/ReaForge/<name>.lua` |
| FX chain | `<resource_root>/FXChains/ReaForge/<name>.RfxChain` |

No artifact may be written outside these three subfolders. The extension MUST refuse (with `INVALID_NAME` or a path-validation error) any path that resolves outside.

#### Scenario: write to a non-ReaForge folder is refused

- GIVEN a request that, after path resolution, would land outside the three `ReaForge/` subfolders
- WHEN the write is attempted
- THEN the response is `400 Bad Request` with `{"error":"INVALID_NAME","message":"path resolves outside ReaForge/ subfolders"}`

### Requirement: Auto-Create on First Write

If `<resource_root>/Effects/ReaForge/`, `<resource_root>/Scripts/ReaForge/`, or `<resource_root>/FXChains/ReaForge/` does not exist, the extension MUST `mkdir -p` the directory (and any necessary parents) before writing. No error is raised for the mkdir itself.

#### Scenario: Effects/ReaForge/ is created on demand

- GIVEN `<REAPER>/Effects/ReaForge/` does not exist
- WHEN `reaforge_save_jsfx` is called
- THEN the directory is created
- AND the file is written
- AND the response is `200 OK`

### Requirement: File Naming Constraints

The `name` parameter (without the extension suffix) MUST match the regex `^[A-Za-z0-9_\-]{1,64}$`:

- ASCII letters, digits, underscore, hyphen only.
- 1 to 64 characters.
- No path separators (`/`, `\`).
- No leading dot (no hidden files).
- No spaces, no unicode.

The extension appends the kind-specific suffix (`.jsfx`, `.lua`, `.RfxChain`) automatically. The agent MUST NOT include the suffix in the `name` argument.

#### Scenario: name with a slash is rejected

- WHEN `reaforge_save_jsfx` is called with `name="../escape"`
- THEN the response is `400` with `INVALID_NAME`

#### Scenario: name with spaces is rejected

- WHEN `reaforge_save_fx_chain` is called with `name="vocal slap"`
- THEN the response is `400` with `INVALID_NAME`

#### Scenario: name with suffix is treated as the literal name

- GIVEN the user passes `name="tape.jsfx"`
- WHEN `reaforge_save_jsfx` is called
- THEN the file written is `<REAPER>/Effects/ReaForge/tape.jsfx.jsfx` (the suffix is appended to whatever the user passed)

### Requirement: Overwrite Flag Semantics

A write to an existing file MUST be refused unless the request body contains `"overwrite": true`. This is enforced server-side in `artifact_writer.cpp` — the bridge cannot bypass it. The flag is the safety gate for the "regenerate" workflow.

| Existing file | `overwrite` in body | Outcome |
|---|---|---|
| absent | any | write succeeds, 200 |
| present | missing or `false` | 409 `FILE_EXISTS`, file unchanged |
| present | `true` | file replaced, 200 |

#### Scenario: overwrite=false preserves existing file

- GIVEN `<REAPER>/Effects/ReaForge/tape_saturation.jsfx` exists
- WHEN `save_jsfx` is called with `name="tape_saturation"`, `code="<new>"`, `overwrite=false`
- THEN the response is `409` with `FILE_EXISTS`
- AND the existing file's content and mtime are unchanged

#### Scenario: overwrite=true replaces the file

- GIVEN the same file
- WHEN `save_jsfx` is called with `name="tape_saturation"`, `code="<new>"`, `overwrite=true`
- THEN the response is `200` with the new `bytes_written`
- AND the file's content is replaced

### Requirement: Wipe is `rm -rf` on the Three Subfolders

The convention makes "what did the agent generate?" answerable by listing the three `ReaForge/` subfolders. The user can wipe all ReaForge-generated artifacts with:

```bash
rm -rf "<REAPER resource>/Effects/ReaForge"
rm -rf "<REAPER resource>/Scripts/ReaForge"
rm -rf "<REAPER resource>/FXChains/ReaForge"
```

The extension does NOT expose a "wipe" tool. The user does this manually when they want a clean slate.

#### Scenario: wipe removes only ReaForge artifacts

- GIVEN the user has their own `Effects/custom.jsfx` (outside `ReaForge/`) and a ReaForge-generated `Effects/ReaForge/tape.jsfx`
- WHEN the user runs the `rm -rf` command on the three `ReaForge/` subfolders
- THEN `tape.jsfx` is gone
- AND `custom.jsfx` is untouched

### Requirement: Forward-Slash Paths in Responses

All `path` fields in JSON responses MUST use forward slashes, regardless of platform. The C++ code may use the platform separator internally; the JSON layer normalizes to `/`.

#### Scenario: Windows path uses forward slashes in JSON

- GIVEN REAPER is on Windows and the resource root is `C:\Users\u\AppData\Roaming\REAPER`
- WHEN `GET /v1/artifacts?kind=jsfx` is called
- THEN every `path` starts with `C:/Users/u/AppData/Roaming/REAPER/Effects/ReaForge/...`

## API Contract

This spec is a cross-cutting rule. It does not define its own HTTP route. It is enforced inside:

| Endpoint | How |
|---|---|
| `POST /v1/jsfx` | resolves `<resource_root>/Effects/ReaForge/<name>.jsfx` |
| `POST /v1/lua` | resolves `<resource_root>/Scripts/ReaForge/<name>.lua` |
| `POST /v1/fxchain` | resolves `<resource_root>/FXChains/ReaForge/<name>.RfxChain` |
| `GET /v1/artifacts` | enumerates the three subfolders |
| `POST /v1/refresh` | rescans the same three subfolders |

## Cross-References

- `bridge-tools-v2/spec.md` — `name`, `overwrite`, and error codes referenced from the 3 Write tools.
- `extension-write-api/spec.md` — `artifact_writer.cpp` enforces the rules server-side.
- `extension-read-api/spec.md` — `GET /v1/artifacts` enumerates the same subfolders.
- `refresh-protocol/spec.md` — `RefreshFXList` and `RefreshActionList` operate on these folders.

## Affected PRs

| PR | Lane | What this spec ships |
|---|---|---|
| 4 | `cpp-unit` + `runtime-reaper` | `artifact_writer.cpp` implements the path resolution, mkdir-on-demand, name validation, overwrite flag enforcement, and forward-slash JSON output. |
| 5 | `cpp-unit` + `runtime-reaper` | `project_reader.cpp` enumerates the three subfolders for `GET /v1/artifacts`, missing-folder tolerant. |
| 6 | `runtime-reaper` | `refresh.cpp` calls REAPER's rescan on the same three subfolders. |
| 1 | `docs` | README quickstart documents the `ReaForge/` subfolder convention. |
