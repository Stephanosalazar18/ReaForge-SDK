# Tanh Soft Clip (Saturation)

## Code
```jsfx
desc:Tanh Soft Clip

slider1:2<1,10,0.1>Drive
slider2:0<-12,12,0.1>Output Gain (dB)

@init
// Denormal prevention — always include this in any JSFX with floating-point DSP.
denorm = 1e-25;

@slider
drive = slider1;
out_gain = 10^(slider2 / 20);

@sample
// Apply tanh saturation with anti-denormal guard.
spl0 = tanh(spl0 * drive) * out_gain;
spl1 = tanh(spl1 * drive) * out_gain;

// Prevent denormals from accumulating in silent sections.
spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Drive | 1–10 | 2 | Input gain before tanh. Higher = more saturation. |
| Output Gain | -12 to +12 dB | 0 | Post-saturation level. Compensate for drive attenuation. |

## Use Case
Warm tube/tape saturation, subtle mastering saturation, adding harmonics to clean signals. The tanh function is smooth and never hard-clips — it asymptotically approaches ±1. Best for signals that need "glue" without harsh distortion.

## Compatibility
- **Before**: DC blocking (if input has offset), EQ (to shape the tone before saturation)
- **After**: DC blocking (tanh can introduce small DC offset at extreme drive), gain staging
- **Requires**: `denorm = 1e-25;` in `@init` to prevent denormal stalls
- **Conflicts**: None — composable with any other primitive

## Source
Standard DSP technique. `tanh()` is available in JSFX as a built-in function. Documented in the REAPER JSFX SDK (`/websites/reaper_fm_sdk_js` on context7). License: public domain.

<!-- test: Feed a 440 Hz sine at -12 dB. With drive=2, output should show visible waveform rounding. With drive=1, output should be nearly transparent. Audio: "warm, slightly compressed tone" -->
