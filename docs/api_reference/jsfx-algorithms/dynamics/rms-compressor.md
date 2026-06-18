# RMS Compressor (Dynamics)

## Code
```jsfx
desc:RMS Compressor

slider1:-12<-60,0,0.1>Threshold (dB)
slider2:4<1,20,0.1>Ratio (:1)
slider3:10<0.1,100,0.1>Attack (ms)
slider4:50<10,500,1>Release (ms)
slider5:0<-12,12,0.1>Makeup Gain (dB)
slider6:0<0,1,1{Peak,RMS}>Detection

@init
denorm = 1e-25;
// Envelope state
env_l = env_r = 0;

@slider
threshold = 10^(slider1 / 20);
ratio = slider2;
// Attack/release coefficients
attack_coef = exp(-1 / (slider3 / 1000 * srate));
release_coef = exp(-1 / (slider4 / 1000 * srate));
makeup = 10^(slider5 / 20);
rms_mode = slider6;

@sample
// Detection: peak or RMS
det_l = rms_mode ? sqrt(env_l) : abs(spl0);
det_r = rms_mode ? sqrt(env_r) : abs(spl1);

// Gain computer: if above threshold, reduce
over_l = det_l > threshold ? det_l / threshold : 1;
over_r = det_r > threshold ? det_r / threshold : 1;
// Ratio: gain_reduction = 1 - (1 - 1/ratio) * (over - 1)
gr_l = over_l > 1 ? 1 / (1 + (over_l - 1) / ratio) : 1;
gr_r = over_r > 1 ? 1 / (1 + (over_r - 1) / ratio) : 1;

// Smooth gain reduction
coef_l = gr_l < env_l ? attack_coef : release_coef;
coef_r = gr_r < env_r ? attack_coef : release_coef;
env_l = env_l * coef_l + gr_l * (1 - coef_l);
env_r = env_r * coef_r + gr_r * (1 - coef_r);

spl0 *= env_l * makeup;
spl1 *= env_r * makeup;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Threshold | -60 to 0 dB | -12 | Level above which compression starts |
| Ratio | 1:1 to 20:1 | 4:1 | Compression ratio |
| Attack | 1–100 ms | 10 | How fast compression engages |
| Release | 10–500 ms | 50 | How fast compression releases |
| Makeup Gain | -12 to +12 dB | 0 | Post-compression level |
| Detection | Peak/RMS | RMS | RMS = smoother, Peak = faster |

## Use Case
Dynamic range control. Smooth out vocal levels, add punch to drums, glue a mix bus. Feed-forward topology — detection is on the input, gain reduction on the output.

## Compatibility
- **Before**: EQ (to shape what triggers compression), saturation
- **After**: Makeup gain, limiting, EQ
- **Requires**: Envelope state, attack/release smoothing
- **Conflicts**: Cascading compressors with fast attack can pump — use serial compression intentionally

## Source
Feed-forward RMS compressor. Based on FAUST `compressor.lib` and standard dynamics processor topology. License: MIT-compatible (FAUST).

<!-- test: Sine at -6 dB. Threshold=-12 dB, Ratio=4:1. Output should show ~1.5 dB gain reduction. -->
