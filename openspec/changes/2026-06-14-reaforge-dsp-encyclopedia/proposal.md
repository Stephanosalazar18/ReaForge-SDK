# Proposal: ReaForge DSP Encyclopedia (Phase 1)

## Intent

Build a **DSP building block library** embedded in the REAPER extension DLL that gives the LLM working JSFX code fragments it can copy and compose. This replaces the current "syntax-only" cheatsheet (~100 lines of JSFX reference) with an **encyclopedia** of 12 primitives (5-30 lines each) and 4 complete algorithms (50-300 lines each). The LLM will generate audibly correct effects instead of hallucinating DSP code like `denorm = 1 <!> e-25;`.

## Capabilities

### New capabilities
- **12 DSP primitives**: working JSFX fragments in 4 categories (saturation, filters, delays, utilities). Each is a self-contained code block the LLM copies verbatim.
- **4 complete algorithms**: RMS compressor, FDN reverb, pitch shift (PSOLA), chorus/flanger. Full `@init/@slider/@sample` implementations.
- **Compatibility matrix** (`00-index.md`): rules for primitives composition (e.g., "saturation before filter", "DC block after gain > 1.0").
- **Bridge enum relaxed**: `reaforge_get_api_reference` target changed from 3 hardcoded values to free-form string, enabling sub-paths like `jsfx-primitives/saturation`.

### Existing capabilities leveraged
- `reaforge_get_api_reference` endpoint already supports any string key via `api_reference_map()` lookup.
- `tools/gen_api_reference_header.py` already supports N markdown files with sub-path keys.
- Embedded content in DLL — no architectural changes to the bridge, HTTP server, or C++ modules.

## Scope (Phase 1)

| Level | Contents | Count | Primary Source |
|---|---|---|---|
| **L1 Syntax** | jsfx.md, reascript_lua.md | 2 | Already exists |
| **L2 Primitives** | saturation (3), filters (3), delays (3), utilities (3) | 12 | musicdsp.org, FAUST |
| **L3 Algorithms** | RMS compressor, FDN reverb, PSOLA pitch shift, chorus/flanger | 4 | FAUST, STK |
| **L4 References** | RBJ cookbook, Smith DSP summary, DAFx index | 0 | Deferred to Phase 2 |

### Primitive categories (L2)

| Category | Primitives |
|---|---|
| **Saturation** | tanh soft clip, asymmetric tanh with bias, hard clip with knee |
| **Filters** | RBJ lowpass, RBJ highpass, one-pole lowpass |
| **Delays** | feedback delay, ping-pong delay, modulated delay (flanger base) |
| **Utilities** | DC blocking, stereo width, denormal prevention (`denorm = 1e-25;`) |

### Algorithm categories (L3)

| Algorithm | Lines (est.) | Complexity | Source |
|---|---|---|---|
| RMS Compressor | ~80 | Medium | FAUST `compressor.lib` |
| FDN Reverb | ~120 | Medium | FAUST `reverb.lib` + CCRMA |
| PSOLA Pitch Shift | ~150 | High | STK `PitchShifter` |
| Chorus/Flanger | ~60 | Low | musicdsp.org |

## Success Criteria

1. Each of the 12 primitives compiles as standalone JSFX in REAPER (no syntax errors)
2. Each of the 4 algorithms compiles and produces audibly correct output
3. The `00-index.md` matrix prevents the LLM from composing incompatible primitives
4. The LLM, asked "generate tape saturation with chorus", produces working JSFX by composing primitives
5. DLL size < 25 MB (from 19 MB baseline)

## Decisions (user-confirmed)

| # | Decision | Answer |
|---|---|---|
| 1 | Phase 1 scope | 12 primitives + 4 algorithms + index + bridge fix |
| 2 | Authorship | Hybrid: LLM generates draft from FAUST/musicdsp → human validates in REAPER |
| 3 | ReaJS accuracy | During implementation, query **context7** (`/websites/reaper_fm_sdk_js`, `/justinfrankel/jsfx`, `/joepvanlier/jsfx`) for idiomatic JSFX transcription |
| 4 | Validation criteria | Compiles + sounds correct + composable with other primitives |
| 5 | Validation tool | Manual in REAPER for Phase 1 (no automated DSP smoke tests) |
| 6 | Level 4 references | Deferred to Phase 2 |

