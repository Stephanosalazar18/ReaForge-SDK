# Proposal: opencode-bridge spike (Python stub)

## Intent

Validate the bridge layer of the Fase 5 stack by implementing it end-to-end against a Python stub that stands in for the C++ extension. The spike produces:

1. A Python stub HTTP server (`tools/stub_reaper_server.py`) that answers the contract from `docs/cross-environment.md` with hardcoded data.
2. An MCP server (`tools/opencode_bridge.py`) that registers 5 tools and proxies each to the stub.
3. A smoke script (`tools/run_bridge_smoke.sh`) that boots both, calls each tool via the MCP client (`mcp`-provided CLI), and asserts the responses.
4. A `bridge-spike-results.md` with the smoke output and a go/no-go for the bridge design.

## Scope

### In Scope

- `tools/stub_reaper_server.py` — ~80 LOC of Python `http.server` with handlers for `/v1/health`, `/v1/state`, `/v1/tracks`, `/v1/extensions`, `/v1/extensions/{id}/run`.
- `tools/opencode_bridge.py` — ~120 LOC of `mcp` server with 5 tools, each making a `httpx` call to the stub.
- `tools/run_bridge_smoke.sh` — starts the stub, then calls the bridge's tools through `mcp`-provided harness, asserts each.
- `bridge-spike-results.md` — final output.

### Out of Scope

- Any C++ change. The C++ extension's real HTTP server is Fase 1a/5 work.
- Real REAPER integration. We mock the values.
- The cross-env WSL↔Windows path. The spike runs on localhost in WSL.
- A proper HTTP client implementation in C++. Deferred to Fase 5.

## Approach

- The stub uses Python's stdlib `http.server.BaseHTTPRequestHandler` — zero deps.
- The bridge uses `mcp.server.Server` and `httpx` for HTTP calls. Both already installed in this environment.
- The smoke script uses `mcp`'s test client (or invokes the bridge as a subprocess and pipes JSON-RPC over stdio).
- All tools return JSON. The bridge is the only place that knows about MCP; the stub is plain HTTP.

## Affected Areas

| Area | Impact | Description |
|---|---|---|
| `tools/stub_reaper_server.py` | New | Python HTTP stub of the REAPER extension |
| `tools/opencode_bridge.py` | New | MCP server that proxies to the stub |
| `tools/run_bridge_smoke.sh` | New | End-to-end smoke test |
| `bridge-spike-results.md` | New | Output of the smoke |
| `docs/cross-environment.md` | Modify | Mark the bridge layer as "validated by spike" |

## Risks

| Risk | Likelihood | Mitigation |
|---|---|---|
| `mcp` Python API has changed and our usage is wrong | Low | We just installed `mcp 1.27.2`; the API is current |
| The bridge cannot run as a subprocess under `mcp run` | Low | Fall back to a hand-rolled JSON-RPC stdio harness |
| Tool schemas are too loose / too strict for opencode | Med | We make them strict and let the smoke test catch mismatches |

## Success Criteria

- [ ] Stub and bridge start without error.
- [ ] `reaforge_health` returns `{"ok": true}`.
- [ ] `reaforge_list_tracks` returns 4 mock tracks.
- [ ] `reaforge_list_extensions` returns 1 mock extension.
- [ ] `reaforge_run_extension` with id `humanize_midi` returns `{"ok": true, "count": 47}`.
- [ ] `bridge-spike-results.md` records the smoke output and a verdict.
- [ ] If go: the bridge is the model for Fase 5 implementation.

## Rollback Plan

Spike lives entirely under `tools/` and `bridge-spike-results.md`. No C++ code touched. `git revert` of the merge commit removes it.
