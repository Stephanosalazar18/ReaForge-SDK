# Stereo Width (Utilities)

## Code
```jsfx
desc:Stereo Width Control

slider1:100<0,200,1>Width (%)

@init
denorm = 1e-25;

@slider
// Width: 0% = mono, 100% = original, 200% = extra wide
width = slider1 / 100;

@sample
// Mid/Side processing
mid = (spl0 + spl1) * 0.5;     // Mid = mono sum
side = (spl0 - spl1) * 0.5;    // Side = stereo difference

// Adjust side level to control width
side *= width;

// Reconstruct L/R from M/S
spl0 = mid + side;
spl1 = mid - side;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Width | 0–200% | 100% | 0=mono, 100=original, 200=extra wide. |

## Use Case
Adjust stereo image width. Narrow a wide synth pad to sit in the mix. Widen a narrow guitar recording. Use before reverb to control how wide the reflection field spreads.

## Compatibility
- **Before**: Any processing
- **After**: Any processing — transparent
- **Requires**: Nothing special
- **Conflicts**: Width > 100% can cause phase issues on mono playback — use with caution

## Source
Mid/Side (M/S) stereo processing. Standard technique in mastering and mixing. License: public domain.

<!-- test: Stereo signal with L=+1, R=-1 (wide). Width=0% should give L=R=0 (mono sum cancels). Width=100% preserves. Width=200% exaggerates. -->
