# Tasks: ReaForge Fase 1a — Host with ReaImGui Panel

## Review Workload Forecast

| Field | Value |
|-------|-------|
| Estimated changed lines | ~900 (host + 3 new components + sample extension + tests) |
| 400-line budget risk | **High** |
| Chained PRs recommended | **Yes** |
| Suggested split | 3 PRs (Foundation / Loader+Sample / Panel+Menu) |
| Delivery strategy | ask-on-risk |

Decision needed before apply: Yes
Chained PRs recommended: Yes
Chain strategy: pending
400-line budget risk: High

### Suggested Work Units

| Unit | Goal | Likely PR | Notes |
|------|------|-----------|-------|
| 1 | Foundation: rename + ReaImGui vendor + host entry point | PR 1 | Branches from `main`; build verifies; no new behavior yet |
| 2 | Extension loader + sample `humanize_midi` | PR 2 | Branches from PR 1; unit-tested; behavior visible only via internal API |
| 3 | Panel + context menu hooks | PR 3 | Branches from PR 2; user-facing; needs REAPER for verification |

The user must pick a chain strategy before apply: **stacked-to-main** (each PR merges to main in order; fast iteration), **feature-branch-chain** (a `fase1a` tracker branch holds the integration; child PRs target the previous PR's branch), or **size:exception** (single PR).

---

## Phase 1: Refactor (spike → host)

- [x] 1.1 Rename `src/spike/` → `src/host/` (preserve git history via `git mv`)
- [ ] 1.2 Update namespaces from `reaforge` to `reaforge::host` across the renamed files — *deferred to a cleanup PR; PR 1 keeps the existing `namespace reaforge` and adds `namespace host` as a sub-namespace only in new files (panel, host.cpp)*
- [x] 1.3 Split `src/spike/meson.build` into a root `meson.build` and `src/host/meson.build` — *partially: root meson.build created; src/host/meson.build is the renamed version with new panel/host targets*
- [ ] 1.4 Verify the rename builds (deferred — needs `meson` + `lua5.4-dev`)

## Phase 2: ReaImGui Vendor and Panel Skeleton

- [x] 2.1 Vendor `reaimgui` (Codeberg) as a git submodule at `third_party/reaimgui/`
- [x] 2.2 Add reaimgui as a subproject in root `meson.build` (subproject declaration; explicit linkage is added in PR 3 when panel widgets are used)
- [x] 2.3 Create `src/host/panel.h` declaring `panel::create`, `panel::destroy`, `panel::render`, `panel::on_reload`
- [x] 2.4 Create `src/host/panel.cpp` with stub implementations (no ReaImGui yet)

## Phase 3: Panel Implementation

- [ ] 3.1 Render the extension list (id, runtime, status) by querying the `ExtensionLoader` registry
- [ ] 3.2 Render a "Run" button per row that invokes the extension via the `Executor`
- [ ] 3.3 Render a "Reload" button per row that calls `ExtensionLoader::reload(id)`
- [ ] 3.4 Render an "Open extensions folder" link in the panel footer
- [ ] 3.5 Show a transient toast for run/reload results (success or error message)
- [ ] 3.6 Unit test: panel state reflects the registry after a Reload (mock the registry)

## Phase 4: Context Menu Integration

- [ ] 4.1 Implement `context_menu::register_hooks` using `HookCustomMenu` for the track menu
- [ ] 4.2 Implement the same for the item menu
- [ ] 4.3 Filter the registered extensions by `target` (track vs item vs midi_item)
- [ ] 4.4 Implement the click callback: build an `InvocationRequest` with the appropriate `args` (track index or item handle) and dispatch via the `Executor`
- [ ] 4.5 Implement `context_menu::invalidate_cache` so the next menu build reflects the latest registry
- [ ] 4.6 Add error visibility: runtime errors during menu invocation go to the REAPER console

## Phase 5: Extension Loader

- [ ] 5.1 Resolve the host extensions directory: `<REAPER resource path>/ReaForge/extensions/`
- [ ] 5.2 Create the directory on first init if it does not exist
- [ ] 5.3 Scan the directory for subfolders; for each, look for `manifest.lua`
- [ ] 5.4 Evaluate the `manifest.lua` in a sandboxed Lua state and parse the returned table
- [ ] 5.5 Build a `std::vector<Manifest>` registry and expose it via `loader::all()`
- [ ] 5.6 Implement `loader::reload(id)` which re-evaluates one manifest + clears the context menu cache
- [ ] 5.7 Unit test: scan a temp dir with 3 mock manifests, assert 3 entries with correct fields
- [ ] 5.8 Unit test: reload a manifest that has been edited on disk, assert the registry updates

## Phase 6: Sample Extension — `humanize_midi`

- [ ] 6.1 Create `src/host/extensions/humanize_midi/manifest.lua` declaring id, name, runtime, target, entry
- [ ] 6.2 Create `src/host/extensions/humanize_midi/humanize_midi.lua` with the `run(ctx)` function (~30 LOC of Lua)
- [ ] 6.3 Confirm at apply time: whether the Lua source can call `reaper.MIDI_GetNotes` directly, or if the bridge needs extending. If the bridge needs extending, raise it as a follow-up spec change (do not silently grow the bridge here)

## Phase 7: Host Entry Point

- [ ] 7.1 Create `src/host/host.cpp` with the `reaper_plugin_info` export
- [ ] 7.2 Wire `host::init()` to call: Executor → Loader → ContextMenu → Panel (in that order)
- [ ] 7.3 Wire `host::shutdown()` to call the reverse
- [ ] 7.4 Add failure isolation: if any component's `init` returns false, run `host::shutdown()` on whatever was already up and return a non-zero status
- [ ] 7.5 Add the main-thread guard at the top of `host::init` and `host::shutdown`

## Phase 8: Documentation

- [ ] 8.1 Add a "Build and load into REAPER" section to `README.md` covering Linux, macOS, and Windows MSVC
- [ ] 8.2 Add a "Writing your first extension" section that walks through the `humanize_midi` source

## Phase 9: Unit Tests

- [ ] 9.1 Add a CTest harness in `src/host/meson.build`
- [ ] 9.2 `test_loader_scan` — create a temp dir with 3 mock manifests, verify the registry
- [ ] 9.3 `test_loader_reload` — edit a manifest on disk, call `reload`, verify the update
- [ ] 9.4 `test_manifest_parse` — feed malformed manifests and verify they are rejected

## Phase 10: Verification

- [ ] 10.1 Build on Linux with `meson setup build && ninja -C build` (deferred — needs REAPER Linux + `meson` + `lua5.4-dev`)
- [ ] 10.2 Build on macOS with the same commands (deferred — needs macOS)
- [ ] 10.3 Build on Windows with MSVC via `vcvarsall.bat amd64 && meson setup build && ninja -C build` (deferred — needs Windows)
- [ ] 10.4 Load the binary in REAPER, confirm the Extensions Manager panel appears (deferred)
- [ ] 10.5 Right-click a MIDI item, pick "Humanize MIDI", confirm notes move (deferred)
- [ ] 10.6 Click "Reload" in the panel after editing the source, confirm the change is picked up (deferred)
- [ ] 10.7 Update `verify-report.md` with pass/fail for the 25 new scenarios (3 capabilities × ~8 scenarios each)

---

**Before apply, the user must pick a chain strategy.** Recommended default: **feature-branch-chain** (a `fase1a` tracker branch holds the integration; child PRs target the previous PR's branch). This keeps the diff of each PR focused and lets us revert one without losing the rest.
