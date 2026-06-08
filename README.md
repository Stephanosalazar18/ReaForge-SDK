# ReaForge

An [opencode](https://github.com/sst/opencode)-powered REAPER extension that **generates native REAPER artifacts on demand** — JSFX effects, ReaScript Lua scripts, and FX chains — from plain-English prompts. Inspired by the [Ableton Extensions SDK](https://www.ableton.com/en/live/extensions/) philosophy ("describe the idea, an AI assistant builds it") but built for REAPER's native runtimes, not a new embedded interpreter.

> ReaForge does **not** mutate live REAPER state. It only reads project context and writes files into REAPER's own folders. The user runs the generated artifacts when they want.

## Status

**Agentic MVP — PR 1 of 7.** Documentation rewrite. The pivot from the previous "multi-runtime SDK" scope to the current "artifact generator" scope is captured in `docs/documentacion/01-vision-and-pivot.md`.

| Item | State |
|---|---|
| Bridge spike (5 pre-pivot tools) | GO (5/5) — see `bridge-spike-results.md` |
| MVP (7 tools, HTTP C++ extension) | In progress — 7 PRs |
| Multi-runtime specs | Rejected by the pivot, archived post-MVP |

## Try it

Open opencode Desktop with the ReaForge bridge configured, type any of these prompts, and within 2 minutes the artifact lands in the right REAPER folder ready to use:

> **"Generate a JSFX that does soft tape saturation"** → `Effects/ReaForge/tape_saturation.jsfx`

> **"Write a Lua script that doubles the velocity of selected MIDI notes"** → `Scripts/ReaForge/double_velocity.lua`

> **"Combine the built-in delay and ReaEQ into a vocal slap chain"** → `FXChains/ReaForge/vocal_slap.RfxChain`

opencode's built-in MCP tool approval shows you the generated code before any `reaforge_save_*` call lands — we do not build our own confirmation UI.

## How it works

```
opencode desktop ──► MCP bridge (Python, 7 tools) ──► HTTP ──► C++ extension inside REAPER ──► filesystem
                                                                                                  │
                                            REAPER loads the artifacts natively from:           ▼
                                            Effects/ReaForge/*.jsfx
                                            Scripts/ReaForge/*.lua
                                            FXChains/ReaForge/*.RfxChain
```

| Layer | Lives in | Role |
|---|---|---|
| Agent | opencode Desktop (external) | Talks to the user, decides what to generate, calls bridge tools |
| Bridge | `tools/opencode_bridge.py` | 7 MCP tools (3 Read + 3 Write + 1 Refresh), translates to HTTP |
| Extension | `src/host/` → `reaper_reaforge_host.dll` | HTTP server, filesystem writes, REAPER refresh hook |
| REAPER | DAW | Loads the artifacts natively — no runtime embedding |

## Build and load

### Linux (WSL2 Ubuntu)

```bash
./scripts/install-build-deps.sh
./scripts/build-and-load-linux.sh
```

The script installs `meson`, `lua5.4`, `g++`, configures the build, and copies `libreaper_reaforge_host.so` into `~/.config/REAPER/UserPlugins/`. Restart REAPER; the host extension auto-registers.

### Windows (MSVC, native)

```cmd
scripts\build-windows.bat
```

Produces `reaper_reaforge_host.dll` and copies it to `%APPDATA%\REAPER\UserPlugins\`. See [`docs/documentacion/`](docs/documentacion/) for the full build walkthrough.

## The 7 bridge tools

| Group | Tool | What it does |
|---|---|---|
| Read | `reaforge_get_state` | Project snapshot (tracks, items, FX per track, BPM, sample rate) |
| Read | `reaforge_list_artifacts` | Files under the 3 `ReaForge/` subfolders with size + mtime |
| Read | `reaforge_get_api_reference(target)` | Embedded cheatsheet for `jsfx` / `reascript_lua` / `fx_chain_format` |
| Write | `reaforge_save_jsfx(name, code)` | Writes `Effects/ReaForge/<name>.jsfx` |
| Write | `reaforge_save_lua(name, code, register_action?)` | Writes `Scripts/ReaForge/<name>.lua`; opt-in action registration |
| Write | `reaforge_save_fx_chain(name, content)` | Writes `FXChains/ReaForge/<name>.RfxChain` |
| Refresh | `reaforge_refresh()` | Asks REAPER to rescan FX + Action List |

`save_*` tools refuse to overwrite existing files unless `overwrite=true` is passed.

## Documentation

| Doc | What it covers |
|---|---|
| [`docs/documentacion/`](docs/documentacion/) | The handoff: vision, architecture, MVP scope, plan |
| [`docs/cross-environment.md`](docs/cross-environment.md) | Transport across WSL → Windows → REAPER (still valid) |
| [`openspec/changes/2026-06-07-reaforge-agentic-mvp/`](openspec/changes/2026-06-07-reaforge-agentic-mvp/) | The current change (proposal, specs, design, tasks) |

## Roadmap

| Phase | Goal | Entry point |
|---|---|---|
| **MVP (now)** | 7 tools live, opencode Desktop drives | opencode Desktop |
| **+1 Polish** | Richer context (FX params, automation), smarter naming | opencode Desktop |
| **+2 Context menu** | Right-click on track/item/FX → "Generate with ReaForge" | REAPER context menu |
| **+3 Internal chat** | ReaImGui chat dock inside REAPER (shells out to opencode) | REAPER dock |
| **+4 Distribution** | ReaPack package, signed builds | ReaPack |

## License

MIT. See [LICENSE](LICENSE).
