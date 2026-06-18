# DSP Design Manifesto

> **MANDATORY READING before generating ANY JSFX effect.**
> If you skip this, your effect will have redundant sliders, poor gain staging,
> and a confusing UI. Read ALL rules. They are non-negotiable.

## Rule 1: One concept, one slider

Every slider must change the sound audibly. If two controls do similar things, **merge them into a mode selector**.

**Good**: `slider1:0<0,2,1{Straight,Dotted,Triplet}>Note Mode` — one control, three behaviors.
**Bad**: `slider1:250<1,2000,1>Time (ms)` AND `slider2:0<0,2,1{Straight,Dotted,Triplet}>Mode` — redundant. If you have a mode, derive the time from BPM. If you have a time slider, let the user dial it in. **Never both.**

## Rule 2: Respect the signal chain

```
DC block → EQ → Saturation → Compression → Delay → Reverb → Modulation → Pitch
```

- DC blocking ALWAYS after any saturation with drive > 2.0
- Denormal prevention ALWAYS: `denorm = 1e-25;` in `@init` + `+=/-=` in `@sample`
- EQ before saturation to shape WHICH frequencies distort
- Compression after saturation to control the added harmonics
- Delay before reverb (reverb adds space to the repeats)
- Modulation last (chorus/flanger on the final sound)

## Rule 3: Gain stage every stage

After each processing block, output level ≈ input level. If a saturation stage adds 6 dB, add -6 dB makeup gain immediately after. Don't let the signal get louder or quieter with each stage — the user should be able to bypass the effect and hear the same volume.

**Pattern**:
```jsfx
// After saturation
spl0 *= makeup_gain;  // compensate for drive attenuation
spl1 *= makeup_gain;
```

## Rule 4: Name sliders for musicians, not programmers

| Bad (programmer) | Good (musician) |
|---|---|
| Lowpass Frequency | Warmth / Tone |
| Asymmetry Bias | Character |
| Feedback Delay Time | Space / Echoes |
| RMS Window Size | Smoothness |
| Biquad Q | Resonance |
| Dry/Wet Mix | Mix |
| Threshold (dB) | Sensitivity |

If a parameter name requires DSP knowledge to understand, rename it.

## Rule 5: Start simple, add complexity only when needed

A 4-slider effect that sounds great > a 20-slider effect that's confusing.

- Start with the minimum parameters that produce a usable sound
- Hide advanced parameters with `sliderX:-Hidden parameter name`
- If the user asks for more control, add it then
- Default values should produce a good sound with zero adjustment

## Rule 6: Always include infrastructure

Every JSFX MUST include:

```jsfx
@init
denorm = 1e-25;

@sample
// ... your DSP code ...
spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

If your effect has saturation with drive > 2.0, ALSO include DC blocking after it.
