# Design: ReaForge Agentic MVP

> Tech design for the **greenfield C++ code** in `src/host/`. Aligns with the 5 specs in `specs/` and the 7-PR breakdown in `proposal.md`. The HTTP server, project reader, artifact writer, and refresh hook are **new code** — `src/host/` has zero HTTP/network code today (Engram #80).

## TL;DR

- **Library choices**: `cpp-httplib` (vendored, header-only) for HTTP routing; `nlohmann/json` (vendored, header-only) for JSON. Both single-header, zero-dependency, no link-time surprises for a REAPER DLL.
- **Module split**: `http_server.{h,cpp}` (routing + JSON envelope) + `artifact_writer.{h,cpp}` (path + name validation + atomic write) + `project_reader.{h,cpp}` (REAPER state, artifact enumeration, embedded API reference) + `refresh.{h,cpp}` (REAPER cache rescan, marshaled to main thread).
- **Threading**: HTTP server runs on a dedicated worker thread. All REAPER-bound calls (`GetTrack`, `TrackFX_GetFXName`, `AddRemoveReaScript`, `RefreshFXList`, etc.) are marshaled to REAPER's main thread via a single `reaper.defer()`-style queue and a `std::condition_variable` rendezvous for the synchronous responses.
- **API reference payload**: embedded as a C array compiled into the DLL (no runtime file lookup, no install-time copy step, no path resolution).
- **Two spec corrections** the design must make (see "Spec corrections" below): `RefreshFXList` is not in the public REAPER SDK — fall back to `Main_OnCommand` for an action-list rescan and document manual FX-browser rescan. `AddRemoveReaScript` signature is `(bool add, int sectionID, const char* scriptfn, bool commit)`, not the shape the spec wrote.

## Architecture overview

```
┌─ opencode desktop (external) ─────────────────────────────────────────┐
│  LLM decides what to write → calls MCP tool  (e.g. save_jsfx)         │
└────────────────────────┬─────────────────────────────────────────────┘
                         │ MCP (stdio)
┌────────────────────────▼─────────────────────────────────────────────┐
│  tools/opencode_bridge.py  (Python)                                  │
│   - validates tool name, params, name regex                           │
│   - serializes to JSON, POSTs to extension                            │
└────────────────────────┬─────────────────────────────────────────────┘
                         │ HTTP/JSON  (POST/GET http://<wsl-ip>:7800/v1/...)
┌────────────────────────▼─────────────────────────────────────────────┐
│  reaper_reaforge_host.dll  (C++, runs inside REAPER.exe)             │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │ http_server.cpp   ── cpp-httplib server (worker thread)        │  │
│  │   ├─ POST /v1/jsfx      ─► artifact_writer::save_jsfx()        │  │
│  │   ├─ POST /v1/lua       ─► artifact_writer::save_lua()         │  │
│  │   ├─ POST /v1/fxchain   ─► artifact_writer::save_fx_chain()    │  │
│  │   ├─ POST /v1/refresh   ─► refresh::run()                      │  │
│  │   ├─ GET  /v1/state     ─► project_reader::get_state()         │  │
│  │   ├─ GET  /v1/artifacts ─► project_reader::list_artifacts()    │  │
│  │   ├─ GET  /v1/api-reference ─► project_reader::get_api_ref()   │  │
│  │   └─ GET  /v1/health    ─► 200 OK                               │  │
│  │   All REAPER-bound calls marshaled via main_thread_queue       │  │
│  └────────────────────────────────────────────────────────────────┘  │
│  ReaForge/                                                            │
│   Effects/ReaForge/<name>.jsfx                                        │
│   Scripts/ReaForge/<name>.lua                                         │
│   FXChains/ReaForge/<name>.RfxChain                                   │
└──────────────────────────────────────────────────────────────────────┘
```

## New C++ modules (file layout)

| File | Lane | Purpose |
|---|---|---|
| `src/host/http_server.{h,cpp}` | `cpp-unit` | HTTP server (cpp-httplib), 7 routes, JSON envelope, error codes. |
| `src/host/artifact_writer.{h,cpp}` | `cpp-unit` | Name regex validation, mkdir-on-demand, atomic write, overwrite gate, forward-slash JSON paths. |
| `src/host/project_reader.{h,cpp}` | `cpp-unit` + `runtime-reaper` | `get_state` (REAPER-bound), `list_artifacts` (FS), `get_api_reference` (embedded payload). |
| `src/host/refresh.{h,cpp}` | `runtime-reaper` | `RefreshFXList` best-effort + `Main_OnCommand` for action-list rescan. |
| `src/host/main_thread_queue.{h,cpp}` | `cpp-unit` | `std::function` queue + `condition_variable` rendezvous for marshaling to main thread. |
| `src/host/host.cpp` (MODIFIED) | n/a | Initialize queues + start/stop HTTP server alongside existing `panel::create`/`destroy`. |
| `src/host/meson.build` (MODIFIED) | n/a | Add 5 new sources, vendor `cpp-httplib` + `nlohmann/json`, add 2 new test() targets. |
| `src/host/tests/test_http_server.cpp` (NEW) | `cpp-unit` | Route dispatch + JSON envelope; pure-C++ test against fixture resource dir. |
| `src/host/tests/test_artifact_writer.cpp` (NEW) | `cpp-unit` | Name validation + mkdir + atomic write + overwrite gate. |
| `docs/api_reference/jsfx.md` (NEW) | data | ~80-120 lines cheatsheet (desc: block, sliders, @sample, common DSP idioms). |
| `docs/api_reference/reascript_lua.md` (NEW) | data | ~80-120 lines cheatsheet (reaper.* calls, MIDI/track iterators, action handlers). |
| `docs/api_reference/fx_chain_format.md` (NEW) | data | ~60-100 lines (RfxChain XML schema snippet, `<FXCHAIN>` tags). |
| `docs/api_reference/embed_data.h` (NEW, generated by `xxd -i`) | build | Three C arrays of `unsigned char` + lengths. |

## Spec corrections (must be reflected in tasks/PR 6 and PR 4)

| Spec claim | Reality in REAPER SDK | Design decision |
|---|---|---|
| `refresh-protocol/spec.md` calls `reaper.RefreshFXList()` | **Not in `reaper_plugin_functions.h`** — searched 443 matches, zero hits. | Best-effort: try `Main_OnCommand` for a known rescan ID; if no command available, log a warning and document that the user must use FX Browser → "Scan for new plug-ins". |
| `bridge-tools-v2/spec.md` writes `reaper.AddRemoveReaScript(0, 0)` and `reaper.AddRemoveReaScript(1, path, true)` | Signature is `int AddRemoveReaScript(bool add, int sectionID, const char* scriptfn, bool commit)`. | Use the real signature: unregister via `AddRemoveReaScript(false, 0, NULL, true)`; register via `AddRemoveReaScript(true, 0, abs_path, true)`. Return value is the new command ID (or `< 0` on failure). |

These corrections will be applied by `sdd-tasks` when it materializes PR 4 (save_lua) and PR 6 (refresh) — the spec text stays authoritative, but the C++ implementation uses the real SDK shape.

## HTTP server design

### Library choice

| Option | Verdict | Reasoning |
|---|---|---|
| **`cpp-httplib` (header-only, vendored)** | **Chosen** | Single header, no link deps, supports blocking + thread-pool, MIT license, ~2MB compiled. Mature, used in many audio projects. |
| `mongoose` (C) | Rejected | C, not C++; thread model harder to reason about; we'd have to wrap it. |
| Custom `socket`/`bind`/`accept` | Rejected | Reinventing routing, JSON framing, keep-alive. ~1k LOC of well-trodden code we don't need. |
| `uWebSockets` | Rejected | Async-first, harder to integrate with REAPER's main-thread discipline. We don't need WebSockets. |

cpp-httplib goes into `third_party/cpp-httplib/httplib.h` (single header). Vendored, not a subproject — keeps the meson file flat.

### Port, host, discovery

| Decision | Value | Reasoning |
|---|---|---|
| Default port | **7800** | Different from REAPER's web interface (default 8080). Documents assume 7800. |
| Bind | `0.0.0.0:7800` | Reachable from WSL via the WSL host IP that the bridge already uses (see `docs/cross-environment.md`). |
| Discovery | Extension writes `<wsl-bridge.txt>` to `<resource>/ReaForge/` on startup with `0.0.0.0:7800` (the address, not the WSL IP — the WSL side is what needs to be discovered, but the bridge already discovers WSL → REAPER via the existing `wsl-bridge.txt` mechanism written by the bridge on its own startup). | Symmetric with the existing pattern. Avoids a new discovery protocol. |
| Port collision | Read `REAFORGE_PORT` env var; default 7800. Document in README. | R5 in `explore.md` is real. |
| Timeouts | Read timeout 5s, write timeout 5s. Keep-alive off (so REAPER's UI thread doesn't get starved). | REAPER UI latency > throughput. |

### Threading model

| Layer | Thread | Notes |
|---|---|---|
| cpp-httplib accept loop | Worker thread (spawned by `http_server::start`) | Single thread is fine — bridge makes <10 req/s, file writes are <100ms. |
| JSON parse / route dispatch | Same worker | Pure C++, no REAPER API touch. |
| Filesystem writes (`artifact_writer`) | Same worker | Pure C++17 `<filesystem>`. |
| REAPER-bound calls (`get_state`, `register_action`, `refresh`) | Marshaled to main thread | Enqueue `std::function` + `std::promise<Result>`; `host::tick()` (called from `host.cpp`'s per-frame hook or `Main_OnCommand` polling) drains the queue and fulfills the promise; worker thread blocks on `future.get()` with a 5s timeout. |
| cpp-httplib thread pool | Disabled (`set_threads(1)`) | Single threaded is enough; avoids the need for fine-grained locking on the API-reference cache and the embedded-payload map. |

The main-thread queue lives in `main_thread_queue.{h,cpp}` so it can be unit-tested without REAPER (just a producer thread + a consumer thread).

### Request → handler dispatch

| Method | Route | Handler | Lane |
|---|---|---|---|
| `GET` | `/v1/health` | inline `200 {"ok":true,"version":"0.1.0"}` | `cpp-unit` |
| `GET` | `/v1/state?summary=<bool>` | `project_reader::get_state` | `runtime-reaper` |
| `GET` | `/v1/artifacts?kind=<kind>` | `project_reader::list_artifacts` | `cpp-unit` |
| `GET` | `/v1/api-reference?target=<t>` | `project_reader::get_api_reference` | `cpp-unit` |
| `POST` | `/v1/jsfx` | `artifact_writer::save_jsfx` | `cpp-unit` |
| `POST` | `/v1/lua` | `artifact_writer::save_lua` | `cpp-unit` + `runtime-reaper` (register) |
| `POST` | `/v1/fxchain` | `artifact_writer::save_fx_chain` | `cpp-unit` |
| `POST` | `/v1/refresh` | `refresh::run` (501 stub in PR 4, real in PR 6) | `runtime-reaper` |

### JSON request/response format

Use nlohmann/json (vendored `third_party/nlohmann/json.hpp`). One example for the `save_jsfx` round-trip:

```
POST /v1/jsfx          → 200 {"path":"C:/Users/u/AppData/Roaming/REAPER/Effects/ReaForge/tape_saturation.jsfx","bytes_written":1234}
POST /v1/jsfx (dup)    → 409 {"error":"FILE_EXISTS","message":"...","path":"..."}
POST /v1/jsfx (bad)    → 400 {"error":"INVALID_NAME","message":"name must match ^[A-Za-z0-9_\\-]{1,64}$"}
POST /v1/jsfx (junk)   → 400 {"error":"INVALID_JSON","message":"expected object"}
```

`Content-Type: application/json` on every response. The 4-error-code set (`INVALID_NAME` / `INVALID_JSON` / `FILE_EXISTS` / `WRITE_FAILED` / `REGISTER_FAILED` / `NOT_IMPLEMENTED`) lives as a single `enum class ErrorCode` in `http_server.h` so handlers and tests share it.

## Project reader design

### `reaforge_get_state`

| Aspect | Decision |
|---|---|
| REAPER API | `CountTracks`, `GetTrack(0, i)`, `GetTrackName`, `TrackFX_GetCount`, `TrackFX_GetFXName`, `GetProjectName`, `GetSetProjectInfo` (BPM, sample rate), `CountSelectedMediaItems`, `GetSelectedMediaItem` → `GetMediaItemTrack` for per-track selected-item count. All confirmed in `reaper_plugin_functions.h`. |
| Lane | `runtime-reaper` — all calls must be marshaled to main thread. |
| `summary=true` | Omit `fx_names[]` per track; keep `fx_count` and `selected_item_count`. |
| No project | Return `409 PROJECT_NOT_OPEN` (per `extension-read-api` spec). |
| Marshal pattern | Worker enqueues `get_state_request`; main thread calls APIs, builds the JSON, fulfills the promise; worker returns the JSON. |

### `reaforge_list_artifacts`

| Aspect | Decision |
|---|---|
| Implementation | `<filesystem>` recursive iteration (non-recursive, just direct children) over the three subfolders. |
| Missing folder | `fs::exists()` check; if absent, return `{artifacts:[]}` — never 404. Per `extension-read-api/spec.md`. |
| `kind` omitted | Merge all three folders, each entry with the correct `kind` field. |
| `kind` invalid | `400 INVALID_KIND`. |
| Lane | `cpp-unit` — pure filesystem, no REAPER. Testable in a fixture dir. |

### `reaforge_get_api_reference`

| Aspect | Decision |
|---|---|
| Payload location | **Embedded as a C array in the DLL**, via `xxd -i docs/api_reference/jsfx.md > src/host/api_reference_data.h`. Three arrays: `jsfx_md`, `reascript_lua_md`, `fx_chain_format_md`. At `host::init()`, copy into a `std::unordered_map<std::string, std::string>` for O(1) lookup. |
| Why embedded | The DLL is the unit of distribution. The REAPER resource path is not always writable for users (corporate installs, portapps). Embedded = self-contained, no install-time copy step, no path-resolution failure mode. |
| Test | `test_http_server.cpp` includes a 4th embedded payload (`test_api_reference`) so tests don't depend on the real markdown content. |
| Lane | `cpp-unit`. |

## Artifact writer design

| Concern | Rule |
|---|---|
| Name regex | `^[A-Za-z0-9_\-]{1,64}$` (matches `extension-write-api/spec.md`). Reject before any FS touch. |
| Path resolution | `fs::weakly_canonical(parent) + name + suffix`; verify the resolved path is under the three `ReaForge/` subfolders (defense-in-depth for symlink attacks). |
| Overwrite gate | If `request["overwrite"] != true` and target exists → return 409 `FILE_EXISTS` with the absolute path. |
| `mkdir -p` | `fs::create_directories(parent)`. No error. |
| Atomic write | `ofstream tmp = path + ".tmp"` → write all → close → `fs::rename(tmp, path)`. The rename is atomic on POSIX and atomic-enough on NTFS for a single writer. |
| `register_action` flow | After the rename succeeds: `int cmd_id = AddRemoveReaScript(false, 0, NULL, true)` (clear prior) then `int new_id = AddRemoveReaScript(true, 0, abs_path, true)`. Return `new_id` if `> 0`, else 500 `REGISTER_FAILED`. |
| Forward-slash paths | Replace `\\` with `/` in any path emitted in JSON. |
| Lane | `cpp-unit` for name regex, mkdir, atomic write, overwrite, path normalization. `runtime-reaper` for the `AddRemoveReaScript` call. |

`save_lua` flow (per `extension-write-api/spec.md`):

1. Validate `name` (regex).
2. Resolve `<resource_root>/Scripts/ReaForge/<name>.lua` (path-safety check).
3. Overwrite gate.
4. `mkdir -p` + atomic write.
5. If `register_action == true`: call `AddRemoveReaScript` (main-thread marshal).
6. Return `{path, action_id?}` (omit `action_id` if step 5 was skipped or failed with best-effort semantics per spec).

## Refresh design

The hard truth: **`reaper.RefreshFXList()` is not in the public REAPER SDK** (we searched the vendored `reaper_plugin_functions.h` exhaustively). The honest design:

| Step | Implementation | Fallback |
|---|---|---|
| 1 | Call `Main_OnCommand(<rescan_id>, 0)` where `<rescan_id>` is `NamedCommandLookup("_S&M_REFRESH")` or the REAPER-native equivalent (TBD by `sdd-tasks` when researching REAPER action IDs). | If `NamedCommandLookup` returns 0 (no such command), log a warning and continue. |
| 2 | Call `AddRemoveReaScript(false, 0, NULL, true)` then re-`AddRemoveReaScript(true, ...)` for any registered paths known to the extension. (Effectively, force a re-scan of `Scripts/` by toggling registration.) | If no scripts were registered in this session, skip. |
| 3 | Return `200 {refreshed_at: <ISO8601>}` with a `warnings[]` array if any step was a no-op. | n/a |

**For JSFX in `Effects/ReaForge/`, REAPER's FX browser does not auto-rescan** — the user must either restart REAPER or right-click the FX browser → "Scan for new plug-ins". Document this in the README quickstart. The agent will print a warning via the `warnings[]` field so the LLM can suggest the manual step.

The PR 6 verification step is to: write a JSFX, call `/v1/refresh`, restart REAPER (or trigger the manual rescan), confirm the JSFX appears in the FX browser. **Without a restart, the JSFX won't appear** — this is a known REAPER behavior, not a ReaForge bug.

## Embedded API reference payload

### Why embed

| Option | Verdict |
|---|---|
| **Embed as C arrays in the DLL** | **Chosen.** Self-contained. No install-time step. No path lookup failure. Diff-friendly: markdown edits become C array diffs. |
| Install to `<resource>/ReaForge/api_reference/` at install | Rejected for MVP. Adds install-time complexity and assumes the resource dir is writable. |
| Ship as a sidecar `.dat` file | Rejected. Adds a file alongside the DLL that the user might delete. |
| Fetch from a URL | Rejected by user-confirmed constraint #5 (offline-first). |

### File format

Three markdown files in `docs/api_reference/`, hand-curated cheatsheets, 50-200 lines each. Goal: ground the LLM, not replace official docs. The cheatsheet should answer *"how do I write a basic `<desc>:` block, how do I read selected items, how do I emit an RfxChain?"* — not *"list every REAPER API function"*.

### Loading at startup

In `host::init()` (after REAPER's API table is available, before the HTTP server starts):

```cpp
api_reference::load("jsfx", jsfx_md, sizeof(jsfx_md));
api_reference::load("reascript_lua", reascript_lua_md, sizeof(reascript_lua_md));
api_reference::load("fx_chain_format", fx_chain_format_md, sizeof(fx_chain_format_md));
```

The `api_reference` module is a single `std::unordered_map<std::string, std::string>` with a `get(target)` accessor. No mutex — loaded once before the server starts accepting requests.

## meson.build changes

```diff
 project('reaforge', 'cpp', 'c',
     version : '0.0.1',
     license : 'MIT',
     default_options : ['cpp_std=c++17', 'warning_level=2'])

+cc = meson.get_compiler('cpp')
+cc_c = meson.get_compiler('c')
+
+# Vendor third-party single-header libs (cpp-httplib, nlohmann/json)
+third_party_inc = include_directories('third_party')
+
+reaper_core = shared_library('reaper_reaforge_host',
+    'host.cpp',
+    'http_server.cpp',
+    'artifact_writer.cpp',
+    'project_reader.cpp',
+    'refresh.cpp',
+    'main_thread_queue.cpp',
+    'panel.cpp',          # keep for now (deferred)
+    'context_menu.cpp',   # keep for now (deferred)
+    'extension_loader.cpp',
+    'executor.cpp',
+    runtime_sources,
+    bridge_sources,
+    include_directories : common_inc,
+    dependencies : [lua_dep],
+    link_with : quickjs_lib,
+    gnu_symbol_visibility : 'default',
+    install : false)

+test_http = executable('test_http_server',
+    'http_server.cpp',
+    'artifact_writer.cpp',
+    'project_reader.cpp',
+    'main_thread_queue.cpp',
+    'tests/test_http_server.cpp',
+    'tests/test_artifact_writer.cpp',
+    include_directories : common_inc,
+    dependencies : [lua_dep],
+    install : false)
+test('http_server', test_http, suite : 'host')
```

`bridge_sources` and `runtime_sources` stay (dead code) so existing builds don't break; they're archived post-MVP.

## Test strategy

| Module | Lane | Test target | What it asserts |
|---|---|---|---|
| `http_server` routing | `cpp-unit` | `tests/test_http_server.cpp` | Each of 8 routes returns the right HTTP status + JSON shape against a fixture resource dir. |
| `http_server` JSON envelope | `cpp-unit` | (same) | `400 INVALID_JSON` for bad body; `405` for wrong method; `404` for unknown route. |
| `artifact_writer` IO | `cpp-unit` | `tests/test_artifact_writer.cpp` | `save_jsfx` creates file + folder; refuses on existing without overwrite; accepts with overwrite; atomic rename leaves no `.tmp`. |
| `artifact_writer` name regex | `cpp-unit` | (same) | `name="../../escape"` → `400 INVALID_NAME`; `name=""` → `400`; `name="ok_name-1"` → `200`. |
| `main_thread_queue` rendezvous | `cpp-unit` | (same or new) | Producer enqueues, consumer thread returns the value; timeout works. |
| `project_reader` list_artifacts | `cpp-unit` | `test_http_server.cpp` | Returns empty array if folder missing; returns entries with correct `kind` if present. |
| `project_reader` get_state | `runtime-reaper` | n/a (manual) | Verified in PR 7 on REAPER Windows against a fixture project. |
| `project_reader` get_api_reference | `cpp-unit` | (same) | Returns the embedded payload verbatim; `400 INVALID_TARGET` for `target=python`. |
| `refresh` | `runtime-reaper` | n/a (manual) | Verified in PR 7. JSFX rescan still requires a manual "Scan for new plug-ins" — documented. |
| `refresh` idempotency | `runtime-reaper` | n/a (manual) | PR 7: call 3x in a row, REAPER does not duplicate entries. |

The fixture resource dir for unit tests is a temp dir (`std::filesystem::temp_directory_path() / "reaforge_test_*"`), populated by each test with the `.jsfx` / `.lua` files it needs. Same pattern as `tests/test_loader.cpp`.

## Lane assignment per spec

| Spec | Lane | PR |
|---|---|---|
| `bridge-tools-v2` | `python-tdd` | 2, 3 |
| `extension-write-api` | `cpp-unit` (write, name, mkdir) + `runtime-reaper` (AddRemoveReaScript) | 4 |
| `extension-read-api` | `cpp-unit` (artifacts, api-ref) + `runtime-reaper` (get_state) | 5 |
| `refresh-protocol` | `runtime-reaper` (with the spec correction noted above) | 6 |
| `artifact-folder-convention` | `cpp-unit` (cross-cutting; enforced inside `artifact_writer` and `project_reader`) | 4, 5 |

## Per-PR diff size forecast

| PR | Lane | ΔLOC | Notes |
|---|---|---|---|
| 1 docs | `docs` | ~80 | README rewrite. |
| 2 bridge 5→7 | `python-tdd` | ~150 | Tool schemas + smoke updates. |
| 3 stub 5→7 | `python-tdd` | ~150 | Mock 7 endpoints. |
| 4 C++ writer | `cpp-unit` + `runtime-reaper` | ~350 | `http_server` (routing only) + `artifact_writer` + `main_thread_queue` + meson + 2 new test targets. |
| 5 C++ reader | `cpp-unit` + `runtime-reaper` | ~350 | `project_reader` + 3 markdown files + `xxd -i` data header. |
| 6 C++ refresh | `runtime-reaper` | ~100 | `refresh.cpp` + the `Main_OnCommand` integration. |
| 7 e2e | `runtime-reaper` | docs+manual | README quickstart + 3-prompt manual verify. |

## Risks revisited (design-level)

| # | Risk | Mitigation |
|---|---|---|
| R1 | LLM generates broken JSFX/Lua. | Opencode's tool approval is the gate. PR 4/5 leave hooks for `reaforge_validate_*` (post-MVP). |
| R3 | `get_state` returns too much data. | `summary=true` flag + per-track FX name list capped at first 16 entries. |
| R6 | `RefreshFXList` / `AddRemoveReaScript` cannot be unit-tested outside REAPER. | `cpp-unit` for everything that doesn't call REAPER. `runtime-reaper` lane for the rest. PR 7 is the manual integration gate. **New**: document in README that JSFX rescan may require manual FX Browser → "Scan for new plug-ins". |
| New: R10 | `cpp-httplib` thread pool disabled + main-thread marshal adds latency. | 5s timeout is fine for the LLM's "wait for write" loop. If latency becomes a problem, defer to phase +1. |
| New: R11 | `xxd -i` requires a build step not currently in meson. | Generate the `.h` once and commit it; meson just compiles the C array. Update via a `make` target in `scripts/` for future revisions. |

## Open design questions

**None blocking.** The two spec corrections (RefreshFXList, AddRemoveReaScript signature) are design-level fixes, not user decisions — they are applied during `sdd-tasks` / `sdd-apply`. The user-confirmed constraints are already in `proposal.md` and don't need re-asking.

## Next phase

`sdd-tasks` produces `tasks.md` with PR-sized work units, lane-tagged, with concrete file paths. **PR 4 and PR 6 need the most careful commit breakdown** because the spec corrections above mean the C++ signatures will not match the spec text exactly — the `work-unit-commits` skill applies: each commit = one verifiable unit (e.g., "add artifact_writer::name regex and test", then "add artifact_writer::atomic_write and test", then "add /v1/jsfx route and test"). PR 4 should split into 3-4 commits to stay inside the 400-line budget per chained PR.
