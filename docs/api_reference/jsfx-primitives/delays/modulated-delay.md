# Modulated Delay / Flanger Base (Delays)

## Code
```jsfx
desc:Modulated Delay / Flanger Base

slider1:5<0.5,20,0.1>Time (ms)
slider2:3<0,15,0.1>Depth (ms)
slider3:0.5<0.05,5,0.01>Rate (Hz)
slider4:0<-20,20,0.1>Wet Gain (dB)

@init
denorm = 1e-25;
buf_len = srate * 0.05; // 50ms buffer
buf_l = 0; buf_r = 0; memset(buf_l, 0, buf_len); memset(buf_r, 0, buf_len);
wpos = 0; phase = 0;

@slider
center_time = slider1 / 1000 * srate;
depth_time = slider2 / 1000 * srate;
rate = slider3;
wet_g = 10^(slider4 / 20);

@sample
// LFO for delay modulation (sine)
phase += rate / srate;
phase >= 1 ? phase -= 1;
mod = sin(phase * 2 * $pi) * depth_time;
total_time = center_time + mod;

// Read from buffer (linear interpolation)
rpos = wpos - total_time; rpos < 0 ? rpos += buf_len;
frac = rpos - floor(rpos); rpos_i = floor(rpos);
rpos_next = (rpos_i + 1) % buf_len;
del_l = buf_l[rpos_i] * (1 - frac) + buf_l[rpos_next] * frac;
del_r = buf_r[rpos_i] * (1 - frac) + buf_r[rpos_next] * frac;

// Write dry signal to buffer (no feedback for flanger)
buf_l[wpos] = spl0; buf_r[wpos] = spl1;
wpos = (wpos + 1) % buf_len;

// Mix
spl0 += del_l * wet_g;
spl1 += del_r * wet_g;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Time | 0.5–20 ms | 5 | Center delay time. Lower = flanger, higher = chorus. |
| Depth | 0–15 ms | 3 | LFO modulation depth. |
| Rate | 0.05–5 Hz | 0.5 | LFO speed. |
| Wet Gain | -20 to +20 dB | 0 | Wet signal level (blended with dry). |

## Use Case
Flanger (short time, high depth), chorus (longer time, low depth), vibrato (wet only). The modulated delay is the foundation for all short-time modulation effects. Add feedback for resonant flanging.

## Compatibility
- **Before**: Saturation (to add harmonics to the modulated signal)
- **After**: Reverb (to spatialize the chorus)
- **Requires**: Small buffer (50ms), LFO phase tracking
- **Conflicts**: Multiple modulated delays at different rates can create comb filtering — use intentionally for ensemble effects

## Source
Standard modulated delay line with sinusoidal LFO. JSFX buffer + `sin()` built-in. License: public domain.

<!-- test: Sine input at 1kHz. Rate=0.5Hz, Depth=5ms. Output should show periodic pitch modulation (vibrato). -->
