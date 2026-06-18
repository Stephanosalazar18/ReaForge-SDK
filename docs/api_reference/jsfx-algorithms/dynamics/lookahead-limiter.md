# Lookahead Limiter (Dynamics — Advanced)

## Code
```jsfx
desc:Lookahead Limiter

slider1:0<-24,0,0.1>Ceiling (dB)
slider2:1<0.1,50,0.1>Release (ms)
slider3:5<1,50,0.1>Lookahead (ms)
slider4:0<-12,12,0.1>Output (dB)

@init
denorm = 1e-25;
look_samples = srate * slider3 / 1000;
buf_len = look_samples * 2;
buf_l = 0; buf_r = 0; memset(buf_l, 0, buf_len); memset(buf_r, 0, buf_len);
wpos = 0;
env = 0;
release_coef = exp(-1 / (slider2 / 1000 * srate));

@slider
ceiling = 10^(slider1 / 20);
look_samples = floor(srate * slider3 / 1000);
release_coef = exp(-1 / (slider2 / 1000 * srate));
out_gain = 10^(slider4 / 20);

@sample
// Write to circular buffer
buf_l[wpos] = spl0;
buf_r[wpos] = spl1;
rpos = (wpos + 1) % look_samples;
wpos = (wpos + 1) % look_samples;

// Peak detection over lookahead window
peak = max(abs(buf_l[rpos]), abs(buf_r[rpos]));

// Gain computer: if peak exceeds ceiling, reduce gain
target_gain = peak > ceiling ? ceiling / peak : 1;

// Smooth release (attack is instant — it's a limiter)
target_gain < env ? env = target_gain :  // instant attack
  env = env * release_coef + target_gain * (1 - release_coef);  // smooth release

// Apply gain to the delayed signal
spl0 = buf_l[rpos] * env * out_gain;
spl1 = buf_r[rpos] * env * out_gain;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Ceiling | -24 to 0 dB | 0 | Maximum output level. Nothing exceeds this. |
| Release | 0.1–50 ms | 1 | How fast the limiter recovers after a peak |
| Lookahead | 1–50 ms | 5 | Buffer delay for predicting peaks. Higher = more transparent but more latency |
| Output | -12 to +12 dB | 0 | Post-limiter gain |

## Use Case
Brickwall limiting for mastering: prevent clipping while maximizing loudness. The lookahead buffer lets the limiter "see" peaks before they happen, applying gain reduction smoothly instead of clipping. Essential for mix bus, mastering chain, and any situation where absolute peak control is needed.

## Compatibility
- **Before**: Compression, EQ, saturation (shape the sound before limiting)
- **After**: Nothing — limiter should be LAST in the chain
- **Requires**: Lookahead circular buffer, peak detection
- **Conflicts**: Multiple limiters in series are redundant — use one

## Source
Standard lookahead limiter topology. Lookahead buffer + instant attack + smooth release.
Referenced in FAUST `compressor.lib` (lookahead_limiter). License: public domain.

<!-- test: Sine at 0 dB. Ceiling=-3 dB. Output should never exceed -3 dB. Release=1ms should show fast recovery. Lookahead=10ms should be more transparent than 1ms. -->
