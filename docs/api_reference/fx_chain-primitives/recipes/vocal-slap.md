# Vocal Slap FX Chain Recipe

## Ingredients

| Order | FX | Purpose |
|---|---|---|
| 1 | ReaDelay (Cockos) | Slap delay (~150ms, single tap) |
| 2 | ReaEQ (Cockos) | Low-pass 12kHz, high-pass 80Hz, gentle scoop at 2.5kHz |

## Completed Placeholders (for chain-builder-template)

```lua
local fx_names = {
    {{"VST: ReaDelay (Cockos)", "VST3: ReaDelay (Cockos)", "JS: ReaDelay"}},
    {{"VST: ReaEQ (Cockos)", "VST3: ReaEQ (Cockos)", "JS: ReaEQ"}}
}

local fx_params = {
    [0] = {  -- ReaDelay
        [0] = 0.25,   -- Length (150ms / 600ms max ≈ 0.25)
        [1] = 0.05,   -- Feedback (-45 dB → almost zero)
        [2] = 1.0,    -- Wet 100%
        [3] = 0.0,    -- Dry 0%
        [4] = 0.5,    -- Low-pass filter freq (12 kHz / 20 kHz max ≈ 0.5)
        [5] = 0.08,   -- High-pass filter freq (80 Hz / 1000 Hz max ≈ 0.08)
    },
    [1] = {  -- ReaEQ
        [0] = 0.5,    -- Band 1 type (0.5 = HPF)
        [1] = 1.0,    -- Band 1 enabled
        [2] = 0.08,   -- Band 1 freq (80 Hz)
        [3] = 0.5,    -- Band 1 Q (0.707 ≈ 0.5)
        [4] = 0.0,    -- Band 1 gain
        [5] = 1.0,    -- Band 2 type (0.5 = Bell)
        [6] = 1.0,    -- Band 2 enabled
        [7] = 0.45,   -- Band 2 freq (2.5 kHz ≈ 0.45 on log scale)
        [8] = 0.35,   -- Band 2 Q (1.0 ≈ 0.35)
        [9] = 0.375,  -- Band 2 gain (-3 dB on -12 to +12 range ≈ 0.375)
        [10] = 0.5 + 2.0/24,  -- Band 3 type (0.5 = Bell)
        [11] = 1.0,   -- Band 3 enabled
        [12] = 0.62,  -- Band 3 freq (4.5 kHz ≈ 0.62)
        [13] = 0.35,  -- Band 3 Q (1.0 ≈ 0.35)
        [14] = 0.417, -- Band 3 gain (-2 dB ≈ 0.417)
    }
}

local chain_name = "vocal_slap"
```

## Parameter Scale Reference

- **ReaDelay Length**: 0–1 maps to 0–600 ms. `0.25` ≈ 150 ms.
- **ReaDelay Feedback**: 0–1 maps to -120 to 0 dB. `0.05` ≈ -45 dB.
- **ReaDelay LP cutoff**: 0–1 maps to 20–20000 Hz (log). `0.5` ≈ 12 kHz.
- **ReaDelay HP cutoff**: 0–1 maps to 20–1000 Hz (log). `0.08` ≈ 80 Hz.
- **ReaEQ Frequency**: 0–1 maps to 20–24000 Hz (log). `0.45` ≈ 2.5 kHz.
- **ReaEQ Gain**: 0–1 maps to -12 to +12 dB. Center = 0.5. Formula: `0.5 + dB/24`.
- **ReaEQ Q**: 0–1 maps to 0.1–4.0. 0.707 ≈ `0.2`, 1.0 ≈ `0.35`.

## User Flow

1. LLM generates the script via `reaforge_save_lua("build_vocal_slap", script, register_action=true)`
2. User selects a vocal track in REAPER
3. User opens Actions → finds `build_vocal_slap` → runs it
4. ReaDelay + ReaEQ are added to the track, chain exported to `FXChains/ReaForge/vocal_slap.RfxChain`
5. The action unregisters itself — gone from the Action List

## Manual Verification

If the script fails, create the chain manually:
1. Add ReaDelay → set Length=150ms, Feedback=-45dB, Wet=100%, Dry=0%, LP=12kHz, HP=80Hz
2. Add ReaEQ → HPF 80Hz, Bell -3dB @2.5kHz Q=1.0, Bell -2dB @4.5kHz Q=1.0
3. Right-click FX chain → "Save chain as..." → `FXChains/ReaForge/vocal_slap.RfxChain`
