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
        return [
            Tool(
                name="reaforge_health",
                description="Check that the ReaForge extension is reachable and report its version.",
                inputSchema={"type": "object", "properties": {}},
            ),
            Tool(
                name="reaforge_get_state",
                description="Return the full REAPER project state: tracks, tempo, cursor, and the extension registry.",
                inputSchema={"type": "object", "properties": {}},
            ),
            Tool(
                name="reaforge_list_tracks",
                description="List the tracks in the current project, with volume, pan, and mute status.",
                inputSchema={"type": "object", "properties": {}},
            ),
            Tool(
                name="reaforge_list_extensions",
                description="List the ReaForge extensions currently loaded.",
                inputSchema={"type": "object", "properties": {}},
            ),
            Tool(
                name="reaforge_run_extension",
                description="Invoke a ReaForge extension by id with the given JSON args.",
                inputSchema={
                    "type": "object",
                    "properties": {
                        "id": {"type": "string", "description": "Extension id (e.g. 'humanize_midi')"},
                        "args": {
                            "type": "object",
                            "description": "JSON args to pass to the extension",
                            "additionalProperties": True,
                        },
                    },
                    "required": ["id"],
                },
            ),
        ]

    @server.call_tool()
    async def call_tool(name: str, arguments: dict[str, Any]) -> list[TextContent]:
        try:
            if name == "reaforge_health":
                r = client.get("/v1/health")
            elif name == "reaforge_get_state":
                r = client.get("/v1/state")
            elif name == "reaforge_list_tracks":
                r = client.get("/v1/tracks")
            elif name == "reaforge_list_extensions":
                r = client.get("/v1/extensions")
            elif name == "reaforge_run_extension":
                ext_id = arguments.get("id")
                if not ext_id:
                    return [TextContent(type="text", text=json.dumps({"error": "missing id"}))]
                args = arguments.get("args", {})
                r = client.post(f"/v1/extensions/{ext_id}/run", json=args)
            else:
                return [TextContent(type="text", text=json.dumps({"error": "unknown tool", "name": name}))]
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
