# One-Pole Lowpass (Filters)

## Code
```jsfx
desc:One-Pole Lowpass Filter

slider1:5000<20,20000,1>Frequency (Hz)

@init
denorm = 1e-25;
a0_l = a0_r = 0;

@slider
// One-pole lowpass: y[n] = a * x[n] + (1-a) * y[n-1]
// where a = 1 - exp(-2*pi*freq/srate)
a0 = 1 - exp(-2 * $pi * slider1 / srate);

@sample
// Left
spl0 = a0 * spl0 + (1 - a0) * spl0_prev;
spl0_prev = spl0;

// Right
spl1 = a0 * spl1 + (1 - a0) * spl1_prev;
spl1_prev = spl1;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Frequency | 20–20000 Hz | 5000 | Cutoff frequency. 6 dB/octave roll-off. |

## Use Case
Quick, cheap lowpass for taming high frequencies without the computational cost of a biquad. Use when you need a simple tone control or anti-aliasing before a nonlinear stage. Not suitable for precise filtering (use RBJ for that).

## Compatibility
- **Before**: Any primitive
- **After**: Any primitive — computationally cheap, safe to cascade
- **Requires**: `denorm = 1e-25;` in `@init`
- **Conflicts**: None — composable with everything

## Source
Standard one-pole IIR filter. The coefficient `a = 1 - exp(-2*pi*fc/fs)` is the canonical formula. Documented in Julius O. Smith's "Introduction to Digital Filters". License: public domain.

<!-- test: White noise. Set freq=1000 Hz. Output should show -3dB at 1000 Hz, -6dB at 2000 Hz (6dB/oct slope). -->
