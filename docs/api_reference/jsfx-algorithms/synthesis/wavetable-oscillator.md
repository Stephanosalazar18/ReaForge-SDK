# Wavetable Oscillator (Synthesis)

## Code
```jsfx
desc:Wavetable Oscillator

slider1:0<-24,24,1>Pitch (semitones from A4)
slider2:0<0,7,0.001>Wave Position
slider3:0<0,1,1{Saw,Square,Sine,Pulse,Noise,Triangle,Double Saw,FM}>Wave Table
slider4:0<-12,12,0.1>Output (dB)

@init
denorm = 1e-25;
phase = 0;
wt_size = 2048;
note_freq = 440;
out_gain = 10^(slider4 / 20);

// Build wavetables in memory
function build_wt(base, type) (
  i = 0;
  while (i < wt_size) (
    pos = i / wt_size;
    type == 0 ? base[i] = 2 * pos - 1 :           // Saw
    type == 1 ? base[i] = pos < 0.5 ? 1 : -1 :     // Square
    type == 2 ? base[i] = sin(pos * 2 * $pi) :      // Sine
    type == 3 ? base[i] = pos < 0.2 ? 1 : -1 :      // Pulse (20% duty)
    type == 4 ? base[i] = rand(2) - 1 :             // Noise
    type == 5 ? base[i] = abs(2 * pos - 1) * 2 - 1 : // Triangle
    type == 6 ? base[i] = (2 * pos - 1) + (2 * ((pos + 0.5) % 1) - 1) * 0.5 : // Double saw
    base[i] = sin(pos * 2 * $pi) * sin(pos * 4 * $pi); // FM-ish
    i += 1;
  );
);

// Build 8 wavetables in sequence
wt0 = 0;       build_wt(wt0, 0);
wt1 = wt_size; build_wt(wt1, 1);
wt2 = wt_size*2; build_wt(wt2, 2);
wt3 = wt_size*3; build_wt(wt3, 3);
wt4 = wt_size*4; build_wt(wt4, 4);
wt5 = wt_size*5; build_wt(wt5, 5);
wt6 = wt_size*6; build_wt(wt6, 6);
wt7 = wt_size*7; build_wt(wt7, 7);

@slider
note_freq = 440 * 2^(slider1 / 12);
wt_pos = slider2;
wt_type = slider3;
out_gain = 10^(slider4 / 20);

@block
midirecv(offset, msg1, msg2, msg3) ? (
  (msg1 & 0xF0) == 0x90 && msg3 > 0 ? (
    note_freq = 440 * 2^((msg2 - 69) / 12);
  );
);

@sample
// Advance phase
phase_inc = note_freq / srate;
phase += phase_inc;
phase >= 1 ? phase -= 1;

// Select wavetable base address
wt_base = wt_type * wt_size;

// Read position within table (with linear interpolation)
read_pos = phase * wt_size;
frac = read_pos - floor(read_pos);
idx = floor(read_pos);
idx_next = (idx + 1) % wt_size;

// Blend between adjacent tables for morphing (wt_pos 0-7)
table_idx = floor(wt_pos);
table_frac = wt_pos - table_idx;
wt_a = table_idx * wt_size;
wt_b = ((table_idx + 1) % 8) * wt_size;

sample_a = wt_base == 0 ? 0;  // placeholder, we use direct read
// Read from selected table
val_a = wt_base[idx] * (1 - frac) + wt_base[idx_next] * frac;
// Crossfade with next table for morphing
val_b = wt_b[idx] * (1 - frac) + wt_b[idx_next] * frac;
output = val_a * (1 - table_frac) + val_b * table_frac;

spl0 = output * out_gain;
spl1 = output * out_gain;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Pitch | -24 to +24 semitones | 0 (A4) | Base pitch (also responds to MIDI) |
| Wave Position | 0–7 | 0 | Morph position between 8 wavetables |
| Wave Table | 0-7 (enum) | 0 (Saw) | Base waveform: Saw, Square, Sine, Pulse, Noise, Triangle, Double Saw, FM |
| Output | -12 to +12 dB | 0 | Output level |

## Use Case
Wavetable synthesis with 8 base waveforms and morphing between them. The Wave Position slider crossfades between adjacent tables for smooth timbral transitions. MIDI-triggered for melodic use. Use for synth bass, leads, pads, and any sound that needs evolving timbre.

## Compatibility
- **Before**: Not applicable (generator)
- **After**: Filter (Moog or SVF for classic subtractive), reverb, delay, chorus
- **Requires**: 8 × 2048 wavetable buffers in memory
- **Conflicts**: Monophonic. For polyphony, instantiate multiple

## Source
Standard wavetable synthesis with linear interpolation and table morphing.
Waveform definitions from musicdsp.org and classic synth DSP.
Referenced in Joep Van Lier's Yutani synth (wavetable support).
License: public domain.

<!-- test: MIDI A4. WaveTable=Saw, Wave Position sweep 0→7. Should hear smooth morph from saw to FM-like. Each table position should have distinct character. -->
