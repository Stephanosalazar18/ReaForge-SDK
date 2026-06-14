# ReaForge MVP Results

This is the **runtime-verify record** for the ReaForge Agentic MVP. It is intentionally a **template** with placeholders — the agent that built the docs (PR 7) cannot run REAPER on a Windows machine, so the PASS/FAIL verdicts and the artifact timestamps must be filled in **by you, the user, after running the 3 acceptance prompts in [`README.md`](README.md#end-to-end-testing).**

The MVP is considered **shipped** only after every section below has a verdict. A "0/3" or "1/3" result is a real outcome — record it honestly and open an issue.

---

## Verify metadata

| Field | Value |
|---|---|
| Date run | _YYYY-MM-DD HH:MM (your local)_ |
| Run by | _your name / handle_ |
| REAPER version | _e.g. REAPER 7.20 / Windows 11_ |
| Host extension version | _commit SHA of the merged `reaper_reaforge_host.dll` you loaded_ |
| opencode version | _e.g. opencode Desktop 0.x_ |
| Bridge revision | _commit SHA of `tools/opencode_bridge.py` running_ |
| Bridge ↔ extension transport | _confirm `wsl-bridge.txt` had `<wsl-ip>:7800` (yes / no)_ |

---

## Prompt 1 — Soft tape saturation JSFX

**Prompt (verbatim, from `proposal.md`):**

> Generate a JSFX that does soft tape saturation

| Field | Value |
|---|---|
| File path | `Effects/ReaForge/tape_saturation.jsfx` |
| Timestamp generated | _YYYY-MM-DD HH:MM:SS_ |
| FX browser found it after | _manual "Scan for new plug-ins" / REAPER restart / etc._ |
| Audio / behavior observed | _describe what you heard when you added it to a track and played audio_ |
| REAPER crashed? | _yes / no — if yes, paste the crash log_ |
| Verdict | **PASS** / **FAIL** / **PARTIAL** |
| Notes | _anything else relevant_ |

---

## Prompt 2 — Double-velocity MIDI Lua

**Prompt (verbatim):**

> Write a Lua script that doubles the velocity of selected MIDI notes

| Field | Value |
|---|---|
| File path | `Scripts/ReaForge/double_velocity.lua` |
| Timestamp generated | _YYYY-MM-DD HH:MM:SS_ |
| `register_action` passed? | _yes / no_ |
| Action appeared in Action List? | _yes / no — if no, ran the script once via "Script: ReaForge/..." menu?_ |
| MIDI item used | _describe the item (e.g. a 4-bar piano clip with 8 notes)_ |
| Velocities before | _a few sample values, e.g. `note 3: vel=64, note 7: vel=80`_ |
| Velocities after | _same notes, e.g. `note 3: vel=128 → clamped to 127, note 7: vel=160 → clamped to 127`_ |
| REAPER crashed? | _yes / no_ |
| Verdict | **PASS** / **FAIL** / **PARTIAL** |
| Notes | _anything else relevant — for example, did the script need the optional `overwrite=true` flag because a previous attempt left a file there?_ |

---

## Prompt 3 — Vocal slap FX chain

**Prompt (verbatim):**

> Combine the built-in delay and ReaEQ into a vocal slap chain

| Field | Value |
|---|---|
| File path | `FXChains/ReaForge/vocal_slap.RfxChain` |
| Timestamp generated | _YYYY-MM-DD HH:MM:SS_ |
| Chain loaded as | _FX → Add FX chain → ReaForge → vocal_slap (yes / no)_ |
| FX order in the loaded chain | _e.g. ReaEQ → ReaDelay (slot 0, 1)_ |
| Audio / behavior observed | _describe what you heard when you applied it to a vocal track_ |
| REAPER crashed? | _yes / no_ |
| Verdict | **PASS** / **FAIL** / **PARTIAL** |
| Notes | _anything else relevant_ |

---

## Overall verdict

| Score | Mark this |
|---|---|
| **3/3 PASS** | _[ ]_ — MVP is shipped |
| 2/3 | _[ ]_ — MVP is _partially_ shipped; open an issue for the failing prompt |
| 1/3 | _[ ]_ — MVP is _not_ shipped; the C++ / bridge / refresh work needs investigation |
| 0/3 | _[ ]_ — something fundamental is broken (transport, function-pointer capture, WSL discovery) — open an issue with REAPER + opencode logs |

---

## Known issues / next steps

_Anything that worked but felt slow, fragile, or surprising. Anything that did not work and what you tried. This section feeds directly into the post-MVP roadmap (phase +1: richer context, +2: context menu, +3: in-REAPER chat)._

- _Example: "Prompt 1 took 90 s end-to-end, mostly the LLM thinking time. The actual write + refresh was < 2 s."_
- _Example: "The Lua action did not appear in the Action List until I restarted REAPER. SWS is installed but the rescan still did not fire — could be the SWS command ID we used is wrong, or SWS_REFRESH is not registered on first load."_

---

## Post-verify SDD follow-up (agent's job, not yours)

Once you have filled in the 3 verdicts above, the next SDD phase is `sdd-archive`:

1. Move the 3 archived specs (`multi-runtime`, `extension-execution`, `runtime-bridge`) from `openspec/specs/` to `openspec/specs/archive/` with a `REJECTED.md` per spec, per `tasks.md` line 229.
2. Mark the change `2026-06-07-reaforge-agentic-mvp` as **shipped** in `openspec/changes/` (the SDD toolchain handles this; the agent will run it).
3. The bridge spike doc `bridge-spike-results.md` (5/5 pre-pivot tools) is **distinct** from this file — it stays as a historical record. This `mvp-results.md` is the post-pivot MVP record.
