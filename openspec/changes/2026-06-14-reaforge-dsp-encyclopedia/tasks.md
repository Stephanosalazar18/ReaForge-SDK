# Tasks: ReaForge DSP Encyclopedia (Phase 1)

> Decomposes `proposal.md` (6 chained PRs) and `design.md` (single-code-touch architecture, dual-generator build) into per-PR implementation steps. Every task lists commit subject, files, ΔLOC, and lane so reviewers can scope a PR diff before opening it.

**Source artifacts**
- Proposal: `openspec/changes/2026-06-14-reaforge-dsp-encyclopedia/proposal.md` (6 PRs, scope, success criteria)
- Design: `openspec/changes/2026-06-14-reaforge-dsp-encyclopedia/design.md` (file format, generators, DLL regen workflow)
- Specs: `specs/dsp-primitive-format/spec.md`, `specs/bridge-enum-relax/spec.md`, `specs/compatibility-matrix/spec.md`

---

## Review Workload Forecast

| Field | Value |
|-------|-------|
| Estimated changed lines (excl. generated header) | ~1,150 (10 + 150 + 200 + 300 + 400 + 90 new source) |
| Generated header delta | 497 → ~3,500 lines (auto, excluded from review scope per `work-unit-commits`) |
| 400-line budget risk | **Low** for every individual PR |
| Chained PRs recommended | **Yes** (already in proposal) |
| Delivery strategy | `auto-chain` (6-PR cadence is pre-committed in the proposal) |
| Chain strategy | **stacked-to-main** (each PR merges to main in order; no integration branch needed) |

Decision needed before apply: No
Chained PRs recommended: Yes
Chain strategy: stacked-to-main
400-line budget risk: Low

### PR-by-PR Workload

| PR | Lane | ΔLOC (new) | ΔLOC (mod) | Files touched | Build required? |
|----|------|-----------:|-----------:|---------------|-----------------|
| 1 | `python-tdd` | 0 | 10 | 2 | No (Python only) |
| 2 | `data` + `runtime-reaper` | 150 | 0 | 3 | No (content) |
| 3 | `data` + `runtime-reaper` | 200 | 0 | 3 | No (content) |
| 4 | `data` + `runtime-reaper` | 300 | 0 | 6 | No (content) |
| 5 | `data` + `runtime-reaper` | 400 | 0 | 4 | No (content) |
| 6 | `data` + `build` | 100 (md) + 150 (py) | 35 (SOURCES/CONST_NAMES) + regen header | 5 | **Yes** (DLL rebuild) |

Every PR independently fits the 400-line review budget. The regenerated `src/host/api_reference_data.h` (497 → ~3,500) lives behind `gen_api_reference_header.py` and is excluded from human review per the `work-unit-commits` skill: review the generator inputs (PRs 2–5), not the generator output.

---

## PR 1 — Bridge enum relax (lane: `python-tdd`)

**Scope**: drop the 3-value `enum` on `reaforge_get_api_reference.target`, expand `description` to list sub-paths, add 3 sub-path assertions to `run_bridge_smoke.sh`. No C++ change. No DLL rebuild. Goal: open sub-paths the runtime already supports.

**Depends on**: nothing.
**Blocks**: PR 2–6 (they are content-only but the LLM needs the relaxed schema to fetch sub-paths).

### Tasks

- [ ] **1.1** Drop the `enum` constraint and add `minLength: 1`
  - Commit: `feat(bridge): relax reaforge_get_api_reference target to free-form string`
  - File: `tools/opencode_bridge.py:107-117` (4 lines removed, 1 line added)
  - ΔLOC: −3
  - Lane: `python-tdd`
  - Spec: REQ-BRIDGE-01, REQ-BRIDGE-02

- [ ] **1.2** Replace `description` with the sub-path prefix list
  - Commit: *(same commit as 1.1)*
  - File: `tools/opencode_bridge.py:113` (single-field rewrite; same commit because diff is in the same tool block)
  - ΔLOC: +11 (expanding the multi-line `description`)
  - Lane: `python-tdd`
  - Spec: REQ-BRIDGE-04
  - Verify description contains `jsfx-primitives/`, `jsfx-algorithms/`, `fx_chain-primitives/`

