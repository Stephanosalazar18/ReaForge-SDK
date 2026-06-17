# Hard Clip with Knee (Saturation)

## Code
```jsfx
desc:Hard Clip with Knee

slider1:0<-24,0,0.1>Threshold (dB)
slider2:2<0.5,10,0.1>Knee Width (dB)
slider3:0<-12,12,0.1>Output Gain (dB)

@init
denorm = 1e-25;

@slider
thresh = 10^(slider1 / 20);   // Convert dB to linear threshold
knee_w = 10^(slider2 / 20);   // Knee width in linear (above threshold)
out_gain = 10^(slider3 / 20);

@sample
// Piecewise hard clip with soft knee transition.
// For |in| < thresh: pass through
// For |in| between thresh and thresh + knee_w: quadratic blend
// For |in| > thresh + knee_w: hard clip to thresh
function clip(in, th, kw) (
    abs_in = abs(in);
    abs_in <= th ? in : (
        abs_in >= th + kw ? sign(in) * th : (
            // Quadratic knee: smooth transition from linear to hard clip.
            local(x) = (abs_in - th) / kw; // 0 at thresh, 1 at thresh+knee
            sign(in) * (th + kw * (x - x*x/2))
        )
    )
);

spl0 = clip(spl0, thresh, knee_w) * out_gain;
spl1 = clip(spl1, thresh, knee_w) * out_gain;

spl0 += denorm; spl0 -= denorm;
spl1 += denorm; spl1 -= denorm;
```

## Parameters
| Slider | Range | Default | Description |
|---|---|---|---|
| Threshold | -24 to 0 dB | 0 | Level above which clipping begins. |
| Knee Width | 0.5–10 dB | 2 | Width of the soft knee transition. Higher = smoother clip. |
| Output Gain | -12 to +12 dB | 0 | Post-clip level. |

## Use Case
Guitar distortion, aggressive mastering clipper, brickwall limiting. The knee provides a smooth transition from clean to clipped, avoiding the harsh artifacts of a pure hard clip. Use with EQ before to shape which frequencies hit the threshold hardest.

## Compatibility
- **Before**: EQ (boost frequencies to push into clip), compression (control dynamics before clipping)
- **After**: Low-pass filter (tame the harmonics), DC blocking
- **Requires**: `denorm = 1e-25;` in `@init`
- **Conflicts**: Cascading multiple hard clips progressively flattens the waveform — usually single-stage is best

## Source
Knee-based clipping is a standard mastering/limiter technique. The quadratic knee formula is from the DAFx soft-clipper literature. JSFX `abs()` and `sign()` are built-in. License: public domain.

<!-- test: Feed a 440 Hz sine at -6 dB. With threshold=-12 dB and knee=3 dB, the waveform should show soft rounding at the peaks. With knee=0.5 dB, it should approach a hard square-ish clip. -->
