# Ping-Pong Delay (Delays)

## Code
```jsfx
desc:Ping-Pong Delay

slider1:300<50,1000,1>Time (ms)
slider2:-8<-60,0,0.1>Feedback (dB)
slider3:0<-20,20,0.1>Wet Gain (dB)
slider4:0<-20,20,0.1>Dry Gain (dB)

@init
denorm = 1e-25;
buf_len = srate * 1; // 1 second mono buffer
buf = 0; memset(buf, 0, buf_len);
wpos = 0;
ping = 1; // 1 = left, 0 = right

@slider
target_dtime = slider1 / 1000 * srate;
smooth = 0.001;
target_fb = 10^(slider2 / 20);
wet_g = 10^(slider3 / 20);
dry_g = 10^(slider4 / 20);

@sample
// Read delayed sample
rpos = wpos - target_dtime; rpos < 0 ? rpos += buf_len;
frac = rpos - floor(rpos); rpos_i = floor(rpos);
rpos_next = (rpos_i + 1) % buf_len;
del = buf[rpos_i] * (1 - frac) + buf[rpos_next] * frac;

// Mono input → buffer, alternating output
mono = (spl0 + spl1) * 0.5;
fb_smooth = fb_smooth * (1 - smooth) + target_fb * smooth;
buf[wpos] = mono + del * fb_smooth;
wpos = (wpos + 1) % buf_len;

// Alternate delayed output between L and R
ping ? (
    spl0 = spl0 * dry_g + del * wet_g;
    spl1 = spl1 * dry_g;
) : (
    spl0 = spl0 * dry_g;
    spl1 = spl1 * dry_g + del * wet_g;
);
ping = !ping;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Time | 50–1000 ms | 300 | Delay time. Mono buffer. |
| Feedback | -60 to 0 dB | -8 | Decay per ping-pong pair. |
| Wet Gain | -20 to +20 dB | 0 | Wet level. |
| Dry Gain | -20 to +20 dB | 0 | Dry level. |

## Use Case
Stereo ping-pong echo. Creates width and movement. Classic for guitars, vocals, synth arpeggios. The alternating L/R output gives a bouncing spatial effect.

## Compatibility
- **Before**: Filters, saturation (to shape the echo tone)
- **After**: Reverb (to smooth the stereo image)
- **Requires**: Mono buffer (ping-pong alternates, not true stereo)
- **Conflicts**: Cascade with feedback delay for multi-tap rhythmic patterns — but watch gain staging

## Source
Standard ping-pong delay. JSFX mono buffer with alternating output. License: public domain.

<!-- test: Impulse. Time=300ms, Feedback=-8dB. Output should alternate L→R→L→R with ~8dB decay per pair. -->
