# Explore: ReaForge Fase 1a — Host with ReaImGui Panel (2026-06-07)

## Context

The Phase 0 spike ([`2026-06-07-reaforge-spike/`](../../2026-06-07-reaforge-spike/)) is in **FAIL** status — code is complete but the spec gate requires runtime verification on a REAPER Linux host, which this WSL2 environment does not provide. The spike is left open until the user runs `./scripts/run_benchmark.sh` on a REAPER-capable machine.

While we wait for that verification, we open **Fase 1a** in parallel: the user-facing C++ host with a ReaImGui panel and context menu hooks. This change builds on the spike's code (which is already committed) but adds the visible UI layer and the menu integration that turns ReaForge from a "library that does no-ops" into a "platform you can use from REAPER".

## What the spike gave us (already in main)

- `src/spike/` — host, 3 runtimes, bridge, executor, benchmark, meson.build.
- `src/spike/bridge/` — 5 read-only REAPER functions exposed to all 3 runtimes.
- `src/spike/runtime/{lua,quickjs,jsfx}_runtime.{h,cpp}` — runtime embeds.
- `third_party/quickjs-ng`, `third_party/reaper-sdk` — vendored submodules.

## What Fase 1a adds

1. **ReaImGui-based Extensions Manager panel** — a dockable panel inside REAPER that lists loaded extensions, shows their status, and offers load/reload/unload controls.
2. **Context menu integration** — right-click on a track or item shows an "Extensions → [list]" submenu, wired to the host's invocation router.
3. **A first real extension** — `humanize-midi` (Lua) shipped as a working sample, with the manifest, source, and registration wiring.
4. **Cross-platform build path** — `meson.build` updated to target Windows MSVC (or the closest available toolchain on the user's machine), macOS clang, and Linux g++.

## ReaImGui: a real risk

cfillion archived `reaimgui` on 2026-06-03. The repo moved to Codeberg. The library still builds and is in active use (ReaPack 1.2.6 still ships it). The risk for Fase 1a is:

| Risk | Likelihood | Mitigation |
|---|---|---|
| ReaImGui becomes incompatible with a future REAPER version | Low | Vendor a specific commit; we control the upgrade path |
| cfillion stops releasing builds | Med | We can build from source via meson; we already vendored `reaper-sdk`, vendoring `reaimgui` is the same pattern |
| ReaImGui's API is C++-only, blocking Lua-script authors from writing UI | Low (acceptable) | UI is a host concern; extension authors write logic, the host renders |

If ReaImGui becomes unusable, the fallback is to write a minimal JSFX-style panel in C++ directly using the REAPER `gmidi_imgui_*` API (cfillion's earlier work) or to vendor Dear ImGui and bind it ourselves. Both are tractable but out of scope for Fase 1a.

## Cross-platform build reality

The REAPER Extension SDK requires **MSVC ABI on Windows** for the binary to load. This is a hard constraint:

- **Linux spike (Phase 0)**: g++ 11, works.
- **macOS**: clang 14+, works (the SDK has no MSVC requirement on Apple platforms).
- **Windows**: requires MSVC `cl.exe` from Visual Studio Build Tools. MingW produces a non-loadable binary. Cross-compile with `x86_64-w64-mingw32` is theoretically possible but not documented to work.

Practical consequence: the Windows build needs to run on a Windows machine (or a Windows VM). CI will use `windows-latest` GitHub Actions runners; the build step invokes `meson` after `vcvarsall.bat amd64` is sourced.

## Scope guardrails for Fase 1a

- No ReaPack distribution yet (Fase 3).
- No web registry (Fase 4).
- No `opencode-bridge` (Fase 5).
- No extensions beyond the one sample (`humanize-midi`).
- No CI configuration (Fase 6 concern; can land in this change only if it is small and orthogonal).

## Open questions for the proposal

1. Should the Extensions Manager panel use ReaImGui's docking or a floating window? (ReaImGui supports both.)
2. Should context menu integration be one entry per extension, or one entry "Extensions →" with a submenu?
3. Is `humanize-midi` the right sample, or should we pick something even simpler (e.g., a "list selected tracks" extension that just prints to console)?

These are addressed in `proposal.md`.
