# Karplus-Strong String Synthesis (Synthesis)

## Code
```jsfx
desc:Karplus-Strong Plucked String

slider1:60<20,200,1>MIDI Note
slider2:0.95<0.5,0.999,0.001>Decay
slider3:0.5<0,1,0.01>Pick Position
slider4:0<0,1,1{White Noise,Sawtooth}>Excitation
slider5:0<-12,12,0.1>Output (dB)

@init
denorm = 1e-25;
buf_len = srate;  // 1 second max
buf = 0; memset(buf, 0, buf_len);
wpos = 0;
delay_len = 0;
note_on = 0;
out_gain = 10^(slider5 / 20);

@slider
// Calculate delay length from MIDI note
freq = 440 * 2^((slider1 - 69) / 12);
delay_len = floor(srate / freq);
decay = slider2;
pick = slider3;
excite_mode = slider4;
out_gain = 10^(slider5 / 20);

@block
// Trigger: check for MIDI note on
midirecv(offset, msg1, msg2, msg3) ? (
  (msg1 & 0xF0) == 0x90 && msg3 > 0 ? (
    // Note on: fill buffer with excitation
    delay_len = floor(srate / (440 * 2^((msg2 - 69) / 12)));
    i = 0;
    while (i < delay_len) (
      excite_mode == 0 ? (
        // White noise excitation with pick position
        pick_pos = i / delay_len;
        buf[i] = (pick_pos < pick) ? rand(2) - 1 : 0;
      ) : (
        // Sawtooth excitation
        buf[i] = (2 * (i / delay_len) - 1) * (i / delay_len < pick ? 1 : 0);
      );
      i += 1;
    );
    wpos = 0;
    note_on = 1;
  );
);

@sample
// Read from buffer
current = buf[wpos];
// Write back: average of current and next sample (lowpass filter in feedback)
// This is the key to Karplus-Strong: the delay line + averaging = decaying harmonics
next = buf[(wpos + 1) % delay_len];
buf[wpos] = (current + next) * 0.5 * decay;
wpos = (wpos + 1) % delay_len;

spl0 = current * out_gain;
spl1 = current * out_gain;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| MIDI Note | 20–200 | 60 (C3) | Pitch of the string (also responds to MIDI input) |
| Decay | 0.5–0.999 | 0.95 | String decay rate. Higher = longer sustain |
| Pick Position | 0–1 | 0.5 | Where on the string the pick strikes. 0=bridge, 0.5=middle, 1=nut |
| Excitation | 0-1 | 0 (Noise) | White noise = plucked string, Sawtooth = bowed string |
| Output | -12 to +12 dB | 0 | Output level |

## Use Case
Physical modeling synthesis of plucked strings (guitar, harp, koto). Send MIDI notes to trigger. The delay line length determines pitch, the averaging filter in the feedback creates the natural harmonic decay. Pick position controls timbre — picking near the bridge = brighter, near the middle = warmer.

## Compatibility
- **Before**: Not applicable (this is a generator, not a processor)
- **After**: Reverb (to add room acoustics), chorus (to thicken), saturation
- **Requires**: MIDI input for triggering, 1-second buffer
- **Conflicts**: Monophonic — one note at a time. For polyphony, instantiate multiple times

## Source
Karplus & Strong "Digital Synthesis of Plucked-String and Drum Timbres"
(Computer Music Journal, 1983). License: public domain (algorithm).

<!-- test: Send MIDI note C3 (60). Should hear a plucked string sound that decays over ~2 seconds. Pick=0.1 should sound brighter (bridge picking). Pick=0.5 should sound warmer. -->
