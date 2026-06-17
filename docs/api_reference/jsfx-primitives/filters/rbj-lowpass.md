# RBJ Lowpass (Filters)

## Code
```jsfx
desc:RBJ Lowpass Filter

slider1:1000<20,20000,1>Frequency (Hz)
slider2:0.707<0.1,4,0.01>Q

@init
denorm = 1e-25;
// Biquad state (transposed direct form II)
b0 = b1 = b2 = a1 = a2 = 0;
x1_l = x2_l = y1_l = y2_l = 0;
x1_r = x2_r = y1_r = y2_r = 0;

@slider
// RBJ cookbook coefficients: lowpass
w0 = 2 * $pi * slider1 / srate;
alpha = sin(w0) / (2 * slider2);
cos_w0 = cos(w0);

b0 = (1 - cos_w0) / 2;
b1 = 1 - cos_w0;
b2 = (1 - cos_w0) / 2;
a0 = 1 + alpha;
a1 = -2 * cos_w0;
a2 = 1 - alpha;

// Normalize by a0
b0 /= a0; b1 /= a0; b2 /= a0;
a1 /= a0; a2 /= a0;

@sample
// Left channel
y0_l = b0 * spl0 + b1 * x1_l + b2 * x2_l - a1 * y1_l - a2 * y2_l;
x2_l = x1_l; x1_l = spl0;
y2_l = y1_l; y1_l = y0_l;
spl0 = y0_l;

// Right channel
y0_r = b0 * spl1 + b1 * x1_r + b2 * x2_r - a1 * y1_r - a2 * y2_r;
x2_r = x1_r; x1_r = spl1;
y2_r = y1_r; y1_r = y0_r;
spl1 = y0_r;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Frequency | 20–20000 Hz | 1000 | Cutoff frequency. |
| Q | 0.1–4 | 0.707 | Resonance. 0.707 = Butterworth (flat). >1 = resonant peak. |

## Use Case
General-purpose lowpass: remove high frequencies, tame harshness, shape tone before saturation. The RBJ biquad is the standard building block for EQ — use it before saturation to control which frequencies distort.

## Compatibility
- **Before**: DC blocking, gain staging
- **After**: Any other primitive (saturation, delay, reverb)
- **Requires**: Biquad state initialization in `@init`
- **Conflicts**: Do NOT cascade multiple RBJ biquads without re-initializing state — use separate instances per band

## Source
Robert Bristow-Johnson's Audio EQ Cookbook (https://www.w3.org/TR/audio-eq-cookbook/). Transposed direct form II implementation. JSFX `$pi`, `sin()`, `cos()` are built-in. License: public domain (cookbook), MIT-compatible.

<!-- test: White noise input. Q=0.707, sweep freq from 20000 to 20 Hz. Output should show progressive high-frequency roll-off. No clicks or instability. -->
