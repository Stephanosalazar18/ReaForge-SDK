# Design: ReaForge Fase 1a — Host with ReaImGui Panel

## Technical Approach

Promote the spike's `src/spike/` directory to `src/host/`. The spike's C++ code becomes the production code path; the directory rename signals the transition. On top of that base, add four new C++ files (`panel`, `context_menu`, `extension_loader`, plus a new `host.cpp` entry point) and one vendored ReaImGui dependency. The extension ships as a single binary that REAPER loads as a native plugin; on load, it registers a dock panel and hooks the track and item context menus.

## Architecture Decisions

### Decision: ReaImGui vendor source

| Option | Tradeoff | Decision |
|---|---|---|
| Vendor `cfillion/reaimgui` (Codeberg) as a submodule at a pinned commit | Direct control over the build; matches SWS/ReaPack pattern | **Chosen** |
| Use a pre-built ReaImGui binary from ReaPack | Simpler integration, but version drift is the user's problem | Rejected |
| Skip ReaImGui, use REAPER's native dock API | We lose Dear ImGui's mature widget set; we'd write a table renderer by hand | Rejected |

### Decision: Extension manifest format

| Option | Tradeoff | Decision |
|---|---|---|
| Lua table evaluated by the Lua runtime (we already have it) | Single language to parse, fits the spike's runtime story | **Chosen** |
| JSON | More portable for non-Lua authors, but requires a JSON parser dependency | Rejected for now (we deferred JSON in the spike too) |
| TOML | Nice, but no embedded TOML in REAPER out of the box | Rejected |

A `manifest.lua` returns a table: `{ id, name, runtime, target, entry = "run" }`. The `target` is one of `track`, `item`, `master`, `midi_item`. The `entry` is the function name to call when invoked from a menu or the panel.

### Decision: Context menu integration

| Option | Tradeoff | Decision |
|---|---|---|
| `reaper_plugin.h`'s `RegisterCustomMenu` and `HookCustomMenu` (cross-OS, documented) | Stable API, supported on all three target OSes | **Chosen** |
| ReaImGui's own popup menus (rendered over the panel) | Nice looking, but does not integrate with REAPER's native context menus | Rejected |
| `g_accel_register` per extension | One accelerator per extension is more work, more keystrokes, less discoverable | Rejected |

### Decision: Panel default position

| Option | Tradeoff | Decision |
|---|---|---|
| Bottom dock, attached by default | Mirrors SWS and ReaPack behavior; the user can move it | **Chosen** |
| Floating window | Discoverable but takes screen space | Rejected |
| No default — first launch has no panel | User has to enable it manually, but it stays out of the way | Rejected |

### Decision: Extension discovery

| Option | Tradeoff | Decision |
|---|---|---|
| Filesystem scan of `<REAPER resource path>/ReaForge/extensions/` at startup and on Reload | Standard for ReaPack-style packs; user-visible path; can be inspected | **Chosen** |
| Watch a ReaPack index file | Tied to ReaPack; we want Fase 1a to work without ReaPack | Rejected |
| Bundle all extensions in the host binary | Makes Fase 1a self-contained but kills the "platform" idea | Rejected |

## Data Flow

```
REAPER loads reaper_reaforge_host.{so,dylib,dll}
  │
  ├── reaper_plugin_info() — registers version, name, callbacks
  │
  └── host.init()
        │
        ├── Executor::init()
        │     ├── LuaRuntime::init()
        │     ├── QuickJSRuntime::init()
        │     ├── JSFXRuntime::init() (stub)
        │     └── register_lua_bridge, register_quickjs_bridge
        │
        ├── ExtensionLoader::init()
        │     ├── scan <REAPER resource path>/ReaForge/extensions/
        │     ├── for each subfolder: load manifest.lua
        │     ├── register in ExtensionRegistry
        │     └── populate ContextMenuCache
        │
        ├── ContextMenu::register_hooks()
        │     ├── track context menu: "ReaForge → [applicable]"
        │     └── item context menu: "ReaForge → [applicable]"
        │
        └── Panel::create()
              ├── ReaImGui context
              ├── dock registration (bottom dock, default)
              └── initial render: list from ExtensionRegistry

User right-clicks a track
  │
  ├── REAPER fires our hook
  ├── ContextMenu::on_track_menu()
  │     ├── filter extensions by target == "track"
  │     ├── if non-empty: add "ReaForge → [list]" submenu
  │     └── else: skip
  │
  ├── User picks "humanize_midi"
  └── ContextMenu::on_click(id="humanize_midi", target=track)
        ├── Executor::invoke({id, fn=entry, args={track:N}})
        ├── LuaRuntime::eval(source)
        └── return result → toast in Panel

User opens Extensions Manager dock
  │
  ├── Panel::render()
  │     ├── for each extension: render row(id, runtime, status, [Run], [Reload])
  │     └── footer: "Open extensions folder" link
  │
  ├── User clicks Reload
  └── Panel::on_reload(id)
        ├── ExtensionLoader::reload(id)
        ├── re-evaluate manifest.lua + entry function
        ├── update ContextMenuCache
        └── trigger re-render

REAPER unloads host
  │
  └── host.shutdown() (reverse order)
        ├── Panel::destroy()
        ├── ContextMenu::unregister_hooks()
        ├── ExtensionLoader::shutdown()
        └── Executor::shutdown()
```

