# JSFX Reference (EEL2)

> Complete syntax reference for REAPER's JSFX audio effect language.
> Source: https://www.reaper.fm/sdk/js/js.php — curated for LLM grounding.

## ⚠️ CRITICAL: Scientific notation

JSFX uses C-style scientific notation for numeric literals:

```
1e-25    → 0.00000000000000000000000001  (NOT "1 times e minus 25")
1e3      → 1000.0
2.5e-4   → 0.00025
```

**The `e` in `1e-25` is part of the number literal, NOT the mathematical constant e.**
Do NOT insert spaces, markers, comments, or any characters between the digits and `e`.
`denorm = 1e-25;` is a single assignment. Write it EXACTLY as shown.
**NEVER write `1 <!> e-25` or `1 * e^(-25)` or `1 * exp(-25)`.**

## File format

A `.jsfx` file is plain text. REAPER scans `<REAPER resource>/Effects/<dir>/*.jsfx`.

```
desc:Effect Name
//tags: category keywords
//author: Name
slider1:0<0,100,1>Parameter Name (units)
```

## Block structure

| Block | When | Purpose |
|---|---|---|
| `desc:` | once | **Required.** FX name in browser. |
| `@init` | on load | Initialize state, buffers, defaults. |
| `@slider` | on slider change | Read `sliderN` values, compute coefficients. |
| `@block` | per audio buffer | Whole-buffer logic (envelopes, LFO phase). |
| `@sample` | per audio sample | Per-sample DSP. **Most audio code lives here.** |
| `@gfx` | on UI redraw | Custom UI drawing. |
| `@serialize` | on save/load | Persist internal state. |

## Sliders

```
slider1:5<0,10,1>Volume                    // int, 0-10, step 1
slider2:0.5<0,1,0.01>Mix                   // float, 0-1, step 0.01
slider3:1000<20,20000,1>:log>Frequency     // log scale
slider4:0<0,2,1{Off,Peak,RMS}>Mode         // enum (dropdown)
slider5:120<40,300,1>-Hidden BPM           // hidden from UI
slider6:/path:default>File selector        // file picker
```

**Slider options:**
- `:log>` — logarithmic response curve
- `:log=X>` — log with midpoint X
- `:sqr>` — quadratic response
- `{A,B,C}` — enum dropdown (starts at 0)
- `-Hidden` — hidden from UI but still automatable

**Read sliders in `@slider` and `@sample`. In `@init`, sliders have their DEFAULT values.**

## Built-in variables

| Variable | Type | Meaning |
|---|---|---|
| `spl0`, `spl1`, ... `spl63` | float | Audio samples (L=0, R=1). Read input, write output. |
| `spl(n)` | float | Access sample by index (slower than `splN`). |
| `srate` | float | Sample rate in Hz (44100, 48000, etc.). |
| `samplesblock` | int | Samples per audio block. |
| `numchan` | int | Number of I/O channels (2 = stereo). |
| `slider1`-`slider64` | varies | Current slider values. |
| `t` | float | Current time in seconds. |
| `tempo` | float | Current BPM. |
| `play_state` | float | 0=stopped, 1=playing, 2=recording. |
| `beat_position` | float | Current position in beats. |
| `$pi` | float | π (3.14159...). |
| `$e` | float | e (2.71828...). **Use this for Euler's number, NOT in scientific notation.** |

## Math functions

