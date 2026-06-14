# JSFX Cheatsheet

> Minimum vocabulary to write a working REAPER JSFX. The full language reference is REAPER's built-in JSFX help (F1 in the JSFX editor); this file is the LLM-grounded version that fits in a single context.

## File format

A `.jsfx` file is plain text. REAPER scans `<REAPER resource>/Effects/<dir>/*.jsfx` and `<REAPER resource>/Effects/ReaForge/*.jsfx` (the ReaForge convention). The filename (minus `.jsfx`) is the FX identifier in the FX browser.

## Block structure

Every JSFX has these blocks, in this order. All are optional except `desc:`.

```
desc: <one-line description shown in the FX browser>
// optional @init, @slider, @block, @sample, @gfx blocks
```

| Block | Frequency | Purpose |
|---|---|---|
| `desc:` | once | **Required.** The FX name/description. |
| `@init` | once, on load | Initialize state, set defaults, allocate buffers. |
| `@slider` | on slider change | Read `sliderN` values when the user moves a knob. |
| `@block` | per audio buffer | Process whole-buffer logic (e.g., set `spl0` once per block). |
| `@sample` | per audio sample | Per-sample DSP. **This is where most audio code lives.** |
| `@gfx` | on UI redraw | Custom UI (sliders, meters). |

## Sliders (the user-facing knobs)

Declare in the header (top of file) — these become the FX UI:

```
desc: Tape Saturation
slider1:0<0,100,1>Drive (%)     // int 0-100
slider2:1<0,1,0.01>Mix          // float 0-1
slider3:8000<200,20000,10>HP (Hz)
```

Syntax: `sliderN:default<min,max,step>Label (units)`. `slider1`-`slider64` are valid.

## Built-in variables

| Variable | Meaning |
|---|---|
| `spl0`, `spl1`, ... | Input sample for channels 0, 1, ... (set these to produce output) |
| `srate` | Sample rate in Hz (e.g., 48000) |
| `samplesblock` | Number of samples in the current audio block |
| `numchan` | Number of I/O channels (2 for stereo) |
| `slider1`-`slider64` | Slider values (read-only inside `@sample`) |
| `t` | Current time in seconds |

## Common DSP idioms

### Bypass (always do this)
```
@sample
(slider2 >= 1) ? (
  // user wants 100% wet: replace with processed signal only
  spl0 = processed_l;
  spl1 = processed_r;
);
```

### Soft saturation (tanh)
```
@sample
drive = slider1 * 0.01;
mix   = slider2;
dry_l = spl0;
dry_r = spl1;
spl0 = tanh(spl0 * (1 + drive * 10)) * (1 - mix) + spl0 * mix;
spl1 = tanh(spl1 * (1 + drive * 10)) * (1 - mix) + spl1 * mix;
```

### Simple lowpass (one-pole)
```
@init
lp_l = lp_r = 0;
@sample
cut = slider1;
a = exp(-2*$pi*cut/srate);
lp_l = spl0 * (1-a) + lp_l * a;
lp_r = spl1 * (1-a) + lp_r * a;
```

### Time-based delay
```
@init
delay_samples = srate * 0.25;  // 250ms
buffer_idx = 0;
@sample
buf_l[buffer_idx] = spl0;
buf_r[buffer_idx] = spl1;
delayed_l = buf_l[(buffer_idx - delay_samples) | 0];
buffer_idx = (buffer_idx + 1) % delay_samples;
spl0 = spl0 + delayed_l * 0.5;
```

## Debugging tips

- Use `slider_show`/`slider_showhide` to surface internal state.
- `convolve` works with FFT convolution; `pdhalf` is `pi/2`; `$pi` is `pi`.
- Compile errors appear in REAPER's Effects → "Show development messages" or the FX error log.
- If a slider doesn't update, check the slider syntax — typos here are the #1 source of bugs.

## Common pitfalls

1. **Forgetting `desc:`** — REAPER won't show the FX.
2. **Forgetting `@init`** — `buf_l[]` allocation crashes if read first.
3. **Slider values inside `@sample`** are valid; sliders values inside `@init` are NOT (they're the defaults).
4. **Block-vs-sample confusion** — `spl0 = ...` in `@block` is applied once; in `@sample` it's per sample.

## ReaForge-specific notes

- Save to `Effects/ReaForge/<name>.jsfx`.
- The agent generates the FULL file (header + all blocks). Don't truncate.
- The user must restart REAPER (or click FX Browser → "Scan for new plug-ins") to see new JSFX.
