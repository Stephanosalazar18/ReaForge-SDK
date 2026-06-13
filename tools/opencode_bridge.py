#!/usr/bin/env python3
"""MCP server that exposes ReaForge tools to opencode.

The bridge translates MCP tool calls into HTTP requests against the
ReaForge extension's HTTP/JSON API (port 7800 by default). The tool
schemas follow MCP conventions: snake_case names, JSON Schema for
inputs, structured content for outputs.
"""
from __future__ import annotations

import json
import os
import sys
from typing import Any

import httpx

from mcp.server import Server
from mcp.server.stdio import stdio_server
from mcp.types import TextContent, Tool

REAPER_BASE_URL = os.environ.get("REAPER_BASE_URL", "http://127.0.0.1:7800")
WINDOWS_BRIDGE_FILE = os.environ.get(
    "WSL_BRIDGE_FILE",
    "/mnt/c/Users/stephano/AppData/Roaming/REAPER/ReaForge/wsl-bridge.txt",
)


def discover_reaper_base() -> str:
    """If running in WSL and the wsl-bridge.txt file exists, use that.
    Otherwise, fall back to the default localhost REAPER_BASE_URL.
    """
    try:
        with open(WINDOWS_BRIDGE_FILE) as f:
            line = f.read().strip()
            if line:
                host, _, port = line.partition(":")
                if host and port:
                    sys.stderr.write(f"bridge: discovered REAPER at {host}:{port}\n")
                    return f"http://{host}:{port}"
    except FileNotFoundError:
        pass
    sys.stderr.write(f"bridge: using default REAPER at {REAPER_BASE_URL}\n")
    return REAPER_BASE_URL


def make_server() -> Server:
    base = discover_reaper_base()
    server = Server("reaforge-bridge")
    client = httpx.Client(base_url=base, timeout=10.0)

    @server.list_tools()
    async def list_tools() -> list[Tool]:
        # MVP tool surface is 7 tools: 3 Read + 3 Write + 1 Refresh.
        # The pre-pivot 5-tool surface (health, list_tracks, list_extensions,
        # run_extension, old get_state) was removed in the agentic pivot.
        # See openspec/changes/2026-06-07-reaforge-agentic-mvp/specs/bridge-tools-v2/spec.md
        return []

    @server.call_tool()
    async def call_tool(name: str, arguments: dict[str, Any]) -> list[TextContent]:
        try:
            r = None  # filled in by the tool branches below
            if False:  # placeholder until the 7 tools are added in 2.3-2.9
                pass
            else:
                return [TextContent(type="text", text=json.dumps({"error": "unknown tool", "name": name}))]
            assert r is not None
            r.raise_for_status()
            body = r.text
        except httpx.HTTPError as e:
            body = json.dumps({"error": "http_error", "message": str(e), "tool": name})
        except Exception as e:
            body = json.dumps({"error": "bridge_error", "message": str(e), "tool": name})
        return [TextContent(type="text", text=body)]

    return server


async def main():
    server = make_server()
    async with stdio_server() as (read_stream, write_stream):
        await server.run(read_stream, write_stream, server.create_initialization_options())


if __name__ == "__main__":
    import asyncio
    asyncio.run(main())
