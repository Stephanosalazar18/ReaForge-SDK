# Ring Modulator (Modulation)

## Code
```jsfx
desc:Ring Modulator

slider1:100<1,5000,1>Carrier Frequency (Hz)
slider2:0<-20,20,0.1>Output Gain (dB)
slider3:100<0,100,1>Dry/Wet Mix (%)

@init
denorm = 1e-25;
phase = 0;

@slider
carrier_freq = slider1;
out_gain = 10^(slider2 / 20);
mix = slider3 / 100;

@sample
// Advance carrier oscillator phase
phase += carrier_freq / srate;
phase >= 1 ? phase -= 1;

// Ring modulation: multiply input by carrier
// Carrier is a sine wave at carrier_freq
carrier = sin(phase * 2 * $pi);

// Wet signal = input * carrier (classic ring mod)
wet_l = spl0 * carrier;
wet_r = spl1 * carrier;

// Mix dry and wet
spl0 = spl0 * (1 - mix) + wet_l * out_gain * mix;
spl1 = spl1 * (1 - mix) + wet_r * out_gain * mix;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Carrier Frequency | 1–5000 Hz | 100 | Frequency of the modulation carrier. Lower = subtle, higher = metallic |
| Output Gain | -20 to +20 dB | 0 | Wet signal level |
| Dry/Wet Mix | 0–100% | 100% | 0% = dry, 100% = full ring mod |

## Use Case
Classic ring modulator effect: creates metallic, bell-like, and alien sounds by multiplying the input with a carrier oscillator. Used for vocal effects (Dalek voice), synthesizer sounds, and experimental sound design. At low carrier frequencies, creates tremolo. At high frequencies, creates inharmonic sidebands.

## Compatibility
- **Before**: Saturation (to shape the input before modulation)
- **After**: Reverb (to add space to the metallic sound), delay
- **Requires**: Phase accumulator for carrier oscillator
- **Conflicts**: None — composable with everything

## Source
Classic amplitude modulation. Ring mod = multiplication of two signals.
Standard DSP technique. License: public domain.

<!-- test: Sine at 440 Hz. Carrier=100 Hz. Output should contain 340 Hz and 540 Hz sidebands (440±100). Carrier=2000 Hz should sound metallic/bell-like. -->
