# REJECTED — Multi-Runtime Specification

Rejected by the 2026-06-07 ReaForge pivot (`openspec/changes/2026-06-07-reaforge-agentic-mvp/`).

The original `multi-runtime/spec.md` required the C++ host to embed Lua 5.4, JSFX, and QuickJS runtimes side-by-side inside `reaper_reaforge_host.{so,dll,dylib}`. The pivot rejects this. **REAPER is the runtime**: it already runs `.lua` and `.jsfx` natively, and the ReaForge extension's only job is to write files into REAPER's standard folders. The 3-runtimes-in-1-DLL design is dead code (`src/host/runtime/` and `src/host/bridge/` are archived, not extended).

This rejection lands when the agentic MVP ships. The 3 multi-runtime requirements (Runtime Coexistence, Main-Thread Discipline, Runtime Lifecycle Isolation, Stable Throughput, REAPER 7+ Linux Target) are not portable to the new architecture — the new host has no embedded runtimes to isolate, no Lua/QuickJS state to protect, and no 3-runtime benchmark to run. The replacement spec set is `bridge-tools-v2`, `extension-write-api`, `extension-read-api`, `refresh-protocol`, and `artifact-folder-convention` in the same change folder.
