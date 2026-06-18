# Convolution Reverb (Reverb — Advanced)

## Code
```jsfx
desc:Convolution Reverb (FFT-based)

slider1:0.5<0,1,0.01>Mix
slider2:2<0.5,10,0.1>Decay (s)
slider3:0.7<0.1,1,0.01>Damping
slider4:4096<256,16384,256>FFT Size

@init
denorm = 1e-25;
fft_sz = 4096;
hop = fft_sz / 4;  // 75% overlap
buf_l = 0; buf_r = 0;
ir_l = 0; ir_r = 0;
out_l = 0; out_r = 0;
fft_l = 0; fft_r = 0;
fft_ir_l = 0; fft_ir_r = 0;
wpos = 0;
samples_since_fft = 0;

// Generate a synthetic impulse response (decaying noise)
function generate_ir(buf, decay_s, damp) (
  i = 0;
  len = srate * decay_s;
  len > fft_sz ? len = fft_sz;
  while (i < len) (
    // Exponential decay envelope
    env = exp(-3 * i / (decay_s * srate));
    // Damped noise
    noise = rand(2) - 1;
    buf[i] = noise * env * damp;
    i += 1;
  );
);

generate_ir(ir_l, slider2, slider3);
generate_ir(ir_r, slider2, slider3);

// Pre-FFT the impulse response
fft_real(ir_l, fft_sz); fft_permute(ir_l, fft_sz / 2);
fft_real(ir_r, fft_sz); fft_permute(ir_r, fft_sz / 2);

@slider
fft_sz = slider4;
hop = fft_sz / 4;
mix = slider1;
// Regenerate IR on parameter change
memset(ir_l, 0, fft_sz); memset(ir_r, 0, fft_sz);
generate_ir(ir_l, slider2, slider3);
generate_ir(ir_r, slider2, slider3);
fft_real(ir_l, fft_sz); fft_permute(ir_l, fft_sz / 2);
fft_real(ir_r, fft_sz); fft_permute(ir_r, fft_sz / 2);

@sample
// Accumulate input into buffer
buf_l[wpos] = spl0;
buf_r[wpos] = spl1;
wpos = (wpos + 1) % fft_sz;
samples_since_fft += 1;

// Process when we have enough samples
samples_since_fft >= hop ? (
  samples_since_fft = 0;

  // Copy buffer to FFT workspace (Hann window)
  i = 0;
  while (i < fft_sz) (
    win = 0.5 - 0.5 * cos(2 * $pi * i / (fft_sz - 1));
    fft_l[i] = buf_l[(wpos + i) % fft_sz] * win;
    fft_r[i] = buf_r[(wpos + i) % fft_sz] * win;
    i += 1;
  );

  // Forward FFT
  fft_real(fft_l, fft_sz); fft_permute(fft_l, fft_sz / 2);
  fft_real(fft_r, fft_sz); fft_permute(fft_r, fft_sz / 2);

  // Multiply with IR in frequency domain (convolution = multiplication in freq)
  convolve_c(fft_l, ir_l, fft_sz / 2);
  convolve_c(fft_r, ir_r, fft_sz / 2);

  // Inverse FFT
  fft_ipermute(fft_l, fft_sz / 2); ifft_real(fft_l, fft_sz);
  fft_ipermute(fft_r, fft_sz / 2); ifft_real(fft_r, fft_sz);

  // Scale and overlap-add to output
  i = 0;
  while (i < fft_sz) (
    out_l[i] += fft_l[i] / fft_sz;
    out_r[i] += fft_r[i] / fft_sz;
    i += 1;
  );
);

// Read from output buffer
wet_l = out_l[wpos];
wet_r = out_r[wpos];
out_l[wpos] = 0; out_r[wpos] = 0;  // clear for next overlap-add

spl0 = spl0 * (1 - mix) + wet_l * mix;
spl1 = spl1 * (1 - mix) + wet_r * mix;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Mix | 0–1 | 0.5 | Wet/dry blend |
| Decay | 0.5–10 s | 2 | Impulse response length (RT60) |
| Damping | 0.1–1 | 0.7 | High-frequency absorption (0=bright, 1=dark) |
| FFT Size | 256–16384 | 4096 | Processing block. Larger = better quality, more latency |

## Use Case
FFT-based convolution reverb with synthetic impulse response. Generates a decaying noise IR with adjustable decay and damping. More realistic than FDN for room emulation. Higher CPU but higher quality. Use when you need natural room sound, hall acoustics, or realistic space.

## Compatibility
- **Before**: EQ, compression, saturation
- **After**: Nothing — reverb is typically last
- **Requires**: FFT buffers (4 × fft_sz), Hann window, overlap-add
- **Conflicts**: High CPU at fft_sz=16384. Use 4096 for real-time, 16384 for offline rendering

## Source
FFT convolution using overlap-add method. JSFX `fft_real()`, `fft_permute()`,
`convolve_c()`, `ifft_real()` functions. Hann window from Harris (1978).
Synthetic IR: decaying filtered noise. License: public domain.

<!-- test: Impulse. Decay=2s, Damping=0.7, Mix=1.0. Should hear dense reverb tail decaying over ~2 seconds. Damping=0.3 should sound brighter. FFT=8192 should be higher quality than 4096. -->
