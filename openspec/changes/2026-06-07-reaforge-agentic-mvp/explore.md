# Explore: ReaForge Agentic MVP

> Phase 0 of the new SDD change. Pre-proposal thinking. Does NOT commit to a specific design.
> All reasoning is grounded in `docs/documentacion/` (authoritative handoff) and Engram observations #79–#81.

## TL;DR

ReaForge pivots from a multi-runtime SDK to an **agentic artifact generator**: opencode Desktop (external) → MCP bridge (Python, 7 tools) → HTTP → C++ extension inside REAPER → writes native files (`Effects/ReaForge/*.jsfx`, `Scripts/ReaForge/*.lua`, `FXChains/ReaForge/*.RfxChain`) + refreshes REAPER caches. MVP ships 7 tools, 7 PRs. **The C++ HTTP server, project reader, artifact writer, and refresh hook are greenfield code** — `src/host/` today has zero HTTP/network code, only the Fase 0 in-DLL bridges (`bridge/`, `runtime/`) that are now dead code post-pivot. Python PRs (2, 3) and the routing/JSON/filesystem slice of the C++ PRs (4, 5) are TDD-friendly; the REAPER-API-touching parts are runtime-only.

## Context (the pivot + what's settled vs what's open)

### Settled by the 2026-06-07 pivot (do not re-derive)

| Decision | Source |
|---|---|
| Agent lives **outside** REAPER, in `opencode desktop` | `01-vision-and-pivot.md` "Decisions that result from the pivot" |
| Agent only **generates and persists files** — never mutates live REAPER state | `01-vision-and-pivot.md` "What ReaForge is" + user quote: *"el agente solo genera codigo, tarea, scripts y los guarda en reaper"* |
| Bridge tools split: 3 Read + 3 Write + 1 Refresh = **7 tools** | `04-mvp-and-handoff.md` §"The 7 bridge tools (MVP)" |
| C++ extension surface shrinks to: HTTP server + filesystem writes + REAPER-API refresh | `02-architecture.md` §"Extension does NOT embed runtimes" |
| `src/host/bridge/` and `src/host/runtime/` are **dead code** post-pivot | Engram #80 + `03-current-state.md` "What gets archived" |
| Transport: MCP (stdio) → HTTP (localhost or WSL→Windows IP) | `02-architecture.md` + Engram #81 |
| Discovery: `wsl-bridge.txt` → `REAPER_BASE_URL` env → `http://127.0.0.1:7800` fallback | `02-architecture.md` §3 + `docs/cross-environment.md` |

### Already confirmed by the user (defaults — apply without re-asking)

| # | Question | Answer |
|---|---|---|
| 1 | Folder convention `<REAPER resource>/{Effects,Scripts,FXChains}/ReaForge/` | **YES** — adopt as canonical |
| 2 | Verify Fase 1a on REAPER Windows before pivoting? | **SKIP** — panel/context-menu/extension-loader are deferred to phase +2/+3 |
| 3 | Bridge tool naming | **`reaforge_*`** (not `rf_*`) — explicit > terse |
| 4 | `register_action` default in `reaforge_save_lua` | **`false`** — opt-in, less surprising |
| 5 | `reaforge_get_api_reference` source | **EMBEDDED docs** (offline-first, no network dep) |
| 6 | `reaforge_save_*` overwrite behavior | **Refuse without `overwrite=true`** — explicit flag, safer default |

### Open (carried forward from handoff — not blockers for explore)

- The `panel.cpp` + `context_menu.cpp` + `extension_loader.cpp` files in `src/host/` are deferred. They can stay (no churn) or move to `src/host/deferred/` for clarity. Cosmetic, decide in `sdd-design`.
- `docs/cross-environment.md` and `docs/user-flows.md` stay as historical artifacts. The transport layer is valid; the 3-flow model is superseded.
- `tools/opencode_bridge.py:25` hardcodes the user's Windows username in `wsl-bridge.txt` path. Engram #81 flags it as a fragility for any other contributor. **Not blocking for MVP**, but should be a `// TODO` comment or derived from `$USERPROFILE` in PR 2.

## The 7 bridge tools (one-liner each, with semantics)

### Read (3) — give the agent context

