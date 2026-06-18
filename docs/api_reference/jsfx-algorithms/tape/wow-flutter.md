# Tape Wow & Flutter (Tape Emulation)

## Code
```jsfx
desc:Tape Wow & Flutter

slider1:2<0,10,0.1>Wow Depth (ms)
slider2:0.5<0.1,5,0.01>Wow Rate (Hz)
slider3:0.5<0,5,0.1>Flutter Depth (ms)
slider4:50<10,200,1>Flutter Rate (Hz)
slider5:0<-12,12,0.1>Output (dB)

@init
denorm = 1e-25;
buf_len = srate * 0.05;  // 50ms buffer
buf_l = 0; buf_r = 0; memset(buf_l, 0, buf_len); memset(buf_r, 0, buf_len);
wpos = 0;
wow_phase = 0;
flutter_phase = 0;

@slider
wow_depth = slider1 / 1000 * srate;
wow_rate = slider2;
flutter_depth = slider3 / 1000 * srate;
flutter_rate = slider4;
out_gain = 10^(slider5 / 20);

@sample
// Write to buffer
buf_l[wpos] = spl0;
buf_r[wpos] = spl1;

// Wow: slow LFO (0.1-5 Hz) modulating delay time
wow_phase += wow_rate / srate;
wow_phase >= 1 ? wow_phase -= 1;
wow_mod = sin(wow_phase * 2 * $pi) * wow_depth;

// Flutter: fast LFO (10-200 Hz) modulating delay time
flutter_phase += flutter_rate / srate;
flutter_phase >= 1 ? flutter_phase -= 1;
flutter_mod = sin(flutter_phase * 2 * $pi) * flutter_depth;

// Total delay modulation
total_mod = wow_mod + flutter_mod;
center_delay = srate * 0.02;  // 20ms center
read_pos = wpos - center_delay - total_mod;

// Wrap and interpolate
read_pos < 0 ? read_pos += buf_len;
frac = read_pos - floor(read_pos);
rpos_i = floor(read_pos) % buf_len;
rpos_next = (rpos_i + 1) % buf_len;

spl0 = (buf_l[rpos_i] * (1 - frac) + buf_l[rpos_next] * frac) * out_gain;
spl1 = (buf_r[rpos_i] * (1 - frac) + buf_r[rpos_next] * frac) * out_gain;

wpos = (wpos + 1) % buf_len;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Wow Depth | 0–10 ms | 2 | Slow pitch variation. The "wobbly" tape sound |
| Wow Rate | 0.1–5 Hz | 0.5 | Speed of wow. Real tape: ~0.5-2 Hz |
| Flutter Depth | 0–5 ms | 0.5 | Fast pitch variation. The "buzzing" tape sound |
| Flutter Rate | 10–200 Hz | 50 | Speed of flutter. Real tape: ~30-100 Hz |
| Output | -12 to +12 dB | 0 | Post-wow output level |

## Use Case
Tape machine emulation. Wow = slow speed variations (motor instability, tape stretch). Flutter = fast variations (tape scrape, bearing noise). Combine with tanh saturation for full tape emulation. Use on vocals, guitars, and any sound that needs "vintage" character.

## Compatibility
- **Before**: EQ, saturation (the tape saturation before the wow/flutter)
- **After**: Reverb, delay, chorus
- **Requires**: 50ms circular buffer, two LFOs with independent phase
- **Conflicts**: Cascading wow/flutter compounds the effect — usually one instance is enough

## Source
Standard modulated delay for tape wow/flutter. Wow = low-frequency LFO (0.5-2 Hz),
flutter = high-frequency LFO (30-100 Hz). Referenced in DAFx papers on tape emulation.
License: public domain.

<!-- test: Sine at 1 kHz. Wow=2ms@0.5Hz should show slow pitch drift. Flutter=0.5ms@50Hz should add a subtle buzz. Combined should sound like an old tape machine. -->
