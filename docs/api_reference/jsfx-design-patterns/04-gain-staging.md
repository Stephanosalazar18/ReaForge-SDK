# Gain Staging Patterns

## Principle: output ≈ input at every stage

When a user bypasses your effect, they should hear the same volume. If your effect makes the signal louder or quieter, the user has to readjust their mix — that's bad design.

## Pattern 1: Makeup gain after saturation

Saturation reduces peak level (tanh asymptotically approaches 1.0). Compensate:

```jsfx
@sample
// Saturation
out = tanh(in * drive);
// Makeup: drive=1 → no compensation, drive=10 → +20dB compensation
compensation = 1 / tanh(drive);  // approximate
out *= compensation * makeup_slider;
```

## Pattern 2: Dry/wet with constant power

When mixing dry and wet, use equal-power crossfade to avoid volume dips:

```jsfx
@sample
mix = slider_mix / 100;
// Equal-power: dry_gain = cos(mix * pi/2), wet_gain = sin(mix * pi/2)
dry_gain = cos(mix * $pi / 2);
wet_gain = sin(mix * $pi / 2);
spl0 = spl0 * dry_gain + wet_l * wet_gain;
spl1 = spl1 * dry_gain + wet_r * wet_gain;
```

## Pattern 3: Filter compensation

Resonant filters can boost the signal at the cutoff frequency. Add a gain trim:

```jsfx
@sample
out = biquad(in);
// If Q > 1, the peak at cutoff can be up to +Q*3 dB
// Compensate: reduce output by the expected peak
peak_compensation = 1 / (1 + (slider_q - 0.707) * 0.5);
out *= peak_compensation;
```

## Pattern 4: Compression output matching

A compressor reduces peaks. Add makeup gain to restore the perceived loudness:

```jsfx
@sample
// Compression
gain_reduction = compute_gr(input, threshold, ratio);
out = input * gain_reduction;
// Makeup: approximately compensate for the average gain reduction
// The user adjusts this by ear, but provide a sensible default
out *= makeup_gain;
```

## Pattern 5: Multi-stage gain budget

For complex effects with multiple stages, track the cumulative gain:

```jsfx
@init
total_gain_db = 0;

@slider
// Each stage contributes to the total
stage1_gain = 10^(slider_drive / 20);     // +X dB from drive
stage1_loss = 1 / tanh(slider_drive);     // -Y dB from saturation
stage2_gain = 10^(slider_makeup / 20);    // +Z dB from makeup
// Total should be ≈ 0 dB
total_gain = stage1_gain * stage1_loss * stage2_gain;
```
