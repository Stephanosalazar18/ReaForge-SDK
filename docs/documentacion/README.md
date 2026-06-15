# ReaForge — Handoff Documentation

> Successor agent: read this first. The files below are ordered. Do not skip.

## What ReaForge is (one line)

A REAPER extension that lets a user describe what they want in natural language and have an AI agent generate the correct REAPER artifact (`.jsfx`, `.lua`, `.RfxChain`) and save it in the right folder, ready to use.

## Status (snapshot at 2026-06-07, commit `7d36013`)

| Item | State |
|---|---|
| Original vision | Pivoted — was overscoped as a multi-runtime SDK for devs |
| Current vision | Agent-powered artifact generator for REAPER users |
| Code on `main` | Multi-runtime spike + Fase 1a host + bridge spike (Python) |
| Bridge spike verdict | GO (5/5 tools round-trip vs Python stub) |
| Fase 1a verify | FAIL pending — must run on REAPER Windows |
| What pivots | Bridge tools (5 → 7, semantics change), C++ extension scope |
| What is archived (do not extend) | Embedding Lua/QuickJS/JSFX runtimes inside the DLL |
| Next change | Open a new SDD: "ReaForge agentic MVP" |

## Read order

1. [`01-vision-and-pivot.md`](./01-vision-and-pivot.md) — what ReaForge is now, what it is NOT, why the original scope was wrong
2. [`02-architecture.md`](./02-architecture.md) — components, data flow, key technical decisions
3. [`03-current-state.md`](./03-current-state.md) — what's in the repo, what survives, what gets archived
4. [`04-mvp-and-handoff.md`](./04-mvp-and-handoff.md) — MVP scope and the concrete next step

## Key repo paths

| Path | Role |
|---|---|
| `src/host/` | C++ REAPER extension (host + panel + context menu + sample) |
| `tools/opencode_bridge.py` | MCP bridge (validated 5/5 — needs tool pivot) |
| `tools/stub_reaper_server.py` | Python HTTP stub of the extension (validation harness) |
| `tools/run_bridge_smoke.sh` | End-to-end smoke for the bridge |
| `tools/interactive_bridge_test.py` | Interactive REPL over the bridge |
| `scripts/{install-build-deps,build-and-load-linux,build-windows}.sh` | Toolchain + build helpers |
| `docs/cross-environment.md` | Pre-pivot cross-env transport (still valid for the HTTP layer) |
| `docs/user-flows.md` | Pre-pivot 3-flow model (superseded — see `01-vision-and-pivot.md`) |
| `openspec/changes/2026-06-07-reaforge-bridge-spike/` | Bridge spike SDD (done, GO) |
| `openspec/changes/2026-06-07-reaforge-fase1a-host/` | Fase 1a host SDD (merged, verify pending) |
| `openspec/specs/{multi-runtime,extension-execution,runtime-bridge}/` | Archived specs from Phase 0 (do not extend) |

## Conversation context

- The user (Stephano) speaks **Rioplatense Spanish (voseo)** in chat. Reply in Spanish, keep code/identifiers/file content/commit messages in English.
- Tone: **senior architect**, direct, push back when wrong, explain why with evidence.
- The user values: solid foundations, concepts over code, the human directs the AI.
- This handoff doc was written immediately after a scope-clarifying conversation. Pre-pivot artifacts (`docs/cross-environment.md`, `docs/user-flows.md`, the existing SDD changes) have **not** been rewritten — they remain as historical context.

## Tooling notes

- The successor agent has access to Engram persistent memory. Search `mem_search "reaforge"` with `all_projects: true` for the full session history. The most recent session summaries are observations #71–#76.
- SDD skills available: `sdd-explore`, `sdd-propose`, `sdd-spec`, `sdd-design`, `sdd-tasks`, `sdd-apply`, `sdd-verify`, `sdd-archive`.
- Build: WSL2 Ubuntu has the Linux toolchain (`meson`, `lua5.4`, `g++`). MSVC cross-build script is `scripts/build-windows.sh` but the user typically builds the Windows DLL natively on Windows.
