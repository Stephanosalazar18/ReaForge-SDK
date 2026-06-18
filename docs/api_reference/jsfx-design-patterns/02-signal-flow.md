# Signal Flow Patterns

## Canonical order

```
Input → DC Block → EQ → Saturation → Compression → Delay → Reverb → Modulation → Pitch → Output
```

This order exists for acoustic reasons:
- DC block first: clean the signal before anything
- EQ before saturation: shape WHICH frequencies distort
- Saturation before compression: saturate the peaks, then control them
- Compression after saturation: tame the harmonics that saturation added
- Delay before reverb: reverb adds space to the repeats
- Modulation last: chorus/flanger on the final sound
- Pitch last: pitch shifting works best on a stable signal

## Serial vs. Parallel routing

### Serial (default)
Each stage feeds the next. Simple, predictable, CPU-efficient.
```jsfx
@sample
spl0 = dc_block(spl0);
spl0 = eq(spl0);
spl0 = saturate(spl0);
spl0 = compress(spl0);
```

### Parallel (for wet/dry or multi-band)
Split the signal, process independently, mix back.
```jsfx
@sample
dry_l = spl0;
wet_l = saturate(spl0);
spl0 = dry_l * (1 - mix) + wet_l * mix;
```

### Multi-band (for compressors, saturators)
Split into frequency bands, process each, sum back.
```jsfx
@sample
// Split into 3 bands
low = lowpass(spl0, 200);
mid = bandpass(spl0, 200, 2000);
high = highpass(spl0, 2000);
// Process each band independently
low = compress(low, threshold_low);
mid = compress(mid, threshold_mid);
high = compress(high, threshold_high);
// Sum
spl0 = low + mid + high;
```

## Feedback paths

Feedback loops require careful design:
- Always include a gain coefficient < 1.0 to prevent runaway
- Always include DC blocking in the feedback path
- Smooth parameter changes to avoid clicks in the feedback

```jsfx
@sample
// Feedback delay with safety
feedback = delay_buffer[rpos] * feedback_gain;  // feedback_gain < 1.0
feedback = dc_block(feedback);                   // prevent DC buildup
delay_buffer[wpos] = spl0 + feedback;
```

## Sidechain routing

For sidechain compression, the detection signal is separate from the audio:

```jsfx
@sample
// Audio path
spl0 = spl0 * gain_reduction;
spl1 = spl1 * gain_reduction;

// Detection path (separate)
detector = sidechain_input;  // could be another track, or a filtered version
gain_reduction = compressor_gain(detector, threshold, ratio);
```
