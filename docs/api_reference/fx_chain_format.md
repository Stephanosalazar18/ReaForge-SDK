# FX Chain Format (REAPER RfxChain)

## 🛑 DO NOT CALL reaforge_save_fx_chain WITH GENERATED XML

REAPER's `.RfxChain` format is a **proprietary text/binary format** that looks like:
```
BYPASS 0 0
<VST "VST: ReaEQ (Cockos)" reaeq.dll 0 "" 1919247729<...base64...> "" ...>
FXID {GUID}
WAK 0 0
```

**The XML format (`<FXCHAIN> <FX id="0" src="...">`) is NOT what REAPER reads.**
If you call `reaforge_save_fx_chain(name, xml_content)`, the file will be unreadable
by REAPER and produce "error al leer".

**The ONLY correct way to create an `.RfxChain` is to let REAPER generate it**
via the Lua chain builder template. See `fx_chain-primitives/chain-builder-template.md`.

## Correct flow

1. Call `reaforge_get_api_reference("fx_chain-primitives/chain-builder-template")` → get the Lua template
2. Fill `{{FX_NAMES}}` and `{{FX_PARAMS}}` with the desired chain
3. Call `reaforge_save_lua("build_<name>", script, register_action=true)` → saves the builder
4. User runs the action from Actions → REAPER adds FX to the track and exports the chain
5. The `.RfxChain` file appears in `FXChains/ReaForge/`

**NEVER call `reaforge_save_fx_chain(name, content)` with hand-generated or LLM-generated XML.**
That endpoint is for programmatic use only (e.g., the Lua builder template calling
`GetTrackFXChain()` programmatically).
