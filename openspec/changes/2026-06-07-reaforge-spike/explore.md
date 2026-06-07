# Explore: ReaForge Spike (2026-06-07)

## Environment Snapshot

- **Host**: WSL2 Ubuntu 22.04, Windows host with REAPER installed.
- **Toolchain present**: g++ 11.4.0, node v24.10.0, pnpm, python3.10, rustc, go, make, git.
- **Toolchain missing**: clang, meson, cmake, lua5.4-dev (apt-installable).
- **REAPER**: installed at `C:\Program Files\REAPER (x64)\` (Windows). Resource path expected at `C:\Users\<user>\AppData\Roaming\REAPER\`.
- **Repo state**: empty on GitHub, freshly cloned at `ReaForge-SDK`. No prior history.

## State of the Art (what already exists)

### MCPs for REAPER (the "agent" path)

- `bonfire-audio/reaper-mcp` (83★) — 58 tools, Python + reapy, Claude Desktop ready.
- `wegitor/reaper-reapy-mcp` (67★) — stronger on musical positioning (measure:beat).
- `xDarkzx/Reaper-MCP` (16★) — 163 tools, broader coverage.

All three are **single-language (Python) + reapy + MCP stdio**. None offers a platform for third-party extensions, none offers a UI inside REAPER, none is AI-extensible.

### REAPER's existing extension surface

| Tool | Type | Purpose |
|---|---|---|
| ReaScript | Embedded Lua / EEL2 / Python | Full ReaScript API access |
| Extension SDK | C++ (MSVC ABI) | Compiled plugins, deepest integration |
| SWS Extension | C++ | Community actions, cycle actions, etc. |
| ReaImGui | C++/Lua | Dear ImGui bindings, UI inside REAPER |
| ReaPack | C++ | Package manager for ReaScripts, themes, extensions |
| OSC | Built-in | External control surface protocol |
| JSFX | Built-in DSL | Audio DSP only (no API access) |

**Key clarification**: JSFX is *not* general-purpose JavaScript. It is a REAPER-embedded DSL with JS-like syntax whose only communication channels with REAPER are audio I/O, MIDI I/O, and effect parameters. It **cannot** read the project state, cannot list tracks, cannot call actions. It is for DSP only.

### Reference: Ableton Extensions SDK

- Live 12.4.5+ Suite, JavaScript/TypeScript on Node.js v24 LTS.
- Extensions are **one-shot** scripts triggered from context menus.
- AI-friendly by design (Ableton markets this explicitly).
- Different from Max for Live (which is a patching environment).
- A platform, not a product — third parties author extensions.

## Architecture Decision: Multi-Runtime

The user explicitly rejected a single-language SDK in favor of a **multi-runtime host** that lets each extension declare its language. Justification:

- **Lua (ReaScript)** for control: track/item/MIDI manipulation, automation, render orchestration. Native, fast, no bridge.
- **JSFX** for audio DSP: synthesizers, effects, processors. Native to REAPER, no extra runtime.
- **TypeScript / JavaScript (via QuickJS-ng embedded)** for complex UI, IPC, integrations, and AI-authored logic. Sandboxed, no Node required.

The **opencode-bridge** agent (Phase 5) is the language router: it knows when to ask for a JSFX, when for Lua, when for TS.

## Open Questions (resolved in the spike)

1. Can `reaper_jsfx_compile()` or equivalent compile a JSFX at runtime from a C++ extension? **Must verify in spike.** If not, JSFX must be pre-compiled and shipped as part of the extension.
2. What is the ABI cost of embedding QuickJS-ng? **Measure in spike.** Target: <5ms overhead per call.
3. Does the C++ host need to manage thread affinity with REAPER's main thread for all three runtimes? **Verify in spike.** REAPER's API is main-thread only.
4. What is the minimum surface of the ReaScript API the spike needs to expose to validate interop? **Define in spike** as a 5-10 function subset.

## Risks Entering the Spike

| Risk | Likelihood | Mitigation |
|---|---|---|
| `reaper_jsfx_compile()` is not public in REAPER's C++ SDK | Med | Spike Task 1.1 will probe this in 30 min; fallback to bundled compiled JSFX |
| MSVC ABI on Windows blocks the g++ spike | Low | Spike targets Linux first (REAPER on Linux is supported); Windows port is Phase 1 concern |
| ReaImGui archived by upstream (cfillion, 2026-06-03) | Med | Spike does not depend on ReaImGui; only the C++ host and runtimes. UI work is Phase 1+. |
| QuickJS-ng ABI breaks between patch versions | Low | Pin to a specific commit; vendor as submodule |
| Multi-runtime coexistence causes symbol collisions or thread issues | Med | The spike is the empirical check; that's the point |

## Next Phase After Spike

If the spike returns **go**, Phase 1 (Lua MVP) will:
- Add a ReaImGui-based Extensions Manager panel.
- Hook context menus (right-click on tracks/items).
- Ship 3 example extensions (humanize MIDI, render selection, FX chain preset).
- Build CI on Windows MSVC, macOS clang, Linux g++.

If the spike returns **no-go**, the pivot options are:
1. **Lua-only** SDK (most conservative, drops TS).
2. **QuickJS-only** SDK (most AI-friendly, drops Lua reusability).
3. **Hybrid JSFX + Lua**, no TS (drops UI-rich extensions).
