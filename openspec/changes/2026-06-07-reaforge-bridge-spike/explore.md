# Explore: opencode-bridge spike (2026-06-07)

## Context

The cross-env architecture for Fase 5 (see `docs/cross-environment.md`) has four actors:

```
REAPER (.dll, HTTP :7800)  ←  opencode-bridge (Python MCP)  ←  opencode serve (:4096)  ←  opencode desktop (UI)
```

We have empirical evidence for the leftmost actor (the C++ extension compiles and the spike passes its benchmark). The middle two actors — the bridge and opencode — are still design-only. This spike implements a minimal end-to-end version with the REAPER side replaced by a Python stub, and validates that:

1. The HTTP server contract on `:7800` is enough to drive an MCP server.
2. The MCP server can register tools and answer `call_tool` requests.
3. The full flow works against `opencode` (or its MCP client) without a real REAPER.

## What we are NOT validating in this spike

- The C++ extension's real HTTP server (Fase 1a/5 implementation; Fase 1a is still in FAIL pending REAPER Windows).
- Real REAPER API calls (we mock the values).
- The cross-env WSL↔Windows path (this spike runs everything in WSL on localhost; the design in `cross-environment.md` covers the cross-env part).

## What we ARE validating

- The MCP tool schema and naming convention are right.
- The bridge discovers REAPER via HTTP health probe.
- The bridge can be launched with `mcp run tools/opencode_bridge.py` and consumed by an MCP client.
- The end-to-end tool call latency is acceptable.

## Tool surface (initial)

| MCP tool | HTTP equivalent | Notes |
|---|---|---|
| `reaforge_health` | `GET /v1/health` | Liveness check |
| `reaforge_list_tracks` | `GET /v1/tracks` | Track list |
| `reaforge_list_extensions` | `GET /v1/extensions` | Extension registry |
| `reaforge_run_extension` | `POST /v1/extensions/{id}/run` | Invoke by id, JSON args |
| `reaforge_get_state` | `GET /v1/state` | Full project state |

## Constraints

- Spike lives under `tools/`; no C++ changes.
- No SDD spec files; this is a 1-day validation spike, not a feature.
- Bridge is `mcp[cli]`-compatible (we just installed it) so it runs under `mcp run` directly.
