# Proposal: ReaForge Agentic MVP

## Intent

ReaForge pivots from a multi-runtime SDK to an **agentic artifact generator**: opencode Desktop (external) drives a 7-tool MCP bridge that calls a greenfield C++ extension inside REAPER, which writes native files (`Effects/ReaForge/*.jsfx`, `Scripts/ReaForge/*.lua`, `FXChains/ReaForge/*.RfxChain`) and asks REAPER to refresh its caches. The agent only reads context and persists files — it never mutates live REAPER state. MVP success: a user types one prompt in opencode and a working REAPER artifact appears in the right folder within 2 minutes.

## Capabilities

### New capabilities (kebab-case; each becomes a new `openspec/specs/<name>/spec.md`)

| Capability | What it covers |
|---|---|
| `bridge-tools-v2` | The 7 MCP tools: 3 Read (state, list, api-reference) + 3 Write (jsfx, lua, fx-chain) + 1 Refresh. Tool schemas, error semantics, `overwrite=true` gating, `register_action` opt-in. |
| `extension-write-api` | HTTP endpoints behind the 3 Write tools. Routes, request/response JSON, filesystem writes into the `ReaForge/` subfolders, `RegisterAction` flow. |
| `extension-read-api` | HTTP endpoints behind the 3 Read tools. `GET /v1/state` (compact project projection), `GET /v1/artifacts` (filesystem enumeration), `GET /v1/api-reference` (embedded payload). |
| `refresh-protocol` | `reaforge_refresh` contract: `reaper.RefreshFXList()` + Action List rescan; idempotent; called once per write burst. |
| `artifact-folder-convention` | `<REAPER resource>/{Effects,Scripts,FXChains}/ReaForge/` namespace. Auto-create, forward-slash paths, `wipe = rm -rf ReaForge/`. |
| `embedded-api-reference` | Offline-first cheatsheets for JSFX, ReaScript Lua, and `.RfxChain` format. Shipped as static payloads inside the extension. |

### Archived capabilities (rejected by the June 2026 pivot)

The three existing specs in `openspec/specs/` are **rejected** by this change and will be moved to `openspec/specs/archive/` with `REJECTED.md` once the MVP ships:

| Existing spec | Why archived |
|---|---|
| `multi-runtime` | REAPER is the runtime. No embedded Lua / QuickJS / JSFX in the DLL. |
| `extension-execution` | "Ship a sample extension" pattern is gone. The agent generates artifacts on demand. `src/host/extensions/humanize_midi/` archived. |
| `runtime-bridge` | The Fase 0 in-DLL bridge (`src/host/bridge/`) is dead code. HTTP replaces it. |

The 5 pre-pivot bridge tools (`reaforge_list_tracks`, `reaforge_run_extension`, …) are replaced, not modified.

### Modified capabilities

None. The pivot is a clean replacement: old specs are archived, new specs are fresh, no in-place edits.

## Approach (high level)

