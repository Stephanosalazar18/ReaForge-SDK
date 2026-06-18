# Asymmetric Tanh (Saturation)

## Code
```jsfx
desc:Asymmetric Tanh Saturation

slider1:3<1,15,0.1>Drive
slider2:0.05<0,0.5,0.01>Bias (DC Offset)
slider3:0<-12,12,0.1>Output Gain (dB)

@init
denorm = 1e-25;

@slider
drive = slider1;
bias = slider2;
out_gain = 10^(slider3 / 20);

@sample
// Asymmetric tanh: bias shifts the saturation curve, introducing even harmonics.
// Compensate for the DC offset introduced by the bias so the output stays centered.
spl0 = tanh((spl0 + bias) * drive) - tanh(bias * drive);
spl1 = tanh((spl1 + bias) * drive) - tanh(bias * drive);

spl0 = spl0 * out_gain;
spl1 = spl1 * out_gain;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Drive | 1–15 | 3 | Input gain before tanh. |
| Bias | 0–0.5 | 0.05 | DC offset for asymmetry. 0 = symmetric. Higher = more even harmonics. |
| Output Gain | -12 to +12 dB | 0 | Post-saturation level. |

## Use Case
Tape emulation, transformer saturation, tube preamp coloration. The asymmetry adds even-order harmonics (musically "warm") that symmetric saturation lacks. Use the Bias slider to control how much "character" the saturation adds.

## Compatibility
- **Before**: EQ (boost the frequencies you want to saturate)
- **After**: DC blocking (asymmetry can leave residual DC), EQ (shape the harmonics)
- **Requires**: `denorm = 1e-25;` in `@init`
- **Conflicts**: Cascading multiple asymmetric stages can accumulate DC — insert a DC block between stages

## Source
Standard asymmetric distortion technique. Documented in musicdsp.org's saturation archive. JSFX `tanh()` is built-in. License: public domain.

<!-- test: Feed a 440 Hz sine at -12 dB. With bias=0.05, the positive half of the waveform should be more rounded than the negative half. Spectrum analyzer should show even harmonics (2nd, 4th) stronger than odd. -->
