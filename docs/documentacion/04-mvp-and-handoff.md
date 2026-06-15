# ReaForge — MVP Scope and Handoff

## MVP definition

**Goal**: validate that an external agent (opencode desktop) can generate useful REAPER artifacts on demand by talking to ReaForge through the bridge.

**Single success criterion**:

> The user opens opencode desktop with the ReaForge bridge configured, types one of the example prompts, and within 2 minutes a working JSFX / Lua / RfxChain file appears in the right REAPER folder and works when used.

**Acceptance checklist** (all must pass):

- [ ] User says: *"Generate a JSFX that does soft tape saturation"* → file at `<REAPER>/Effects/ReaForge/tape_saturation.jsfx`, audibly saturates when added to a track, does not crash REAPER.
- [ ] User says: *"Write a Lua script that doubles the velocity of selected MIDI notes"* → file at `<REAPER>/Scripts/ReaForge/double_velocity.lua`, runs as a REAPER action, modifies the notes.
- [ ] User says: *"Combine the built-in delay and ReaEQ into a vocal slap chain"* → file at `<REAPER>/FXChains/ReaForge/vocal_slap.RfxChain`, loads as a chain.

## The 7 bridge tools (MVP)

### Read — 3 tools (give the agent context)

| Tool | Returns | Used for |
|---|---|---|
| `reaforge_get_state` | Project snapshot: tracks, items, selected items, active FX per track, sample rate, project name | Agent grounds generation in the user's current project |
| `reaforge_list_artifacts` | List of files under `Effects/ReaForge/`, `Scripts/ReaForge/`, `FXChains/ReaForge/` with timestamps | Avoid duplicates, support "regenerate" |
| `reaforge_get_api_reference(target)` | Embedded docs / cheatsheet for `target ∈ {"jsfx", "reascript_lua", "fx_chain_format"}` | Reduce hallucination in generated code |

### Write — 3 tools (agent's only side effect)

| Tool | Effect | Returns |
|---|---|---|
| `reaforge_save_jsfx(name, code)` | Writes `<REAPER>/Effects/ReaForge/<name>.jsfx` (creates folder if missing) | Absolute path |
| `reaforge_save_lua(name, code, register_action: bool = false)` | Writes `<REAPER>/Scripts/ReaForge/<name>.lua`. If `register_action=true`, registers it in REAPER's action list. | Absolute path + action ID if registered |
| `reaforge_save_fx_chain(name, content)` | Writes `<REAPER>/FXChains/ReaForge/<name>.RfxChain` | Absolute path |

### Refresh — 1 tool (make REAPER see the new artifact)

| Tool | Effect |
|---|---|
| `reaforge_refresh()` | Triggers REAPER to rescan FX list and action list |

**Note**: opencode's built-in MCP tool approval covers the *"user reviews the code before save"* gate. We do not build our own confirmation UI.

## What is out of MVP scope

| Out | Reason |
|---|---|
| Internal ReaImGui chat panel | Last phase. MVP is opencode desktop only. |
| Context menu hooks (right-click → "Generate X") | Last phase. |
| Tools that mutate live REAPER state (open automation, set param, play, stop) | Explicitly rejected by the user. |
| Artifact versioning / git integration | Defer. User can git their REAPER resource folder. |
| Templates library, project profiles, presets | Defer. |
| Authentication, multi-user, cloud sync | Out of scope. Single-user local tool. |
| Re-embedding any runtime in the DLL | Explicitly rejected by the pivot. |

## Phasing after MVP

| Phase | Goal |
|---|---|
| **MVP** (this) | 7 tools live. opencode desktop drives. User validates the 3 acceptance prompts. |
| **+1 polish** | Smarter context (selected FX param values, automation envelopes for reference). Better artifact naming. |
| **+2 context menu** | Right-click on track/item/FX → "Generate with ReaForge" → shells out to `opencode --prompt …` |
| **+3 internal chat panel** | ReaImGui chat dock inside REAPER. Pipes prompt to opencode under the hood (no embedded LLM client). |
| **+4 distribution** | ReaPack package, install script, signed binaries. |

## Concrete next step for the successor agent