| Layer | What | Why |
|---|---|---|
| Agent | opencode Desktop outside REAPER | Reuses chat UI, history, multi-model, **built-in MCP tool approval** is our review gate |
| Bridge | Python MCP server in `tools/opencode_bridge.py` | Validated 5/5 GO (`bridge-spike-results.md`). Pivots 5 → 7 tools. |
| Extension | C++ in `src/host/` — `http_server.{h,cpp}` + `artifact_writer.{h,cpp}` + `project_reader.{h,cpp}` + `refresh.{h,cpp}` | Greenfield. `src/host/` has zero HTTP code today (Engram #80). |
| Files | `<REAPER resource>/{Effects,Scripts,FXChains}/ReaForge/` | Native REAPER folders. Convention agreed by user (#1). |

## Success criterion (verbatim, from the handoff)

> The user opens opencode desktop with the ReaForge bridge configured, types one of the example prompts, and within 2 minutes a working JSFX / Lua / RfxChain file appears in the right REAPER folder and works when used.

## Acceptance checklist (3 prompts that MUST all pass)

- [ ] *"Generate a JSFX that does soft tape saturation"* → `Effects/ReaForge/tape_saturation.jsfx`, audibly saturates when added to a track, does not crash REAPER.
- [ ] *"Write a Lua script that doubles the velocity of selected MIDI notes"* → `Scripts/ReaForge/double_velocity.lua`, runs as a REAPER action, modifies the notes.
- [ ] *"Combine the built-in delay and ReaEQ into a vocal slap chain"* → `FXChains/ReaForge/vocal_slap.RfxChain`, loads as a chain.

## Decisions (user-confirmed — do not re-ask in later phases)

| # | Question | Answer |
|---|---|---|
| 1 | Folder convention `<REAPER resource>/{Effects,Scripts,FXChains}/ReaForge/` | **YES** — adopt as canonical |
| 2 | Verify Fase 1a on REAPER Windows before pivoting? | **SKIP** — panel/context-menu/extension-loader deferred to phase +2/+3 |
| 3 | Bridge tool naming | **`reaforge_*`** (explicit, not `rf_*`) |
| 4 | `register_action` default in `reaforge_save_lua` | **`false`** — opt-in, less surprising |
| 5 | `reaforge_get_api_reference` source | **EMBEDDED docs** (offline-first, no network dep) |
| 6 | `reaforge_save_*` overwrite behavior | **Refuse without `overwrite=true`** — explicit flag, safer default |

## Affected areas

| Area | Impact | Description |
|---|---|---|
| `tools/opencode_bridge.py` | Modified (PR 2) | Pivot 5 → 7 `reaforge_*` tools. TDD-friendly. |
| `tools/stub_reaper_server.py` | Modified (PR 3) | Extend stub to 7 endpoints. TDD-friendly. |
| `tools/run_bridge_smoke.sh` | Modified (PR 2) | Update smoke assertions for 7 tools. |
| `src/host/http_server.{h,cpp}` | **New** (PR 4) | Greenfield HTTP server. meson `shared_library` source. |
| `src/host/artifact_writer.{h,cpp}` | **New** (PR 4) | Greenfield. Writes 3 file types. `cpp-unit` for filesystem logic. |
| `src/host/project_reader.{h,cpp}` | **New** (PR 5) | Greenfield. REAPER-API-bound. `runtime-reaper` lane. |
| `src/host/refresh.{h,cpp}` | **New** (PR 6) | Greenfield. `reaper.RefreshFXList()` + Action List rescan. `runtime-reaper`. |
| `src/host/meson.build` | Modified (PR 4) | Wire 4 new source files into `reaper_reaforge_host`. |
| `README.md` | Modified (PR 1) | Rewrite to reflect agentic vision. |
| `docs/documentacion/` | Unchanged | Already authoritative. |
| `openspec/specs/{multi-runtime,extension-execution,runtime-bridge}/` | Archived (post-MVP) | Move to `openspec/specs/archive/` with `REJECTED.md`. |

## Delivery strategy

### The 7 PRs (from `04-mvp-and-handoff.md` §"Suggested PR breakdown" + lane tags)

| PR | Lane | Scope | Size |
|---|---|---|---|
| 1 | `docs` | Rewrite `README.md`. Remove multi-runtime SDK language. Point to `docs/documentacion/`. | small (~80 LOC) |
| 2 | `python-tdd` | Pivot `tools/opencode_bridge.py`: 5 tools → 7 `reaforge_*`. Update `tools/run_bridge_smoke.sh`. | medium (~150 LOC) |
| 3 | `python-tdd` | Pivot `tools/stub_reaper_server.py`: mock 7 endpoints. Smoke update if not in PR 2. | medium (~150 LOC) |
| 4 | `cpp-unit` (routing/JSON/file-IO) + `runtime-reaper` (filesystem only) | C++: new `http_server.{h,cpp}` + `artifact_writer.{h,cpp}`. Implement the 3 Write endpoints. Wire `meson.build`. **Greenfield.** | medium (~350 LOC) |
| 5 | `cpp-unit` (parsing) + `runtime-reaper` (REAPER API) | C++: new `project_reader.{h,cpp}`. Implement the 3 Read endpoints + embedded `api_reference` payload. | medium (~350 LOC) |
| 6 | `runtime-reaper` | C++: new `refresh.{h,cpp}`. Implement `reaforge_refresh`. | small (~100 LOC) |
| 7 | `runtime-reaper` (manual) | End-to-end on REAPER Windows. Document setup in README. Run the 3 acceptance prompts. | docs + manual verify |

### Why chained (not single PR)

- C++ greenfield code in PR 4–6 makes one mega-PR ≫ 400 lines.
- Reviewer burden: a single PR with HTTP server + reader + writer + refresh + 7 MCP tool swaps is unmanageable.
- Each PR is independently shippable; can pause and merge partial progress.

### Order dependencies

```
PR1 (docs) ──────────────────────────────────────────────┐
PR2 (bridge 5→7) ──┐                                     │
PR3 (stub 5→7) ────┤                                     │
                   ├──► PR4 (C++ writer) ──► PR5 (C++ reader) ──► PR6 (C++ refresh) ──► PR7 (e2e)
                   │            ▲
                   └────────────┘  depends on meson wiring in PR4 + API spec from PR2/3
```

PR 2/3 are independent and can land in any order. PR 4 unblocks PR 5 and PR 6. PR 7 requires all of the above.

## Risks (top 3 from explore, with mitigation)

| # | Risk | Mitigation |
|---|---|---|
| R1 | LLM generates broken JSFX/Lua that crashes REAPER on load | Stub `reaforge_validate_jsfx` / `_lua` Read tools in PR 2/3; wire to real checks in PR 4/5. opencode's tool approval is the final gate. |
| R3 | `reaforge_get_state` returns too much data for the LLM context | Compact projection in PR 5 spec: track count/names, selected item count, FX name list per track, sample rate, BPM — no audio. Add `?summary=true` flag. |
| R6 | C++ code that calls `reaper.AddRemoveReaScript()` / `RefreshFXList()` cannot be unit-tested outside REAPER | Lane-tagged verification: `cpp-unit` for pure parts (routing, JSON, filesystem), `runtime-reaper` for REAPER-bound parts. PR 7 is the integration gate. |

Full 9-risk table: see `explore.md` §"Key technical risks".

## Out of scope (re-stated to prevent scope creep)

- Internal ReaImGui chat panel (phase +3)
- Context menu hooks: right-click → "Generate X" (phase +2)
- Tools that mutate live REAPER state (`open_automation`, `set_parameter`, `play`, `stop`) — explicitly rejected
- Artifact versioning / git integration (defer; user can `git init` their REAPER resource)
- Templates library, project profiles, presets (defer)
- Authentication, multi-user, cloud sync (single-user local tool)
- Re-embedding any runtime in the DLL (explicitly rejected by the pivot)
- Verify-or-pivot decision for Fase 1a host (skipped, those features are deferred)
- Optimization of large-artifact writes via direct WSL `/mnt/c/` filesystem access (post-MVP)
- Fix for the hardcoded `wsl-bridge.txt` path in `tools/opencode_bridge.py:25` (tag as `TODO`, not blocking)

## Anti-patterns to avoid (re-stated)

- Re-embedding Lua / QuickJS / JSFX runtimes in the DLL — explicitly rejected.
- Adding tools that mutate live REAPER state in this MVP.
- Building a chat UI inside REAPER first — MVP is opencode Desktop only.
- Designing for multiple users / cloud sync.
- Treating the C++ HTTP server as a refactor — **it is greenfield**. Do not invent a fictional predecessor.
- Sneaking back the Fase 0 in-DLL bridge for any reason.
- Letting the LLM bypass `overwrite=true` silently — the flag exists for safety; document and enforce in the bridge.

## Lane annotation convention (for sdd-tasks)

Every task in `tasks.md` carries a `lane: ` tag in its metadata. The orchestrator's review-workload guard uses this to decide if a PR is TDD-friendly (`python-tdd` or `cpp-unit`) or runtime-only (`runtime-reaper`). Strict TDD is `false` for this project (Engram #79): REAPER-API-bound code is runtime-verified, not unit-tested.

## Next phase

`sdd-spec` produces three delta specs in `openspec/changes/2026-06-07-reaforge-agentic-mvp/specs/`:

| Spec | Replaces | Scope |
|---|---|---|
| `bridge-tools-v2.md` | Archived `runtime-bridge` | 7 tools, contracts, error semantics, `overwrite`/`register_action` flags |
| `extension-write-api.md` | Archived `extension-execution` | HTTP endpoints for the 3 Write tools + `RegisterAction` flow |
| `extension-read-api.md` + `refresh-protocol.md` | Archived `multi-runtime` | Read endpoints, embedded `api_reference`, refresh contract |

These specs will replace the archived `multi-runtime`, `extension-execution`, and `runtime-bridge` capabilities.
