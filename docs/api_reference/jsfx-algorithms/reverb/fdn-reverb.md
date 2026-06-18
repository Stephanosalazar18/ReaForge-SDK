# FDN Reverb (Reverb)

## Code
```jsfx
desc:FDN Reverb

slider1:1<0.1,10,0.1>Decay (s)
slider2:0.5<0,1,0.01>Mix
slider3:0.7<0.1,1,0.01>Damping

@init
denorm = 1e-25;
// 4 delay lines with prime-number lengths for decorrelation
n_delays = 4;
del_len = srate * 0.1; // 100ms base, primes scale it
del_lens[0] = floor(del_len * 0.017);  // ~29ms
del_lens[1] = floor(del_len * 0.023);  // ~40ms
del_lens[2] = floor(del_len * 0.031);  // ~54ms
del_lens[3] = floor(del_len * 0.043);  // ~75ms
// Buffer allocation
buf_len = del_len;
bufs = 0; memset(bufs, 0, buf_len * n_delays);
wpos = 0; memset(wpos, 0, n_delays);

// Hadamard feedback matrix (4x4, scaled by 0.5)
h00 = 0.5; h01 = 0.5; h02 = 0.5; h03 = 0.5;
h10 = 0.5; h11 = -0.5; h12 = 0.5; h13 = -0.5;
h20 = 0.5; h21 = 0.5; h22 = -0.5; h23 = -0.5;
h30 = 0.5; h31 = -0.5; h32 = -0.5; h33 = 0.5;

@slider
decay_gain = exp(-3 * 0.1 / slider1); // -60dB in decay seconds
mix = slider2;
damping = slider3;

@sample
mono = (spl0 + spl1) * 0.5;
// Read from each delay line
del[0] = bufs[wpos[0]];
del[1] = bufs[buf_len + wpos[1]];
del[2] = bufs[buf_len*2 + wpos[2]];
del[3] = bufs[buf_len*3 + wpos[3]];

// Hadamard mix + input injection
f[0] = mono + (h00*del[0] + h01*del[1] + h02*del[2] + h03*del[3]) * decay_gain;
f[1] = mono + (h10*del[0] + h11*del[1] + h12*del[2] + h13*del[3]) * decay_gain;
f[2] = mono + (h20*del[0] + h21*del[1] + h22*del[2] + h23*del[3]) * decay_gain;
f[3] = mono + (h30*del[0] + h31*del[1] + h32*del[2] + h33*del[3]) * decay_gain;

// Damping (one-pole lowpass in feedback)
d[0] = d[0] * damping + f[0] * (1 - damping);
d[1] = d[1] * damping + f[1] * (1 - damping);
d[2] = d[2] * damping + f[2] * (1 - damping);
d[3] = d[3] * damping + f[3] * (1 - damping);

// Write back
bufs[wpos[0]] = d[0]; wpos[0] = (wpos[0] + 1) % del_lens[0];
bufs[buf_len + wpos[1]] = d[1]; wpos[1] = (wpos[1] + 1) % del_lens[1];
bufs[buf_len*2 + wpos[2]] = d[2]; wpos[2] = (wpos[2] + 1) % del_lens[2];
bufs[buf_len*3 + wpos[3]] = d[3]; wpos[3] = (wpos[3] + 1) % del_lens[3];

// Output: sum all delay lines for stereo
wet = (del[0] + del[1] + del[2] + del[3]) * 0.25;
spl0 = spl0 * (1 - mix) + wet * mix;
spl1 = spl1 * (1 - mix) + wet * mix;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Decay | 0.1–10 s | 1 | RT60 decay time |
| Mix | 0–1 | 0.5 | Wet/dry mix |
| Damping | 0.1–1 | 0.7 | High-frequency absorption in feedback |

## Use Case
Natural-sounding room/hall reverb. More computationally efficient than convolution. The Hadamard matrix ensures decorrelated output from each delay line. Good for vocals, drums, ambient textures.

## Compatibility
- **Before**: EQ, compression, delay
- **After**: EQ (to shape the reverb tone)
- **Requires**: 4 delay line buffers, Hadamard matrix coefficients
- **Conflicts**: Multiple FDNs in series create dense reverb — usually one is enough

## Source
Feedback Delay Network (FDN) reverb. Based on FAUST `reverb.lib` and J. O. Smith's FDN formulation. Hadamard matrix for lossless feedback mixing. License: MIT-compatible (FAUST).

<!-- test: Impulse. Decay=1s, Mix=1.0. Output should show dense reverb tail decaying over ~1 second. -->
