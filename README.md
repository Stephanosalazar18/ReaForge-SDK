# ReaForge SDK

Open SDK + runtime for building **Extensions** for [REAPER](https://www.reaper.fm/), inspired by the [Ableton Extensions SDK](https://www.ableton.com/en/live/extensions/).

ReaForge turns REAPER into an extensible platform: third parties (humans and AI agents alike) can ship small, focused tools that integrate into REAPER's context menus, action list, and dock panels — written in Lua, JSFX, or TypeScript, all running inside a single host extension.

## Status

**Phase 0 — Spike.** No code yet. See [`openspec/changes/2026-06-07-reaforge-spike/`](openspec/changes/2026-06-07-reaforge-spike/) for the active change that validates the multi-runtime architecture.

## What this is

- A C++ REAPER extension that hosts multiple language runtimes.
- A typed SDK in TypeScript with Lua as the primary authoring language.
- A distribution channel via ReaPack and a web registry.
- A first-party **opencode-bridge** extension that exposes REaForge as tools to an AI agent.

## What this is not

- A replacement for SWS, ReaPack, ReaImGui, or any existing REAPER ecosystem tool.
- A copy of the Ableton Extensions SDK. We share the *philosophy* (one-shot extensions triggered from context menus, AI-friendly by design), not the runtime.
- A Max for Live competitor. ReaForge is a platform, not a sound-design environment.

## License

MIT. See [LICENSE](LICENSE).
