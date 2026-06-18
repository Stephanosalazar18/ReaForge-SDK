# Design: ReaForge DSP Encyclopedia (Phase 1)

## TL;DR

The runtime architecture is **unchanged**. The DSP encyclopedia is **content**, not code: 12 L2 primitives + 4 L3 algorithms + a compatibility matrix, all markdown, all embedded in the existing `api_reference_map()` via `R"MD(...)MD"` raw-string literals. The **single code change** is dropping the 3-value `enum` on `target` in `tools/opencode_bridge.py:112`. Two Python tools own the build: `gen_api_reference_header.py` (modify) emits the C++ header, `gen_dsp_index.py` (new) regenerates `00-index.md` from the `## Compatibility` blocks of every primitive file.

---

## 1. Architecture overview

```
opencode (LLM)
   │ tool: reaforge_get_api_reference(target="jsfx-primitives/saturation/tanh-soft")
   ▼
tools/opencode_bridge.py      ← schema: target is now free-form string (no enum)
   │  client.get("/v1/api-reference?target=jsfx-primitives/saturation/tanh-soft")
   ▼
src/host/http_server.cpp      ← generic route, unchanged
   │  GET /v1/api-reference
   ▼
src/host/project_reader.cpp   ← lookup_api_reference(target) at line 117
   │  const auto& m = api_reference_map(); auto it = m.find(target);
   ▼
src/host/api_reference_data.h ← generated header; std::unordered_map<string,string>
   │  {"jsfx-primitives/saturation/tanh-soft", kTanhSoftClipRef}
   ▼
inline constexpr const char* kTanhSoftClipRef = R"MD(# Tanh Soft Clip ...)MD";
```

**New sub-path lookup flow** (the only behavioral delta): LLM sends `target="jsfx-primitives/saturation/tanh-soft"` → bridge schema accepts (no enum) → HTTP forwards verbatim → C++ `m.find()` returns the raw-string literal → LLM copies the markdown body.

The DLL is rebuilt (`ninja -C build-mingw`, ~2 min) whenever a primitive or the header changes. No hot-reload.

---

## 2. Primitive file format (concrete)

Every `.md` under `jsfx-primitives/<cat>/` and `jsfx-algorithms/<cat>/` follows this schema. The example is a real Phase 1 entry:

```markdown
# Tanh Soft Clip (Saturation)

> Symmetric tanh saturation. The most common "warm" nonlinearity in DSP.

<!-- test: 440 Hz sine at -12 dB → THD ~ 3% at drive=1, ~ 12% at drive=4 -->

## Code

```jsfx
desc: Tanh Soft Clip (primitive)
slider1:1<0.5,8,0.01>Drive
slider2:1<0,1,0.01>Mix

@sample
  in0 = spl0;
  in1 = spl1;
  d = slider1;
  spl0 = tanh(in0 * d);
  spl1 = tanh(in1 * d);
  spl0 = in0 * (1 - slider2) + spl0 * slider2;
  spl1 = in1 * (1 - slider2) + spl1 * slider2;
```

## Parameters

| Slider | Range | Default | Description |
|---|---|---|---|
| 1 | 0.5–8 | 1 | Drive (linear gain into tanh) |
| 2 | 0–1 | 1 | Dry/wet mix |

## Use Case

First-stage warmth on vocals, drums, or buses. Pairs naturally with a high-pass before and a DC block after (see Compatibility). Drive < 2 stays subtle; > 4 enters obvious harmonic-saturation territory.

## Compatibility

- **Before:** `filters/one-pole-lowpass` (smooth input to avoid aliasing harshness)
- **After:** `utilities/dc-block` (REQUIRED when drive > 2.0)
- **Requires:** `utilities/denormal-prevention` in any chain that feeds this
- **Conflicts:** none

## Source

- Origin: FAUST `saturation.lib` `aa_saturator` (Julius Smith formulation)
- License: MIT
- Adapted: 2026-06-15
```

**Why this format survives embedding verbatim:** the generator wraps the file in `R"MD(...)MD"` (C++11 raw string literal). The `MD` delimiter is chosen so it cannot appear inside JSFX code blocks (we audit each primitive to confirm — no `)MD` substring). Backticks, pipes, asterisks, and HTML comments pass through unescaped. `gen_api_reference_header.py` does **no transformation**: `read_text()` → embed. The `<!-- test: ... -->` comments ride along unchanged per REQ-FMT-05; the LLM ignores them, humans read them.

---

## 3. Directory structure (exact paths)

