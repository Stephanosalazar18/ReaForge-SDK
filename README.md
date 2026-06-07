# ReaForge SDK

Open SDK + runtime for building **Extensions** for [REAPER](https://www.reaper.fm/), inspired by the [Ableton Extensions SDK](https://www.ableton.com/en/live/extensions/).

ReaForge turns REAPER into an extensible platform: third parties (humans and AI agents alike) can ship small, focused tools that integrate into REAPER's context menus, action list, and dock panels — written in Lua, JSFX, or TypeScript, all running inside a single host extension.

## Status

**Fase 1a — Host with ReaImGui panel.** Code complete, verification deferred (requires REAPER 7 Linux). The two active changes are:

- [`openspec/changes/2026-06-07-reaforge-spike/`](openspec/changes/2026-06-07-reaforge-spike/) — Phase 0 spike (FAIL, pending runtime verification)
- [`openspec/changes/2026-06-07-reaforge-fase1a-host/`](openspec/changes/2026-06-07-reaforge-fase1a-host/) — Fase 1a, the user-facing host

## What this is

- A C++ REAPER extension that hosts multiple language runtimes.
- A typed SDK in TypeScript with Lua as the primary authoring language.
- A distribution channel via ReaPack and a web registry.
- A first-party **opencode-bridge** extension (Fase 5) that exposes ReaForge as tools to an AI agent.

## How users interact with ReaForge

There are three flows, shipped in order. See [`docs/user-flows.md`](docs/user-flows.md) for the full diagrams and [`docs/cross-environment.md`](docs/cross-environment.md) for how the four actors (REAPER, extension, opencode-bridge, opencode) communicate.

| Flow | Description | Phase | Requires opencode? |
|---|---|---|---|
| **1** | Native — panel + context menu inside REAPER | Fase 1a | No |
| **2** | opencode as external CLI control plane | Fase 5 | Yes, external |
| **3** | opencode chat embedded in REAPER as a second dock | Fase 5+ | Yes, embedded |

Fase 1a (the current change) ships Flow 1. opencode integration is deferred and will not block this milestone.

## Build and load into REAPER

### Linux (Ubuntu 22.04+ or similar)

```bash
sudo apt install meson lua5.4-dev g++ liblua5.4-dev
git clone --recurse-submodules https://github.com/Stephanosalazar18/ReaForge-SDK.git
cd ReaForge-SDK
meson setup build
ninja -C build
cp build/libreaper_reaforge_host.so ~/.config/REAPER/UserPlugins/
```

Then restart REAPER. The host's panel appears in the bottom dock by default.

### macOS

```bash
brew install meson lua
git clone --recurse-submodules https://github.com/Stephanosalazar18/ReaForge-SDK.git
cd ReaForge-SDK
meson setup build
ninja -C build
cp build/libreaper_reaforge_host.dylib ~/Library/Application\ Support/REAPER/UserPlugins/
```

### Windows (MSVC)

```cmd
git clone --recurse-submodules https://github.com/Stephanosalazar18/ReaForge-SDK.git
cd ReaForge-SDK
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" amd64
meson setup build
ninja -C build
copy build\reaper_reaforge_host.dll "%APPDATA%\REAPER\UserPlugins\"
```

## Writing your first extension

Extensions live in `<REAPER resource path>/ReaForge/extensions/<id>/` and consist of a `manifest.lua` plus a source file in the declared runtime. A complete example is shipped at `src/host/extensions/humanize_midi/`:

```lua
-- manifest.lua
return {
    id      = "humanize_midi",
    name    = "Humanize MIDI",
    runtime = "lua",
    target  = "midi_item",
    entry   = "run",
}
```

```lua
-- humanize_midi.lua
local M = {}

function M.run(ctx)
    if not ctx or not ctx.item then
        return { ok = false, error = "no item in context" }
    end
    local item = ctx.item
    local ok, notes = pcall(reaper.MIDI_GetNotes, item)
    if not ok then return { ok = false, error = "MIDI_GetNotes failed" } end
    for _, n in ipairs(notes) do
        n.pos = n.pos + (math.random() - 0.5) * 0.012
        n.vel = math.max(1, math.min(127, n.vel + math.floor((math.random() - 0.5) * 8)))
    end
    pcall(reaper.MIDI_SetNotes, item, notes, nil, nil, "Humanize MIDI")
    return { ok = true, count = #notes }
end

return M
```

The extension is auto-discovered on host startup and on Reload.

## License

MIT. See [LICENSE](LICENSE).
