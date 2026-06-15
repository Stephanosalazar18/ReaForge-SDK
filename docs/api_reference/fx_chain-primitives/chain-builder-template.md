# FX Chain Builder — Lua Script Template

Use this template to build FX chains programmatically. The LLM replaces `{{PLACEHOLDERS}}` with the specific FX names and parameter values, then calls `reaforge_save_lua()` to save the script and optionally execute it.

## Template

```lua
-- Auto-generated FX chain builder for "{{CHAIN_NAME}}"
-- Run once: REAPER creates the .RfxChain file, then you can delete this script.

local fx_names = {{FX_NAMES}}  -- e.g.: {"VST: ReaDelay (Cockos)", "VST: ReaEQ (Cockos)"}
local fx_params = {{FX_PARAMS}}  -- e.g.: {[0] = {[0] = 0.5, [1] = 0.3}, [1] = {[0] = 1.0}}
local chain_name = "{{CHAIN_NAME}}"

-- Helper: try multiple name formats for a plugin
local function add_fx_by_name(track, names)
    for _, name in ipairs(names) do
        local idx = reaper.TrackFX_AddByName(track, name, false, 1)
        if idx >= 0 then
            reaper.ShowConsoleMsg("Added: " .. name .. "\n")
            return idx
        end
    end
    reaper.ShowConsoleMsg("FAILED to add any variant of: " .. table.concat(names, ", ") .. "\n")
    return -1
end

-- Create a temporary track
reaper.InsertTrackAtIndex(0, true)
local track = reaper.GetTrack(0, 0)

-- Add each FX
local added = {}
for i, names in ipairs(fx_names) do
    -- If names is a string, wrap it; if it's already a table of alternates, use it
    local variants = type(names) == "table" and names or {names}
    local idx = add_fx_by_name(track, variants)
    if idx >= 0 then
        table.insert(added, idx)
    end
end

-- Set parameters for each added FX
for i, idx in ipairs(added) do
    local params = fx_params[i - 1]  -- 0-indexed in Lua
    if params then
        for param_id, value in pairs(params) do
            reaper.TrackFX_SetParam(track, idx, param_id, value)
        end
    end
end

-- Export the chain using REAPER's native format
local resource_path = reaper.GetResourcePath()
local chain_path = resource_path .. "/FXChains/ReaForge/" .. chain_name .. ".RfxChain"

-- GetTrackFXChain returns (boolean, string) — the string is the binary chain
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
    reaper.ShowConsoleMsg("GetTrackFXChain failed. REAPER 6.x+ required.\n")
    reaper.ShowConsoleMsg("Manual workaround: add FX manually, right-click FX chain -> Save chain as...\n")
end

-- Clean up the temporary track
reaper.DeleteTrack(track)
```

## Placeholder Guide

| Placeholder | Type | Example |
|---|---|---|
| `{{CHAIN_NAME}}` | string | `"vocal_slap"` |
| `{{FX_NAMES}}` | Lua table of strings or tables | `{{"VST: ReaDelay (Cockos)", "VST: ReaEQ (Cockos)"}}` |
| `{{FX_PARAMS}}` | Lua table: `{[fx_index] = {[param_id] = value}}` | `{[0] = {[0] = 0.25, [1] = 0.5}}` |

**FX index is 0-based** (first FX added = index 0).  
**Param IDs**: look up each plugin's parameter list. Common Cockos params are documented in the recipes.

## Multi-format name resolution

Each entry in `fx_names` can be a **string** (single name) or a **table of strings** (alternate formats to try):

```lua
-- Single format:
{"VST: ReaDelay (Cockos)"}

-- Multiple fallbacks (first match wins):
{{"VST: ReaDelay (Cockos)", "VST3: ReaDelay (Cockos)", "JS: ReaDelay"}}
```

## Post-generation

After the script runs once:
1. The `.RfxChain` file is in `FXChains/ReaForge/<name>.RfxChain`
2. The user applies it via right-click track → "FX chain" → "Load FX chain..."
3. The builder script can be deleted (it was single-use)
4. The chain file works without the script
