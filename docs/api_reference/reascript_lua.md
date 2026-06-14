# ReaScript Lua Cheatsheet

> Minimum vocabulary to write a working REAPER ReaScript in Lua. The full API is REAPER's "ReaScript API" (search inside the Actions window); this is the LLM-grounded cheat sheet.

## File format

A `.lua` file is plain Lua. Save to `<REAPER resource>/Scripts/ReaForge/<name>.lua` (the ReaForge convention).

A ReaScript is just a Lua file with a `reaper.*` API. There is no required entry point; the script's top-level body runs when invoked.

## Invocation modes

| Mode | Setup | Use case |
|---|---|---|
| **ReaScript menu** | Just save the file. | Manual one-off actions. |
| **Action (registered)** | `save_lua` with `register_action=true`. The script's path is passed to `AddRemoveReaScript(true, 0, abs_path, true)` and gets a command ID. | Bound to a key, toolbar, or MIDI. |
| **`dofile()` from another script** | Load via `dofile("<path>")`. | Composability. |

The script runs in REAPER's process. **All `reaper.*` calls are not safe to call from any thread** — they expect the REAPER main thread.

## Common API surface

### Project / Tracks

| Call | What it does |
|---|---|
| `reaper.CountTracks(0)` | Track count (proj=0 means active project). |
| `reaper.GetTrack(0, i)` | Get track by index. |
| `reaper.GetTrackName(track, "")` | Track name; pass empty buf to learn length, then pass sized buf. |
| `reaper.GetMediaItemTrack(item)` | The track that owns a media item. |
| `reaper.GetTrackNumMediaItems(track)` | Items on a track. |
| `reaper.GetTrackGUID(track)` | Stable GUID for a track. |
| `reaper.InsertTrackAtIndex(idx, false)` | Create a new track. |

### Selected items / tracks

| Call | What it does |
|---|---|
| `reaper.CountSelectedMediaItems(0)` | Selected items count. |
| `reaper.GetSelectedMediaItem(0, i)` | i-th selected item. |
| `reaper.IsMediaItemSelected(item)` | Is this item selected? |
| `reaper.CountSelectedTracks(0)` | Selected tracks count. |
| `reaper.GetSelectedTrack(0, i)` | i-th selected track. |

### MIDI

| Call | What it does |
|---|---|
| `reaper.MIDI_GetNumEvents(item)` | Number of MIDI events. |
| `reaper.MIDI_GetNote(item, i)` | Get a note (returns ok, selected, muted, startppq, endppq, chan, pitch, vel). |
| `reaper.MIDI_SetNote(item, i, ...)` | Modify a note. |
| `reaper.MIDI_InsertNote(item, ...)` | Add a note. |
| `reaper.MIDI_Sort(item)` | Re-sort after edits. |

### Transport / Playback

| Call | What it does |
|---|---|
| `reaper.GetPlayState()` | 0=stopped, 1=playing, 2=paused, etc. |
| `reaper.GetCursorPosition()` | Edit cursor (seconds). |
| `reaper.SetEditCurPos(t, false, false)` | Move cursor. |

### Project metadata

| Call | What it does |
|---|---|
| `reaper.GetProjectName(0, "", 0)` | Project name (pass empty buf to learn length). |
| `reaper.GetProjectName(0, buf, #buf+1)` | After sizing. |

## Undo handling (REQUIRED for any mutation)

Any script that mutates REAPER state **must** bracket the mutation with `Undo_BeginBlock` / `Undo_EndBlock` so the user can Ctrl-Z it:

```lua
reaper.Undo_BeginBlock()
reaper.PreventUIRefresh(1)
-- ... do stuff ...
reaper.PreventUIRefresh(-1)
reaper.Undo_EndBlock("My script name", -1)
```

Without this, REAPER's undo will skip the changes and the user has to live with the result.

## Common pattern: double velocity of selected MIDI notes

```lua
reaper.Undo_BeginBlock()
reaper.PreventUIRefresh(1)

for i = 0, reaper.CountSelectedMediaItems(0) - 1 do
  local item = reaper.GetSelectedMediaItem(0, i)
  local take = reaper.GetActiveTake(item)
  if take and reaper.TakeIsMIDI(take) then
    for j = 0, reaper.MIDI_GetNumEvents(take) - 1 do
      local ok, sel, mut, s, e, ch, pit, vel = reaper.MIDI_GetNote(take, j)
      if ok and not mut then
        reaper.MIDI_SetNote(take, j, sel, mut, s, e, ch, pit, math.min(127, vel * 2))
      end
    end
    reaper.MIDI_Sort(take)
  end
end

reaper.PreventUIRefresh(-1)
reaper.Undo_EndBlock("Double velocity of selected MIDI notes", -1)
```

## Buffer sizing for `Get*Name` functions

Functions like `GetTrackName` and `GetProjectName` write into a buffer; you must size it correctly. The idiom is:

```lua
local function get_track_name(track)
  local _, name = reaper.GetTrackName(track, "")
  return name
end
```

The empty string trick tells REAPER "tell me how big"; the second call writes into the right-sized buffer.

## ReaForge-specific notes

- Save to `Scripts/ReaForge/<name>.lua`.
- The agent generates the FULL file (not a snippet).
- `register_action=true` is the default OFF (opt-in). The user toggles it per save.
- The agent does NOT need to call `reaper.defer()` — the script runs in REAPER's main thread.