## File Changes

| File | Action | Description |
|---|---|---|
| `src/spike/` | Rename | Becomes `src/host/`; the spike namespace → `host` namespace |
| `src/host/host.h` | New | Public host API (`init`, `shutdown`, `invoke`) — currently in `src/spike/host.h` |
| `src/host/host.cpp` | New | REAPER entry point, ordered init/shutdown, failure isolation |
| `src/host/panel.{h,cpp}` | New | ReaImGui Extensions Manager |
| `src/host/context_menu.{h,cpp}` | New | Track + item context menu hooks |
| `src/host/extension_loader.{h,cpp}` | New | Filesystem-based extension discovery |
| `src/host/extensions/humanize_midi/manifest.lua` | New | Sample extension manifest |
| `src/host/extensions/humanize_midi/humanize_midi.lua` | New | Sample Lua source (~30 LOC) |
| `src/host/meson.build` | Rename | From `src/spike/meson.build`; adds ReaImGui dep and cross-platform target names |
| `meson.build` | New (root) | Top-level meson file that includes `src/host/meson.build` |
| `third_party/reaimgui` | New (submodule) | cfillion's ReaImGui, pinned commit |
| `README.md` | Modify | Add "Build and load into REAPER" section |
| `openspec/specs/{multi-runtime,extension-execution,runtime-bridge}/spec.md` | New (after archive) | Promote spike specs to main `openspec/specs/` |

## Interfaces / Contracts

```cpp
// host.h (promoted from spike)
namespace reaforge::host {
    int  init();           // returns 0 on success
    void shutdown();
    int  reaper_plugin_info(void* rec);  // REAPER entry
}
```

```cpp
// panel.h
namespace reaforge::host::panel {
    bool create();
    void destroy();
    void render();
    void on_reload(const std::string& extension_id);
}
```

```cpp
// context_menu.h
namespace reaforge::host::context_menu {
    bool register_hooks();
    void unregister_hooks();
    void invalidate_cache();  // called when extensions change
}
```

```cpp
// extension_loader.h
namespace reaforge::host::loader {
    struct Manifest {
        std::string id;
        std::string name;
        std::string runtime;     // "lua" | "quickjs" | "jsfx"
        std::string target;      // "track" | "item" | "master" | "midi_item"
        std::string entry;       // function name to invoke
        std::string source_path; // absolute path to source file
    };
    std::vector<Manifest> scan();
    bool reload(const std::string& id);
    std::string extensions_dir();
}
```

```lua
-- extensions/humanize_midi/manifest.lua
return {
    id      = "humanize_midi",
    name    = "Humanize MIDI",
    runtime = "lua",
    target  = "midi_item",
    entry   = "run",
}
```

```lua
-- extensions/humanize_midi/humanize_midi.lua
local function run(ctx)
    local item = ctx.item
    if not item then return { ok = false, error = "no item" } end
    local notes = reaper.MIDI_GetNotes(item)  -- via REAPER's Lua API
    for _, n in ipairs(notes) do
        n.pos = n.pos + (math.random() - 0.5) * 0.012  -- 12ms
        n.vel = math.max(1, math.min(127, n.vel + math.floor((math.random() - 0.5) * 8)))
    end
    reaper.MIDI_SetNotes(item, notes, nil, nil, "Humanize MIDI")
    return { ok = true, count = #notes }
end
return { run = run }
```

## Testing Strategy

| Layer | What | How |
|---|---|---|
| Unit | `ExtensionLoader::scan()` on a temp dir with mock manifests | Add a `test/` dir with a `scan_test.cpp` driven by a small CTest harness in meson |
| Unit | `Panel::on_reload` updates the registry | Same test harness, mock the panel render |
| Integration | `ContextMenu::register_hooks` + simulated click invokes the right extension | Deferred — needs REAPER (Fase 1a verification step) |
| Manual | Load in REAPER, click Reload, run `humanize_midi` on a real MIDI item | Documented in the new README section; the user does this on a REAPER-capable host |

The unit-test layer is the only verification that can run in this WSL2 environment. Manual + integration testing requires REAPER.

## Migration / Rollout

This is the first production-grade change. The spike is promoted; no data migration. Reverting this change is `git revert` of the merge commit; the spike's `openspec/changes/2026-06-07-reaforge-spike/` folder stays open and is unaffected.

## Open Questions

- [ ] Confirm the exact `HookCustomMenu` callback signature in `reaper_plugin.h` (we assume `intptr_t ctx, int menu, int id, int sel`). Will be validated by inspection at apply time.
- [ ] Decide whether ReaImGui context is created per panel instance or shared. ReaImGui's docs suggest shared. We will start with shared and document the assumption.
- [ ] Decide on the extensions directory fallback when `<REAPER resource path>/ReaForge/extensions/` does not exist. The loader creates it on first run (REAPER gives us the resource path via a documented API).
- [ ] The `humanize_midi` sample calls `reaper.MIDI_GetNotes` directly — this is the Lua binding, not our bridge. Confirm at apply time that the Lua runtime is given access to the REAPER Lua global namespace, OR re-route the call through our 5-function bridge (in which case we'd need to extend the bridge to expose MIDI APIs, which is out of scope for this change).
