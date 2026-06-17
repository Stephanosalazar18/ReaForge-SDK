# FX Chain Builder — Lua Script Template

Builds an FX chain on the currently selected track, exports it to
`FXChains/ReaForge/<name>.RfxChain`, then unregisters itself from the
Action List. The user runs this ONCE from Actions → it does its job and
disappears.

## How it works

1. **LLM** fills `{{FX_NAMES}}` and `{{FX_PARAMS}}` with the desired chain
2. LLM calls `reaforge_save_lua("build_{{CHAIN_NAME}}", script, register_action=true)`
3. **User** opens Actions → finds `build_{{CHAIN_NAME}}` → runs it
4. Script adds FX to the **selected track**, sets params, exports the
   chain to `FXChains/ReaForge/`, and **unregisters itself**
5. Chain file is ready. Builder action is gone.

No tracks are created or deleted — the FX are applied to whatever track
the user has selected when they run the action.

## Template

```lua
-- Auto-generated FX chain builder for "{{CHAIN_NAME}}"
-- Run ONCE from Actions. Applies FX to selected track, exports chain,
-- then unregisters itself.

local fx_names = {{FX_NAMES}}
local fx_params = {{FX_PARAMS}}
local chain_name = "{{CHAIN_NAME}}"
-- REAPER resource path + folder where this script lives
local resource_path = reaper.GetResourcePath()
local script_dir = resource_path .. "/Scripts/ReaForge"
local script_filename = "build_" .. chain_name .. ".lua"
local script_full_path = script_dir .. "/" .. script_filename

-- Helper: try multiple name formats for a plugin
local function add_fx_by_name(track, names)
    local variants = type(names) == "table" and names or {names}
    for _, v_name in ipairs(variants) do
        local idx = reaper.TrackFX_AddByName(track, v_name, false, 1)
        if idx >= 0 then
            reaper.ShowConsoleMsg("Added: " .. v_name .. " (idx=" .. idx .. ")\n")
            return idx
        end
    end
    reaper.ShowConsoleMsg("FAILED: " .. table.concat(variants, ", ") .. "\n")
    return -1
end

-- Get the selected track, or create one if nothing selected
local track = reaper.GetSelectedTrack(0, 0)
if not track then
    reaper.InsertTrackAtIndex(0, true)
    track = reaper.GetTrack(0, 0)
    reaper.SetTrackSelected(track, true)
    reaper.ShowConsoleMsg("No track selected — created a new one.\n")
end

-- Add each FX
for i, names in ipairs(fx_names) do
    local idx = add_fx_by_name(track, names)
    -- Set parameters
    if idx >= 0 then
        local params = fx_params[i - 1]  -- 0-indexed
        if params then
            for param_id, value in pairs(params) do
                reaper.TrackFX_SetParam(track, idx, param_id, value)
            end
        end
    end
end

-- Export the chain
local resource_path = reaper.GetResourcePath()
local chain_dir = resource_path .. "/FXChains/ReaForge"
reaper.RecursiveCreateDirectory(chain_dir, 0)
local chain_path = chain_dir .. "/" .. chain_name .. ".RfxChain"

local ok, chain_data = reaper.GetTrackFXChain(track, 0)
if ok and chain_data and #chain_data > 0 then
    local f = io.open(chain_path, "wb")
    if f then
        f:write(chain_data)
        f:close()
        reaper.ShowConsoleMsg("Chain saved: " .. chain_path .. "\n")
    else
        reaper.ShowConsoleMsg("FAILED to write: " .. chain_path .. "\n")
    end
else
    reaper.ShowConsoleMsg("GetTrackFXChain failed (REAPER 6.x+ required).\n")
    reaper.ShowConsoleMsg("Manual: right-click FX chain -> Save chain as... -> " .. chain_path .. "\n")
end

-- Unregister this action so it disappears from the Action List
reaper.AddRemoveReaScript(false, 0, script_full_path, true)
reaper.ShowConsoleMsg("Action unregistered: " .. script_filename .. " at " .. script_full_path .. "\n")
```

## Placeholder Guide

| Placeholder | Type | Example |
|---|---|---|
| `{{CHAIN_NAME}}` | string | `"vocal_slap"` |
| `{{FX_NAMES}}` | Lua table of strings or tables | `{{"VST: ReaDelay (Cockos)", "VST: ReaEQ (Cockos)"}}` |
| `{{FX_PARAMS}}` | Lua table: `{[fx_index] = {[param_id] = value}}` | `{[0] = {[0] = 0.25}, [1] = {[0] = 0.5}}` |

## Multi-format name resolution

Each entry in `fx_names` can be a **string** or a **table of alternates**:

```lua
-- Single format:
{"VST: ReaDelay (Cockos)"}

-- Multiple fallbacks (first match wins):
{{"VST: ReaDelay (Cockos)", "VST3: ReaDelay (Cockos)", "JS: ReaDelay"}}
```