```
docs/api_reference/
├── jsfx.md                                    # L1, exists
├── jsfx-primitives/                           # L2, NEW
│   ├── saturation/
│   │   ├── tanh-soft-clip.md
│   │   ├── asymmetric-tanh.md
│   │   └── hard-clip-knee.md
│   ├── filters/
│   │   ├── rbj-lowpass.md
│   │   ├── rbj-highpass.md
│   │   └── one-pole-lowpass.md
│   ├── delays/
│   │   ├── feedback-delay.md
│   │   ├── ping-pong-delay.md
│   │   └── modulated-delay.md
│   └── utilities/
│       ├── dc-block.md
│       ├── stereo-width.md
│       └── denormal-prevention.md
├── jsfx-algorithms/                           # L3, NEW
│   ├── dynamics/
│   │   └── rms-compressor.md
│   ├── reverb/
│   │   └── fdn-reverb.md
│   ├── pitch/
│   │   └── psola-pitch-shift.md
│   └── modulation/
│       └── chorus-flanger.md
├── 00-index.md                                # NEW: top-level compatibility matrix
├── reascript_lua.md                           # L1, exists
├── fx_chain_format.md                         # L1, exists
└── fx_chain-primitives/                       # L1+, exists
    ├── chain-builder-template.md
    └── recipes/vocal-slap.md
```

**Canonical key naming** (target string the LLM passes):

| File path | `target` key |
|---|---|
| `jsfx-primitives/saturation/tanh-soft-clip.md` | `jsfx-primitives/saturation/tanh-soft-clip` |
| `jsfx-primitives/00-index.md` (if added) | `jsfx-primitives/00-index` |
| `jsfx-algorithms/reverb/fdn-reverb.md` | `jsfx-algorithms/reverb/fdn-reverb` |
| `00-index.md` (top level) | `00-index` |

The dsp-primitive-format spec also allows per-category sub-indices (`jsfx-primitives/00-index.md`, `jsfx-algorithms/00-index.md`). Phase 1 ships only the top-level `00-index.md`; sub-indices are deferred unless the catalog grows past ~30 entries.

---

## 4. Generator tool design

### 4.1 `tools/gen_api_reference_header.py` (MODIFY)

Add 16 new entries to the `SOURCES` dict (and matching `CONST_NAMES`). The script itself needs no logic change — it already iterates `SOURCES.keys()` to build both the constexpr declarations and the lookup table.

```python
SOURCES = {
    # ... existing 5 keys ...
    "jsfx-primitives/saturation/tanh-soft-clip":  REPO/"docs"/"api_reference"/"jsfx-primitives"/"saturation"/"tanh-soft-clip.md",
    "jsfx-primitives/saturation/asymmetric-tanh": REPO/"docs"/"api_reference"/"jsfx-primitives"/"saturation"/"asymmetric-tanh.md",
    # ... 10 more primitive entries ...
    "jsfx-algorithms/dynamics/rms-compressor":    REPO/"docs"/"api_reference"/"jsfx-algorithms"/"dynamics"/"rms-compressor.md",
    # ... 3 more algorithm entries ...
    "00-index":                                    REPO/"docs"/"api_reference"/"00-index.md",
}

CONST_NAMES = {
    # ... existing 5 keys ...
    "jsfx-primitives/saturation/tanh-soft-clip":  "kTanhSoftClipRef",
    # ... 15 more ...
    "00-index":                                    "kDspEncyclopediaIndexRef",
}
```

CONST_NAME convention: `k` + PascalCase(key with separators stripped). The Python helper `key_to_const_name(key)` can derive this mechanically if we want zero manual mapping; we will add it in the same PR.

### 4.2 `tools/gen_dsp_index.py` (NEW)

Single-purpose tool: scan both primitive/algorithm directories, parse `## Compatibility` from each file, regenerate `docs/api_reference/00-index.md`. Pseudocode:

```python
def main():
    entries = scan_dir("docs/api_reference/jsfx-primitives/") + scan_dir(".../jsfx-algorithms/")
    write_index("docs/api_reference/00-index.md",
        catalog=[format_catalog_row(e) for e in entries],
        matrix=build_matrix(entries),       # see §6 — cells from ## Compatibility
        rules=collect_mandatory_rules(entries),
        order=STAGE_ORDER)                  # utilities→…→pitch
```

Run from repo root: `python3 tools/gen_dsp_index.py`. No args, no flags. **Idempotent** (byte-identical on re-run). Exits non-zero if any primitive file is missing `## Compatibility` (REQ-FMT-02 enforced at build time).

---

## 5. Bridge enum change (precise)

**File:** `tools/opencode_bridge.py`
**Lines:** 100–117 (the `reaforge_get_api_reference` Tool definition)

**Before (lines 110–114):**

```python
                        "target": {
                            "type": "string",
                            "enum": ["jsfx", "reascript_lua", "fx_chain_format"],
                            "description": "Which API reference markdown to return.",
                        },
```

**After:**

