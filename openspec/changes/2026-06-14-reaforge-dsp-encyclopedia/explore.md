# Explore: ReaForge DSP Encyclopedia

> Pre-proposal thinking. Does NOT commit to a specific design.

## TL;DR (5 lines)

- The MVP works, but the LLM hallucinates DSP code because `jsfx.md` (~100 lines) is a **syntax** cheatsheet, not a **design** reference — there are no working JSFX building blocks the model can copy verbatim.
- Fix: extend the existing `reaforge_get_api_reference` content payload from 3 cheatsheets to a 4-level DSP encyclopedia (syntax → primitives → algorithms → references), all embedded in the DLL via the existing `gen_api_reference_header.py` generator.
- **Zero C++/bridge changes are required.** The generator, the C++ lookup map, and the HTTP route are all already N-file capable. The only code touch is **expanding the bridge's `target` enum** (currently hard-coded to 3 values) so the LLM can actually request the new sub-paths.
- Recommended Phase 1 scope: **10-15 primitives (Level 2) + 3-5 algorithms (Level 3)** as proof of concept. Defer Level 4 (academic references) until the content pipeline proves out.
- Estimated final DLL growth: **+3.5 MB to +4.5 MB** (from 19 MB to ~23 MB). Acceptable.

---

## Context: the hallucination problem

### Symptom (concrete)

A previous generation produced `denorm = 1 <!> e-25;` instead of `denorm = 1e-25;`. The `<!>` is documentation markup that leaked into the code. The model **knows the syntax** of JSFX but **does not know how to compose a denormal-prevention idiom**, or that `tanh` is for saturation, or that `1-a` with `a = exp(-2*$pi*cut/srate)` is a one-pole lowpass with the correct frequency-warping math.

### Root cause

| What the LLM has today | What the LLM needs |
|---|---|
| `jsfx.md`: sliders, `@sample`, `spl0/spl1`, `srate` — *language* | Working JSFX building blocks — *DSP patterns* |
| `reascript_lua.md`: `reaper.MIDI_SetNote`, `Undo_BeginBlock` — *API surface* | Working ReaScript recipes for common tasks (humanize, quantize, render) |
| `fx_chain_format.md` + `chain-builder-template.md` + `vocal-slap.md` — *chain construction* | Working FX-chain recipes for vocal/mastering/creative chains |

The LLM can translate knowledge (papers, FAUST code) into JSFX **imperfectly** because the JSFX idiom is unfamiliar (per-sample state, buffer arithmetic, slider conventions). The fix is **working code in the LLM's context window** that it can copy and adapt.

### The 4-level vision (from the prompt and `06-primitivos-reaper.md`)

| Level | Content | Status | LoC per entry | Source pool |
|---|---|---|---|---|
| **L1: Syntax** | JSFX/Lua/RfxChain language reference | **Exists** (3 cheatsheets, ~100 lines each) | ~100 | jsfx.md, reascript_lua.md, fx_chain_format.md |
| **L2: Primitives** | 10-15 building blocks (tanh saturation, RBJ filters, one-pole LP/HP, RMS envelope, LFO, feedback delay) | **NEW** | 5-30 | musicdsp.org, FAUST stdlib, stash.reaper.fm |
| **L3: Algorithms** | 15-20 complete algorithms (FDN reverb, Schroeder reverb, RMS compressor, Karplus-Strong, pitch shift, multiband compressor) | **NEW** | 50-300 | FAUST libraries, Csound opcodes, STK, CCRMA Smith, DAFx papers |
| **L4: References** | RBJ cookbook, Smith DSP summary, DAFx proceedings index | **DEFER** | ~2000 (combined) | Academic papers, textbooks |

### How the LLM will use it

