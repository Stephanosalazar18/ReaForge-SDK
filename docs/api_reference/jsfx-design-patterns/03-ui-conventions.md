# UI Conventions for JSFX

## Slider ordering

REAPER displays sliders in numerical order (slider1, slider2, ...). Group them logically:

```
1-3:   Input controls (gain, drive, threshold)
4-7:   Processing controls (the effect's core parameters)
8-10:  Output controls (makeup gain, mix, output level)
11+:   Advanced/hidden controls
```

## Slider naming conventions

| Category | Convention | Examples |
|---|---|---|
| Gain | "(dB)" suffix | "Input Gain (dB)", "Makeup Gain (dB)" |
| Time | "(ms)" or "(s)" suffix | "Attack (ms)", "Decay (s)" |
| Frequency | "(Hz)" suffix | "Cutoff (Hz)", "Center Freq (Hz)" |
| Percentage | "(%)" suffix | "Mix (%)", "Width (%)" |
| Ratios | "(:1)" suffix | "Ratio (:1)" |
| Modes | Enum with descriptive labels | "{Peak,RMS}", "{Free,Sync}" |
| Musical | No suffix, use musical terms | "Warmth", "Character", "Space" |

## @gfx section (optional but recommended for complex effects)

```jsfx
@gfx 300 200
// Clear background
gfx_clear = 0x202020;

// Draw title
gfx_r = gfx_g = gfx_b = 1;
gfx_x = 10; gfx_y = 10;
gfx_drawstr("Effect Name");

// Draw parameter values
gfx_x = 10; gfx_y = 30;
gfx_printf("Drive: %.1f", slider1);
gfx_x = 10; gfx_y = 50;
gfx_printf("Output: %.1f dB", slider2);
```

## Metering (optional)

For dynamics effects, show gain reduction:

```jsfx
@gfx 300 100
// Gain reduction meter
gr_db = 20 * log10(max(env_l, 0.0001));
gr_normalized = min(1, -gr_db / 20);  // 0 to 1, 20dB reduction = full
gfx_r = 0.8; gfx_g = 0.2; gfx_b = 0.2;
gfx_rect(10, 10, gr_normalized * 200, 20);
```

## Tag conventions

Use `desc:` and `tags:` for discoverability:

```jsfx
desc:My Effect Name
//tags: delay modulation tempo-sync
//author: ReaForge
```
