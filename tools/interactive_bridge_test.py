#!/usr/bin/env python3
"""Interactive REPL for the opencode-bridge spike.

Spawns the stub REAPER server, spawns the opencode-bridge as an MCP
server subprocess, and lets you call any of the 5 tools by name with
JSON args. Use this to feel the bridge without needing opencode itself.

Usage:
    python3 tools/interactive_bridge_test.py
    > list
    > reaforge_health
    > reaforge_run_extension {"id": "humanize_midi", "args": {}}
    > quit
"""
from __future__ import annotations

import asyncio
import json
import shlex
import sys
from pathlib import Path

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

HERE = Path(__file__).resolve().parent
STUB = HERE / "stub_reaper_server.py"
BRIDGE = HERE / "opencode_bridge.py"


def fmt(obj) -> str:
    return json.dumps(obj, indent=2, sort_keys=True)


async def repl():
    print("interactive_bridge_test: booting stub on 127.0.0.1:7800")
    stub_proc = await asyncio.create_subprocess_exec(
        sys.executable, str(STUB), stdout=asyncio.subprocess.DEVNULL, stderr=asyncio.subprocess.DEVNULL
    )
    await asyncio.sleep(0.4)
    try:
        bridge_params = StdioServerParameters(command=sys.executable, args=[str(BRIDGE)])
        async with stdio_client(bridge_params) as (read, write):
            async with ClientSession(read, write) as session:
                await session.initialize()
                tools = (await session.list_tools()).tools
                names = [t.name for t in tools]

                print(f"interactive_bridge_test: bridge up, {len(tools)} tools available")
                print("Type a tool name (optionally with JSON args) or 'list' / 'help' / 'quit'.")
                print()

                while True:
                    try:
                        raw = input("reaforge> ").strip()
                    except (EOFError, KeyboardInterrupt):
                        print()
                        break
                    if not raw:
                        continue
                    if raw in ("quit", "exit", ":q"):
                        break
                    if raw in ("help", "?"):
                        print("Available commands:")
                        print("  list                       show tool names + schemas")
                        print("  <tool_name> [json_args]    call a tool, args as a single JSON object")
                        print("  quit / exit                leave the REPL")
                        continue
                    if raw == "list":
                        for t in tools:
                            print(f"  {t.name}  -- {t.description}")
                            print(f"    schema: {fmt(t.inputSchema)}")
                        continue

                    parts = shlex.split(raw)
                    name = parts[0]
                    if name not in names:
                        print(f"unknown tool '{name}'. type 'list' to see what's available.")
                        continue
                    try:
                        args = json.loads(" ".join(parts[1:])) if len(parts) > 1 else {}
                    except json.JSONDecodeError as e:
                        print(f"invalid JSON args: {e}")
                        continue

                    try:
                        result = await session.call_tool(name, args)
                        for c in result.content:
                            try:
                                print(fmt(json.loads(c.text)))
                            except json.JSONDecodeError:
                                print(c.text)
                    except Exception as e:
                        print(f"call_tool failed: {e}")
    finally:
        stub_proc.terminate()
        try:
            await asyncio.wait_for(stub_proc.wait(), timeout=2.0)
        except asyncio.TimeoutError:
            stub_proc.kill()


if __name__ == "__main__":
    asyncio.run(repl())
