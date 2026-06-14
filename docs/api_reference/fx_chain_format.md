# FX Chain (RfxChain) Format

> Minimum vocabulary to write a valid REAPER FX Chain (`.RfxChain`) file. RfxChain files are loaded via Track → FX → "Load chain" or as inserts in a track template.

## File format

Plain XML, with a `<FXCHAIN>` root. Saved as `<REAPER resource>/FXChains/ReaForge/<name>.RfxChain` (the ReaForge convention).

## Minimal template

```xml
<FXCHAIN
  WNDRECT="0 0 0 0"
  SHOW="0"
  LASTSEL="0"
  DOCKED="0"
>
<FX id="0" src="VST:ReaEQ (Cockos)" UINPUT="0">
  <NAME>ReaEQ</NAME>
  <PRESET>
    <plain>
    </plain>
  </PRESET>
  <PARAMBEGINS>
    <P name="B1 On" vt="0"/>
    <P name="B2 On" vt="0"/>
  </PARAMBEGINS>
</FX>
<FX id="1" src="VST:ReaDelay (Cockos)" UINPUT="0">
  <NAME>ReaDelay</NAME>
  <PARAMBEGINS>
    <P name="Wet" vt="0.5"/>
  </PARAMBEGINS>
</FX>
</FXCHAIN>
```

## Tag reference

| Tag | Where | Meaning |
|---|---|---|
| `<FXCHAIN>` | root | The chain itself. Attributes: `WNDRECT` (window position), `SHOW` (0/1), `LASTSEL`, `DOCKED`. |
| `<FX id="N" src="..." UINPUT="0">` | child of FXCHAIN | One effect. `id` is position in chain (0-indexed). `src` is the plugin identifier. |
| `<NAME>` | child of FX | Display name (must match plugin). |
| `<PRESET>` | child of FX | Optional preset block. `<plain>` means no preset. |
| `<PARAMBEGINS>` | child of FX | One `<P name="ParamName" vt="Value"/>` per param. Values are strings. |

## Plugin `src` formats

| Format | Example | Notes |
|---|---|---|
| `VST:Name` | `VST:ReaEQ (Cockos)` | VST2 plugins. |
| `VST3:Name` | `VST3:ReaEQ (Cockos)` | VST3 plugins. |
| `JS:Name` | `JS:ReaDelay` | Built-in JS effects. |
| `AU:Name` | `AU:AppleAUNames` | macOS only. |
| `CLAP:Name` | `CLAP:Plugin Name` | CLAP plugins. |
| `DX:Name` | `DX:DirectXPlugin` | Windows only. |

The string after the colon is the **plugin display name as REAPER sees it** — case-sensitive. Mismatches cause the chain to load with the missing FX shown as "?".

## Common built-in FX identifiers

These are the safe-to-use ones that ship with REAPER:

- `VST:ReaEQ (Cockos)`
- `VST:ReaDelay (Cockos)`
- `VST:ReaComp (Cockos)`
- `VST:ReaGate (Cockos)`
- `VST:ReaPitch (Cockos)`
- `VST:ReaVerb (Cockos)`
- `VST:ReaXcomp (Cockos)`
- `VST:ReaSynth (Cockos)`
- `VST:ReaSamplomatic (Cockos)`
- `VST:ReaVoice (Cockos)`
- `JS:Volume` / `JS:Pan`
- `JS:Gain` (utility)

For third-party plugins, use whatever REAPER shows in the FX browser.

## Parameter name rules

`<P name="..."/>` names are the **display labels** of the parameters, not internal IDs. They must match exactly. Common param names on built-in plugins:

- ReaEQ: `B1 On`, `B1 Type`, `B1 Freq`, `B1 Gain`, `B1 Q`, `B1 Bandwidth`
- ReaDelay: `Wet`, `Time`, `Length ms`, `Feedback`
- ReaComp: `Threshold`, `Ratio`, `Attack`, `Release`, `Makeup`

The numeric value of `vt` is **always a string** (use the format you'd see in the UI). For a -6 dB threshold: `<P name="Threshold" vt="-6"/>`.

## Bypassing an FX

Set `BYPASS="1"` on the `<FX>` tag.

## ReaForge-specific notes

- Save to `FXChains/ReaForge/<name>.RfxChain`.
- The file must be valid XML; an unclosed tag breaks the whole chain on load.
- The order of `<FX>` elements is the chain order — first one is top of chain.
- Adding a new chain does NOT require a REAPER rescan (unlike JSFX); the chain appears in the "FX: Load chain" menu immediately.
- For vocal slap chains, a common pattern is: ReaEQ (high-pass at 100Hz, gentle boost at 3kHz) → ReaDelay (200ms, 25% feedback, 30% wet).