- [ ] **1.3** Add sub-path smoke assertion (`jsfx-primitives/*` returns non-empty markdown)
  - Commit: `test(bridge): assert sub-path keys reach the extension`
  - File: `tools/run_bridge_smoke.sh` (add 3 new `call_tool` invocations + `assert`s)
  - ΔLOC: +30
  - Lane: `python-tdd`
  - Coverage: (a) sub-path key returns 2xx, (b) unknown key returns 400 with `INVALID_TARGET`, (c) existing 3 top-level keys still return their cheatsheets

- [ ] **1.4** Verify no Python list/set on the bridge side mirrors the old enum
  - Commit: `chore(bridge): assert no client-side target allowlist`
  - File: `tools/opencode_bridge.py` (add a `grep` pre-test in smoke script, no new code in the module)
  - ΔLOC: +2
  - Lane: `python-tdd`
  - Spec: REQ-BRIDGE-02, REQ-BRIDGE-03

- [ ] **1.5** Run `tools/run_bridge_smoke.sh` and confirm 7/7 tools + 3 new sub-path assertions pass
  - Commit: *(no code change; verification step)*
  - Lane: `python-tdd`
  - Acceptance: `ALL_PASS` in the script output; existing 4/4 C++ tests in `src/host/tests/` untouched

---

## PR 2 — Saturation primitives (lane: `data` + `runtime-reaper`)

**Scope**: 3 L2 primitive files under `docs/api_reference/jsfx-primitives/saturation/`. Each follows the schema in `specs/dsp-primitive-format/spec.md` (REQ-FMT-01..05). LLM drafts from FAUST `saturation.lib` → human validates in REAPER.

**Depends on**: PR 1 merged.
**Blocks**: PR 5, PR 6 (the matrix needs the saturation rows).

### Tasks

- [ ] **2.1** Create `docs/api_reference/jsfx-primitives/saturation/tanh-soft-clip.md`
  - Commit: `feat(primitives): add tanh-soft-clip saturation primitive`
  - File: new, ~35 lines (matches the design §2 example verbatim)
  - ΔLOC: +35
  - Lane: `data`
  - Spec: REQ-FMT-01, REQ-FMT-02, REQ-FMT-03, REQ-FMT-05
  - Source: FAUST `saturation.lib` `aa_saturator` (MIT)
  - Validation: paste code block in REAPER, feed 440 Hz sine at -12 dB, expect THD ~ 3% at drive=1 / ~ 12% at drive=4 (recorded in `<!-- test: ... -->`)

- [ ] **2.2** Create `docs/api_reference/jsfx-primitives/saturation/asymmetric-tanh.md`
  - Commit: `feat(primitives): add asymmetric-tanh saturation primitive`
  - File: new, ~40 lines (12 lines JSFX + tables + source)
  - ΔLOC: +40
  - Lane: `data`
  - Source: FAUST `saturation.lib` `aa_clipboard` (MIT)
  - Validation: feed sine, expect even harmonics (2nd > 3rd); default `bias=0.1`

- [ ] **2.3** Create `docs/api_reference/jsfx-primitives/saturation/hard-clip-knee.md`
  - Commit: `feat(primitives): add hard-clip-knee saturation primitive`
  - File: new, ~38 lines (11 lines JSFX + tables + source)
  - ΔLOC: +38
  - Lane: `data`
  - Source: musicdsp.org "soft knee hard clip" (Public Domain)
  - Validation: feed sine at drive=2, expect 0 dBFS ceiling, no aliasing at 2x Nyquist

- [ ] **2.4** Audit: no file contains the substring `)MD`
  - Commit: `chore(primitives): audit saturation files for raw-string delimiter collision`
  - File: 3 files (verification only, no edit)
  - ΔLOC: 0
  - Lane: `data`
  - Design §2 invariant: `R"MD(...)MD"` embedding is verbatim; a `)MD` inside any file breaks compilation

