# Vocal Slap FX Chain Recipe

## Ingredients

| Order | FX | Purpose |
|---|---|---|
| 1 | ReaDelay (Cockos) | Slap delay (~150ms, single tap, no feedback) |
| 2 | ReaEQ (Cockos) | High-pass filter to clean low end, slight presence boost |

## ReaDelay Parameters

```
Length (time):      150 ms    (param 0 → 0.25 on a 0–1 normalized scale)
Feedback:           -45 dB    (param 1 → 0.05, almost no feedback)
Wet:                100%      (param 2 → 1.0)
Dry:                0%        (param 3 → 0.0)
```

## ReaEQ Parameters

```
Band 1 (HPF):       enabled, freq=200 Hz, Q=0.707, gain=0   (param 0–3)
Band 2 (Bell):      freq=3.2 kHz, Q=1.0, gain=+2.5 dB      (param 4–6)
Band 3 (disabled):  —                                           
Band 4 (disabled):  —
```

## Completed Placeholders

```lua
local fx_names = {
    {{"VST: ReaDelay (Cockos)", "VST3: ReaDelay (Cockos)", "JS: ReaDelay"}},
    {{"VST: ReaEQ (Cockos)", "VST3: ReaEQ (Cockos)", "JS: ReaEQ"}}
}

local fx_params = {
    [0] = {  -- ReaDelay
        [0] = 0.25,   -- Length (150ms / 600ms max ≈ 0.25)
        [1] = 0.05,   -- Feedback (-45 dB → almost zero)
        [2] = 1.0,    -- Wet
        [3] = 0.0     -- Dry
    },
    [1] = {  -- ReaEQ
        [0] = 0.5,    -- Band 1 type (0.5 = HPF)
        [1] = 1.0,    -- Band 1 enabled
        [2] = 0.33,   -- Band 1 frequency (200 Hz / 600 Hz max ≈ 0.33)
        [3] = 0.5,    -- Band 1 Q (0.707 ≈ 0.5 on 0–1 scale)
        [4] = 0.0,    -- Band 1 gain
        [5] = 1.0,    -- Band 2 enabled
        [6] = 0.5,    -- Band 2 type (0.5 = Bell)
        [7] = 0.55,   -- Band 2 frequency (3.2 kHz / 5800 Hz max ≈ 0.55)
        [8] = 0.5,    -- Band 2 Q (1.0 ≈ 0.5)
        [9] = 0.625   -- Band 2 gain (+2.5 dB on -12 to +12 range ≈ 0.625)
    }
}

local chain_name = "vocal_slap"
```

## Parameter Scale Notes

- **ReaDelay Length**: 0.0–1.0 maps to 0–600 ms. `0.25` ≈ 150 ms.
- **ReaDelay Feedback**: 0.0–1.0 maps to -120 to 0 dB. `0.05` ≈ -45 dB.
- **ReaEQ Frequency**: 0.0–1.0 maps to 20 Hz–24 kHz (log scale). For 200 Hz on band 1 (HPF default max is ~600 Hz), use `0.33`. For 3.2 kHz on band 2 (bell default max is ~5800 Hz), use `0.55`.
- **ReaEQ Gain**: 0.0–1.0 maps to -12 to +12 dB. Center (0 dB) = 0.5. +2.5 dB = `0.5 + 2.5/24 = 0.604`.
- **ReaEQ Q**: 0.0–1.0 maps to 0.1–4.0. 0.707 ≈ `0.2`, 1.0 ≈ `0.3`.

## Manual Verification

If the script fails, the user can create this chain manually:
1. Add ReaDelay to a track → set Length=150ms, Feedback=-45dB, Wet=100%, Dry=0%
2. Add ReaEQ → Band 1: HPF 200 Hz, Band 2: Bell 3.2 kHz +2.5 dB
3. Right-click FX chain → "Save chain as..." → `FXChains/ReaForge/vocal_slap.RfxChain`

## Variations

| Variation | Change |
|---|---|
| Longer slap | Length → 200–300 ms (0.33–0.5) |
| Slap with feedback | Feedback → -20 dB (0.33) |
| Brighter vocal | ReaEQ Band 2 gain → +4 dB (0.67) |
| Add de-esser | Add ReaXComp (Cockos) as FX #3, set ratio to ∞:1, threshold to -25 dB, frequency to 6–8 kHz |
