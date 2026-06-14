#!/usr/bin/env bash
# End-to-end smoke for the opencode-bridge.
# Starts the PR 3 stub REAPER server (1 readiness probe + 7 post-pivot
# endpoints), invokes each of the 7 tools through the bridge using the
# mcp Python client, asserts the stub actually answered with 2xx for
# every call.
#
# Pre-pivot tool names (reaforge_health, reaforge_list_tracks,
# reaforge_list_extensions, reaforge_run_extension) MUST NOT appear.
# The 5→7 swap is clean, not a 5+2 hybrid.

set -euo pipefail

here="$(cd "$(dirname "$0")/.." && pwd)"
cd "$here"

stub_log="$(mktemp)"

cleanup() {
    [[ -n "${stub_pid:-}" ]] && kill "$stub_pid" 2>/dev/null || true
    rm -f "$stub_log"
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
echo "OK: stub up (1 readiness probe + 7 post-pivot endpoints)"

echo "==[2]== call each of the 7 tools through the bridge and assert 2xx"
python3 - <<'PY' > /tmp/reaforge_bridge_smoke.out
import asyncio, json, sys, random
rand = random.randint(0, 999999)
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

            # Each call must come back with the success body the PR 3
            # stub ships. The smoke asserts the wire shape, not the
            # exact response — that's tests/test_stub_endpoints.py's job.
            results = []

            r = await s.call_tool("reaforge_get_state", {"summary": True})
            obj = json.loads(r.content[0].text)
            assert "project_name" in obj, obj
            assert "tracks" in obj, obj
            results.append(("reaforge_get_state", obj))
            print("OK: reaforge_get_state -> 2xx (compact projection)")

            r = await s.call_tool("reaforge_list_artifacts", {})
            obj = json.loads(r.content[0].text)
            assert "artifacts" in obj, obj
            results.append(("reaforge_list_artifacts", obj))
            print("OK: reaforge_list_artifacts -> 2xx (missing-folder-tolerant)")

            r = await s.call_tool("reaforge_get_api_reference", {"target": "jsfx"})
            obj = json.loads(r.content[0].text)
            assert obj.get("target") == "jsfx", obj
            assert isinstance(obj.get("reference"), str) and obj["reference"], obj
            results.append(("reaforge_get_api_reference", obj))
            print("OK: reaforge_get_api_reference -> 2xx (embedded markdown)")

            r = await s.call_tool("reaforge_save_jsfx", {"name": "" + str(rand) + "_tape", "code": "// smoke"})
            obj = json.loads(r.content[0].text)
            assert "path" in obj and "bytes_written" in obj, obj
            results.append(("reaforge_save_jsfx", obj))
            print("OK: reaforge_save_jsfx -> 2xx (wrote to fixture)")

            r = await s.call_tool("reaforge_save_lua", {"name": "" + str(rand) + "_lua", "code": "-- smoke", "register_action": False, "overwrite": False})
            obj = json.loads(r.content[0].text)
            assert "path" in obj, obj
            results.append(("reaforge_save_lua", obj))
            print("OK: reaforge_save_lua -> 2xx (wrote to fixture)")

            r = await s.call_tool("reaforge_save_fx_chain", {"name": "" + str(rand) + "_chain", "content": "<smoke/>"})
            obj = json.loads(r.content[0].text)
            assert "path" in obj and "bytes_written" in obj, obj
            results.append(("reaforge_save_fx_chain", obj))
            print("OK: reaforge_save_fx_chain -> 2xx (wrote to fixture)")

            r = await s.call_tool("reaforge_refresh", {})
            obj = json.loads(r.content[0].text)
            assert "refreshed_at" in obj, obj
            assert isinstance(obj.get("warnings"), list), obj
            results.append(("reaforge_refresh", obj))
            print("OK: reaforge_refresh -> 2xx (timestamped response)")

            assert len(results) == 7, f"expected 7 results, got {len(results)}"
            print("ALL_PASS")

asyncio.run(main())
PY
result="$(cat /tmp/reaforge_bridge_smoke.out)"

echo "$result"

if [[ "$result" != *"ALL_PASS"* ]]; then
    echo "==[FAIL]== bridge smoke failed"
    echo "stub log:"
    cat "$stub_log"
    exit 1
fi

echo "==[DONE]== 7/7 tools round-trip through the bridge with real 2xx from the stub"