- [ ] **2.5** Manually validate all 3 primitives in REAPER (compile + audio test)
  - Commit: *(no code change; reviewer sign-off in PR description)*
  - Lane: `runtime-reaper`
  - Acceptance: 3/3 paste-into-New-JSFX compiles; 3/3 audio tests match the `<!-- test: ... -->` expectations

---

## PR 3 — Filter primitives (lane: `data` + `runtime-reaper`)

**Scope**: 3 L2 primitive files under `docs/api_reference/jsfx-primitives/filters/`. RBJ cookbook + one-pole LP.

**Depends on**: PR 1 merged (no code dep on PR 2; can ship in parallel, but sequenced for review cadence).
**Blocks**: PR 5, PR 6.

### Tasks

- [ ] **3.1** Create `docs/api_reference/jsfx-primitives/filters/rbj-lowpass.md`
  - Commit: `feat(primitives): add rbj-lowpass filter primitive`
  - File: new, ~50 lines (18 lines JSFX + tables + source)
  - ΔLOC: +50
  - Lane: `data`
  - Source: RBJ Audio EQ Cookbook (Public Domain)
  - Spec: REQ-FMT-01..05

- [ ] **3.2** Create `docs/api_reference/jsfx-primitives/filters/rbj-highpass.md`
  - Commit: `feat(primitives): add rbj-highpass filter primitive`
  - File: new, ~50 lines
  - ΔLOC: +50
  - Lane: `data`
  - Source: RBJ Audio EQ Cookbook (Public Domain)
  - Validation: feed white noise, sweep cutoff; verify -3 dB at slider1

- [ ] **3.3** Create `docs/api_reference/jsfx-primitives/filters/one-pole-lowpass.md`
  - Commit: `feat(primitives): add one-pole-lowpass filter primitive`
  - File: new, ~30 lines (6 lines JSFX + tables + source)
  - ΔLOC: +30
  - Lane: `data`
  - Source: musicdsp.org "single-pole LP" (Public Domain)
  - Validation: impulse response decays exponentially; cutoff = slider1/(2π·srate)

- [ ] **3.4** Audit `)MD` substring absence across 3 files
  - Commit: `chore(primitives): audit filter files for raw-string delimiter collision`
  - ΔLOC: 0
  - Lane: `data`

- [ ] **3.5** Manually validate all 3 primitives in REAPER
  - Lane: `runtime-reaper`
  - Acceptance: 3/3 compile; cutoff sweep matches -3 dB at slider1 within 0.5 dB

---

## PR 4 — Delay + utility primitives (lane: `data` + `runtime-reaper`)

**Scope**: 6 L2 primitive files. 3 under `delays/`, 3 under `utilities/`. The utility primitives are the building blocks every other PR's `## Compatibility` block references.

**Depends on**: PR 1 merged.
**Blocks**: PR 5 (the FDN reverb and modulated-delay algorithms require `denormal-prevention` and `dc-block` to compose cleanly); PR 6 (matrix rows for utilities).

### Tasks

- [ ] **4.1** Create `docs/api_reference/jsfx-primitives/delays/feedback-delay.md`
  - Commit: `feat(primitives): add feedback-delay delay primitive`
  - File: new, ~45 lines (14 lines JSFX + tables + source)
  - ΔLOC: +45
  - Lane: `data`
  - Source: musicdsp.org "feedback delay" (Public Domain)
  - Spec: REQ-FMT-01..05

- [ ] **4.2** Create `docs/api_reference/jsfx-primitives/delays/ping-pong-delay.md`
  - Commit: `feat(primitives): add ping-pong-delay delay primitive`
  - File: new, ~55 lines (22 lines JSFX + tables + source)
  - ΔLOC: +55
  - Lane: `data`
  - Source: musicdsp.org "ping-pong delay" (Public Domain)
  - Validation: feed mono impulse; verify L delay = feedback·R delay (phase check)

