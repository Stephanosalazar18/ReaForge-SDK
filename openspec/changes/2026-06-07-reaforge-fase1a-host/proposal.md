# Proposal: ReaForge Fase 1a — Host with ReaImGui Panel

## Intent

Promote the Phase 0 spike from "library that does no-ops" to "platform you can see and use from inside REAPER". This change delivers the user-facing host for **Flow 1** (see `docs/user-flows.md`): a ReaImGui dock panel, context menu integration, and one working sample extension. The user interacts with ReaForge entirely from inside REAPER; no external agent is required.

**Flow 2** (opencode as an external CLI control plane) and **Flow 3** (opencode chat embedded as a ReaImGui dock) are explicitly **deferred to Fase 5** (`opencode-bridge` change). They are not part of this proposal and do not block the Fase 1a ship.

## Scope

### In Scope

- `src/host/` — promoted from `src/spike/` (the spike becomes the production code path).
  - `host.h` / `host.cpp` — public entry, registers with REAPER, owns the executor and panel.
  - `panel.h` / `panel.cpp` — ReaImGui-based Extensions Manager.
  - `context_menu.h` / `context_menu.cpp` — hook into REAPER's track and item context menus.
  - `extension_loader.h` / `extension_loader.cpp` — discover and load extensions from a configurable path.
- `src/host/extensions/humanize_midi/` — one shipped Lua extension with manifest, source, and registration entry.
- Updated `meson.build` to produce a `.dll` / `.dylib` / `.so` that REAPER can load on each target platform.
- Updated `README.md` with a "Build and load into REAPER" section.

### Out of Scope

- ReaPack packaging and distribution (Fase 3).
- Web registry (Fase 4).
- **`opencode-bridge` and any AI/agent integration (Fase 5).** This includes the opencode chat dock (Flow 3) and the opencode-as-MCP-server flow (Flow 2). They are not deliverables of Fase 1a.
- Multi-platform CI (Fase 6 — can be added in a follow-up change).
- More than one sample extension.
- Bridge surface expansion beyond the 5 functions from the spike.
- JSFX runtime beyond the stub.

## Capabilities

### New Capabilities

- `extensions-manager-panel` — a ReaImGui dock panel inside REAPER that lists loaded extensions and offers load/reload/unload controls.
- `context-menu-integration` — right-click on a track or item shows an "Extensions → [list]" submenu; clicking an entry invokes the extension.
- `host-lifecycle` — the host registers itself with REAPER on load, unregisters on unload, and survives project switches.

### Modified Capabilities

- `multi-runtime`, `extension-execution`, `runtime-bridge` — promoted from the spike's change folder to the main `openspec/specs/` (will be done at archive time if this change archives; not in this proposal).

## Approach

- **Panel**: ReaImGui via a `pimpl` wrapper. The host keeps an `ImGui` context per docked instance. Initial UI: a list view (extension id, status, runtime) with a "Reload" button per row and an "Open extension folder" link.
- **Context menu**: ReaImGui's menu API + REAPER's `g_accel_register` mechanism (via `reaper_plugin.h`). One top-level entry "ReaForge → [extension list]" injected into the track and item right-click menus.
- **Loader**: scans a configurable directory (default: `<REAPER resource path>/ReaForge/extensions/`) at startup and on demand. Each extension lives in a subfolder with a `manifest.lua` that declares id, name, runtime, entry function, and target type (`track`, `item`, `master`).
- **Sample extension**: `humanize_midi` is a single Lua file that takes the selected MIDI item, randomizes note timing and velocity within user-specified ranges, and writes back. ~30 LOC of Lua. Demonstrates: manifest format, bridge access, undo points, structured result.

## Affected Areas

| Area | Impact | Description |
|------|--------|-------------|
| `src/spike/` | Rename / Move | Becomes `src/host/`; `spike` namespace → `host` namespace. The spike stays in git history; the directory name is the only thing that changes. |
| `src/host/panel.{h,cpp}` | New | ReaImGui Extensions Manager |
| `src/host/context_menu.{h,cpp}` | New | Menu integration |
| `src/host/extension_loader.{h,cpp}` | New | Filesystem-based discovery |
| `src/host/extensions/humanize_midi/` | New | Sample Lua extension |
| `meson.build` | Modify | Cross-platform target names and ReaImGui dependency |
| `README.md` | Modify | Add "Build and load into REAPER" section |
| `third_party/reaimgui` | New (submodule) | cfillion's ReaImGui, vendored |

## Risks

| Risk | Likelihood | Mitigation |
|---|---|---|
| ReaImGui archived upstream — break in future | Med | Vendor a pinned commit; bump deliberately |
| MSVC ABI breaks Windows build | Med | CI on `windows-latest` will catch it before release |
| Renaming `src/spike/` to `src/host/` creates merge conflicts with the open spike change | Low | The spike change is in `openspec/changes/` and won't conflict with `src/` renames. Apply this change on a fresh branch from `main` after the spike archives (or alongside; the file paths don't collide). |
| `humanize_midi` ships with subtle bug in note math | Low | Phase 0's `runtime-bridge` spec gives us the floor; if math is wrong it's a content bug, not architectural |
| ReaImGui context creation needs the REAPER main thread | High | Document the constraint; the spike's main-thread guard pattern extends to the panel |

## Rollback Plan

- The change is contained in `src/host/`, `meson.build`, and `README.md`. The spike stays at `src/spike/` (untouched). Reverting is `git revert` of the merge commit.
- If ReaImGui turns out to be a dead end, the panel and menu files are removed; the loader and host stay. The `humanize_midi` sample is reusable regardless of UI.

## Dependencies

- All Phase 0 spike code (already in main, just in `src/spike/`).
- ReaImGui submodule (new vendor; commit pinned).
- A REAPER-capable build host (Linux/macOS for now; Windows MSVC in Fase 6).

## Success Criteria

- [ ] The host builds on Linux, macOS, and Windows (MSVC).
- [ ] Loading the binary into REAPER shows the Extensions Manager panel.
- [ ] Right-clicking a track or item shows the "ReaForge → [list]" submenu.
- [ ] The `humanize_midi` extension runs on a real MIDI item and produces a measurable change in note positions or velocities.
- [ ] Reload button in the panel causes the extension to re-execute its source.
- [ ] No crashes on REAPER shutdown.
- [ ] ReaForge 1.0 can be tagged after this change ships.
