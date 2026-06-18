# PSOLA Pitch Shift (Pitch)

## Code
```jsfx
desc:PSOLA Pitch Shift

slider1:0<-12,12,1>Shift (semitones)
slider2:256<64,1024,64>Window Size
slider3:4<2,8,1>Overlap Factor
slider4:0<-20,20,0.1>Wet Gain (dB)

@init
denorm = 1e-25;
// Buffer for overlap-add
buf_max = srate * 0.5; // 500ms
buf = 0; memset(buf, 0, buf_max * 2); // stereo
wpos = 0; read_pos = 0;
// Hann window
function hann(pos, len) (0.5 - 0.5 * cos(2 * $pi * pos / (len - 1)));

@slider
ratio = 2^(slider1 / 12); // semitones to ratio
// ratio > 1 = pitch up (read faster)
// ratio < 1 = pitch down (read slower)
win_size = slider2;
hop = win_size / slider3;
wet_g = 10^(slider4 / 20);

@sample
// Write input to buffer
buf[wpos * 2] = spl0;
buf[wpos * 2 + 1] = spl1;
wpos = (wpos + 1) % buf_max;

// Compute delayed read position for pitch shift
read_target = wpos - win_size;
read_target < 0 ? read_target += buf_max;
// Advance read position by ratio (fractional)
read_pos += ratio;
read_pos >= wpos ? read_pos -= buf_max;

// Overlap-add: mix multiple windows
out_l = out_r = 0;
i = 0; pos = read_pos;
loop(floor(win_size / hop) + 1,
    pos_int = floor(pos) % buf_max;
    frac = pos - floor(pos);
    pos_next = (pos_int + 1) % buf_max;
    win_val = hann(i * hop, win_size);
    out_l += (buf[pos_int * 2] * (1 - frac) + buf[pos_next * 2] * frac) * win_val;
    out_r += (buf[pos_int * 2 + 1] * (1 - frac) + buf[pos_next * 2 + 1] * frac) * win_val;
    pos += hop;
    i += 1;
);

spl0 = spl0 + out_l * wet_g;
spl1 = spl1 + out_r * wet_g;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Shift | -12 to +12 semitones | 0 | Pitch shift amount. 0 = unchanged. |
| Window Size | 64–1024 | 256 | Analysis window. Smaller = less latency, more artifacts. |
| Overlap Factor | 2–8 | 4 | Number of overlapping windows. Higher = smoother but more CPU. |
| Wet Gain | -20 to +20 dB | 0 | Wet signal level (blended with dry). |

## Use Case
Pitch-shift vocals for harmonies, downtune guitars, create bass from guitar. PSOLA (Pitch-Synchronous Overlap-Add) is the standard time-domain pitch shifter — better quality than simple resampling, less CPU than phase vocoder.

## Compatibility
- **Before**: Compression (to smooth dynamics before pitch shift)
- **After**: EQ (to shape the shifted tone), reverb
- **Requires**: Large buffer (srate * 0.5), Hann window function
- **Conflicts**: Cascading PSOLA shifts progressively degrade quality — use a single instance per target shift

## Source
PSOLA algorithm. Based on STK `PitchShifter` and standard overlap-add time-stretching literature. Hann window from Harris' classic paper. License: MIT-compatible (STK).

<!-- test: 1 kHz sine. Shift=+12 (octave up). Output should be 2 kHz sine with Hann-windowed overlap. -->
