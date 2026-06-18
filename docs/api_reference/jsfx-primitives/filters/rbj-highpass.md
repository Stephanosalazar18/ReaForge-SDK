# RBJ Highpass (Filters)

## Code
```jsfx
desc:RBJ Highpass Filter

slider1:500<20,20000,1>Frequency (Hz)
slider2:0.707<0.1,4,0.01>Q

@init
denorm = 1e-25;
b0 = b1 = b2 = a1 = a2 = 0;
x1_l = x2_l = y1_l = y2_l = 0;
x1_r = x2_r = y1_r = y2_r = 0;

@slider
// RBJ cookbook coefficients: highpass
w0 = 2 * $pi * slider1 / srate;
alpha = sin(w0) / (2 * slider2);
cos_w0 = cos(w0);

b0 = (1 + cos_w0) / 2;
b1 = -(1 + cos_w0);
b2 = (1 + cos_w0) / 2;
a0 = 1 + alpha;
a1 = -2 * cos_w0;
a2 = 1 - alpha;

b0 /= a0; b1 /= a0; b2 /= a0;
a1 /= a0; a2 /= a0;

@sample
// Left
y0_l = b0 * spl0 + b1 * x1_l + b2 * x2_l - a1 * y1_l - a2 * y2_l;
x2_l = x1_l; x1_l = spl0;
y2_l = y1_l; y1_l = y0_l;
spl0 = y0_l;

// Right
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
| Frequency | 20–20000 Hz | 500 | Cutoff frequency. Frequencies below this are attenuated. |
| Q | 0.1–4 | 0.707 | Resonance. 0.707 = Butterworth (no peak). >1 = resonant bump at cutoff. |

## Use Case
Remove low-frequency rumble, DC offset, or mud before other processing. Highpass before saturation prevents low frequencies from dominating the distortion. Essential for vocal chains and mastering.

## Compatibility
- **Before**: DC blocking (redundant — highpass already blocks DC)
- **After**: Saturation, compression, EQ
- **Requires**: Biquad state initialization
- **Conflicts**: Cascading multiple highpass filters progressively attenuates the low end — be intentional about cutoff stacking

## Source
RBJ Audio EQ Cookbook. Transposed direct form II. JSFX built-in math. License: public domain.

<!-- test: White noise input. Sweep freq from 20 to 20000 Hz. Output should show progressive attenuation below cutoff. Q=0.707 = -3dB at cutoff. -->
