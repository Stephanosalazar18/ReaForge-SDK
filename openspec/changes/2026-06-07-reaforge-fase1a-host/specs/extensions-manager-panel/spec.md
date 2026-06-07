# Extensions Manager Panel Specification

## Purpose

Define the behavior of the ReaImGui dock panel that ships inside REAPER as part of the ReaForge host. The panel is the user's primary entry point to Flow 1 (see `docs/user-flows.md`): it lists loaded extensions, shows their runtime and status, and offers per-extension controls.

## Requirements

### Requirement: Dockable Panel

The host MUST register a dockable ReaImGui panel with REAPER's docking system. The panel MUST be visible by default at first launch and MAY be hidden, floated, or re-docked by the user via REAPER's standard docking UI.

#### Scenario: Panel appears in REAPER's dock list

- GIVEN the host extension is loaded by REAPER 7+
- WHEN REAPER renders the docking layout
- THEN a panel titled "ReaForge" appears in the dock list
- AND the panel is initially attached to the bottom dock

#### Scenario: User can float the panel

- GIVEN the panel is attached to a dock
- WHEN the user drags the panel title to a position outside any dock
- THEN REAPER re-parents the panel to a floating window
- AND the panel's content remains interactive

### Requirement: Extension List

The panel MUST display, for each loaded extension, at minimum: the extension's `id`, its `runtime` (lua / quickjs / jsfx), and a status indicator (`loaded` / `error` / `disabled`).

#### Scenario: Empty state is rendered when no extensions are loaded

- GIVEN the loader has found zero extensions on startup
- WHEN the panel renders
- THEN the list area shows the text "No extensions loaded"
- AND no error is shown to the user

#### Scenario: Loaded extension appears in the list

- GIVEN the loader has registered an extension `humanize_midi` (runtime `lua`)
- WHEN the panel renders
- THEN a row with `id = "humanize_midi"`, `runtime = "lua"`, `status = "loaded"` is visible
- AND the row includes a "Reload" button and a "Run" button

### Requirement: Per-Extension Controls

For each loaded extension, the panel MUST offer a "Run" button and a "Reload" button. The "Run" button MUST invoke the extension with no args. The "Reload" button MUST re-read the extension's source from disk and re-initialize it.

#### Scenario: Run button invokes the extension

- GIVEN the user clicks the "Run" button on the `humanize_midi` row
- WHEN the click event reaches the panel
- THEN the host invokes `{id: "humanize_midi", fn: "run", args: ""}` via the executor
- AND the result is reported back as a transient toast in the panel (success or error message)

#### Scenario: Reload button re-reads source

- GIVEN the user has edited `humanize_midi.lua` on disk
- WHEN the user clicks the "Reload" button on the row
- THEN the host re-loads the source from disk
- AND the status indicator updates to `loaded` (or `error` if the new source fails to parse)
- AND the previous source is discarded

### Requirement: Open Extension Folder

The panel MUST expose a way to open the host's extensions directory in the system file explorer.

#### Scenario: Open folder link reveals the directory

- GIVEN the user clicks the "Open extensions folder" link
- WHEN the click event reaches the panel
- THEN the host invokes the system handler for the extensions directory
- AND the directory opens in the platform's default file manager

### Requirement: Lifecycle Integration

The panel MUST be created during `host.init()` and destroyed during `host.shutdown()`. The panel MUST be hidden (not destroyed) when REAPER closes the docking area it is attached to, and shown again when REAPER re-opens it.

#### Scenario: Panel survives a project switch

- GIVEN the user is in project A with the panel visible
- WHEN the user switches to project B
- THEN the panel remains visible
- AND the extension list is unchanged (extensions are global, not per-project)
