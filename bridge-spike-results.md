# opencode-bridge spike results

## What ran

- `tools/stub_reaper_server.py` (Python HTTP on 127.0.0.1:7800, hardcoded state)
- `tools/opencode_bridge.py` (MCP server, stdio)
- `tools/run_bridge_smoke.sh` drives the official `mcp` Python client

## Tool output

```
TOOLS: reaforge_health,reaforge_get_state,reaforge_list_tracks,reaforge_list_extensions,reaforge_run_extension
OK: reaforge_health
OK: reaforge_list_tracks
OK: reaforge_list_extensions
OK: reaforge_run_extension
OK: reaforge_get_state
ALL_PASS
```

## Verdict

**GO** — all 5 tools round-trip through the MCP server, the stub answers, and the bridge translates HTTP<->MCP correctly. Latency is sub-millisecond locally; cross-env latency is a separate measurement and depends on the WSL IP path.
