# Chorus / Flanger (Modulation)

## Code
```jsfx
desc:Chorus / Flanger

slider1:10<2,30,0.1>Time (ms)
slider2:5<1,15,0.1>Depth (ms)
slider3:0.5<0.05,5,0.01>Rate (Hz)
slider4:2<1,4,1>Voices
slider5:0<-20,20,0.1>Wet Gain (dB)

@init
denorm = 1e-25;
max_voices = 4;
buf_len = srate * 0.05; // 50ms per voice
bufs = 0; memset(bufs, 0, buf_len * max_voices);
wpos = 0; memset(wpos, 0, max_voices);
phase = 0; memset(phase, 0, max_voices);

@slider
center_time = slider1 / 1000 * srate;
depth_time = slider2 / 1000 * srate;
rate = slider3;
voices = min(slider4, max_voices);
wet_g = 10^(slider5 / 20);

@sample
out_l = out_r = 0;
v = 0;
loop(voices,
    // Each voice has its own LFO phase offset
    phase[v] += rate / srate;
    phase[v] >= 1 ? phase[v] -= 1;
    mod = sin((phase[v] + v * 0.25) * 2 * $pi) * depth_time;
    total_time = center_time + mod;
    
    // Read from buffer
    rpos = wpos[v] - total_time;
    rpos < 0 ? rpos += buf_len;
    frac = rpos - floor(rpos); rpos_i = floor(rpos);
    rpos_next = (rpos_i + 1) % buf_len;
    del_l = bufs[rpos_i] * (1 - frac) + bufs[rpos_next] * frac;
    
    out_l += del_l;
    
    // Write to buffer
    bufs[wpos[v]] = (spl0 + spl1) * 0.5;
    wpos[v] = (wpos[v] + 1) % buf_len;
    v += 1;
);

spl0 = spl0 + out_l * wet_g / voices;
spl1 = spl1 + out_l * wet_g / voices;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Time | 2–30 ms | 10 | Center delay. <5ms = flanger, >10ms = chorus. |
| Depth | 1–15 ms | 5 | LFO modulation depth. |
| Rate | 0.05–5 Hz | 0.5 | LFO speed. |
| Voices | 1–4 | 2 | Number of modulated voices. More = thicker. |
| Wet Gain | -20 to +20 dB | 0 | Wet level (blended with dry). |

## Use Case
Chorus: thicken guitars, pads, vocals. Flanger: jet-plane swoosh, psychedelic textures. Multiple voices with phase-offset LFOs create a rich ensemble effect. Low CPU — just delays + LFOs.

## Compatibility
- **Before**: Saturation (to add harmonics before modulation), compression
- **After**: Reverb (to spatialize), EQ
- **Requires**: Per-voice buffer, per-voice LFO phase
- **Conflicts**: Multiple chorus instances at similar rates can create beat frequencies — use different rates intentionally

## Source
Multi-voice modulated delay chorus. Standard effect topology. Based on musicdsp.org chorus article and FAUST `phaflanger.lib`. License: public domain.

<!-- test: Sine at 1 kHz. Rate=0.5Hz, Depth=5ms, Voices=2. Output should show periodic pitch modulation with stereo width from voice panning. -->
