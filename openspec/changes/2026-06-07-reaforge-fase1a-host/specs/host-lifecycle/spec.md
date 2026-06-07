# Host Lifecycle Specification

## Purpose

Define the lifecycle of the ReaForge C++ host extension as REAPER loads, runs, and unloads it. The host is the boundary between REAPER's plugin loading model and ReaForge's internal state (executor, panel, context menu, loader). This spec guarantees the host survives the normal REAPER session events without leaking state or crashing.

## Requirements

### Requirement: Plugin Registration

The host MUST export `reaper_plugin_info` and register itself with REAPER's plugin system on load. Registration MUST include: the host's name, version, and a callback table for the REAPER events the host handles.

#### Scenario: REAPER loads the host on session start

- GIVEN `reaper_reaforge_host.{so,dylib,dll}` is present in REAPER's `UserPlugins/`
- WHEN REAPER starts a session and scans plugins
- THEN the host is loaded
- AND `reaper_plugin_info` is called once
- AND the host's name and version appear in REAPER's plugin list

### Requirement: Ordered Initialization

`host.init()` MUST initialize components in this order: executor, extension loader, context menu cache, panel. Reverse order MUST be used in `host.shutdown()`.

#### Scenario: Components initialize in dependency order

- GIVEN `host.init()` is invoked
- WHEN the call sequence runs
- THEN `Executor::init()` runs first
- AND the loader runs second
- AND the context menu cache is populated third
- AND the panel is created and registered last

#### Scenario: Components shutdown in reverse order

- GIVEN `host.shutdown()` is invoked
- WHEN the call sequence runs
- THEN the panel is destroyed first
- AND the context menu cache is cleared
- AND the loader releases its file handles
- AND `Executor::shutdown()` runs last

### Requirement: Failure Isolation

If any component fails to initialize, the host MUST abort the rest of the sequence and call `host.shutdown()` on whatever was already initialized, returning a non-zero status to REAPER. The host MUST NOT continue with a half-initialized state.

#### Scenario: Executor init failure rolls back the host

- GIVEN `Executor::init()` returns `false` (e.g., Lua state allocation failed)
- WHEN `host.init()` continues
- THEN the host calls `host.shutdown()` on the components that were already initialized
- AND `reaper_plugin_info` returns a non-zero status
- AND REAPER marks the plugin as load-failed

### Requirement: Project Switch Survival

The host MUST remain functional across project open, project close, and project tab switch. Loading a new project MUST NOT unload extensions; closing a project MUST NOT unload extensions.

#### Scenario: Closing a project preserves extensions

- GIVEN the host is loaded and three extensions are active
- WHEN the user closes the current project
- THEN the host remains loaded
- AND all three extensions remain in the Extensions Manager panel
- AND the next project opens without re-initializing the host

### Requirement: Unload Cleans Up

When REAPER unloads the host (session end or manual unload), `host.shutdown()` MUST be called. The host MUST release: the executor, all loaded extensions, the panel, the context menu registration, and any cached file handles.

#### Scenario: REAPER unload runs shutdown

- GIVEN the host is loaded
- WHEN the user unloads it via REAPER's plugin manager
- THEN `host.shutdown()` is called
- AND on return, no ReaForge dock appears in REAPER's dock list
- AND the `UserPlugins/` directory still contains the host's binary (REAPER does not delete it)

### Requirement: Main-Thread Invariant

All host code MUST run on REAPER's main thread. The host's `init`, `shutdown`, panel render, context menu callback, and extension invocation MUST all be invoked from the main thread. If a non-main-thread call is detected, the host MUST log a warning and return without performing the action.

#### Scenario: Off-thread call is rejected

- GIVEN a worker thread holds a reference to the panel
- WHEN the worker thread calls `panel.render()`
- THEN the host logs a warning
- AND the render is not performed
- AND REAPER's main thread is not interrupted
