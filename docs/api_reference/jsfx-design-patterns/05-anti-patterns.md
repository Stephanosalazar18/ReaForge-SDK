# Anti-Patterns: What NOT to do

## 1. Redundant sliders for the same concept

**Bad**: Time slider AND mode selector for delay time.
```jsfx
slider1:250<1,2000,1>Time (ms)
slider2:0<0,2,1{Straight,Dotted,Triplet}>Mode
```
**Why it's bad**: The user doesn't know which one controls the delay. If mode is "Dotted", does the time slider still apply? It's ambiguous.
**Fix**: Use mode selector, derive time from BPM. Or use time slider, no mode.

## 2. Missing denormal prevention

**Bad**: No `denorm` in a JSFX with feedback loops or filters.
**Why it's bad**: Silent sections cause CPU spikes (100% core usage). REAPER freezes.
**Fix**: Always include `denorm = 1e-25;` in `@init` and `+=/-=` in `@sample`.

## 3. Missing DC blocking after saturation

**Bad**: Asymmetric tanh without DC blocking.
**Why it's bad**: DC offset accumulates, causes clicks when the signal hits the next stage, reduces headroom, messes up meters.
**Fix**: Add a DC blocking filter after any saturation with drive > 2.0 or bias > 0.

## 4. No gain staging

**Bad**: Saturation with drive=10 but no makeup gain.
**Why it's bad**: The output is much quieter than the input. The user has to crank their mixer to compensate, introducing noise.
**Fix**: Add a makeup gain slider. Default to approximately compensating for the drive.

## 5. Too many sliders

**Bad**: 20 visible sliders for a simple delay.
**Why it's bad**: The user is overwhelmed. They can't find the parameter they want. The UI is cluttered.
**Fix**: Start with 4-6 sliders. Hide advanced parameters with `-Hidden`.

## 6. Programmer-named sliders

**Bad**: `slider1:0.707<0.1,4,0.01>Biquad Q Factor`
**Why it's bad**: "Biquad Q Factor" means nothing to a musician.
**Fix**: `slider1:0.707<0.1,4,0.01>Resonance`

## 7. No dry/wet mix

**Bad**: Effect is 100% wet, no mix control.
**Why it's bad**: The user can't blend the effect with the original signal. They have to use REAPER's dry/wet which doesn't sound as good.
**Fix**: Always include a Mix slider (0-100%).

## 8. Unstable feedback

**Bad**: Feedback gain can exceed 1.0.
**Why it's bad**: The feedback loop runs away, volume increases exponentially, speakers blow out.
**Fix**: Clamp feedback gain: `fb = min(fb, 0.99);`

## 9. No smoothing on parameter changes

**Bad**: Changing a slider causes an audible click.
**Why it's bad**: Automating parameters sounds terrible.
**Fix**: Smooth parameter changes in @block:
```jsfx
@block
target_freq = slider1;
current_freq = current_freq * 0.999 + target_freq * 0.001;
```

## 10. Buffer too small

**Bad**: 100ms buffer for a 2-second delay.
**Why it's bad**: The delay wraps around and you hear the wrong audio.
**Fix**: Calculate buffer size from the maximum possible delay time: `buf_len = srate * max_delay_seconds;`

## 11. No `@serialize` for state preservation

**Bad**: Filter state is lost when REAPER saves/loads the project.
**Why it's bad**: The filter "jumps" when reopening a project.
**Fix**: Use `@serialize` to save/restore internal state.

## 12. Separate Dry and Wet sliders

**Bad**: `slider9:0<-20,20>Dry (dB)` AND `slider10:0<-20,20>Wet (dB)`
**Why it's bad**: Redundant. The user has to adjust two sliders to change one thing (the mix).
**Fix**: Single `Mix (%)` slider.

## 13. No tempo sync option

**Bad**: Delay only has manual ms, no way to sync to BPM.
**Why it's bad**: If the user changes tempo, they have to manually adjust the delay time.
**Fix**: Add a Free/Sync mode selector (see parameter design pattern 4).

## 14. Clipping at the output

**Bad**: No output level control, signal can exceed 0 dBFS.
**Why it's bad**: Digital clipping, harsh sound, damaged speakers.
**Fix**: Add an output gain slider. Optionally add a soft clipper at the output: `spl0 = tanh(spl0);`

## 15. Ignoring stereo

**Bad**: Processing only spl0, ignoring spl1.
**Why it's bad**: The right channel is silent or unprocessed.
**Fix**: Always process both channels. If the effect is mono, sum to mono first, then duplicate to both outputs.
