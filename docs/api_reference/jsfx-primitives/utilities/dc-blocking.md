# DC Blocking (Utilities)

## Code
```jsfx
desc:DC Blocking Filter

@init
denorm = 1e-25;
// One-pole highpass at ~5 Hz — removes DC without affecting audio.
// Coefficient: a = exp(-2*pi*5/srate)
a0 = exp(-2 * $pi * 5 / srate);
x1_l = x1_r = y1_l = y1_r = 0;

@sample
// DC blocking filter: y[n] = x[n] - x[n-1] + a0 * y[n-1]
// Left
y0_l = spl0 - x1_l + a0 * y1_l;
x1_l = spl0;
y1_l = y0_l;
spl0 = y0_l;

// Right
y0_r = spl1 - x1_r + a0 * y1_r;
x1_r = spl1;
y1_r = y0_r;
spl1 = y0_r;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
No sliders. This is a fixed 5 Hz highpass filter that removes DC offset without audibly affecting the signal.

## Use Case
**Mandatory after any saturation primitive with drive > 2.0.** Saturation (especially asymmetric) can introduce small DC offsets. If left uncorrected, DC accumulates through subsequent processing stages, causing clicks, meter inaccuracy, and reduced headroom. Place DC blocking immediately after any nonlinear stage.

## Compatibility
- **Before**: Nothing needed — it's self-contained
- **After**: Everything. DC blocking should be one of the first stages in any chain.
- **Requires**: State variables in `@init`
- **Conflicts**: None — completely transparent to audio

## Source
One-pole highpass DC blocker. Standard technique from Julius O. Smith's "Introduction to Digital Filters". License: public domain.

<!-- test: Feed a signal with +0.5 DC offset. Output should center around zero. No audible change to audio content. -->
