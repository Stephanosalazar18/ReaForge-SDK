# REJECTED — Runtime Bridge Specification

Rejected by the 2026-06-07 ReaForge pivot (`openspec/changes/2026-06-07-reaforge-agentic-mvp/`).

The original `runtime-bridge/spec.md` defined a thin C++ layer that exposed a read-only subset of the REAPER ReaScript API (`get_cursor_position`, `get_project_tempo`, `get_track_count`, `get_track_name`, `get_master_track_volume`) to all three embedded runtimes. The pivot rejects this. There are no embedded runtimes in the new architecture, so there is nothing for the bridge to expose to.

The replacement is the Read API in `extension-read-api/spec.md` plus the greenfield C++ HTTP server in `extension-write-api/spec.md`. Project state is exposed over HTTP to the Python bridge, which surfaces it to the LLM as `reaforge_get_state`. The replacement spec set is `bridge-tools-v2`, `extension-write-api`, `extension-read-api`, `refresh-protocol`, and `artifact-folder-convention` in the same change folder. `src/host/bridge/` is dead code post-pivot.
