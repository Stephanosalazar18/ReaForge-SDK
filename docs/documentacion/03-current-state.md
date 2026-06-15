# ReaForge — Current Code State

Repository at HEAD: commit `7d36013` on branch `main`.

## File inventory and status

| Path | Purpose | Status after pivot |
|---|---|---|
| `src/host/` | C++ REAPER extension (host + panel + context menu + extension loader + sample) | **Survives shell.** HTTP server stub + lifecycle stays. Panel + context menu + extension loader pause until last phase. |
| `src/host/extensions/humanize_midi/` | Sample Lua extension shipped with Fase 1a | **Pre-pivot pattern.** Keeps working as a smoke. The "ship a sample extension" pattern is superseded by "agent generates on demand". |
| `tools/opencode_bridge.py` | MCP server with 5 tools, validated GO | **Pivots.** Skeleton (mcp + httpx + stdio) survives. The 5 tools (`reaforge_list_tracks`, `reaforge_run_extension`, etc.) get replaced by the 7 new tools (see [`04-mvp-and-handoff.md`](./04-mvp-and-handoff.md)). |
| `tools/stub_reaper_server.py` | Python HTTP stub of the extension | **Survives.** Will be extended to mock the 7 new tools. |
| `tools/run_bridge_smoke.sh` | End-to-end smoke (mcp client drives bridge) | **Survives.** Update for the new tool list. |
| `tools/interactive_bridge_test.py` | Interactive REPL over the bridge | **Survives.** Manual testing remains useful. |
| `bridge-spike-results.md` | Smoke output, verdict GO | **Historical artifact.** Do not modify. |
| `spike-results.md` | Multi-runtime spike result (FAIL pending) | **Historical.** Do not extend. |
| `scripts/install-build-deps.sh` | Toolchain installer | Survives. |
| `scripts/build-and-load-linux.sh` | Build + install on Linux | Survives. |
| `scripts/build-windows.sh` | Cross-build from WSL | Survives. |
| `scripts/run_benchmark.sh` | Multi-runtime benchmark | **Archive.** Not needed in agentic MVP. |
| `docs/cross-environment.md` | Cross-env transport architecture | **Survives as historical.** Transport layer (HTTP + `wsl-bridge.txt`) is still valid. The C++ extension surface described there will shrink. |
| `docs/user-flows.md` | Pre-pivot 3-flow model | **Superseded** by `docs/documentacion/01-vision-and-pivot.md`. Keep as historical. |
| `openspec/changes/2026-06-07-reaforge-bridge-spike/` | Bridge spike SDD | **Done, GO.** Will be archived once the agentic MVP supersedes the tool list. |
| `openspec/changes/2026-06-07-reaforge-fase1a-host/` | Fase 1a host SDD | **Merged but verify pending.** See "Open decision" below. |
| `openspec/specs/multi-runtime/` | Spec for multi-runtime host | **Archived (rejected by pivot).** Do not extend. |
| `openspec/specs/extension-execution/` | Spec for extension execution | **Archived.** Replaced by "agent writes files, REAPER executes natively". |
| `openspec/specs/runtime-bridge/` | Spec for the in-DLL runtime bridge | **Archived.** The bridge concept survives as the MCP bridge, not the in-DLL bridge. |
| `meson.build` | Build config for the C++ extension | Survives. |
| `third_party/` | Vendored deps (ReaImGui, etc.) | Survives. ReaImGui is for the deferred panel phase. |
| `README.md` | Top-level README | **Outdated.** Still describes the multi-runtime SDK vision. Needs a rewrite (next SDD change). |

## Commit history relevant to the pivot

| Commit | Description |
|---|---|
| `7d36013` | feat(scripts): install-build-deps, build-and-load-linux, build-windows |
| `dba4af5` | feat(tools): interactive REPL for the bridge spike |
| `583dbe9` | docs: bridge layer validated by spike |
| `b057a85` | feat(tools): opencode-bridge spike validates the bridge layer end-to-end |
| `895232a` | docs: clarify opencode serve and opencode desktop role |
| `9696f7d` | docs: cross-environment communication design |
| `6733ae9` | archive(spike): promote spike specs to main and archive the change |
| `9329472` | merge: Fase 1a chain (3 PRs + fixes) into main |
| `c8a13eb` | fix(host): link Lua with extern "C" and fix QuickJS global object leak |
| `a8d135e` | feat(fase1a)!: PR 1 foundation — rename spike to host, vendor ReaImGui |

The pivot happened **after** these commits. No code has been written for the new vision yet.

## Test status

| Test | State | Where |
|---|---|---|
| Bridge spike smoke (5/5 tools round-trip via MCP client) | **PASS** | `bridge-spike-results.md` |
| Multi-runtime spike build & smoke | **FAIL pending runtime verify** | `spike-results.md` |
| Fase 1a host runtime verify on REAPER | **FAIL pending** | `openspec/changes/2026-06-07-reaforge-fase1a-host/verify-report.md` |

## Build environment

| Platform | Toolchain | Status |
|---|---|---|
| WSL2 Ubuntu | `meson`, `lua5.4-dev`, `g++` installed via `scripts/install-build-deps.sh` | Working |
| Windows MSVC | Build cross-script exists (`scripts/build-windows.sh`) but the user typically builds natively on Windows | Untested in this session |
| macOS | `meson` + `brew install lua` | Untested |

## Open decision (ask user before MVP work starts)

**Fase 1a verify on REAPER Windows**: the merged Fase 1a code (panel + context menu + extension loader + sample) was never runtime-verified. Two paths:

1. **Verify before pivoting** — confirm the host loads in REAPER Windows, the panel opens, the sample runs. Then pivot. Gives confidence the C++ shell is sound.
2. **Skip and pivot directly** — the panel and context menu are deferred to the last phase anyway. The HTTP server + filesystem writes can be built and tested independently.

Path 2 is faster but leaves a known-uncertain artifact in `main`. Path 1 closes the loop.

## What survives, summarized

- Bridge MCP skeleton + httpx + stdio pattern (`tools/opencode_bridge.py` shape)
- Stub server pattern for harness (`tools/stub_reaper_server.py` shape)
- Smoke test pattern with `mcp` client (`tools/run_bridge_smoke.sh` shape)
- Cross-env HTTP transport + `wsl-bridge.txt` discovery
- C++ host shell (extension lifecycle, REAPER plugin registration)
- Build scripts and toolchain setup
- `meson.build`

## What gets archived (do not extend)

- Multi-runtime spike specs (`openspec/specs/{multi-runtime,extension-execution,runtime-bridge}/`)
- The 5 bridge tools as currently named (`reaforge_list_tracks`, `reaforge_run_extension`, …)
- Fase 1a "ship a sample extension" pattern (`src/host/extensions/humanize_midi/`)
- The 3-flow user model in `docs/user-flows.md`
- The multi-runtime sections of `README.md`