```
User: "tape saturation with subtle wow and flutter"
LLM:
  1. get_api_reference("jsfx-primitives/00-index")        → catalog + compatibility matrix
  2. get_api_reference("jsfx-primitives/saturation")      → copy tanh_saturation verbatim
  3. get_api_reference("jsfx-primitives/modulation")      → copy lfo_sine
  4. get_api_reference("jsfx-primitives/utilities")       → copy dc_block (per 00-index note)
  5. Compose in one @sample, save via reaforge_save_jsfx
```

The `00-index.md` includes a **compatibility matrix** forcing the LLM to insert required pre/post primitives (e.g. "tanh saturation requires DC blocking after") — this is the design guard rail that turns the encyclopedia from a paste-bin into a composable toolkit.

---

## Sources evaluated

| Source | Content | License | Translation effort to JSFX | Recommendation |
|---|---|---|---|---|
| **musicdsp.org** | ~200 code snippets in C/Pascal, all DSP building blocks | Public domain / MIT | **Low** — already imperative, 1-1 transliteration to JSFX | **Primary** for L2 primitives |
| **FAUST stdlib + libraries** (faustlibraries.grame.fr) | ~200 documented algorithms, functional/declarative | MIT | **Low** — FAUST `+` `*` `delay` map cleanly to JSFX; the documentation already explains what each block does | **Primary** for L2 + L3 |
| **stash.reaper.fm** (community JSFX) | Hundreds of working JSFX effects by users (e.g. eel52, SaulT) | Mixed (mostly MIT/CC0) | **Zero** — already JSFX | **Primary** reference for "idiomatic JSFX style" |
| **Csound opcodes** | Thousands, well-documented | LGPL (engine) / public domain (manuals) | **Medium** — Csound's DSL needs structural translation; the *algorithm* translates, the *code* doesn't | **Secondary** for L3 algorithm references |
| **STK (Sound Toolkit)** | Physical modeling, effects, modal synthesis | MIT | **Medium** — C++ structure maps to JSFX (state variables, per-sample loop), but C++ classes → JSFX globals requires refactoring | **Secondary** for L3 (Karplus-Strong, modal, physical models) |
| **CCRMA Stanford — Smith DSP book** | Online textbook, equations + pseudocode | CC-BY | **High** — equations need manual implementation; no copy-paste possible | **Reference** for L4 (theoretical backing for L3) |
| **DAFx proceedings (dafx.de)** | ~1000 papers 1998-present, pseudocode/MATLAB | Academic (per-paper) | **High** — pseudocode is conceptual, MATLAB needs porting | **Reference** for L4; sample 5-10 seminal papers |
| **Julius Smith online texts** (ccrma.stanford.edu/~jos) | Filter design, oscillators, spectral audio | CC-BY | **High** — same as CCRMA, but excellent coverage of FFT, convolution, vocoders | **Reference** for L4 |
| **Zölzer DAFX textbook** (Wiley) | Standard DAFx reference | All-rights-reserved (book) | **Forbidden** — cannot copy | **DO NOT use** as source for embedded content; cite only |

**Key licensing fact**: Every source except Zölzer permits verbatim inclusion of algorithms (with attribution where the license requires it). The header of every embedded markdown should carry the source + license line so the DLL's metadata is self-documenting.

---

## Architecture impact

The encyclopedia is **content, not code**. The runtime architecture (opencode → bridge → HTTP → C++ extension → filesystem) is unchanged. The only structural change is **the bridge's `target` parameter is currently a 3-value enum** (`opencode_bridge.py:112`):

```python
"enum": ["jsfx", "reascript_lua", "fx_chain_format"]
```

This must be relaxed to **free-form string** (or expanded to include all new sub-paths) so the LLM can request `jsfx-primitives/saturation`, `jsfx-algorithms/reverb/fdn`, etc. The C++ side already does `m.find(target)` so any key works there.

