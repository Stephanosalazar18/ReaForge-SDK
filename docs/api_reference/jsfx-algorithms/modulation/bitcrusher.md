# Bitcrusher (Lo-fi / Degradation)

## Code
```jsfx
desc:Bitcrusher

slider1:8<1,16,1>Bit Depth
slider2:44100<1000,96000,100>Sample Rate (Hz)
slider3:0<-20,20,0.1>Output Gain (dB)
slider4:100<0,100,1>Mix (%)

@init
denorm = 1e-25;
hold_l = hold_r = 0;
sample_counter = 0;

@slider
bits = slider1;
levels = 2^bits;
sample_rate_div = srate / slider2;  // how many input samples per output sample
out_gain = 10^(slider3 / 20);
mix = slider4 / 100;

@sample
// --- Sample rate reduction (hold-and-sample) ---
sample_counter += 1;
sample_counter >= sample_rate_div ? (
  sample_counter = 0;
  hold_l = spl0;
  hold_r = spl1;
);

// --- Bit depth reduction (quantization) ---
// Quantize to 'levels' steps, centered at zero
quant_l = floor(hold_l * levels / 2 + 0.5) / (levels / 2);
quant_r = floor(hold_r * levels / 2 + 0.5) / (levels / 2);

// Clamp to [-1, 1]
quant_l = max(-1, min(1, quant_l));
quant_r = max(-1, min(1, quant_r));

// Mix dry and wet
spl0 = spl0 * (1 - mix) + quant_l * out_gain * mix;
spl1 = spl1 * (1 - mix) + quant_r * out_gain * mix;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Bit Depth | 1–16 | 8 | Number of bits. Lower = more quantization noise. 16 = no degradation |
| Sample Rate | 1000–96000 Hz | 44100 | Virtual sample rate. Lower = more aliasing/hold noise |
| Output Gain | -20 to +20 dB | 0 | Post-crush level |
| Mix | 0–100% | 100% | Dry/wet blend |

## Use Case
Lo-fi degradation: retro game sounds, chip-tune aesthetics, industrial distortion, vocal电话 effect. Bit depth controls quantization noise (8-bit = NES, 4-bit = Atari). Sample rate reduction adds aliasing and "stepped" artifacts. Combine with saturation for tape-degradation effects.

## Compatibility
- **Before**: Saturation, EQ
- **After**: Reverb, delay (to add space to the degraded sound)
- **Requires**: Hold buffer for sample rate reduction
- **Conflicts**: Multiple bitcrushers in series compound the degradation — usually one is enough

## Source
Standard quantization and decimation. Bit depth reduction via `floor(x * levels) / levels`.
Sample rate reduction via hold-and-sample. License: public domain.

<!-- test: Sine at 1 kHz. Bit Depth=4 should show visible staircasing. Sample Rate=2000 should show aliasing artifacts. Mix=50% blends clean and crushed. -->
