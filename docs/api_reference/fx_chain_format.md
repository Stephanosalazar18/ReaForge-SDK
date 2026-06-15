# FX Chain Format (REAPER RfxChain)

## ⚠️ CRITICAL: Do NOT write RfxChain files by hand

REAPER's `.RfxChain` format is a **proprietary binary/base64 format**, NOT XML. The `<FXCHAIN>` XML format documented in earlier versions of this reference was **incorrect** and will produce files that REAPER cannot read.

**The ONLY reliable way to create an `.RfxChain` is to let REAPER generate it** via a Lua script that:

1. Creates a temporary track
2. Adds the desired FX via `TrackFX_AddByName()`
3. Sets parameters via `TrackFX_SetParam()`
4. Calls `reaper.GetTrackFXChain()` to export the binary chain
5. Writes the result to `FXChains/ReaForge/<name>.RfxChain`
6. Cleans up the temporary track

## Chain Builder Template

Use the Lua template in `fx_chain-primitives/chain-builder-template.md` as the base. The template is a Lua script that the LLM fills in with:
- `{{FX_NAMES}}` — array of FX identifiers (VST: or JS: format)
- `{{FX_PARAMS}}` — per-FX parameter table: `{[fx_index] = {[param_id] = value}}`
- `{{CHAIN_NAME}}` — output filename (without extension)

The LLM calls `reaforge_save_lua("build_{{CHAIN_NAME}}", script, register_action=false)` to save and execute the builder script once, then the chain file is ready in `FXChains/ReaForge/`.

## FX Name Resolution

REAPER identifies FX by name string. The format depends on how the plugin is registered:

| Type | Format | Example |
|---|---|---|
| VST/VST3 | `VST: Name (Vendor)` or `VST3: Name (Vendor)` | `VST: ReaDelay (Cockos)` |
| JSFX | `JS: filename` | `JS: ReaDelay` |
| Built-in Cockos | May appear as VST, JS, or built-in depending on REAPER config | `VST: ReaEQ (Cockos)` or `JS: ReaEQ` |

**Fallback strategy**: if `TrackFX_AddByName(fx_name)` returns -1, try alternate formats. The template includes this logic.

## reaper.GetTrackFXChain() availability

- **REAPER 6.x+**: available as `reaper.GetTrackFXChain(track, fx_index)`
- **REAPER 5.x and earlier**: NOT available. Fall back to manual save: user adds FX manually, right-clicks the FX chain → "Save chain as..."