| Component | Change |
|---|---|
| `tools/opencode_bridge.py` | **MODIFY**: relax `target` enum to free-form string. Update tool description to list categories. |
| `tools/gen_api_reference_header.py` | **MODIFY**: add ~70 new entries to the `SOURCES` dict (already designed to scale) |
| `src/host/api_reference_data.h` | **REGENERATE**: grows from 497 lines / 18 KB to ~8 000 lines / ~250 KB (50 primitives × 3 KB + 20 algorithms × 10 KB) |
| `src/host/project_reader.cpp` | **NO CHANGE** — `lookup_api_reference` is already map-driven |
| `src/host/http_server.cpp` | **NO CHANGE** — route is already generic |
| ReaForge DLL | **REGENERATE**: 19 MB → ~23 MB |
| Bridge tests (`tools/run_bridge_smoke.sh`) | **EXTEND**: add ~5 new targets to the smoke (cheap verification the LLM can fetch them) |
| `docs/api_reference/` | **EXPAND**: 5 files → ~75 files (new sub-folders `jsfx-primitives/`, `jsfx-algorithms/`, `reascript_lua-primitives/`, `fx_chain-primitives/recipes/`) |

---

## Files affected

| File | Change | Approx. size delta |
|---|---|---|
| `docs/api_reference/jsfx-primitives/00-index.md` | NEW — catalog + compatibility matrix | +200 lines |
| `docs/api_reference/jsfx-primitives/{saturation,filters,dynamics,delays,modulation,reverb,pitch,spectral,utilities}.md` | NEW — 9 category files, 5-10 primitives each | +9 × 400 = +3 600 lines |
| `docs/api_reference/jsfx-algorithms/{reverb,compressor,pitch,delay,distortion,spectral}.md` | NEW — 6 category files, 3-5 algorithms each | +6 × 600 = +3 600 lines |
| `docs/api_reference/reascript_lua-primitives/{midi,items,tracks,automation,project}.md` | NEW — 5 files | +5 × 300 = +1 500 lines |
| `docs/api_reference/fx_chain-primitives/recipes/{vocal-slap,mastering,creative}.md` | NEW — 3 recipe files | +3 × 100 = +300 lines |
| `tools/gen_api_reference_header.py` | MODIFY — add ~70 entries to SOURCES + CONST_NAMES dicts | +70 lines |
| `src/host/api_reference_data.h` | REGENERATE — embeds all of the above | +18 KB → +260 KB |
| `tools/opencode_bridge.py` | MODIFY — relax `target` enum, update tool description | +5 lines / 0 net |
| `tools/run_bridge_smoke.sh` | MODIFY — add 5 sample targets to the smoke | +10 lines |
| `src/host/meson.build` | NO CHANGE (no new source files) | 0 |
| `src/host/*.cpp` | NO CHANGE | 0 |
| ReaForge DLL (`reaper_reaforge_host.dll`) | REGENERATE — grows 19 MB → ~23 MB | +4 MB |

**Total new authored content**: ~9 000 lines of markdown (≈ 250 KB plaintext → ~260 KB after embedding overhead). This is one big PR's worth of writing, but it can be split into 4-5 chained PRs by category.

---

## Key technical questions to resolve