## Affected Areas

| File | Impact | PR |
|---|---|---|
| `tools/opencode_bridge.py` | MODIFY: relax target enum from 3 values to free string | 1 |
| `docs/api_reference/jsfx-primitives/*.md` | NEW: 12 files + 1 index | 2-4 |
| `docs/api_reference/jsfx-algorithms/*.md` | NEW: 4 files + 1 algorithm index | 5 |
| `docs/api_reference/00-index.md` | NEW: compatibility matrix | 6 |
| `tools/gen_api_reference_header.py` | MODIFY: add 16 new sources | 6 |
| `src/host/api_reference_data.h` | REGENERATE: 500 → ~4000 lines | 6 |
| `src/host/project_reader.cpp` | NO CHANGE (already supports any string key) | — |
| `src/host/mvp_host.cpp` | NO CHANGE | — |

## Delivery Strategy

6 chained PRs, ~1 week:

| PR | Scope | Size | Validation |
|---|---|---|---|
| 1 | Relax bridge enum (3 hardcoded → free-form) | ~10 LOC | existing tests |
| 2 | Saturation primitives (3 files) | ~150 lines JSFX | compile in REAPER + audio test |
| 3 | Filter primitives (3 files) | ~200 lines JSFX | compile in REAPER + audio test |
| 4 | Delay + utility primitives (6 files) | ~300 lines JSFX | compile in REAPER + audio test |
| 5 | 4 algorithms + algorithm index | ~400 lines JSFX | compile + audio test per algorithm |
| 6 | Compatibility matrix `00-index.md` + regen DLL | ~100 lines markdown | LLM composition test |

## Risks

| Risk | Likelihood | Mitigation |
|---|---|---|
| LLM mistranslates FAUST to JSFX | Medium | Human validates every primitive in REAPER before merge. Use context7 for ReaJS idiom check. |
| Algorithm too complex for LLM to translate correctly | Medium | Phase 1 picks simpler algorithms (RMS comp, chorus). FFT-based algorithms deferred to Phase 2. |
| Compatibility matrix too restrictive | Low | Start with basic rules; iterate based on LLM composition failures. |
| DLL regen breaks existing tests | Low | Only `api_reference_data.h` changes — no C++ logic changes. Existing 4/4 tests unaffected. |

## Out of Scope (deferred to later phases)

- Level 4 references (RBJ cookbook, Smith DSP, DAFx index)
- Lua primitives (ReaScript workflow blocks)
- FFT-based algorithms (phase vocoder, spectral morphing, CDP8 cross-synthesis)
- Automated REAPER smoke tests (manual validation only for Phase 1)
- Hot-reload of primitives (DLL rebuild required)
- `reaforge_get_api_reference` performance optimization (N lookups from 3 to 20+ — still negligible)

## Approach

### Primitives pipeline

```
FAUST/musicdsp.org source
    │
    ▼
LLM translates to JSFX (using context7 for ReaJS idioms)
    │
    ▼
Human tests in REAPER:
   copies code into a new JSFX
   checks for syntax errors (REAPER's built-in JIT compiler)
   feeds audio through it, verifies output
   checks composability with adjacent primitives
    │
    ▼
Primitive saved as docs/api_reference/jsfx-primitives/<category>/<name>.md
    │
    ▼
gen_api_reference_header.py regenerates api_reference_data.h
    │
    ▼
DLL rebuilt (mingw-w64 cross-compile, 2 min)
    │
    ▼
Bridge serves new primitives via reaforge_get_api_reference
```

### Validation per primitive

Each primitive file includes a **test block** (hidden from the LLM via HTML comments) with:
- Expected audio behavior
- Known working parameter ranges
- Audit sample: what it should sound like with a 440 Hz sine at -12 dB

## Next Phase

`sdd-spec` should produce 3 delta specs:
- `specs/dsp-primitive-format.md` — standardized format for each primitive file (code block, parameters table, use case, precautions, compatibility tags)
- `specs/bridge-enum-relax.md` — target validation change in `opencode_bridge.py`
- `specs/compatibility-matrix.md` — rules for the `00-index.md` matrix