| Tool | Semantics (one-liner) | Returns |
|---|---|---|
| `reaforge_get_state` | Snapshot of the current project: tracks, selected items, FX per track, sample rate, BPM, project name. | JSON project state |
| `reaforge_list_artifacts` | Enumerates files under `Effects/ReaForge/`, `Scripts/ReaForge/`, `FXChains/ReaForge/` with size + mtime. | JSON array of `{path, size, mtime, kind}` |
| `reaforge_get_api_reference(target)` | Returns embedded cheatsheet for `target ∈ {"jsfx", "reascript_lua", "fx_chain_format"}`. **Embedded, not fetched.** | Markdown/text reference |

### Write (3) — agent's only side effect

| Tool | Semantics (one-liner) | Returns |
|---|---|---|
| `reaforge_save_jsfx(name, code)` | Writes `<REAPER>/Effects/ReaForge/<name>.jsfx`. Creates the `ReaForge/` subfolder if missing. **Refuses if file exists unless `overwrite=true`.** | `{path, bytes_written}` |
| `reaforge_save_lua(name, code, register_action=false)` | Writes `<REAPER>/Scripts/ReaForge/<name>.lua`. If `register_action=true`, registers via `reaper.AddRemoveReaScript()`. **Refuses if file exists unless `overwrite=true`.** | `{path, action_id?}` |
| `reaforge_save_fx_chain(name, content)` | Writes `<REAPER>/FXChains/ReaForge/<name>.RfxChain`. **Refuses if file exists unless `overwrite=true`.** | `{path, bytes_written}` |

### Refresh (1) — make REAPER see the new artifact

| Tool | Semantics (one-liner) |
|---|---|
| `reaforge_refresh()` | Triggers REAPER to rescan FX list and Action List. Idempotent. |

**Approval gate**: opencode's built-in MCP tool approval shows the user the code/content before any `reaforge_save_*` call lands. We do **not** build our own confirmation UI.

## Alternatives considered

