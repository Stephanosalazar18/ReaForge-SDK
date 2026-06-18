# FM Synthesis (Synthesis)

## Code
```jsfx
desc:FM Synthesizer

slider1:0<-24,24,1>Carrier Freq (semitones from A4)
slider2:1<0.1,10,0.1>Modulator Ratio
slider3:0<0,10,0.1>Modulation Depth
slider4:0<0,1,1{Sine,Square,Saw}>Carrier Wave
slider5:0<0,1,1{Sine,Square,Saw}>Modulator Wave
slider6:0<0,1,0.001>Attack
slider7:0.3<0,1,0.001>Decay
slider8:0<-12,12,0.1>Output (dB)

@init
denorm = 1e-25;
carrier_phase = 0;
mod_phase = 0;
env = 0;
note_freq = 440;

@slider
carrier_freq = 440 * 2^(slider1 / 12);
mod_ratio = slider2;
mod_depth = slider3 * carrier_freq;  // depth in Hz
out_gain = 10^(slider6 / 20);

@block
// MIDI input
midirecv(offset, msg1, msg2, msg3) ? (
  (msg1 & 0xF0) == 0x90 && msg3 > 0 ? (
    note_freq = 440 * 2^((msg2 - 69) / 12);
    env_state = 1;  // attack
    env = 0;
  ) : (msg1 & 0xF0) == 0x80 ? (
    env_state = 2;  // release
  );
);

@sample
// Advance phases
carrier_phase += note_freq / srate;
mod_phase += (note_freq * mod_ratio) / srate;
carrier_phase >= 1 ? carrier_phase -= 1;
mod_phase >= 1 ? mod_phase -= 1;

// Modulator waveform
mod_wave = slider5 == 0 ? sin(mod_phase * 2 * $pi) :
           slider5 == 1 ? (mod_phase > 0.5 ? 1 : -1) :
           (2 * mod_phase - 1);

// Carrier with FM
freq_mod = carrier_phase + (mod_wave * mod_depth) / srate;
carrier_wave = slider4 == 0 ? sin(freq_mod * 2 * $pi) :
               slider4 == 1 ? (freq_mod > 0.5 ? 1 : -1) :
               (2 * freq_mod - 1);

// Envelope (ADSR simplified to AR)
env_state == 1 ? (
  env += (1 - env) * slider6;
  env >= 0.999 ? env_state = 0;  // sustain
) : env_state == 2 ? (
  env *= 0.999;
  env < 0.001 ? env = 0;
);

spl0 = carrier_wave * env * out_gain;
spl1 = carrier_wave * env * out_gain;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Carrier Freq | -24 to +24 semitones | 0 (A4=440Hz) | Base pitch (also responds to MIDI) |
| Modulator Ratio | 0.1–10 | 1 | Ratio of modulator to carrier. Integer ratios = harmonic, non-integer = inharmonic |
| Modulation Depth | 0–10 | 0 | FM intensity. 0 = pure sine, higher = more complex harmonics |
| Carrier Wave | Sine/Square/Saw | Sine | Carrier oscillator waveform |
| Modulator Wave | Sine/Square/Saw | Sine | Modulator oscillator waveform |
| Attack | 0–1 | 0 | Envelope attack speed |
| Decay | 0–1 | 0.3 | Envelope decay speed |
| Output | -12 to +12 dB | 0 | Output level |

## Use Case
Classic FM synthesis (Yamaha DX7 style). Creates bell, electric piano, brass, and metallic sounds. Modulator ratio controls the harmonic structure: 1:1 = warm, 2:1 = bright, 3:1 = bell-like, non-integer = inharmonic/metallic. MIDI-triggered for melodic use.

## Compatibility
- **Before**: Not applicable (generator)
- **After**: Reverb, chorus, delay, saturation
- **Requires**: Two oscillators with independent phase, envelope state
- **Conflicts**: Monophonic. For polyphony, instantiate multiple times

## Source
Chowning FM synthesis (1973). Standard FM: carrier + modulator * depth.
Referenced in FAUST `oscillator.lib` and Joep Van Lier's Yutani synth.
License: public domain (algorithm).

<!-- test: MIDI note A4 (69). ModRatio=2, Depth=5 should sound like a bell. ModRatio=1, Depth=1 should sound like a warm synth. ModRatio=3.5 should sound metallic/inharmonic. -->