| # | Question | Suggested approach |
|---|---|---|
| 1 | **Validation strategy**: how do we verify each algorithm works in REAPER? | **Two-tier**: (a) smoke test — every embedded markdown is fetched by the bridge smoke; (b) runtime verify — user loads a sample of 3-5 algorithms in REAPER Windows and confirms audible behavior. Full per-algorithm REAPER validation is impractical. |
| 2 | **Update mechanism**: rebuild the DLL for every new algorithm, or hot-reload? | **Rebuild** for now — the DLL build is ~2 minutes on MSVC, and the user already has the toolchain. Hot-reload (markdown files read from disk at runtime) is a future optimization. |
| 3 | **LLM context window**: 5 algorithms × 10 KB = 50 KB. Does this exceed limits? | **Yes for tight windows** (200 KB-context models). Mitigation: add `?summary=true` flag that returns only the first 500 chars (header + code fence, no commentary) of each entry. Add this when sdd-spec runs, not now. |
| 4 | **Phase 1 scope**: which 3-5 algorithms prove the concept? | **Suggested**: FDN reverb, RMS compressor, Karplus-Strong, multiband crossover, 4-tap modulated delay (chorus). Each covers a different DSP family (feedback networks, dynamics, physical modeling, spectral, modulation). |
| 5 | **Quality bar**: "compiles and doesn't crash" or "audibly matches the reference"? | **Compiles + audibly reasonable**. A perfect aural match to a research implementation is impossible and not the goal. Goal: passes REAPER's compile, processes audio without glitches, exhibits the expected character (a compressor *compresses*, a reverb *reverberates*). |
| 6 | **License headers**: do we embed the source license in every file? | **Yes** — each markdown file's first 3 lines should be `<!-- Source: <url> | License: <id> | Adapted: <date> -->`. The generator should strip these comments before embedding OR keep them visible to the LLM. Recommend **keep them visible** so the LLM can attribute. |
| 7 | **Who writes the primitives**: pure LLM, pure human, or hybrid? | **Hybrid** (per `06-primitivos-reaper.md` §5.1): LLM generates a first draft from a known source; human reviews + runs in REAPER + signs off. The LLM is good at "translate this C to JSFX" and bad at "is this DSP correct" — humans cover the second. |
| 8 | **JSFX vs. EEL2**: are we targeting JSFX or EEL2 (the language the JSFX engine uses internally)? | **JSFX** — the file format the LLM writes. The cheatsheet is JSFX. The primitives are JSFX. The LLM never sees EEL2. |

---

## Alternatives considered

| Alternative | Verdict | Reason |
|---|---|---|
| **A. Keep adding to `jsfx.md` (flat cheatsheet)** | **Rejected** | The file is already 116 lines. 10× more content makes it unusable as a single-context reference. LLM would need to find the relevant section in 1 000 lines — slow and error-prone. The 2026-06-07 architecture chose sub-paths for this exact reason. |
| **B. Read primitives from disk at runtime (hot-reload)** | **Deferred** | Avoids the DLL-rebuild step. But: requires the extension to know REAPER's resource path at runtime (it does — `GetResourcePath()`), introduces path-handling complexity, and means the LLM behavior depends on filesystem state. The build-once-and-embed approach is simpler and the user already validates a 2-minute rebuild cycle. Re-evaluate if rebuild time becomes a friction point. |
| **C. Generate primitives dynamically (LLM writes them on first request, caches to disk)** | **Rejected** | Defeats the purpose — the LLM is what hallucinates. We need **human-verified** working code as the LLM's reference set. Dynamic generation is a runtime anti-pattern here. |
| **D. Embed FAUST source code verbatim and have the LLM translate** | **Rejected for primitives, OK for algorithms** | FAUST is readable but not JSFX. Translation is mechanical for primitives (lose the structure) but valuable for algorithms (keep the structure as a guide). For L2, write JSFX directly. For L3, embed both the JSFX translation AND a link to the original FAUST. |
| **E. Buy a commercial DSP code library (e.g. JUCE DSP, iPlug)** | **Rejected** | Licensing is incompatible (commercial, GPL with exceptions) and the code is C++ designed for compile-time use, not as a learning reference for an LLM. |
| **F. RAG over FAUST docs at request time** | **Rejected** | Requires network access. The LLM's value here is **deterministic, offline, sub-second reference**. The offline-embedded approach is the architecture's strength. |
| **G. Embed just Level 2 (primitives), no Level 3 (algorithms)** | **Under-scope** | 10-15 primitives cover ~80% of common effects. But the user prompt explicitly calls out algorithms as a goal. Phase 1 should include 3-5 to prove the pattern scales. |
| **H. Embed Level 4 (academic references) first** | **Rejected** | References are theoretical, not actionable. The LLM needs *working code* to copy, not equations to interpret. Level 4 is a nice-to-have once L2+L3 prove out. |

