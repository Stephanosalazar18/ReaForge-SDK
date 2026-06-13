#!/usr/bin/env python3
"""Test the opencode_bridge MCP tool surface.

This test boots the bridge as a subprocess over stdio and asks the MCP
server to enumerate its tools. The contract comes from
`openspec/changes/2026-06-07-reaforge-agentic-mvp/specs/bridge-tools-v2/spec.md`
(the agentic MVP tool surface — 7 `reaforge_*` tools, 5→7 swap is a clean
replacement, not a 5+2 hybrid).
"""
from __future__ import annotations

import asyncio
import sys
from pathlib import Path

import pytest
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

HERE = Path(__file__).resolve().parent
BRIDGE = HERE.parent / "tools" / "opencode_bridge.py"

EXPECTED_TOOLS = {
    "reaforge_get_state",
    "reaforge_list_artifacts",
    "reaforge_get_api_reference",
    "reaforge_save_jsfx",
    "reaforge_save_lua",
    "reaforge_save_fx_chain",
    "reaforge_refresh",
}

PRE_PIVOT_TOOLS = {
    "reaforge_health",
    "reaforge_list_tracks",
    "reaforge_list_extensions",
    "reaforge_run_extension",
}


async def _list_tools() -> list[str]:
    """Boot the bridge over stdio and return its advertised tool names."""
    params = StdioServerParameters(command=sys.executable, args=[str(BRIDGE)])
    async with stdio_client(params) as (read, write):
        async with ClientSession(read, write) as session:
            await session.initialize()
            result = await session.list_tools()
            return [t.name for t in result.tools]


def test_bridge_exposes_exactly_seven_reaforge_tools():
    """The bridge MUST expose exactly 7 `reaforge_*` tools (no more, no less).

    Verifies the `Tool Surface is Exactly 7` requirement from
    `bridge-tools-v2/spec.md`. The bridge does not need REAPER to be
    running for `tools/list` — we never call any tool here.
    """
    names = asyncio.run(_list_tools())
    assert set(names) == EXPECTED_TOOLS, (
        f"bridge tool surface mismatch.\n"
        f"  expected: {sorted(EXPECTED_TOOLS)}\n"
        f"  got     : {sorted(names)}\n"
        f"  missing : {sorted(EXPECTED_TOOLS - set(names))}\n"
        f"  extra   : {sorted(set(names) - EXPECTED_TOOLS)}"
    )


def test_bridge_does_not_expose_pre_pivot_tools():
    """The 5 pre-pivot tools MUST be removed (clean swap, not 5+2 hybrid)."""
    names = set(asyncio.run(_list_tools()))
    leaked = names & PRE_PIVOT_TOOLS
    assert not leaked, f"pre-pivot tools still exposed: {sorted(leaked)}"


def test_bridge_tool_count_is_seven():
    """Sanity: the count is 7, not 5, not 12."""
    names = asyncio.run(_list_tools())
    assert len(names) == 7, f"expected 7 tools, got {len(names)}: {names}"
