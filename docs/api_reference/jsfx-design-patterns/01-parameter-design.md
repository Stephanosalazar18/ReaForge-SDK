# Parameter Design Patterns

## Pattern 1: Mode selector vs. Separate sliders

**When to use a mode selector (`sliderX:0<0,N,1{Option1,Option2,...}>`):**
- The parameter has discrete, named options (dotted, triplet, straight)
- The options are mutually exclusive
- The user thinks in categories, not numbers

**When to use a continuous slider:**
- The parameter is a range (0-100%, 20-20000 Hz, -12 to +12 dB)
- The user needs fine control
- The value space is continuous

**ANTI-PATTERN: Don't use BOTH for the same concept.**
If you have a Mode selector, derive the actual value from it programmatically.
If you have a continuous slider, let the user dial it in.
Do NOT put both a mode selector and a slider for the same parameter.

**Example: Delay time with tempo sync**
```jsfx
// GOOD: Mode selector drives the time
slider1:0<0,3,1{Free,Straight,Dotted,Triplet}>Sync Mode
slider2:120<40,300,1>BPM (Hidden)
// In @slider: derive time from mode + BPM
// In Free mode: use slider3 for manual ms
slider3:250<1,2000,1>Time (ms, Free mode only)

// BAD: Both slider AND mode for the same thing
slider1:250<1,2000,1>Time (ms)
slider2:0<0,2,1{Straight,Dotted,Triplet}>Mode
// User doesn't know which one to use. Confusing.
```

## Pattern 2: Hidden sliders for internal state

Use `sliderX:-Hidden parameter name` for:
- Internal state the user shouldn't touch (envelope state, filter coefficients)
- Parameters set once and forgotten (oscillator phase, buffer positions)
- Debug values that only matter during development
- BPM sync source (set automatically, not user-adjustable)

```jsfx
slider20:120<40,300,1>-BPM (auto-synced)
slider21:0<0,1,1>-Internal smoothing state
```

## Pattern 3: Group related parameters

Organize sliders in logical groups. REAPER displays them in order.

```jsfx
// Input section (sliders 1-3)
slider1:0<-24,24,0.1>Input Gain (dB)
slider2:0<0,1,1{Peak,RMS}>Detection Mode

// Processing section (sliders 4-7)
slider4:-12<-60,0,0.1>Threshold (dB)
slider5:4<1,20,0.1>Ratio
slider6:10<0.1,100,0.1>Attack (ms)
slider7:50<10,500,1>Release (ms)

// Output section (sliders 8-9)
slider8:0<-12,12,0.1>Makeup Gain (dB)
slider9:100<0,100,1>Mix (%)
```

## Pattern 4: Tempo sync pattern

When an effect supports tempo sync, use this pattern:

```jsfx
slider1:0<0,1,1{Free,Sync}>Time Mode
// Free mode: manual time
slider2:250<1,2000,1>Time (ms)
// Sync mode: note division
slider3:0<0,5,1{1/4,1/8,1/16,1/4T,1/8T,1/16T}>Note Division

@slider
time_mode == 0 ? (
  delay_samples = slider2 / 1000 * srate;
) : (
  // Derive from BPM + note division
  beat_samples = 60 / tempo * srate;  // tempo is a JSFX built-in
  division_factor = (slider3 == 0) ? 1 :    // 1/4
                    (slider3 == 1) ? 0.5 :  // 1/8
                    (slider3 == 2) ? 0.25 : // 1/16
                    (slider3 == 3) ? 0.75 : // 1/4T (triplet)
                    (slider3 == 4) ? 0.375 : // 1/8T
                    0.1875;                   // 1/16T
  delay_samples = beat_samples * division_factor;
);
```

## Pattern 5: Dry/wet mix

Always use a single Mix slider, not separate Dry and Wet sliders:

```jsfx
// GOOD: Single mix slider
slider9:50<0,100,1>Mix (%)

@sample
mix = slider9 / 100;
spl0 = spl0 * (1 - mix) + wet_l * mix;
spl1 = spl1 * (1 - mix) + wet_r * mix;

// BAD: Separate dry and wet (confusing, redundant)
slider9:0<-20,20,0.1>Dry (dB)
slider10:0<-20,20,0.1>Wet (dB)
```
