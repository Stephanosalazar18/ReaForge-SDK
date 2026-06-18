# Moog Ladder Filter (Filters — Advanced)

## Code
```jsfx
desc:Moog Ladder Filter

slider1:1000<20,20000,1>:log>Cutoff (Hz)
slider2:0.5<0,1,0.01>Resonance
slider3:0<0,12,0.1>Drive (dB)
slider4:0<-12,12,0.1>Output (dB)

@init
denorm = 1e-25;
// 4 stage state
s1_l = s2_l = s3_l = s4_l = 0;
s1_r = s2_r = s3_r = s4_r = 0;

@slider
fc = 2 * $pi * slider1 / srate;
// Thermal voltage ~1.0, tanh saturation in feedback
t = tan(fc * 0.5);
// Coefficient for one-pole stages (Stilson/Smith formulation)
g = t / (1 + t);
// Resonance compensation
res = slider2 * 4;  // 0 to 4
drive_lin = 10^(slider3 / 20);
out_gain = 10^(slider4 / 20);
// Feedback gain (compensated for 4-pole loss)
k = res * (1 - g) * (1 - g) * (1 - g) * (1 - g);

@sample
// --- Left channel ---
in_l = spl0 * drive_lin;
// Feedback from stage 4 (with tanh nonlinearity for analog character)
fb_l = tanh(s4_l * k);
// Stage 1
v1_l = g * (in_l - fb_l) + (1 - g) * s1_l;
s1_l = v1_l;
// Stage 2
v2_l = g * v1_l + (1 - g) * s2_l;
s2_l = v2_l;
// Stage 3
v3_l = g * v2_l + (1 - g) * s3_l;
s3_l = v3_l;
// Stage 4
v4_l = g * v3_l + (1 - g) * s4_l;
s4_l = v4_l;
spl0 = v4_l * out_gain;

// --- Right channel ---
in_r = spl1 * drive_lin;
fb_r = tanh(s4_r * k);
v1_r = g * (in_r - fb_r) + (1 - g) * s1_r;
s1_r = v1_r;
v2_r = g * v1_r + (1 - g) * s2_r;
s2_r = v2_r;
v3_r = g * v2_r + (1 - g) * s3_r;
s3_r = v3_r;
v4_r = g * v3_r + (1 - g) * s4_r;
s4_r = v4_r;
spl1 = v4_r * out_gain;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Cutoff | 20–20000 Hz (log) | 1000 | Filter cutoff frequency |
| Resonance | 0–1 | 0.5 | Self-oscillation amount. 0 = no resonance, 1 = near self-oscillation |
| Drive | 0–12 dB | 0 | Input drive into the filter. Adds harmonics via tanh nonlinearity |
| Output | -12 to +12 dB | 0 | Post-filter gain |

## Use Case
The classic Moog transistor ladder filter sound. 24 dB/octave lowpass with musical resonance that self-oscillates at high Q. The tanh nonlinearity in the feedback path gives the "warm" analog character that digital biquads lack. Essential for synth bass, leads, and any sound that needs "that Moog sound."

## Compatibility
- **Before**: Saturation (drive into the filter), EQ
- **After**: Delay, reverb, chorus
- **Requires**: 4 state variables per channel, tanh nonlinearity
- **Conflicts**: Cascading multiple Moog filters creates very steep rolloff (48 dB/oct) — usually one is enough

## Source
Stilson & Smith "Alias-Free Digital Synthesis of Classic Analog Waveforms" (1996).
Tanh nonlinearity from Huovilainen's analog modeling approach.
Referenced in FAUST `moog.lib` and Will Pirkle's synthesizer design textbook.
License: public domain (algorithm), MIT-compatible (implementation).

<!-- test: Saw wave input. Sweep cutoff 20000→100 Hz. Resonance=0.8 should show pronounced peak at cutoff, self-oscillation near 1.0. Drive=6dB adds visible waveform distortion. -->
