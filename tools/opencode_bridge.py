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
        return [
            Tool(
                name="reaforge_get_state",
                description=(
                    "Return a compact projection of the current REAPER project: "
                    "project name, sample rate, BPM, per-track FX count and names, "
                    "and selected item count. Pass summary=true to omit per-track "
                    "fx_names for a smaller payload."
                ),
                inputSchema={
                    "type": "object",
                    "properties": {
                        "summary": {
                            "type": "boolean",
                            "description": "When true, omit per-track fx_names; only counts are returned.",
                            "default": False,
                        },
                    },
                },
            ),
            Tool(
                name="reaforge_list_artifacts",
                description=(
                    "List REAPER-native artifacts the agent can edit, grouped by "
                    "kind: 'jsfx' (Effects/ReaForge/*.jsfx), 'lua' "
                    "(Scripts/ReaForge/*.lua), 'fx_chain' (FXChains/ReaForge/*.RfxChain). "
                    "Omit kind to merge all three folders. A missing folder returns "
                    "an empty list for that kind (never an error)."
                ),
                inputSchema={
                    "type": "object",
                    "properties": {
                        "kind": {
                            "type": "string",
                            "enum": ["jsfx", "lua", "fx_chain"],
                            "description": "Filter to one folder. Omit to list all three.",
                        },
                    },
                },
            ),
            Tool(
                name="reaforge_get_api_reference",
                description=(
                    "Return the offline-bundled API reference markdown for one of "
                    "three targets: 'jsfx' (JSFX cheatsheet), 'reascript_lua' (REAPER "
                    "Lua API cheatsheet), 'fx_chain_format' (RfxChain XML format). "
                    "Use this to ground code generation — the payloads are static, "
                    "no network fetch."
                ),
                inputSchema={
                    "type": "object",
                    "properties": {
                        "target": {
                            "type": "string",
                            "enum": ["jsfx", "reascript_lua", "fx_chain_format"],
                            "description": "Which API reference markdown to return.",
                        },
                    },
                    "required": ["target"],
                },
            ),
            Tool(
                name="reaforge_save_jsfx",
                description=(
                    "Write a JSFX file into <REAPER>/Effects/ReaForge/<name>.jsfx. "
                    "Refuses to overwrite an existing file unless overwrite=true. "
                    "The extension creates the ReaForge/ subfolder on demand."
                ),
                inputSchema={
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "Base name without extension (regex ^[A-Za-z0-9_-]{1,64}$).",
                        },
                        "code": {
                            "type": "string",
                            "description": "JSFX source code (full file contents).",
                        },
                        "overwrite": {
                            "type": "boolean",
                            "description": "Set true to replace an existing file. Defaults to false.",
                            "default": False,
                        },
                    },
                    "required": ["name", "code"],
                },
            ),
            Tool(
                name="reaforge_save_lua",
                description=(
                    "Write a ReaScript Lua file into <REAPER>/Scripts/ReaForge/<name>.lua. "
                    "If register_action=true (opt-in), the extension also calls "
                    "reaper.AddRemoveReaScript to expose the script as a REAPER action "
                    "and returns the new action_id. If run_action=true (requires "
                    "register_action=true), the script is executed immediately via "
                    "Main_OnCommand so the user doesn't need to run it manually. "
                    "Refuses to overwrite unless overwrite=true."
                ),
                inputSchema={
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "Base name without extension (regex ^[A-Za-z0-9_-]{1,64}$).",
                        },
                        "code": {
                            "type": "string",
                            "description": "Lua source code (full file contents).",
                        },
                        "register_action": {
                            "type": "boolean",
                            "description": "Opt-in: register the script as a REAPER action. Defaults to false.",
                            "default": False,
                        },
                        "run_action": {
                            "type": "boolean",
                            "description": "After registering, execute the action immediately. Requires register_action=true.",
                            "default": False,
                        },
                        "overwrite": {
                            "type": "boolean",
                            "description": "Set true to replace an existing file. Defaults to false.",
                            "default": False,
                        },
                    },
                    "required": ["name", "code"],
                },
            ),
            Tool(
                name="reaforge_save_fx_chain",
                description=(
                    "Write a REAPER FX chain (.RfxChain) into "
                    "<REAPER>/FXChains/ReaForge/<name>.RfxChain. "
                    "Refuses to overwrite unless overwrite=true."
                ),
                inputSchema={
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "Base name without extension (regex ^[A-Za-z0-9_-]{1,64}$).",
                        },
                        "content": {
                            "type": "string",
                            "description": "RfxChain file contents (REAPER's chain DSL).",
                        },
                        "overwrite": {
                            "type": "boolean",
                            "description": "Set true to replace an existing file. Defaults to false.",
                            "default": False,
                        },
                    },
                    "required": ["name", "content"],
                },
            ),
            Tool(
                name="reaforge_refresh",
                description=(
                    "Ask the REAPER extension to rescan its FX list and Action List "
                    "so the newly-written artifacts are immediately visible in the REAPER UI. "
                    "Idempotent. Returns the timestamp of the refresh and any warnings "
                    "(e.g. JSFX rescan requires manual 'Scan for new plug-ins')."
                ),
                inputSchema={
                    "type": "object",
                    "properties": {},
                    "required": [],
                },
            ),
        ]

    @server.call_tool()
    async def call_tool(name: str, arguments: dict[str, Any]) -> list[TextContent]:
        try:
            if name == "reaforge_get_state":
                summary = bool(arguments.get("summary", False))
                r = client.get("/v1/state", params={"summary": "true" if summary else "false"})
            elif name == "reaforge_list_artifacts":
                kind = arguments.get("kind")
                params = {"kind": kind} if kind else None
                r = client.get("/v1/artifacts", params=params)
            elif name == "reaforge_get_api_reference":
                target = arguments.get("target")
                if not target:
                    return [TextContent(type="text", text=json.dumps({"error": "INVALID_TARGET", "message": "target is required", "target": target}))]
                r = client.get("/v1/api-reference", params={"target": target})
            elif name == "reaforge_save_jsfx":
                payload = {
                    "name": arguments.get("name"),
                    "code": arguments.get("code"),
                    "overwrite": bool(arguments.get("overwrite", False)),
                }
                r = client.post("/v1/save/jsfx", json=payload)
            elif name == "reaforge_save_lua":
                payload = {
                    "name": arguments.get("name"),
                    "code": arguments.get("code"),
                    "register_action": bool(arguments.get("register_action", False)),
                    "run_action": bool(arguments.get("run_action", False)),
                    "overwrite": bool(arguments.get("overwrite", False)),
                }
                r = client.post("/v1/save/lua", json=payload)
            elif name == "reaforge_save_fx_chain":
                payload = {
                    "name": arguments.get("name"),
                    "content": arguments.get("content"),
                    "overwrite": bool(arguments.get("overwrite", False)),
                }
                r = client.post("/v1/save/fx-chain", json=payload)
            elif name == "reaforge_refresh":
                r = client.post("/v1/refresh", json={})
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
