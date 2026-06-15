# Tasks: ReaForge Agentic MVP

> Commit-level breakdown for the 7 chained PRs. Every task carries a `lane:` tag. Each PR is independently shippable and reviewable in ≤60 min. The orchestrator's review-workload guard uses the per-PR ΔLOC totals in the table at the end.

## TL;DR

- **7 PRs**, chained, ordered PR1 → PR7. PR 1/2/3 = `docs` + `python-tdd` (safe first batch). PR 4/5/6 = C++ greenfield (`cpp-unit` + `runtime-reaper`). PR 7 = docs + manual verify.
- **Two spec corrections** carried into PR 4 and PR 6 (RefreshFXList, AddRemoveReaScript signature) — see the callout box in PR 4 / PR 6.
- **Lane is non-negotiable**: pure C++ (routing, JSON, FS) = `cpp-unit`; anything calling `reaper.*` = `runtime-reaper`. Strict TDD is `false` (Engram #79), so REAPER-bound code is verified manually in PR 7.
- **No single PR exceeds 400 LOC**. PR 4 and PR 5 are the tightest (~350 each) — they have explicit subgroup split notes if the diff grows.
- **C++ HTTP server, artifact writer, project reader, refresh hook, main-thread queue, and the 3 embedded API reference markdowns are all greenfield** — `src/host/` has zero HTTP code today (Engram #80).

## Spec corrections (must be reflected by sdd-apply)

| Spec claim | Reality | Where the correction lands |
|---|---|---|
| `refresh-protocol/spec.md` calls `reaper.RefreshFXList()` | **Not in the public REAPER SDK.** | **PR 6 / task 6.1**: use `Main_OnCommand(<rescan_id>, 0)`; if no command ID, return `warnings[]` asking the user to right-click FX Browser → "Scan for new plug-ins". |
| `bridge-tools-v2/spec.md` writes `reaper.AddRemoveReaScript(0, 0)` and `reaper.AddRemoveReaScript(1, path, true)` | Real signature: `int AddRemoveReaScript(bool add, int sectionID, const char* scriptfn, bool commit)`. Return value = new command ID (or `< 0` on failure). | **PR 4 / task 4.9**: register with `(true, 0, abs_path, true)`. Deregister all by re-registering each known path with `(false, 0, path, true)`. MVP registers only, never deregisters — document the trade-off. |

---

## Per-PR task breakdown

### PR 1 — Rewrite README for the agentic vision

**Lane:** `docs`
**Target PR size:** ~80 LOC
**Depends on:** nothing
**Skills used:** `cognitive-doc-design`

| # | Task | Files | ΔLOC | Lane | Commit subject |
|---|---|---|---|---|---|
| 1.1 | Rewrite `README.md`: lead paragraph, agentic vision, link to `docs/documentacion/`, drop multi-runtime SDK language, add a "Try it" section with the 3 acceptance prompts from `04-mvp-and-handoff.md`. | `README.md` | ~80 | `docs` | `docs: rewrite README for agentic vision` |

**Acceptance:**
- Lead paragraph explains ReaForge as an AI agent that generates REAPER artifacts (no "multi-runtime SDK" framing).
- Build/load section points to `docs/documentacion/`.
- 3 acceptance prompts are quoted verbatim in a "Try it" section.

---

### PR 2 — Bridge: replace 5 tools with 7 (TDD)

**Lane:** `python-tdd`
**Target PR size:** ~150 LOC
**Depends on:** PR 1 (cosmetic — could land in either order)
**Skills used:** TDD discipline

| # | Task | Files | ΔLOC | Lane | Commit subject |
|---|---|---|---|---|---|
| 2.1 | RED — write failing test asserting the bridge exposes exactly 7 `reaforge_*` tool names with the correct schemas. | `tests/test_bridge_tools.py` (new) | ~30 | `python-tdd` | `test(bridge): assert 7 reaforge_* tools exposed` |
| 2.2 | Remove the 5 pre-pivot tool registrations from the bridge (clean swap, no aliases). | `tools/opencode_bridge.py` | ~-30 | `python-tdd` | `refactor(bridge): remove 5 pre-pivot tools` |
| 2.3 | GREEN — add `reaforge_get_state` (POSTs to `/v1/state`). | `tools/opencode_bridge.py` | ~25 | `python-tdd` | `feat(bridge): add reaforge_get_state` |
| 2.4 | Add `reaforge_list_artifacts` (GET `/v1/artifacts`). | `tools/opencode_bridge.py` | ~20 | `python-tdd` | `feat(bridge): add reaforge_list_artifacts` |
| 2.5 | Add `reaforge_get_api_reference` (GET `/v1/api-reference?target=X`). | `tools/opencode_bridge.py` | ~20 | `python-tdd` | `feat(bridge): add reaforge_get_api_reference` |
| 2.6 | Add `reaforge_save_jsfx` (POST `/v1/save/jsfx`) — refuse if file exists, accept `overwrite=true`. | `tools/opencode_bridge.py` | ~25 | `python-tdd` | `feat(bridge): add reaforge_save_jsfx` |
| 2.7 | Add `reaforge_save_lua` (POST `/v1/save/lua`) — `register_action` default `false`. | `tools/opencode_bridge.py` | ~25 | `python-tdd` | `feat(bridge): add reaforge_save_lua` |
| 2.8 | Add `reaforge_save_fx_chain` (POST `/v1/save/fx-chain`). | `tools/opencode_bridge.py` | ~20 | `python-tdd` | `feat(bridge): add reaforge_save_fx_chain` |
| 2.9 | Add `reaforge_refresh` (POST `/v1/refresh`). | `tools/opencode_bridge.py` | ~15 | `python-tdd` | `feat(bridge): add reaforge_refresh` |
| 2.10 | Add `TODO` comment on the hardcoded `wsl-bridge.txt` path (`tools/opencode_bridge.py:25`); not blocking. | `tools/opencode_bridge.py` | ~1 | `docs` | `docs(bridge): TODO on wsl-bridge.txt path` |

**Acceptance:**
- `tools/run_bridge_smoke.sh` exits 0 against PR 3's stub (or a placeholder stub if PR 3 isn't merged yet — smoke checks the 7 tool names, not the wire format).
- `tests/test_bridge_tools.py` passes.
- The 5 pre-pivot tool names (`reaforge_list_tracks`, `reaforge_run_extension`, …) no longer appear anywhere in `tools/opencode_bridge.py`.

---

### PR 3 — Stub server: extend 5 endpoints to 7 (TDD)

**Lane:** `python-tdd`
**Target PR size:** ~150 LOC
**Depends on:** PR 2 (the bridge calls the new endpoints)
**Skills used:** TDD discipline

| # | Task | Files | ΔLOC | Lane | Commit subject |
|---|---|---|---|---|---|
| 3.1 | RED — write failing test for the 7 stub endpoints. | `tests/test_stub_endpoints.py` (new) | ~30 | `python-tdd` | `test(stub): assert 7 endpoints respond` |
| 3.2 | Remove the 5 pre-pivot stub endpoints. | `tools/stub_reaper_server.py` | ~-25 | `python-tdd` | `refactor(stub): remove 5 pre-pivot endpoints` |
| 3.3 | Add `GET /v1/state` returning a hardcoded fixture project. | `tools/stub_reaper_server.py` | ~20 | `python-tdd` | `feat(stub): add GET /v1/state` |
| 3.4 | Add `GET /v1/artifacts` returning a hardcoded list. | `tools/stub_reaper_server.py` | ~15 | `python-tdd` | `feat(stub): add GET /v1/artifacts` |
| 3.5 | Add `GET /v1/api-reference?target=X` returning a hardcoded markdown blob. | `tools/stub_reaper_server.py` | ~20 | `python-tdd` | `feat(stub): add GET /v1/api-reference` |
| 3.6 | Add `POST /v1/save/jsfx` writing to a fixture dir, honors `overwrite`. | `tools/stub_reaper_server.py` | ~25 | `python-tdd` | `feat(stub): add POST /v1/save/jsfx` |
| 3.7 | Add `POST /v1/save/lua` writing to a fixture dir, honors `register_action`. | `tools/stub_reaper_server.py` | ~25 | `python-tdd` | `feat(stub): add POST /v1/save/lua` |
| 3.8 | Add `POST /v1/save/fx-chain`. | `tools/stub_reaper_server.py` | ~20 | `python-tdd` | `feat(stub): add POST /v1/save/fx-chain` |
| 3.9 | Add `POST /v1/refresh` (no-op returning timestamp). | `tools/stub_reaper_server.py` | ~10 | `python-tdd` | `feat(stub): add POST /v1/refresh` |
| 3.10 | Update smoke to run end-to-end with 7/7 GO. | `tools/run_bridge_smoke.sh` | ~10 | `python-tdd` | `test(smoke): end-to-end 7/7 tools` |

**Acceptance:**
- `tests/test_stub_endpoints.py` passes.
- `tools/run_bridge_smoke.sh` exits 0 with `7/7 GO`.
- The 5 pre-pivot endpoint paths no longer exist in the stub.

---

### PR 4 — C++ greenfield: HTTP server + artifact writer

**Lane:** `cpp-unit` (routing/JSON/FS) + `runtime-reaper` (mkdir in REAPER resource dir, `AddRemoveReaScript`)
**Target PR size:** ~350 LOC
**Depends on:** PR 2, PR 3 (contract stable), meson wiring prerequisites
**Skills used:** `work-unit-commits` (tightest PR — keep commit subjects outcome-oriented)

| # | Task | Files | ΔLOC | Lane | Commit subject |
|---|---|---|---|---|---|
| 4.1 | Vendor `cpp-httplib` + `nlohmann/json` single headers. | `third_party/cpp-httplib/httplib.h` (new), `third_party/nlohmann/json.hpp` (new) | ~5 | `cpp-unit` | `chore(deps): vendor cpp-httplib and nlohmann/json` |
| 4.2 | Add `main_thread_queue` (`std::function` + `condition_variable` rendezvous, 5s timeout). Includes unit test. | `src/host/main_thread_queue.{h,cpp}` (new), `src/host/tests/test_main_thread_queue.cpp` (new) | ~70 | `cpp-unit` | `feat(host): main_thread_queue with rendezvous test` |
| 4.3 | RED — write failing test asserting the 7 routes dispatch correctly + JSON envelope on bad input. | `src/host/tests/test_http_server.cpp` (new) | ~60 | `cpp-unit` | `test(host): routing contract for 7 endpoints` |
| 4.4 | Implement `http_server` skeleton: cpp-httplib server on `0.0.0.0:7800` (env override `REAFORGE_PORT`), 1 worker thread, 5s timeouts, 404 for unknown routes, `enum class ErrorCode` shared with tests. | `src/host/http_server.{h,cpp}` (new) | ~80 | `cpp-unit` | `feat(host): http_server skeleton with 404 fallback` |
| 4.5 | Add `GET /v1/health` → `200 {"ok":true,"version":"0.1.0"}`. | `src/host/http_server.cpp` | ~10 | `cpp-unit` | `feat(host): GET /v1/health` |
| 4.6 | RED — write failing test for `artifact_writer` (name regex, mkdir, atomic write, overwrite gate, path-safety). | `src/host/tests/test_artifact_writer.cpp` (new) | ~60 | `cpp-unit` | `test(host): artifact_writer contract` |
| 4.7 | GREEN — implement `artifact_writer` (name regex `^[A-Za-z0-9_\-]{1,64}$`, `weakly_canonical` path-safety, `create_directories`, `tmp + rename` atomic write, forward-slash JSON paths, `FILE_EXISTS` on dup without `overwrite`). | `src/host/artifact_writer.{h,cpp}` (new) | ~80 | `cpp-unit` | `feat(host): artifact_writer with name regex and atomic write` |
| 4.8 | Wire `POST /v1/save/jsfx` and `POST /v1/save/fx-chain` to `artifact_writer`. | `src/host/http_server.cpp` | ~25 | `cpp-unit` | `feat(host): wire /v1/save/jsfx and /v1/save/fx-chain` |
| 4.9 | Wire `POST /v1/save/lua` to `artifact_writer` (write only). | `src/host/http_server.cpp`, `src/host/artifact_writer.cpp` | ~15 | `cpp-unit` | `feat(host): wire /v1/save/lua (write only)` |
| 4.10 | Add `register_action` flow using the **real** `AddRemoveReaScript(bool add, int sectionID, const char* scriptfn, bool commit)` signature: register with `(true, 0, abs_path, true)`. MVP does **not** deregister — return `new_id` if `> 0`, else `REGISTER_FAILED`. Document trade-off in code comment + README. | `src/host/artifact_writer.cpp` | ~20 | `runtime-reaper` | `feat(host): register_action via real AddRemoveReaScript` |
| 4.11 | Update `src/host/meson.build` and root `meson.build`: add 5 new sources + vendor includes, add 2 test() targets (`test_http_server`, `test_artifact_writer`). Keep `panel.cpp` / `context_menu.cpp` / `bridge_sources` / `runtime_sources` for now (dead code, archived post-MVP). | `src/host/meson.build`, `meson.build` | ~25 | n/a (build) | `build(host): wire http_server, artifact_writer, tests` |

**Acceptance:**
- `ninja -C build` builds clean.
- `meson test -C build` passes `test_http_server` and `test_artifact_writer`.
- `test_http_server` covers all 7 routes + 405 + 404 + 400 (bad JSON) + 400 (bad name) + 409 (duplicate without overwrite).
- `test_artifact_writer` covers name regex, mkdir, atomic rename leaves no `.tmp`, overwrite gate.
- PR is at or under 400 lines diff (orchestrator guard).

**Risk note:** Tasks 4.1–4.5 form a self-contained "routing skeleton" subgroup (~225 LOC, pure `cpp-unit`). Tasks 4.6–4.10 form the "writer" subgroup (~200 LOC, mostly `cpp-unit` with one `runtime-reaper` task). If the diff is tight, the subgroups can become follow-up chained PRs.

**Spec correction reminder:** task 4.10 uses the real `AddRemoveReaScript` signature, **not** what `bridge-tools-v2/spec.md` wrote. The spec text stays authoritative for the contract; the C++ implementation matches the SDK.

---

### PR 5 — C++ greenfield: project reader (3 Read endpoints + embedded API reference)

**Lane:** `cpp-unit` (list_artifacts, embedded payload) + `runtime-reaper` (`get_state` via REAPER API)
**Target PR size:** ~350 LOC
**Depends on:** PR 4 (`http_server` exists)
**Skills used:** `work-unit-commits`

| # | Task | Files | ΔLOC | Lane | Commit subject |
|---|---|---|---|---|---|
| 5.1 | Author 3 API reference cheatsheets (hand-curated, 50–200 lines each). | `docs/api_reference/{jsfx,reascript_lua,fx_chain_format}.md` (new) | ~250 (data) | n/a (data) | `docs(api-reference): add 3 cheatsheets` |
| 5.2 | Generate `api_reference_data.h` from the 3 markdowns (`xxd -i` one-shot, committed). | `src/host/api_reference_data.h` (new, generated) | ~250 (data) | n/a (data) | `chore(host): generate api_reference_data.h` |
| 5.3 | Implement `project_reader::list_artifacts` (`<filesystem>` non-recursive over the 3 subfolders, returns `{path, size, mtime, kind}`; empty array if folder missing). | `src/host/project_reader.{h,cpp}` (new) | ~50 | `cpp-unit` | `feat(host): project_reader::list_artifacts` |
| 5.4 | Implement `project_reader::get_api_reference` (reads from `api_reference_data.h` arrays, `unordered_map<string,string>` built at `host::init()`). | `src/host/project_reader.cpp` | ~30 | `cpp-unit` | `feat(host): project_reader::get_api_reference` |
| 5.5 | Wire `GET /v1/artifacts` and `GET /v1/api-reference?target=X` to `project_reader`. | `src/host/http_server.cpp` | ~25 | `cpp-unit` | `feat(host): wire /v1/artifacts and /v1/api-reference` |
| 5.6 | Implement `project_reader::get_state` via REAPER API (`CountTracks`, `GetTrack`, `GetTrackName`, `TrackFX_GetCount`, `TrackFX_GetFXName`, `GetProjectName`, BPM/sample rate via `GetSetProjectInfo`, selected items count), marshaled to main thread. | `src/host/project_reader.cpp` | ~80 | `runtime-reaper` | `feat(host): project_reader::get_state (REAPER API)` |
| 5.7 | Wire `GET /v1/state?summary=<bool>` to `project_reader`; `summary=true` omits `fx_names[]` per track. | `src/host/http_server.cpp` | ~20 | `runtime-reaper` | `feat(host): wire /v1/state with summary flag` |
| 5.8 | Update `src/host/meson.build`: add `project_reader.cpp` to `reaper_reaforge_host` sources and to `test_http_server` sources. | `src/host/meson.build` | ~5 | n/a (build) | `build(host): add project_reader` |

**Acceptance:**
- `ninja -C build` builds clean.
- `meson test -C build` passes (routing + writer + reader where pure-C++).
- `get_state` is `runtime-reaper` only — verified manually in PR 7.

---

### PR 6 — C++ greenfield: refresh hook

**Lane:** `runtime-reaper` (no `cpp-unit` parts)
**Target PR size:** ~100 LOC
**Depends on:** PR 4 (`artifact_writer` for `register_action` path), PR 5 (`http_server` has all routes wired)

| # | Task | Files | ΔLOC | Lane | Commit subject |
|---|---|---|---|---|---|
| 6.1 | Implement `refresh::run` using `Main_OnCommand(<rescan_id>, 0)` (look up via `NamedCommandLookup("_S&M_REFRESH")` or REAPER-native equivalent). **Spec correction:** do **not** call `reaper.RefreshFXList()` — it is not in the public REAPER SDK. If no command ID is found, log warning + continue. | `src/host/refresh.{h,cpp}` (new) | ~50 | `runtime-reaper` | `feat(host): refresh via Main_OnCommand` |
| 6.2 | Wire `POST /v1/refresh` to `refresh::run`. Return `200 {refreshed_at: <ISO8601>, warnings: [...]}`. The `warnings[]` array must include the message that **JSFX rescan requires manual FX Browser → "Scan for new plug-ins"** — that limitation is real and out of scope to fix. | `src/host/http_server.cpp` | ~30 | `runtime-reaper` | `feat(host): wire /v1/refresh with warnings` |
| 6.3 | Update `src/host/meson.build`: add `refresh.cpp` to `reaper_reaforge_host` sources. | `src/host/meson.build` | ~5 | n/a (build) | `build(host): add refresh` |

**Acceptance:**
- `ninja -C build` builds clean.
- Endpoint returns 200 with `{refreshed_at, warnings: ["JSFX rescan requires manual FX Browser → Scan for new plug-ins"]}`.
- Idempotent: 3 calls in a row do not duplicate Action List entries (verified in PR 7).

---

### PR 7 — End-to-end verify on REAPER Windows

**Lane:** `runtime-reaper` (manual)
**Target PR size:** ~110 LOC (docs) + manual verify
**Depends on:** PR 1–6 merged

| # | Task | Files | ΔLOC | Lane | Commit subject |
|---|---|---|---|---|---|
| 7.1 | Add "End-to-end testing" section to `README.md` with the 3 acceptance prompts and the troubleshooting note about JSFX manual rescan. | `README.md` | ~40 | n/a (docs) | `docs: end-to-end testing section in README` |
| 7.2 | Add a "Troubleshooting" section: "JSFX not showing up" → "click FX Browser → Scan for new plug-ins (or restart REAPER)". | `README.md` | ~20 | n/a (docs) | `docs: troubleshooting for FX rescan` |
| 7.3 | Run the 3 acceptance prompts on REAPER Windows. Capture screenshots. Verify no REAPER crash. | n/a | n/a | `runtime-reaper` | (no commit; manual verify) |
| 7.4 | Rename `bridge-spike-results.md` → `mvp-results.md`; update with the 3/3 GO verifications and any screenshots. | `mvp-results.md` (new) | ~50 | n/a (docs) | `docs: MVP results — 3/3 acceptance prompts` |

**Acceptance (the MVP's success criterion, verbatim from `04-mvp-and-handoff.md`):**

> The user opens opencode Desktop with the ReaForge bridge configured, types one of the example prompts, and within 2 minutes a working JSFX / Lua / RfxChain file appears in the right REAPER folder and works when used.

- [ ] *"Generate a JSFX that does soft tape saturation"* → `Effects/ReaForge/tape_saturation.jsfx` exists, audibly saturates when added to a track, no REAPER crash.
- [ ] *"Write a Lua script that doubles the velocity of selected MIDI notes"* → `Scripts/ReaForge/double_velocity.lua` exists, runs as a REAPER action, modifies the notes.
- [ ] *"Combine the built-in delay and ReaEQ into a vocal slap chain"* → `FXChains/ReaForge/vocal_slap.RfxChain` exists, loads as a chain.

---

## Per-PR diff size summary (orchestrator's review workload input)

| PR | Lane | ΔLOC | vs 400-line budget |
|---|---|---|---|
| 1 | `docs` | ~80 | ✅ safe |
| 2 | `python-tdd` | ~150 | ✅ safe |
| 3 | `python-tdd` | ~150 | ✅ safe |
| 4 | `cpp-unit` + `runtime-reaper` | ~350 | ⚠️ tight — subgroup split note in body |
| 5 | `cpp-unit` + `runtime-reaper` | ~350 | ⚠️ tight — `api_reference_data.h` is generated data, not reviewable code |
| 6 | `runtime-reaper` | ~100 | ✅ safe |
| 7 | `runtime-reaper` (manual) | ~110 (docs) | ✅ safe |
| **Total** | — | **~1290** | — |

**Chained PRs recommended: YES.** No single PR exceeds 400 lines on its own. PR 4 and PR 5 are the tight ones — both have explicit subgroup split notes for a follow-up chained PR if the diff grows.

## Apply strategy notes (for sdd-apply)

- **PR 1, 2, 3 = safest first batch** (docs + 2 Python TDD PRs). Each builds and verifies independently with `pytest` + `tools/run_bridge_smoke.sh`. Land in this order.
- **PR 4, 5, 6 = C++ greenfield**, sequential and dependent. Apply 4 → 5 → 6 in order, running `ninja -C build && meson test -C build` between each.
- **PR 7 = docs + manual verify**, lands after the user's runtime smoke is green.
- Per session pre-flight (`ask-always`), the orchestrator will pause before `sdd-apply` to confirm the chain strategy (`stacked-to-main` vs `feature-branch-chain`). This is an orchestrator concern, not a task concern.

## Open design questions resolved by user (do not re-ask)

Folder convention = `<REAPER resource>/{Effects,Scripts,FXChains}/ReaForge/` (canonical, native REAPER subfolders). Fase 1a verify is SKIPPED (panel/context-menu deferred to phase +2/+3). Bridge tool naming is `reaforge_*` (explicit, not `rf_*`). `register_action` defaults to `false` in `save_lua` (opt-in, less surprising). `get_api_reference` ships EMBEDDED docs (offline-first, no network dep). `save_*` REFUSES to overwrite without `overwrite=true` (explicit flag, safer default).

## Anti-patterns to avoid (re-state)

Do **not** re-embed Lua / QuickJS / JSFX runtimes in the DLL (explicitly rejected by the pivot). Do **not** add tools that mutate live REAPER state in this MVP (`open_automation`, `set_parameter`, `play`, `stop` are out). Do **not** build a chat UI inside REAPER first (deferred to phase +3). Do **not** bypass `overwrite=true` silently — the flag exists for safety, enforce in bridge. Do **not** treat the C++ HTTP server as a refactor — **it is greenfield** (Engram #80). Do **not** sneak the Fase 0 in-DLL bridge back. Do **not** mix `cpp-unit` and `runtime-reaper` in the same test target — REAPER-bound code is runtime-verified in PR 7.

## Next phase

`sdd-apply` will start the implementation. The orchestrator's review-workload guard will re-check the per-PR ΔLOC vs the 400-line budget before each PR. Per session pre-flight (`ask-always`), the orchestrator pauses before `sdd-apply` to confirm the chain strategy (`stacked-to-main` vs `feature-branch-chain`) — orchestrator concern, not a task concern. After PR 7 passes the 3 acceptance prompts, `sdd-archive` moves the 3 archived specs (`multi-runtime`, `extension-execution`, `runtime-bridge`) to `openspec/specs/archive/` with `REJECTED.md`.
