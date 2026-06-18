# DSP Primitives Index

## Signal Chain Order
**utilities → filters → saturation → dynamics → delays → reverb → modulation → pitch**

## Primitive Catalog

| Target Key | Category | Lines | Description |
|---|---|---|---|
| `delays/feedback-delay` | Delays | 47 | Feedback Delay |
| `delays/modulated-delay` | Delays | 43 | Modulated Delay / Flanger Base |
| `delays/ping-pong-delay` | Delays | 46 | Ping-Pong Delay |
| `dynamics/lookahead-limiter` | Dynamics — Advanced | 45 | Lookahead Limiter |
| `dynamics/rms-compressor` | Dynamics | 46 | RMS Compressor |
| `filters/moog-ladder` | Filters — Advanced | 60 | Moog Ladder Filter |
| `filters/one-pole-lowpass` | Filters | 24 | One-Pole Lowpass |
| `filters/rbj-highpass` | Filters | 42 | RBJ Highpass |
| `filters/rbj-lowpass` | Filters | 44 | RBJ Lowpass |
| `filters/svf-chamberlin` | SVF) — Chamberlin (Filters — Advanced | 41 | State Variable Filter |
| `modulation/bitcrusher` | Lo-fi / Degradation | 43 | Bitcrusher |
| `modulation/chorus-flanger` | Modulation | 53 | Chorus / Flanger |
| `modulation/ring-modulator` | Modulation | 34 | Ring Modulator |
| `pitch/psola-pitch-shift` | Pitch | 56 | PSOLA Pitch Shift |
| `reverb/convolution-reverb` | Reverb — Advanced | 103 | Convolution Reverb |
| `reverb/fdn-reverb` | Reverb | 64 | FDN Reverb |
| `saturation/asymmetric-tanh` | Saturation | 25 | Asymmetric Tanh |
| `saturation/hard-clip-knee` | Saturation | 35 | Hard Clip with Knee |
| `saturation/tanh-soft-clip` | Saturation | 21 | Tanh Soft Clip |
| `synthesis/fm-synthesis` | Synthesis | 68 | FM Synthesis |
| `synthesis/karplus-strong` | Synthesis | 63 | Karplus-Strong String Synthesis |
| `synthesis/wavetable-oscillator` | Synthesis | 87 | Wavetable Oscillator |
| `tape/wow-flutter` | Tape Emulation | 56 | Tape Wow & Flutter |
| `utilities/dc-blocking` | Utilities | 25 | DC Blocking |
| `utilities/denormal-prevention` | Utilities | 13 | Denormal Prevention |
| `utilities/stereo-width` | Utilities | 25 | Stereo Width |

## Compatibility Matrix

| | `feedback-del` | `modulated-de` | `ping-pong-de` | `one-pole-low` | `rbj-highpass` | `rbj-lowpass` | `asymmetric-t` | `hard-clip-kn` | `tanh-soft-cl` | `dc-blocking` | `denormal-pre` | `stereo-width` | `lookahead-li` | `rms-compress` | `moog-ladder` | `svf-chamberl` | `bitcrusher` | `chorus-flang` | `ring-modulat` | `psola-pitch-` | `convolution-` | `fdn-reverb` | `fm-synthesis` | `karplus-stro` | `wavetable-os` | `wow-flutter` |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `feedback-del` | — | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | ✓✓ | △ | △ | △ | △ |
| `modulated-de` | △ | — | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | ✓✓ | △ | △ | △ | △ |
| `ping-pong-de` | ✗ | △ | — | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | ✓✓ | △ | △ | △ | △ |
| `one-pole-low` | △ | △ | △ | — | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ |
| `rbj-highpass` | △ | △ | △ | △ | — | △ | ✓✓ | ✓✓ | ✓✓ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ |
| `rbj-lowpass` | △ | △ | △ | △ | △ | — | ✓✓ | ✓✓ | ✓✓ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | ✓✓ | △ | △ | △ | △ |
| `asymmetric-t` | △ | △ | △ | △ | △ | △ | — | △ | △ | ✓ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ |
| `hard-clip-kn` | △ | △ | △ | △ | △ | △ | △ | — | △ | ✓ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ |
| `tanh-soft-cl` | △ | △ | △ | △ | △ | △ | △ | △ | — | ✓ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ |
| `dc-blocking` | △ | △ | △ | △ | △ | △ | △ | △ | △ | — | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ |
| `denormal-pre` | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | — | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ |
| `stereo-width` | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | — | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ |
| `lookahead-li` | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | — | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ |
| `rms-compress` | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | — | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ |
| `moog-ladder` | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | — | △ | △ | △ | △ | △ | △ | ✓✓ | △ | △ | △ | △ |
| `svf-chamberl` | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | — | △ | ✓✓ | ✓✓ | △ | △ | ✓✓ | △ | △ | △ | △ |
| `bitcrusher` | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | — | △ | △ | △ | △ | ✓✓ | △ | △ | △ | △ |
| `chorus-flang` | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | — | △ | △ | △ | ✓✓ | △ | △ | △ | △ |
| `ring-modulat` | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | — | △ | △ | ✓✓ | △ | △ | △ | △ |
| `psola-pitch-` | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | — | △ | ✓✓ | △ | △ | △ | △ |
| `convolution-` | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | — | ✓✓ | △ | △ | △ | △ |
| `fdn-reverb` | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | — | △ | △ | △ | △ |
| `fm-synthesis` | △ | △ | △ | △ | △ | △ | ✓✓ | ✓✓ | ✓✓ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | ✓✓ | — | △ | △ | △ |
| `karplus-stro` | △ | △ | △ | △ | △ | △ | ✓✓ | ✓✓ | ✓✓ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | ✓✓ | △ | — | △ | △ |
| `wavetable-os` | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | ✓✓ | △ | △ | — | △ |
| `wow-flutter` | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | △ | ✓✓ | △ | △ | △ | — |

**Legend:** — = self | ✓ = compatible | ✓✓ = strongly compatible (category-level) | ✗ = conflicts | △ = needs verification

## Mandatory Composition Rules

1. **DC blocking AFTER any saturation primitive with drive > 2.0**
2. **Denormal prevention ALWAYS include `denorm = 1e-25;` + `+=/-=` pattern**
3. **Category order: utilities → filters → saturation → dynamics → delays → reverb → modulation → pitch**
4. **EQ before saturation to shape which frequencies distort**
5. **Gain stage between primitives: output level ≈ input level**

---
Generated by `tools/gen_dsp_index.py`. Regenerate after adding or modifying primitives.