| Alternative | Verdict | Reason |
|---|---|---|
| Agent INSIDE REAPER (C++ LLM client) | **Rejected** | Reinventing opencode (chat UI, history, multi-model, tool approval). 3–6 months of work for zero unique value. The bridge is the contract — any external agent works. |
| REAPER's web interface for project state | **Rejected** | Only exposes transport, basic mixer, markers. Does **not** expose selected items, FX per track, automation, project structure. Inadequate for grounding the LLM. Use our C++ reader instead. (`05-plan-arquitectonico.md` §3 "❌ Lo que hay que AJUSTAR".) |
| Direct filesystem writes from WSL to `/mnt/c/.../REAPER/ReaForge/` | **Considered, partial** | Works and is faster than HTTP for large payloads. But it skips the C++ extension entirely, losing the `register_action` flow and `reaforge_refresh` integration. Keep as a **future optimization for very large artifacts**, not the MVP default. The MVP must route writes through the C++ extension. |
| Keep the 5 pre-pivot bridge tools and add 2 more | **Rejected** | The 5 imperative tools (`list_tracks`, `run_extension`, …) don't match the new "read + write + refresh" shape. A clean 5→7 swap is clearer than a 5+2=7 hybrid. (`04-mvp-and-handoff.md` §"Suggested PR breakdown".) |
| Reuse `src/host/bridge/` (Fase 0 in-DLL bridge) for the HTTP layer | **Rejected** | The Fase 0 bridge was a runtime-call bridge to embedded Lua/QuickJS/JSFX. The pivot rejects those runtimes. HTTP server is greenfield. (Engram #80.) |
| `dofile()` the generated Lua inside the chat script | **Rejected** | Loses undo points, no Action List visibility, not reusable. `AddRemoveReaScript()` is the correct path. (`05-plan-arquitectonico.md` §3.) |
| Build the ReaImGui internal chat panel as part of MVP | **Rejected** | Deferred to phase +3 (`04-mvp-and-handoff.md` §"Phasing after MVP"). MVP is opencode Desktop only. |
| Two channels of communication (web interface + HTTP + filesystem) | **Rejected** | Adds complexity, three failure paths for the agent. One channel: MCP → HTTP → extension. (`05-plan-arquitectonico.md` §3.) |

## Key technical risks

| # | Risk | Likelihood | Mitigation |
|---|---|---|---|
| R1 | LLM generates broken JSFX/Lua that crashes REAPER on load | **High** | Add `reaforge_validate_jsfx` (and `_lua`) Read tools that pre-check syntax / look for forbidden patterns. Result is informational — the user still approves via opencode's tool approval. Surface in PR 2/3 as a stub; wire to real checks in PR 4/5. |
| R2 | Cross-env HTTP fails: bridge in WSL can't reach extension on Windows | Medium | Discovery chain (`wsl-bridge.txt` → `REAPER_BASE_URL` → `127.0.0.1:7800`) already validated in spike. Document the failure mode in README. Add a clear error in the bridge when the extension is unreachable. |
| R3 | `reaforge_get_state` returns a project too large for the LLM context | Medium | Define a compact projection (track count, names, selected item count, FX name list per track, sample rate, BPM — no audio data). Add a `?summary=true` flag in PR 5 to truncate further. |
| R4 | `overwrite=true` semantics leak the user's intent to the LLM | Low | Document the flag clearly. The LLM should default to **not** overwriting and explicitly ask the user. README example must show the right pattern. |
| R5 | C++ HTTP server races with REAPER's own HTTP (`reaper www_start`)? | Low | REAPER's web interface binds a port the user configures. Default 7800 for ReaForge is a deliberate different port. Document the collision in PR 4 and let the user configure via env. |
| R6 | The C++ code that calls `reaper.AddRemoveReaScript()` / `RefreshFXList()` cannot be unit-tested outside REAPER | **Certain** | This is why we set `strict_tdd: false` (Engram #79). Pure C++ parts (routing, JSON, filesystem IO) get meson test targets; REAPER-bound parts are runtime-verified only. Lane-tag every task explicitly. |
| R7 | Path encoding / Windows backslashes in artifact paths | Low | All paths emitted by the extension use forward slashes (REAPER accepts them). Document in PR 4. |
| R8 | The `wsl-bridge.txt` discovery in `tools/opencode_bridge.py:25` hardcodes the user's username | Low | Not blocking for MVP (Engram #81). Tag as `TODO` in PR 2; fix when a second contributor shows up. |
| R9 | `register_action=true` may register the action in a way the user can't easily undo | Low | Document the `Script: ReaForge/<name>` action name pattern in the tool's return value. The user can disable via REAPER's action list filter. |

## Recommendation (what sdd-propose should commit to)

Adopt the 7-PR breakdown from `04-mvp-and-handoff.md` §"Suggested PR breakdown" verbatim, with these assignments:

| PR | Lane | Scope | Size |
|---|---|---|---|
| 1 | docs | Rewrite `README.md` to reflect the agentic vision. Remove multi-runtime SDK language. Point to `docs/documentacion/`. | small (~80 LOC) |
| 2 | `python-tdd` | Pivot `tools/opencode_bridge.py`: replace 5 tools with **7** named `reaforge_*`. Update smoke assertions. | medium (~150 LOC) |
| 3 | `python-tdd` | Pivot `tools/stub_reaper_server.py`: mock the 7 endpoints with hardcoded data. Update `tools/run_bridge_smoke.sh`. | medium |
| 4 | `cpp-unit` (routing/JSON/file-IO) + `runtime-reaper` (REAPER API) | C++: new `http_server.{h,cpp}` + `artifact_writer.{h,cpp}`. Implement the 3 Write endpoints to `Effects/ReaForge/`, `Scripts/ReaForge/`, `FXChains/ReaForge/`. Wire `meson.build`. **Greenfield code.** | medium |
| 5 | `cpp-unit` (parsing) + `runtime-reaper` (REAPER API) | C++: new `project_reader.{h,cpp}`. Implement the 3 Read endpoints: `get_state`, `list_artifacts`, `get_api_reference` (embedded docs). | medium |
| 6 | `runtime-reaper` | C++: new `refresh.{h,cpp}`. Implement `reaforge_refresh` — call `reaper.RefreshFXList()` + Action List rescan. | small |
| 7 | `runtime-reaper` (manual) | End-to-end on REAPER Windows. Document setup in README. Run the 3 acceptance prompts. | docs + manual verify |

Lane assignment follows the `strict_tdd: false` decision (Engram #79): PR 2/3 are the TDD-friendly island; PR 4/5 split into pure-C++ (TDD) and REAPER-bound (runtime) modules; PR 6/7 are runtime-only. **Critical to flag in `sdd-propose`**: the C++ PRs (4/5/6) are **greenfield**, not a pivot of existing modules. `sdd-tasks` should not assume any HTTP code exists today.

## What's deliberately out of scope for this change

(Mirrors `04-mvp-and-handoff.md` §"What is out of MVP scope".)

- Internal ReaImGui chat panel (phase +3)
- Context menu hooks: right-click → "Generate X" (phase +2)
- Tools that mutate live REAPER state (`open_automation`, `set_parameter`, `play`, `stop`) — explicitly rejected
- Artifact versioning / git integration (defer)
- Templates library, project profiles, presets (defer)
- Authentication, multi-user, cloud sync (single-user local tool)
- Re-embedding any runtime (Lua / QuickJS / JSFX) in the DLL — explicitly rejected by the pivot
- Verify-or-pivot decision for Fase 1a host — **skipped**, those features are deferred
- Optimization of large-artifact writes via direct WSL `/mnt/c/` filesystem access — consider post-MVP
- Fix for the hardcoded `wsl-bridge.txt` path in `tools/opencode_bridge.py:25` — tag as `TODO`, not blocking

## Anti-patterns to avoid

(Mirrors `04-mvp-and-handoff.md` §"Anti-patterns to avoid", with project-specific additions.)

- **Re-embedding Lua / QuickJS / JSFX runtimes in the DLL.** Explicitly rejected. REAPER is the runtime. (`src/host/runtime/` and `src/host/bridge/` are dead code.)
- **Adding tools that mutate live REAPER state in this MVP.** Out of scope.
- **Building a chat UI inside REAPER first.** Deferred. MVP is opencode Desktop only.
- **Designing for multiple users / cloud sync.** Single-user local tool.
- **Presenting the user with option menus instead of direct recommendations.** The user wants recommendations with reasoning, not lists.
- **Switching language to English in chat replies.** User speaks Rioplatense Spanish. Code/docs/commits stay English. (Persona scope rule.)
- **Skipping `mem_search` on first message.** Load Engram context before replying.
- **Treating the C++ HTTP server as a refactor.** It is greenfield — `src/host/http_server.cpp` does not exist. Do not invent a fictional predecessor to pivot from.
- **Sneaking back the Fase 0 in-DLL bridge for any reason.** The pivot rejected it; the multi-runtime specs are archived.
- **Letting the LLM bypass `overwrite=true` silently.** The flag exists for safety. Document and enforce in the bridge.

## Next phase

`sdd-propose` should produce a `proposal.md` that:

1. **Captures the intent**: ReaForge becomes an agentic MVP, pivoting the bridge 5 → 7 and shrinking the C++ extension surface to HTTP + filesystem writes + REAPER refresh.
2. **Lists the 7 PRs** from `04-mvp-and-handoff.md` §"Suggested PR breakdown" as a tentative delivery strategy, with the lane tags (`python-tdd` / `cpp-unit` / `runtime-reaper`) from Engram #79.
3. **Identifies the new capabilities** vs the archived multi-runtime ones:
   - **New**: 7 bridge tools, HTTP server, artifact writer, project reader, refresh hook, embedded API reference.
   - **Archived**: in-DLL Lua/QuickJS/JSFX bridges, embedded runtimes, "ship a sample extension" pattern, 3-flow user model.
4. **States the success criterion** verbatim: *"The user opens opencode desktop, types one of the example prompts, and within 2 minutes a working JSFX/Lua/RfxChain file appears in the right REAPER folder, ready to use."*
5. **Captures the 6 user-confirmed defaults** as a "Decisions" table so `sdd-spec` doesn't re-ask.
6. **References this `explore.md`** for the full reasoning.

After `sdd-propose`, the standard flow continues: `sdd-spec` (delta specs for `bridge-tools-v2`, `extension-write-api`, `refresh-protocol`) → `sdd-design` (tech design for the 4 new C++ modules + the embedded `api_reference` payload) → `sdd-tasks` (PR-sized work units, lane-tagged) → `sdd-apply` → `sdd-verify` → `sdd-archive`.