```python
                        "target": {
                            "type": "string",
                            "minLength": 1,
                            "description": (
                                "Which API reference markdown to return. Free-form key. "
                                "Top-level L1 cheatsheets: 'jsfx', 'reascript_lua', 'fx_chain_format'. "
                                "Sub-paths: 'jsfx-primitives/<category>/<name>' "
                                "(saturation, filters, delays, utilities), "
                                "'jsfx-algorithms/<category>/<name>' "
                                "(dynamics, reverb, pitch, modulation), "
                                "'jsfx-primitives/00-index' or '00-index' for the "
                                "compatibility matrix, "
                                "'fx_chain-primitives/chain-builder-template', "
                                "'fx_chain-primitives/recipes/vocal-slap'. "
                                "Unknown keys return INVALID_TARGET from the extension."
                            ),
                        },
```

The `call_tool` handler (line 244–248) needs **no change** — it already does `if not target` and forwards `params={"target": target}`. The pass-through behavior satisfies REQ-BRIDGE-02/03.

Smoke test extension: `tools/run_bridge_smoke.sh` gains 3 assertions: a sub-path key returns non-empty markdown; an unknown key returns 400 with `INVALID_TARGET`; the existing 3 top-level keys still return their cheatsheets.

---

## 6. Compatibility matrix schema

The top-level `docs/api_reference/00-index.md` (generated, not hand-edited):

```markdown
# ReaForge DSP Encyclopedia — Index

> First markdown the LLM should read before composing primitives. Bold rules
> are mandatory; the matrix is a design guard rail, not a suggestion.

## Primitive Catalog

| Target Key | Category | Lines | Description |
|---|---|---|---|
| `jsfx-primitives/saturation/tanh-soft-clip` | saturation | 9 | Symmetric tanh soft clip with drive + mix |
| `jsfx-primitives/saturation/asymmetric-tanh` | saturation | 12 | Even-harmonic bias via DC offset on tanh |
| `jsfx-primitives/saturation/hard-clip-knee` | saturation | 11 | Linear region + soft knee into hard clip |
| `jsfx-primitives/filters/rbj-lowpass` | filters | 18 | RBJ Audio EQ Cookbook lowpass biquad |
| `jsfx-primitives/filters/rbj-highpass` | filters | 18 | RBJ Audio EQ Cookbook highpass biquad |
| `jsfx-primitives/filters/one-pole-lowpass` | filters | 6 | Single-pole LP via `y = y + a*(x - y)` |
| `jsfx-primitives/delays/feedback-delay` | delays | 14 | Delay line with feedback gain and LP in loop |
| `jsfx-primitives/delays/ping-pong-delay` | delays | 22 | Stereo cross-feedback delay |
| `jsfx-primitives/delays/modulated-delay` | delays | 19 | Delay with internal LFO (chorus/flanger base) |
| `jsfx-primitives/utilities/dc-block` | utilities | 5 | First-order HP at ~5 Hz to remove DC offset |
| `jsfx-primitives/utilities/stereo-width` | utilities | 7 | M/S width control |
| `jsfx-primitives/utilities/denormal-prevention` | utilities | 3 | `denorm = 1e-25;` to flush subnormals |
| `jsfx-algorithms/dynamics/rms-compressor` | dynamics | 80 | RMS detector + soft-knee gain reduction |
| `jsfx-algorithms/reverb/fdn-reverb` | reverb | 120 | 4-line FDN with Hadamard feedback matrix |
| `jsfx-algorithms/pitch/psola-pitch-shift` | pitch | 150 | Windowed overlap-add pitch transposition |
| `jsfx-algorithms/modulation/chorus-flanger` | modulation | 60 | Modulated delay with feedback (chorus ↔ flanger via depth) |

## Compatibility Matrix

| | tanh-soft | asym-tanh | rbj-lp | one-pole-lp | fdbk-delay | dc-block | denorm-kill |
|---|---|---|---|---|---|---|---|
| **tanh-soft** | — | △ | ✓ | ✓ | ✓ | ✓✓ | ✓ |
| **rbj-lp** | ✓ | ✓ | — | ✗ | ✓ | — | — |
| **fdbk-delay** | ✓ | ✓ | ✓ | ✓ | — | — | ✓✓ |
| **dc-block** | ✓ | ✓ | — | — | — | — | — |

Legend: `✓` composes · `✗` state collision · `△` conditional · `✓✓` strongly recommended.
Phase 1 covers the saturation × filter, saturation × delay, filter × delay intersections (REQ-MTX-02). Full matrix expands as the catalog grows; `gen_dsp_index.py` defaults new cells to `△` with a "verify" footnote.

## Mandatory Composition Rules

- **DC block AFTER any saturation primitive with default drive > 2.0** (tanh-soft-clip, asymmetric-tanh, hard-clip-knee)
- **Denormal prevention ALWAYS before any feedback loop** (insert at top of `@init`, before feedback-delay, fdn-reverb, modulated-delay)
- **One-pole lowpass BEFORE nonlinearities when smoothing cascaded saturation** (avoid inter-modulation aliasing)
- **Modulated delay MUST sit AFTER any DC-coupled stage** (modulated-delay has no internal DC blocker)
- **RMS compressor MUST follow gain staging; the detector does NOT see raw input** (place after `saturation`/`filters`, not before)

## Recommended Signal Chain Order

1. **utilities** — denormal prevention, input gain staging (clean signal first)
2. **filters** — EQ and pre-emphasis (shape spectrum before nonlinearities)
3. **saturation** — controlled harmonics (drive-based, demands clean input)
4. **dynamics** — compression/limiting (control level after coloration)
5. **delays** — time-based effects (after dynamics so they don't pump)
6. **reverb** — final spatial layer (wet-only send, after delays)
7. **modulation** — chorus/flanger as dry/wet wrapper, late in chain
8. **pitch** — order-independent, near the end (PSOLA reads formants)
```

