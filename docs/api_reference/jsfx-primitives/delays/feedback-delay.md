# Feedback Delay (Delays)

## Code
```jsfx
desc:Feedback Delay

slider1:250<1,2000,1>Time (ms)
slider2:-12<-60,0,0.1>Feedback (dB)
slider3:0<-20,20,0.1>Wet Gain (dB)
slider4:0<-20,20,0.1>Dry Gain (dB)

@init
denorm = 1e-25;
// Circular buffer
buf_len = srate * 2; // 2 seconds max
buf_l = 0; buf_r = 0;
memset(buf_l, 0, buf_len);
memset(buf_r, 0, buf_len);
wpos = 0;

@slider
// Smooth parameter changes to avoid clicks
target_dtime = slider1 / 1000 * srate;  // delay in samples
smooth = 0.001;  // smoothing coefficient for feedback
target_fb = 10^(slider2 / 20);           // feedback linear
wet_g = 10^(slider3 / 20);
dry_g = 10^(slider4 / 20);

@sample
// Read from circular buffer
rpos = wpos - target_dtime;
rpos < 0 ? rpos += buf_len;
// Linear interpolation for fractional delay
frac = rpos - floor(rpos);
rpos_i = floor(rpos);
rpos_next = (rpos_i + 1) % buf_len;
del_l = buf_l[rpos_i] * (1 - frac) + buf_l[rpos_next] * frac;
del_r = buf_r[rpos_i] * (1 - frac) + buf_r[rpos_next] * frac;

// Write input + feedback to buffer
fb_smooth = fb_smooth * (1 - smooth) + target_fb * smooth;
buf_l[wpos] = spl0 + del_l * fb_smooth;
buf_r[wpos] = spl1 + del_r * fb_smooth;
wpos = (wpos + 1) % buf_len;

// Wet/dry mix
spl0 = spl0 * dry_g + del_l * wet_g;
spl1 = spl1 * dry_g + del_r * wet_g;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Time | 1–2000 ms | 250 | Delay time. Up to 2 seconds. |
| Feedback | -60 to 0 dB | -12 | Feedback level. -60 = single repeat. 0 = infinite. |
| Wet Gain | -20 to +20 dB | 0 | Wet signal level. |
| Dry Gain | -20 to +20 dB | 0 | Dry signal level. |

## Use Case
Basic echo, slap delay, rhythmic repeats. Foundation for all delay-based effects. Combine with filters in the feedback loop for tape-style degradation.

## Compatibility
- **Before**: Saturation (to distort the repeats), filters (to shape the echo tone)
- **After**: Reverb (to add space to the repeats)
- **Requires**: Circular buffer allocation in `@init` (srate * 2 samples)
- **Conflicts**: Multiple parallel delays need separate instances or a multi-tap design

## Source
Standard circular buffer delay with linear interpolation. JSFX `memset()` for buffer init. License: public domain.

<!-- test: Impulse at 0 dB. Time=500ms, Feedback=-6dB. Output should show distinct repeats decaying ~6dB each. No clicks, no DC buildup. -->
