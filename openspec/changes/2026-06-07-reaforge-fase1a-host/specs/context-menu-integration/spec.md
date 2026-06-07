# Context Menu Integration Specification

## Purpose

Define how ReaForge injects its extensions into REAPER's right-click context menus. The integration is the second user entry point of Flow 1 (see `docs/user-flows.md`): the user right-clicks a track or item, picks an extension from the ReaForge submenu, and the extension is invoked against that target.

## Requirements

### Requirement: Menu Injection

The host MUST inject a "ReaForge" submenu into REAPER's track context menu and item context menu. The submenu MUST be regenerated whenever the set of loaded extensions changes.

#### Scenario: Submenu appears on track right-click

- GIVEN at least one extension is loaded and applicable to tracks
- WHEN the user right-clicks a track header
- THEN a "ReaForge" entry appears in the context menu
- AND hovering it shows a submenu listing applicable extensions

#### Scenario: Submenu appears on item right-click

- GIVEN at least one extension is loaded and applicable to items
- WHEN the user right-clicks a media item (audio or MIDI)
- THEN a "ReaForge" entry appears in the context menu
- AND hovering it shows a submenu listing applicable extensions

#### Scenario: No applicable extensions hides the submenu

- GIVEN the only loaded extension is `master-effects` (target = `master`)
- WHEN the user right-clicks a track
- THEN no "ReaForge" entry appears (since `master-effects` is not applicable to a track)
- AND the context menu is otherwise unchanged

### Requirement: Extension Targeting

Each extension's manifest MUST declare a `target` value from the set `{track, item, master, midi_item}`. The host MUST only show an extension in a context menu if the menu's target type matches the extension's `target`.

#### Scenario: Track-targeted extension in track menu

- GIVEN an extension declares `target = "track"`
- WHEN the user opens the track context menu
- THEN the extension appears in the submenu

#### Scenario: Item-targeted extension not in track menu

- GIVEN an extension declares `target = "item"`
- WHEN the user opens the track context menu
- THEN the extension does NOT appear in the submenu

### Requirement: Invocation Through Click

Clicking an extension in the submenu MUST invoke the extension via the executor with the appropriate target passed as part of the invocation request. The host MUST pass enough information for the extension to identify the targeted track or item.

#### Scenario: Track-targeted extension receives the track index

- GIVEN the user right-clicks track index 3 and picks "humanize_midi" (target = `track`)
- WHEN the submenu click is processed
- THEN the host invokes `{id: "humanize_midi", fn: "run", args: "{\"track\":3}"}`
- AND the executor dispatches to the Lua runtime

#### Scenario: MIDI item extension receives the item handle

- GIVEN the user right-clicks a MIDI item and picks "humanize_midi" (target = `midi_item`)
- WHEN the submenu click is processed
- THEN the host invokes `{id: "humanize_midi", fn: "run", args: "{\"item\":\"<handle>\"}"}`
- AND the executor dispatches to the Lua runtime

### Requirement: Error Visibility

If an extension throws an error during context-menu invocation, the host MUST surface the error to the user (via REAPER's console or a toast) and MUST NOT crash REAPER.

#### Scenario: Runtime error in context menu invocation

- GIVEN the user picks an extension from the submenu
- WHEN the extension throws a runtime error
- THEN the error message is written to the REAPER console
- AND REAPER's main thread remains responsive
- AND no further menu actions are blocked

### Requirement: Menu Refresh on Extension Reload

When the user clicks "Reload" in the Extensions Manager panel, the context menu cache MUST be invalidated and the next right-click MUST show the updated set of extensions.

#### Scenario: Reloaded extension appears in the menu

- GIVEN the user has edited an extension's source on disk
- WHEN the user clicks "Reload" in the Extensions Manager
- AND then right-clicks a track
- THEN the submenu reflects the reloaded extension
- AND the previous version of the extension is no longer present
