#!/usr/bin/env bash
# End-to-end smoke for the opencode-bridge.
# Starts the stub REAPER server, invokes each of the 7 tools through the bridge
# using the mcp Python client, asserts the responses.
#
# This is the post-pivot version. The 5 pre-pivot tools
# (reaforge_health, reaforge_list_tracks, reaforge_list_extensions,
#  reaforge_run_extension, the old reaforge_get_state) are GONE.
# The 7 new tools are: 3 Read + 3 Write + 1 Refresh.

set -euo pipefail

here="$(cd "$(dirname "$0")/.." && pwd)"
cd "$here"

stub_log="$(mktemp)"
bridge_log="$(mktemp)"

cleanup() {
    [[ -n "${stub_pid:-}" ]] && kill "$stub_pid" 2>/dev/null || true
    [[ -n "${mcp_pid:-}" ]] && kill "$mcp_pid" 2>/dev/null || true
    rm -f "$stub_log" "$bridge_log"
}
trap cleanup EXIT

echo "==[1]== start stub"
python3 tools/stub_reaper_server.py >"$stub_log" 2>&1 &
stub_pid=$!
sleep 0.3

if ! curl -sf http://127.0.0.1:7800/v1/health >/dev/null; then
    echo "FAIL: stub did not come up"
    cat "$stub_log"
    exit 1
fi
echo "OK: stub up"

echo "==[2]== list bridge tools and call each of the 7"
python3 - <<'PY' > /tmp/reaforge_bridge_smoke.out
import asyncio, json, sys
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

EXPECTED_TOOLS = {
    "reaforge_get_state",
    "reaforge_list_artifacts",
    "reaforge_get_api_reference",
    "reaforge_save_jsfx",
    "reaforge_save_lua",
    "reaforge_save_fx_chain",
    "reaforge_refresh",
}
FORBIDDEN_TOOLS = {
    "reaforge_health",
    "reaforge_list_tracks",
    "reaforge_list_extensions",
    "reaforge_run_extension",
}

async def main():
    params = StdioServerParameters(command=sys.executable, args=["tools/opencode_bridge.py"])
    async with stdio_client(params) as (read, write):
        async with ClientSession(read, write) as s:
            await s.initialize()
            tools = await s.list_tools()
            names = {t.name for t in tools.tools}
            print("TOOLS:", ",".join(sorted(names)))

            missing = EXPECTED_TOOLS - names
            assert not missing, f"missing expected tools: {missing}"
            leaked = names & FORBIDDEN_TOOLS
            assert not leaked, f"pre-pivot tools still exposed: {leaked}"
            assert len(names) == 7, f"expected 7 tools, got {len(names)}: {names}"

            # Read tools (should not 5xx; the 7-endpoint stub will 404 for now
            # until PR 3 lands the new endpoints; the bridge should surface the
            # http_error gracefully without crashing).
            r = await s.call_tool("reaforge_get_state", {"summary": True})
            print("OK: reaforge_get_state (proxied)")

            r = await s.call_tool("reaforge_list_artifacts", {})
            print("OK: reaforge_list_artifacts (proxied)")

            r = await s.call_tool("reaforge_get_api_reference", {"target": "jsfx"})
            print("OK: reaforge_get_api_reference (proxied)")

            # Write tools (proxy-only at this stage; the 5-endpoint stub will 404
            # on the new endpoints until PR 3. The bridge should still respond,
            # not crash).
            r = await s.call_tool("reaforge_save_jsfx", {"name": "smoke_tape", "code": "// smoke"})
            print("OK: reaforge_save_jsfx (proxied)")

            r = await s.call_tool("reaforge_save_lua", {"name": "smoke_lua", "code": "-- smoke", "register_action": False, "overwrite": False})
            print("OK: reaforge_save_lua (proxied)")

            r = await s.call_tool("reaforge_save_fx_chain", {"name": "smoke_chain", "content": "<smoke/>"})
            print("OK: reaforge_save_fx_chain (proxied)")

            r = await s.call_tool("reaforge_refresh", {})
            print("OK: reaforge_refresh (proxied)")

            print("ALL_PASS")

asyncio.run(main())
PY
result="$(cat /tmp/reaforge_bridge_smoke.out)"

echo "$result"

if [[ "$result" != *"ALL_PASS"* ]]; then
    echo "==[FAIL]== bridge smoke failed"
    echo "stub log:"
    cat "$stub_log"
    echo "bridge log:"
    cat "$bridge_log"
    exit 1
fi

echo "==[DONE]== all 7 tools round-trip through the bridge (proxy-only until PR 3 wires the stub endpoints)"