Open a new SDD change. Suggested name: `2026-06-XX-reaforge-agentic-mvp`.

```
openspec/changes/<date>-reaforge-agentic-mvp/
├─ explore.md       # Capture pivot reasoning, link this folder
├─ proposal.md      # Pivot bridge tools (5→7), pivot extension scope
├─ specs/           # Delta specs:
│   ├─ bridge-tools-v2.md
│   ├─ extension-write-api.md
│   └─ refresh-protocol.md
├─ design.md        # Tech design for 7 tools + C++ filesystem writer + refresh hook
├─ tasks.md         # PR-sized work units
└─ verify-report.md # (after implementation)
```

Use the existing SDD skills in order: `sdd-explore` → `sdd-propose` → `sdd-spec` → `sdd-design` → `sdd-tasks` → `sdd-apply` → `sdd-verify` → `sdd-archive`.

### Suggested PR breakdown

| PR | Scope | Size |
|---|---|---|
| 1 | Update README.md to reflect new vision. Remove multi-runtime SDK language. Point to `docs/documentacion/`. | small (~80 LOC) |
| 2 | Pivot `tools/opencode_bridge.py`: replace 5 tools with 7. Stub backends. | medium (~150 LOC) |
| 3 | Pivot `tools/stub_reaper_server.py`: mock the 7 endpoints with hardcoded data. Update smoke. | medium |
| 4 | C++ extension: implement the 3 Write endpoints (filesystem writes to `Effects/ReaForge/`, `Scripts/ReaForge/`, `FXChains/ReaForge/`). | medium |
| 5 | C++ extension: implement the 3 Read endpoints (project state, artifact list, API reference) | medium |
| 6 | C++ extension: implement `reaforge_refresh` (call REAPER's API to rescan FX + actions) | small |
| 7 | End-to-end test on REAPER Windows. Document setup in README. | docs + manual verify |

Each PR ≤ ~400 LOC where possible. Use the `chained-pr` skill for the chain.

## Open questions to confirm with the user BEFORE opening the SDD change

| # | Question | Default if user agrees |
|---|---|---|
| 1 | Confirm folder convention: `<REAPER resource>/{Effects,Scripts,FXChains}/ReaForge/` | Yes |
| 2 | What to do with Fase 1a verify (panel + context menu + sample)? Verify first, or skip and pivot directly? | Skip — those features are deferred to phase +2/+3 anyway |
| 3 | Bridge tool naming: keep `reaforge_*` prefix or shorten to `rf_*`? | Keep `reaforge_*` (more explicit) |
| 4 | `register_action` default for `reaforge_save_lua`: `true` or `false`? | `false` (less surprising; user opts in) |
| 5 | Should `reaforge_get_api_reference` ship embedded docs in the DLL, or fetch from a URL? | Embedded (offline-first, no network dep) |
| 6 | Should `reaforge_save_*` overwrite existing files silently, or refuse and require an `overwrite` flag? | Refuse without `overwrite=true` (safer) |

## Anti-patterns to avoid

| Anti-pattern | Why bad |
|---|---|
| Re-embedding Lua / QuickJS / JSFX runtimes in the DLL | Explicitly rejected. REAPER is the runtime. |
| Adding tools that mutate live REAPER state in this MVP | Explicitly out of scope. |
| Building a chat UI inside REAPER first | Deferred. MVP is opencode desktop only. |
| Designing for multiple users / cloud sync | Single-user local tool. |
| Presenting the user with option menus instead of direct recommendations | The user wants recommendations with reasoning, not lists. |
| Switching language to English in chat replies | User speaks Rioplatense Spanish. Code/docs/commits stay English. |
| Skipping `mem_search` on first message | The user's "lost the previous session" message means: load Engram context BEFORE replying. |

## When the MVP is shipped

After the 3 acceptance prompts pass:

1. Update `README.md` with the new vision + quickstart
2. Open the next SDD change for phase +1 (polish)
3. Archive the current `openspec/changes/2026-06-07-reaforge-fase1a-host/` and `2026-06-07-reaforge-bridge-spike/` changes
4. Move the multi-runtime specs in `openspec/specs/` to `openspec/specs/archive/` with a `REJECTED.md` explaining the pivot
