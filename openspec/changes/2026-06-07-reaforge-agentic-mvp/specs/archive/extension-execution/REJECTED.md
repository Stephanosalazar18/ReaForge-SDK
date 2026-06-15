# REJECTED — Extension Execution Specification

Rejected by the 2026-06-07 ReaForge pivot (`openspec/changes/2026-06-07-reaforge-agentic-mvp/`).

The original `extension-execution/spec.md` defined a unified invocation contract for shipping hand-authored extensions (Lua, QuickJS, JSFX) bundled under `src/host/extensions/`. The pivot rejects this pattern. The agent generates artifacts on demand — there are no "shipped" extensions to invoke. The `src/host/extensions/humanize_midi/` sample is archived.

The replacement is the Write API in `extension-write-api/spec.md`: the extension exposes HTTP endpoints that the bridge calls to persist `.jsfx`, `.lua`, and `.RfxChain` files. The host never executes user code; REAPER does, when the user drops the FX or runs the action. The replacement spec set is `bridge-tools-v2`, `extension-write-api`, `extension-read-api`, `refresh-protocol`, and `artifact-folder-convention` in the same change folder.
