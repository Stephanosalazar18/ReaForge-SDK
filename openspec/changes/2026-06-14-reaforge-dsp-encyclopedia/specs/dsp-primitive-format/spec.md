# DSP Primitive Format Specification

## Purpose

Standardized markdown schema for every DSP primitive (L2) and algorithm (L3) file under `docs/api_reference/jsfx-primitives/` and `docs/api_reference/jsfx-algorithms/`. The LLM reads these files verbatim and composes their code blocks, so the schema MUST be uniform and copy-paste safe.

## Scope

| Level | Folder | JSFX lines | Phase 1 count |
|---|---|---|---|
| L2 primitives | `jsfx-primitives/<category>/<name>.md` | 5–30 | 12 |
| L3 algorithms | `jsfx-algorithms/<category>/<name>.md` | 50–300 | 4 |

## Required File Schema

Every primitive/algorithm file MUST contain these sections in order: `# <Name> (<Category>)`, `## Code` (fenced `jsfx` block), `## Parameters` (table: Slider / Range / Default / Description), `## Use Case` (≤ 4 sentences), `## Compatibility` (sub-bullets: Before / After / Requires / Conflicts), `## Source` (attribution + license id). HTML comments `<!-- test: ... -->` MAY appear for human reviewers and survive embedding unchanged.

## Requirements

### Requirement: Working Standalone JSFX (REQ-FMT-01)

Every primitive/algorithm file MUST include a fenced ` ```jsfx ` code block under `## Code` that compiles in REAPER's New JSFX editor with zero syntax errors. State variables and `slider<N>` declarations MUST live inside the `@init` / `@slider` / `@sample` blocks the file declares. The block MUST NOT depend on globals from other primitives.

#### Scenario: saturation primitive compiles standalone

- GIVEN `jsfx-primitives/saturation/tanh-soft.md` contains a ` ```jsfx ` block
- WHEN the code is pasted into REAPER's New JSFX editor and compiled
- THEN REAPER reports no syntax errors
- AND changing the `drive` slider audibly changes the saturation character

#### Scenario: algorithm declares all three sections

- GIVEN `jsfx-algorithms/dynamics/rms-compressor.md`
- WHEN the file is opened
- THEN it contains `@init`, `@slider`, and `@sample` blocks (none missing)

### Requirement: Compatibility Block (REQ-FMT-02)

Every primitive/algorithm file MUST declare composition constraints under `## Compatibility` with four sub-bullets: `Before`, `After`, `Requires`, `Conflicts`. `Requires` MUST list at least one rule OR the literal `none`. A primitive MAY list zero entries for the other three.

#### Scenario: saturated primitive requires DC block

- GIVEN a saturation primitive with `drive` default > 2.0
- WHEN the file is read
- THEN `## Compatibility → Requires:` mandates DC blocking AFTER the primitive

#### Scenario: two compatible primitives compose without glitches

- GIVEN A lists B under `After:` AND B lists A under `Before:`
- WHEN both code blocks are concatenated inside a single `@sample`
- THEN the result processes audio without clicks, denormals, or NaN

### Requirement: Source Attribution With License (REQ-FMT-03)

Every primitive/algorithm file MUST include a `## Source` section with origin (FAUST library name, musicdsp.org URL, STK class), license id (`MIT`, `CC-BY`, `Public Domain`, `LGPL`), and adaptation date. Files without attribution MUST NOT be merged.

#### Scenario: FAUST-derived primitive cites FAUST

- GIVEN a primitive translated from `faustlibraries.grame.fr`
- WHEN the file is opened
- THEN `## Source` references the FAUST library name AND the license is `MIT`

### Requirement: Algorithm Files Share The Schema (REQ-FMT-04)

Algorithm files (L3) MUST use the identical section ordering as primitives. The only differences are longer code blocks (50–300 lines) and more `@init` state (delay lines / filter memories). No algorithm-specific sections are allowed.

#### Scenario: algorithm parameters table matches primitive shape

- GIVEN any `jsfx-algorithms/<cat>/<name>.md`
- WHEN the `## Parameters` table is parsed
- THEN it has the same 4 columns as a primitive file

### Requirement: HTML Test Comments (REQ-FMT-05)

A file MAY contain `<!-- test: ... -->` comments anywhere. `tools/gen_api_reference_header.py` MUST preserve them verbatim so human reviewers see the validation notes; the LLM MAY ignore them.

#### Scenario: test comment survives embedding

- GIVEN a primitive file contains `<!-- test: 440 Hz sine at -12 dB, expect 3rd harmonic at -38 dB -->`
- WHEN `gen_api_reference_header.py` regenerates `api_reference_data.h`
- THEN the comment appears unchanged in the embedded string literal

## Out of Scope

Multi-file primitives (one `.md` = one primitive), inline images, `[[wiki]]` cross-references (use the `Source` URL instead).

## Affected PRs

| PR | Lane | What |
|---|---|---|
| 2–5 | `runtime-reaper` | Primitive + algorithm files adopt this schema |
| 6 | review | `00-index.md` cross-references primitive ids from these files |