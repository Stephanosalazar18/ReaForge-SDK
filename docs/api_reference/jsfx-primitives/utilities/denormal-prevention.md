# Denormal Prevention (Utilities)

## Code
```jsfx
desc:Denormal Prevention

@init
// The standard JSFX denormal prevention pattern.
// Add this to the @init section of every JSFX that does floating-point DSP.
// Without it, very quiet signals can cause the CPU to stall on denormal
// floating-point operations, consuming 100% CPU.
denorm = 1e-25;

@sample
// After all DSP processing in @sample, add this anti-denormal guard:
spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
No sliders. This is a code pattern, not a standalone effect. Copy the `@init` and `@sample` lines into any JSFX.

## Use Case
**Include in EVERY JSFX.** Denormal numbers are extremely small floating-point values that occur when audio fades to silence and the FPU enters a slow microcode path. The `+= denorm; -= denorm` pattern adds and immediately removes an inaudible value, flushing the FPU pipeline back to normal speed. Without this, silent sections can spike CPU usage.

## Compatibility
- **Before**: Not applicable — this is infrastructure, not processing
- **After**: Not applicable
- **Requires**: `denorm = 1e-25;` in `@init`, and the `+=/-=` pattern in `@sample`
- **Conflicts**: None — safe to include everywhere. If it's already present, adding it twice is harmless.

## Source
Documented in the REAPER JSFX SDK and community wiki. Standard pattern since JSFX inception. License: public domain.

<!-- test: Not testable as a standalone effect. Verify that the CPU meter in REAPER stays low when the track is silent. -->