- [ ] **4.3** Create `docs/api_reference/jsfx-primitives/delays/modulated-delay.md`
  - Commit: `feat(primitives): add modulated-delay delay primitive`
  - File: new, ~52 lines (19 lines JSFX + tables + source)
  - ΔLOC: +52
  - Lane: `data`
  - Source: musicdsp.org "chorus base" (Public Domain)
  - Note: REQ-MTX-03 mandatory rule — "modulated delay MUST sit AFTER any DC-coupled stage"

- [ ] **4.4** Create `docs/api_reference/jsfx-primitives/utilities/dc-block.md`
  - Commit: `feat(primitives): add dc-block utility primitive`
  - File: new, ~25 lines (5 lines JSFX + tables + source)
  - ΔLOC: +25
  - Lane: `data`
  - Source: standard first-order HP @ ~5 Hz (Public Domain)
  - Validation: feed +5V offset; verify output mean → 0 within 100 ms

- [ ] **4.5** Create `docs/api_reference/jsfx-primitives/utilities/stereo-width.md`
  - Commit: `feat(primitives): add stereo-width utility primitive`
  - File: new, ~30 lines (7 lines JSFX + tables + source)
  - ΔLOC: +30
  - Lane: `data`
  - Source: classic M/S width (Public Domain)

- [ ] **4.6** Create `docs/api_reference/jsfx-primitives/utilities/denormal-prevention.md`
  - Commit: `feat(primitives): add denormal-prevention utility primitive`
  - File: new, ~22 lines (3 lines JSFX — `denorm = 1e-25;` is the whole primitive)
  - ΔLOC: +22
  - Lane: `data`
  - Source: REAPER forum canonical workaround (Public Domain)
  - Note: REQ-MTX-03 mandatory rule — insert at top of `@init` before any feedback loop

- [ ] **4.7** Audit `)MD` substring across 6 files
  - ΔLOC: 0
  - Lane: `data`

- [ ] **4.8** Manually validate all 6 primitives in REAPER
  - Lane: `runtime-reaper`
  - Acceptance: 6/6 compile; ping-pong L/R phase check passes; dc-block removes +5V offset

---

## PR 5 — Algorithms (lane: `data` + `runtime-reaper`)

**Scope**: 4 L3 algorithm files under `docs/api_reference/jsfx-algorithms/<category>/`. Each composes primitives from PRs 2–4. Per the proposal, picks 4 non-FFT algorithms (R2 mitigation).

**Depends on**: PRs 2, 3, 4 merged (algorithms use primitives).
**Blocks**: PR 6 (matrix rules + catalog rows).

### Tasks

- [ ] **5.1** Create `docs/api_reference/jsfx-algorithms/dynamics/rms-compressor.md`
  - Commit: `feat(algorithms): add rms-compressor algorithm`
  - File: new, ~110 lines (80 lines JSFX + tables + source)
  - ΔLOC: +110
  - Lane: `data`
  - Source: FAUST `compressor.lib` (MIT)
  - Spec: REQ-FMT-04 (algorithm uses identical schema as primitives)
  - Validation: feed pink noise at -6 dBFS; at threshold=-12 ratio=4, expect 3 dB compression

- [ ] **5.2** Create `docs/api_reference/jsfx-algorithms/reverb/fdn-reverb.md`
  - Commit: `feat(algorithms): add fdn-reverb algorithm`
  - File: new, ~150 lines (120 lines JSFX + tables + source)
  - ΔLOC: +150
  - Lane: `data`
  - Source: FAUST `reverb.lib` + CCRMA Hadamard FDN (MIT)
  - Note: MUST list `utilities/denormal-prevention` under `## Compatibility → Requires:` (REB-FMT-02 + REQ-MTX-03 mandatory rule)

- [ ] **5.3** Create `docs/api_reference/jsfx-algorithms/pitch/psola-pitch-shift.md`
  - Commit: `feat(algorithms): add psola-pitch-shift algorithm`
  - File: new, ~180 lines (150 lines JSFX + tables + source)
  - ΔLOC: +180
  - Lane: `data`
  - Source: STK `PitchShifter` (MIT-like)
  - Note: highest-complexity algorithm in Phase 1; windowed overlap-add (non-FFT per R2 mitigation)