| Function | Description |
|---|---|
| `sin(x)`, `cos(x)`, `tan(x)` | Trigonometric (radians). |
| `asin(x)`, `acos(x)`, `atan(x)` | Inverse trig. |
| `atan2(y, x)` | 2-argument arctangent. |
| `tanh(x)` | Hyperbolic tangent. **Key for saturation.** |
| `exp(x)` | e^x (Euler's number to power x). |
| `log(x)` | Natural log (base e). |
| `log10(x)` | Base-10 log. |
| `sqrt(x)` | Square root. |
| `sqr(x)` | x² (square). |
| `abs(x)` | Absolute value. |
| `sign(x)` | Sign: -1, 0, or 1. |
| `min(x,y)` | Minimum. |
| `max(x,y)` | Maximum. |
| `floor(x)` | Round down. |
| `ceil(x)` | Round up. |
| `pow(x,y)` | x^y. |
| `invsqrt(x)` | Fast 1/√x approximation. |
| `rand(x)` | Random 0 to x. |

## Memory functions

| Function | Description |
|---|---|
| `memset(offset, value, length)` | Fill memory. |
| `memcpy(dest, src, length)` | Copy memory. |
| `strcpy(dest, src)` | Copy string. |
| `strlen(str)` | String length. |
| `strcmp(a, b)` | Compare strings. |
| `sprintf(dest, fmt, ...)` | Format string. |

## User-defined functions

```
function mySine(x)
(
  x - (x^3)/(3*2) + (x^5)/(5*4*3*2);
);

// Call:
y = mySine($pi * 18000 / srate);
```

- Functions can have up to 40 parameters
- Parameters are private (don't affect globals)
- Functions CANNOT be recursive
- Functions defined in `@init` are accessible from all sections

## Control flow

```
// Ternary
y = condition ? value_if_true : value_if_false;

// If/else
condition ? (
  // true block
) : (
  // false block
);

// While loop
i = 0;
while (i < 10) (
  buffer[i] = sin(i * 0.1);
  i += 1;
);

// Counted loop
loop(100,
  sum += rand(1);
);
```

## FFT / spectral

| Function | Description |
|---|---|
| `fft(buffer, size)` | Forward FFT (in-place). |
| `ifft(buffer, size)` | Inverse FFT (in-place). |
| `fft_permute(buffer, size)` | Reorder bins to natural order. |
| `fft_ipermute(buffer, size)` | Inverse permute. |
| `convolve(dest, src, size)` | FFT convolution. |
| `pdhalf(len, dir)` | Phase dispersion half. |

## Graphics (@gfx)

| Variable | Meaning |
|---|---|
| `gfx_w`, `gfx_h` | Window width/height. |
| `gfx_x`, `gfx_y` | Current draw position. |
| `gfx_r`, `gfx_g`, `gfx_b`, `gfx_a` | Draw color (0-1). |
| `gfx_clear` | Background color (RGB packed, or -1 for no clear). |
| `gfx_texth` | Text line height (read-only). |

| Function | Description |
|---|---|
| `gfx_setfont(index, "name", size)` | Set font. |
| `gfx_drawstr("text")` | Draw string at gfx_x, gfx_y. |
| `gfx_drawnumber(n, dec)` | Draw number. |
| `gfx_rect(x, y, w, h)` | Fill rectangle. |
| `gfx_line(x1, y1, x2, y2)` | Draw line. |
| `gfx_circle(x, y, r, fill)` | Draw circle. |
| `gfx_blit(image, scale, x, y)` | Blit image. |
| `gfx_showmenu("item1|item2|item3")` | Popup menu, returns index. |

## MIDI (in @block)

| Variable | Meaning |
|---|---|
| `midirecv(offset, msg1, msg2, msg3)` | Receive MIDI event. |
| `midisend(offset, msg1, msg2, msg3)` | Send MIDI event. |

MIDI message types: `0x90` = note on, `0x80` = note off, `0xB0` = CC, `0xE0` = pitch bend.

## Denormal prevention (MANDATORY in every JSFX)

```jsfx
@init
denorm = 1e-25;    // Scientific notation: 0.00000000000000000000000001
                    // Do NOT write "1 <!> e-25" or "1 * e^(-25)"
                    // "1e-25" is a SINGLE numeric literal

@sample
// ... your DSP code ...
spl0 += denorm; spl0 -= denorm;   // flush denormals
spl1 += denorm; spl1 -= denorm;
```

## Common DSP idioms

### Soft saturation (tanh)
```jsfx
@sample
spl0 = tanh(spl0 * drive);
spl1 = tanh(spl1 * drive);
```

### One-pole lowpass
```jsfx
@init
lp_l = lp_r = 0;
@slider
a = exp(-2*$pi*slider1/srate);
@sample
lp_l = spl0 * (1-a) + lp_l * a;
lp_r = spl1 * (1-a) + lp_r * a;
spl0 = lp_l; spl1 = lp_r;
```

### Delay line
```jsfx
@init
buf_len = srate * 2;  // 2 seconds
wpos = 0;
@sample
rpos = (wpos - delay_samples + buf_len) % buf_len;
out = buf_l[rpos];
buf_l[wpos] = spl0;
wpos = (wpos + 1) % buf_len;
spl0 = spl0 * (1-mix) + out * mix;
```

## Common pitfalls

1. **Forgetting `desc:`** — REAPER won't show the FX.
2. **Forgetting `@init`** — buffer access crashes if not initialized.
3. **Slider values in `@init`** — they're DEFAULTS, not current values. Read in `@slider`.
4. **Scientific notation** — `1e-25` is ONE token. NOT `1 * e^(-25)`.
5. **`$pi` vs `pi`** — use `$pi` (the built-in constant), not `pi` (undefined variable).
6. **`$e` vs `e`** — use `$e` for Euler's number. `e` alone is undefined.
7. **Denormals** — always include `denorm = 1e-25;` and the `+=/-=` pattern.

## ReaForge-specific notes

- Save to `Effects/ReaForge/<name>.jsfx`.
- The agent generates the FULL file (header + all blocks). Don't truncate.
- The user must click FX Browser → "Scan for new plug-ins" to see new JSFX.
- For complex syntax questions, query context7: `/websites/reaper_fm_sdk_js`.
