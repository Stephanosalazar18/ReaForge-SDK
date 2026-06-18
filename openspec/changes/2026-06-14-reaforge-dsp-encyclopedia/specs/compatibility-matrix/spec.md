# Compatibility Matrix Specification

## Purpose

Define the content and structure of `docs/api_reference/00-index.md`, the FIRST markdown the LLM fetches before composing primitives. The index is the design guard rail: it forces the LLM to pick compatible primitives, follow mandatory pre/post rules, and respect the canonical signal-chain order.

## Required Sections (In Order)

| # | Section | Purpose |
|---|---|---|
| 1 | `# ReaForge DSP Encyclopedia — Index` | H1 title |
| 2 | `## Primitive Catalog` | Table: every primitive + target key |
| 3 | `## Compatibility Matrix` | Table: primitives × primitives, ✓/✗/△ cells |
| 4 | `## Mandatory Composition Rules` | Bold rules the LLM MUST obey |
| 5 | `## Recommended Signal Chain Order` | Numbered category ordering with rationale |

## Requirements

### Requirement: Catalog Lists Every Primitive (REQ-MTX-01)

The `## Primitive Catalog` MUST contain a markdown table with one row per Phase 1 entry (12 L2 + 4 L3 = 16 rows). Columns: Category, Primitive, Target Key, One-line description (≤ 15 words). Target keys MUST match the canonical names declared in `tools/gen_api_reference_header.py` `SOURCES`.

#### Scenario: LLM composes tape saturation + chorus from the catalog

- GIVEN the LLM has read `00-index.md`
- WHEN the user requests "tape saturation with subtle chorus"
- THEN the LLM finds `saturation#tanh-soft` (target `jsfx-primitives/saturation/tanh-soft`) AND `delays#modulated-delay` (target `jsfx-primitives/delays/modulated-delay`)
- AND calls `reaforge_get_api_reference` on both

### Requirement: Compatibility Matrix As Markdown Table (REQ-MTX-02)

`## Compatibility Matrix` MUST be a markdown table whose both axes are primitive names (✓ = composes cleanly, ✗ = conflicts, △ = conditional with footnote). The table MUST include at minimum the saturation × filter, saturation × delay, and filter × delay intersections.

#### Scenario: incompatible pair visible as ✗

- GIVEN primitive A (output writes state X) and primitive B (reads state X)
- WHEN the LLM looks up the A → B cell
- THEN it sees ✗ (or △ with a documented workaround)
- AND picks a different primitive

### Requirement: Mandatory Rules In Bold (REQ-MTX-03)

`## Mandatory Composition Rules` MUST list every MUST-follow rule the LLM would otherwise skip. Rules MUST be bold (`**...**`) so they survive summarization. Phase 1 rule set: **DC block AFTER any saturation primitive with default drive > 2.0**, **one-pole lowpass BEFORE nonlinearities when smoothing cascaded saturation**, **modulated delay MUST sit AFTER any DC-coupled stage**, **RMS compressor MUST follow gain staging; the detector does NOT see raw input**.

#### Scenario: rule forces prerequisite insertion

- GIVEN the LLM composes `tanh-soft` (default drive 3.0) → `feedback-delay`
- WHEN it re-reads the rules
- THEN it inserts `utilities/dc-block` between the two primitives per the bold DC blocking rule

### Requirement: Explicit Category Order (REQ-MTX-04)

`## Recommended Signal Chain Order` MUST be a numbered list with category name + 1-line rationale per entry. Phase 1 order: `utilities` (clean signal first) → `filters` (shape spectrum before nonlinearities) → `saturation` (controlled harmonics) → `dynamics` (control level after coloration) → `delays` (time-based after dynamics) → `reverb` (final spatial layer) → `modulation` (dry/wet wrapper, last) → `pitch` (order-independent, near the end).

#### Scenario: LLM orders an unfamiliar chain

- GIVEN the LLM must build "vocal warmth" (saturation + EQ + delay + reverb)
- WHEN it consults the order list
- THEN it sequences: `filters` (EQ) → `saturation` → `delays` → `reverb`

### Requirement: Index Regenerates When Primitives Change (REQ-MTX-05)

A regeneration step MUST add new primitives to the catalog and matrix automatically. The existing `gen_api_reference_header.py` already iterates `SOURCES`; the index regeneration MAY be a separate script (`tools/gen_dsp_index.py`) walking the same keys. It MUST be runnable via a single command from the repo root.

#### Scenario: new primitive appears after regen

- GIVEN a new file `jsfx-primitives/saturation/tube-ish.md` is added AND `SOURCES` is extended with its key
- WHEN `tools/gen_dsp_index.py` runs
- THEN `00-index.md` shows the new primitive in the catalog table
- AND adds a new row + column in the matrix (default cells = △ with footnote "verify")
- AND `git diff` shows no other file changed

## Out of Scope

Lua primitives index (Phase 2), per-category sub-indices beyond the two `00-index.md` files noted in the dsp-primitive-format spec, auto-generating matrix cells from primitive metadata (hand-authored for Phase 1).

## Affected PRs

| PR | Lane | What |
|---|---|---|
| 6 | review | Hand-author `00-index.md`, `jsfx-primitives/00-index.md`, `jsfx-algorithms/00-index.md`; add `tools/gen_dsp_index.py` |