- [ ] **5.4** Create `docs/api_reference/jsfx-algorithms/modulation/chorus-flanger.md`
  - Commit: `feat(algorithms): add chorus-flanger algorithm`
  - File: new, ~85 lines (60 lines JSFX + tables + source)
  - ΔLOC: +85
  - Lane: `data`
  - Source: musicdsp.org "chorus/flanger" (Public Domain)
  - Note: parameter sweep covers chorus → flanger via depth slider

- [ ] **5.5** Audit `)MD` substring across 4 files
  - ΔLOC: 0
  - Lane: `data`

- [ ] **5.6** Manually validate all 4 algorithms in REAPER
  - Lane: `runtime-reaper`
  - Acceptance: 4/4 compile; one audio test per algorithm matches the `<!-- test: ... -->` expectation

---

## PR 6 — Compatibility matrix + DLL rebuild (lane: `data` + `build`)

**Scope**: hand-author `docs/api_reference/00-index.md`, create `tools/gen_dsp_index.py`, extend `tools/gen_api_reference_header.py` `SOURCES` + `CONST_NAMES` with 17 new entries, regenerate header, cross-compile DLL, copy to `<REAPER>/UserPlugins/`. Final PR — closes the loop.

**Depends on**: PRs 1, 2, 3, 4, 5 merged.
**Blocks**: nothing (this is the change's final state).

### Tasks

- [ ] **6.1** Create `tools/gen_dsp_index.py`
  - Commit: `feat(tools): add gen_dsp_index.py for compatibility matrix regeneration`
  - File: new, ~150 lines
  - ΔLOC: +150
  - Lane: `data`
  - Behavior: scan `jsfx-primitives/` and `jsfx-algorithms/`, parse `## Compatibility`, write `00-index.md` (catalog + matrix + rules + order). Idempotent (byte-identical on re-run).
  - Spec: REQ-MTX-05
  - Hard failure: exit non-zero if any file missing `## Compatibility` (REQ-FMT-02 enforced at build time)

- [ ] **6.2** Hand-author `docs/api_reference/00-index.md` initial draft
  - Commit: `feat(encyclopedia): add 00-index.md compatibility matrix`
  - File: new, ~100 lines (catalog + matrix + rules + order)
  - ΔLOC: +100
  - Lane: `data`
  - Spec: REQ-MTX-01..04
  - Note: 16-row catalog; matrix covers saturation × filter, saturation × delay, filter × delay intersections
  - After this commit, `gen_dsp_index.py` must reproduce it byte-for-byte (REQ-MTX-05 + design §4.2)

- [ ] **6.3** Extend `SOURCES` + `CONST_NAMES` in `gen_api_reference_header.py`
  - Commit: `feat(tools): add 16 primitive/algorithm sources to header generator`
  - File: `tools/gen_api_reference_header.py:20-39` (add 16 entries to each dict)
  - ΔLOC: +35
  - Lane: `data`
  - Key list: `jsfx-primitives/saturation/{tanh-soft-clip,asymmetric-tanh,hard-clip-knee}`, `jsfx-primitives/filters/{rbj-lowpass,rbj-highpass,one-pole-lowpass}`, `jsfx-primitives/delays/{feedback-delay,ping-pong-delay,modulated-delay}`, `jsfx-primitives/utilities/{dc-block,stereo-width,denormal-prevention}`, `jsfx-algorithms/{dynamics/rms-compressor,reverb/fdn-reverb,pitch/psola-pitch-shift,modulation/chorus-flanger}`, `00-index`
  - Total: 17 new entries (16 primitive/algorithm + 1 index)

- [ ] **6.4** Add `key_to_const_name(key)` helper to `gen_api_reference_header.py`
  - Commit: *(same commit as 6.3 — refactor opportunity)*
  - File: `tools/gen_api_reference_header.py` (new function, optional)
  - ΔLOC: +10
  - Lane: `data`
  - Note: makes future entries mechanical; `k` + PascalCase(key with separators stripped)

- [ ] **6.5** Run `python3 tools/gen_api_reference_header.py` and regenerate `api_reference_data.h`
  - Commit: `chore(host): regenerate api_reference_data.h with 17 new sources`
  - File: `src/host/api_reference_data.h` (497 → ~3,500 lines, auto)
  - ΔLOC: +3,000 (generated, **excluded from review**)
  - Lane: `build`
  - Verify: `R"MD(...)MD"` raw string literals compile; `api_reference_map().find()` returns all 17 new keys

- [ ] **6.6** Cross-compile: `ninja -C build-mingw`
  - Commit: *(no source change; the build artifact is the DLL in 6.7)*
  - Lane: `build`
  - Acceptance: ~2 min; zero warnings; new `api_reference_map()` size 22 (was 5)

- [ ] **6.7** Copy `reaper_reaforge_host.dll` to `<REAPER>/UserPlugins/`
  - Commit: *(no code change; out-of-repo binary handoff)*
  - Lane: `build`
  - Verify: DLL size 19 MB → ~21 MB (well under 25 MB ceiling per R5)

- [ ] **6.8** Re-run `tools/run_bridge_smoke.sh` with 3 new sub-path assertions
  - Commit: *(no code change; verification step)*
  - Lane: `build`
  - Acceptance: `ALL_PASS`; 7/7 tools round-trip; 3/3 sub-path assertions pass

- [ ] **6.9** LLM composition test: prompt opencode with "tape saturation with subtle chorus"
  - Commit: *(no code change; manual verification)*
  - Lane: `data`
  - Acceptance: LLM fetches `jsfx-primitives/saturation/tanh-soft-clip` and `jsfx-algorithms/modulation/chorus-flanger`, composes them respecting the bold rules (DC block after saturation per REQ-MTX-03, modulated delay after DC block), produces working JSFX
  - This is the proposal's success criterion #4

---

## Cross-cutting concerns

### Test plan

| Test | PR | Lane | Description |
|------|----|------|-------------|
| `tools/run_bridge_smoke.sh` | 1, 6 | `python-tdd` | 7 tools round-trip + 3 sub-path assertions; 100% automated |
| `src/host/tests/test_http_server.cpp:430` | 1, 6 | `c++-gtest` | `INVALID_TARGET` 400 response for unknown keys (existing) |
| `src/host/tests/test_lookup_api_reference.cpp` | 6 | `c++-gtest` | All 22 keys return non-empty; unknown returns empty (existing) |
| Manual REAPER audio test | 2–5 | `runtime-reaper` | One validation per primitive + one per algorithm; criteria in `<!-- test: ... -->` comments |
| LLM composition test | 6 | `data` | "tape saturation with subtle chorus" prompt; expected 2-step chain respecting bold rules |

### Dependency graph

```
PR 1 (bridge enum) ──→ PR 2 (saturation) ──┐
                  ──→ PR 3 (filters)    ──┤
                  ──→ PR 4 (delays+util) ──┼──→ PR 5 (algorithms) ──→ PR 6 (matrix + DLL)
```

PRs 2, 3, 4 can ship in any order (no inter-deps). PR 5 waits on all three. PR 6 is the integrator.

### Generated artifacts (excluded from review per `work-unit-commits`)

- `src/host/api_reference_data.h` — regenerable from `gen_api_reference_header.py`
- `build-mingw/src/host/reaper_reaforge_host.dll` — regenerable from `ninja -C build-mingw`
- Reviewers audit the generator inputs (PR 5's algorithm files) and the generator logic, not the generator output

### Out of scope (deferred, do NOT add tasks)

- Level 4 references (RBJ cookbook prose, Smith DSP summary, DAFx index) → Phase 2
- Lua primitives (ReaScript workflow blocks) → Phase 2
- FFT-based algorithms (phase vocoder, spectral morphing, CDP8) → Phase 2
- Automated REAPER smoke tests → Phase 2
- Hot-reload of primitives → out of scope (DLL rebuild required)
- `summary=true` flag for tight context windows → Phase 2
- Per-category sub-indices (`jsfx-primitives/00-index.md`) → deferred unless catalog > 30
- Renaming the `target` parameter → rejected (breaking schema change)
