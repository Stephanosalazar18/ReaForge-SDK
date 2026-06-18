# State Variable Filter (SVF) — Chamberlin (Filters — Advanced)

## Code
```jsfx
desc:State Variable Filter (SVF)

slider1:1000<20,20000,1>:log>Cutoff (Hz)
slider2:1<0.1,10,0.01>Resonance (Q)
slider3:0<0,3,1{Lowpass,Bandpass,Highpass,Notch}>Output Mode

@init
denorm = 1e-25;
lp_l = bp_l = hp_l = 0;
lp_r = bp_r = hp_r = 0;

@slider
f = 2 * sin($pi * slider1 / srate);  // Chamberlin frequency coefficient
// Clamp to stability range (< 2 * sin(pi * nyquist / srate))
f = min(f, 0.99);
q = 1 / slider2;  // Damping from Q

@sample
// --- Left channel ---
hp_l = spl0 - lp_l - q * bp_l;
bp_l = bp_l + f * hp_l;
lp_l = lp_l + f * bp_l;

// Select output
slider3 == 0 ? spl0 = lp_l :    // Lowpass
slider3 == 1 ? spl0 = bp_l :    // Bandpass
slider3 == 2 ? spl0 = hp_l :    // Highpass
spl0 = lp_l + hp_l;              // Notch (4th mode)

// --- Right channel ---
hp_r = spl1 - lp_r - q * bp_r;
bp_r = bp_r + f * hp_r;
lp_r = lp_r + f * bp_r;

slider3 == 0 ? spl1 = lp_r :
slider3 == 1 ? spl1 = bp_r :
slider3 == 2 ? spl1 = hp_r :
spl1 = lp_r + hp_r;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Cutoff | 20–20000 Hz (log) | 1000 | Filter center frequency |
| Resonance | 0.1–10 | 1 | Q factor. 0.707 = Butterworth. High Q = resonant peak |
| Output Mode | 0-3 | 0 (LP) | Lowpass, Bandpass, Highpass, Notch — all from one filter |

## Use Case
The most versatile filter in DSP. One instance gives you LP, BP, HP, and Notch simultaneously — just select the output mode. The SVF is numerically stable (unlike biquad at low frequencies) and efficient (2 multiplies per sample). Use for synths, EQ, and any application where you need multiple filter modes from one cutoff.

## Compatibility
- **Before**: Saturation, DC blocking
- **After**: Delay, reverb, modulation
- **Requires**: 2 state variables per channel (bp + lp)
- **Conflicts**: Chamberlin SVF is unstable above ~6 kHz at 44.1 kHz sample rate. For high frequencies, use RBJ biquad instead

## Source
Chamberlin "Musical Applications of Microprocessors" (1985).
Standard 2nd-order SVF topology. Referenced in FAUST `svfilter.lib` and
Csound `svfilter` opcode. License: public domain.

<!-- test: White noise. Mode=LP, sweep 20000→100 Hz. Should show smooth lowpass. Mode=BP should show peaked bandpass at cutoff. Q=5 should ring visibly. -->