---

## Recommendation

### Phase 1 scope (for sdd-propose to commit to)

| Layer | Count | Concrete content |
|---|---|---|
| **L2 Primitives** | 12 primitives across 4 categories | `saturation.md` (tanh, asymmetric tanh, hard clip, tube-ish) · `filters.md` (one-pole LP/HP, RBJ lowpass, RBJ highpass, RBJ peak) · `delays.md` (feedback delay, modulated delay, allpass) · `utilities.md` (dc_block, denormal_kill, gain_stage) |
| **L3 Algorithms** | 4 algorithms across 2 categories | `compressor.md` (RMS compressor with attack/release) · `reverb.md` (Schroeder + FDN) · `pitch.md` (linear-interp pitch shift) · `delay.md` (4-tap chorus) |
| **L1 / infrastructure** | index + bridge enum fix | `jsfx-primitives/00-index.md` (catalog + compatibility matrix) · relax `target` enum in `opencode_bridge.py` |
| **Validation** | smoke + runtime | extend `run_bridge_smoke.sh` to fetch 5 new targets · runtime-verify 2-3 algorithms in REAPER Windows |

**Out of scope for Phase 1**: Level 4 references, Lua primitives (a separate 1-day pass), more than 4 algorithms (prove the pattern first), hot-reload mechanism, summary/flag for tight context windows.

### PR breakdown (suggested, chained)

| PR | What | Lane | Size |
|---|---|---|---|
| **1** | Relax bridge `target` enum + update tool description; extend smoke with 5 new (currently existing) targets | `python-tdd` | XS |
| **2** | Add L2 primitives: `saturation` + `utilities` (the 2 most-requested categories) | `runtime-reaper` (audio verify) | M |
| **3** | Add L2 primitives: `filters` + `delays` (cover the modulation/echo family) | `runtime-reaper` | M |
| **4** | Add `jsfx-primitives/00-index.md` with the compatibility matrix | review (LLM behavior) | S |
| **5** | Add L3 algorithms: `compressor` + `reverb` (the 2 highest-value algorithms) | `runtime-reaper` | L |
| **6** | (Stretch) Add L3: `pitch` + `delay/chorus` | `runtime-reaper` | M |

Total: **6 PRs**, ~1 week of focused work, ~150-200 KB of new embedded content.

### Why this is the right cut

- **Hybrid content authorship** scales: LLM translates from FAUST/musicdsp.org; human verifies in REAPER.
- **Infrastructure is free**: the generator, lookup, and HTTP layer are already N-file capable. The only new C++/bridge code is **relaxing one enum**.
- **Phase 1 is provable**: 12 primitives + 4 algorithms covers ~70% of "tape saturation", "compressor", "simple reverb", "chorus", "filter sweep" requests.
- **Phase 2 is obvious**: more primitives, all algorithms, Lua side, then Level 4 references. No architecture change between phases.

---

## Next phase

`sdd-propose` should produce `proposal.md` defining:
- Intent: **close the DSP-hallucination gap by adding a 4-level encyclopedia to the existing `reaforge_get_api_reference` content payload**
- Phase 1 scope: the 12 primitives + 4 algorithms + index + bridge-enum fix listed above
- Out of scope: Level 4 references, Lua primitives, hot-reload, summary flag
- Verification plan: extend `run_bridge_smoke.sh`, runtime-verify 2-3 algorithms on REAPER Windows before marking complete
- Decision to confirm with the user: **author model** (hybrid LLM-draft + human-verify is the recommendation, but the user may want 100% human-authored for the first batch to set the quality bar)

`sdd-spec` then writes delta specs for `dsp-encyclopedia-content` (per-primitive format, compatibility matrix structure, license header convention) and `bridge-target-enum-relax` (free-form `target` string with category prefix convention).