The matrix is symmetric; cells use the conventions from REQ-MTX-02. `gen_dsp_index.py` builds the table from `## Compatibility` blocks; cells default to `△` with a "verify" footnote when a new primitive is added (per REQ-MTX-05), and the human author tightens them in the next pass.

---

## 7. DLL regeneration workflow

```
1. Human validates a primitive in REAPER
   (paste code block into New JSFX, compile, feed audio, verify character)
2. Save primitive .md to docs/api_reference/jsfx-primitives/<cat>/<name>.md
3. Add the new (key, path) pair to SOURCES + CONST_NAMES in gen_api_reference_header.py
4. python3 tools/gen_dsp_index.py
     → regenerates 00-index.md (catalog + matrix + rules)
5. python3 tools/gen_api_reference_header.py
     → regenerates src/host/api_reference_data.h
     → file grows from ~18 KB to ~250 KB; DLL grows from 19 MB to ~23 MB
6. ninja -C build-mingw
     → cross-compile, ~2 min
7. Copy reaper_reaforge_host.dll to <REAPER>/UserPlugins/
8. Restart REAPER (extension loads on startup)
9. Re-run tools/run_bridge_smoke.sh with the new target key
```

Steps 1–3 are content work (LLM drafts from FAUST/musicdsp, human signs off). Steps 4–5 are mechanical and safe to script in a single `make regen` target. Step 6 is the only compile. Steps 7–9 close the loop.

---

## 8. Lane assignment per PR

| PR | Scope | Lane | Validation |
|---|---|---|---|
| **1** | Bridge enum relax + 3 new smoke assertions | `python-tdd` | run_bridge_smoke.sh passes; existing 5 calls unaffected |
| **2** | 3 saturation primitives | `data` (content) + `runtime-reaper` (audio) | compile in REAPER; feed 440 Hz sine; verify expected THD |
| **3** | 3 filter primitives | `data` + `runtime-reaper` | sweep filter cutoff; verify -3 dB at slider1 |
| **4** | 3 delay + 3 utility primitives | `data` + `runtime-reaper` | ping-pong L/R phase check; dc-block removes +5V offset |
| **5** | 4 algorithms (compressor, FDN, PSOLA, chorus) | `data` + `runtime-reaper` | one validation per algorithm; sample at 3 dB compression, etc. |
| **6** | `00-index.md` + `gen_dsp_index.py` + regen header + rebuild DLL | `data` + `build` | full regen is byte-identical on second run; LLM composition test passes |

---

## 9. Risks re-evaluated (with design-level mitigations)

| # | Risk | Design mitigation |
|---|---|---|
| R1 | LLM mistranslates FAUST/musicdsp to JSFX | `R"MD(...)MD"` embedding is verbatim; the LLM copies, never translates, a human-validated file. |
| R2 | Algorithm too complex for LLM to translate | Phase 1 picks 4 non-FFT algorithms (RMS envelope, delay-line FDN, windowed PSOLA, modulated delay). |
| R3 | Matrix too restrictive | `△` cells + footnotes; the matrix is script-regenerated, one-line edits to `## Compatibility` blocks. |
| R4 | DLL regen breaks existing tests | Only `api_reference_data.h` changes; HTTP route, `lookup_api_reference` signature, C++ tests untouched. |
| R5 | DLL size > 25 MB | 19 → ~23 MB leaves 2 MB headroom; Phase 2 path: split embedded vs. disk-read via `GetResourcePath()`. |
| R6 | Schema change breaks other opencode clients | Relaxed schema is a **superset** of the old one; no string that was valid becomes invalid. |
| R7 | Primitive added without `## Compatibility` | `gen_dsp_index.py` exits non-zero on missing block; CI step runs the script — missing block = red build. |

---

**Next phase:** `sdd-tasks` decomposes the 6-PR table in §8 into implementation tasks. Engram topic: `sdd/2026-06-14-reaforge-dsp-encyclopedia/design`.
