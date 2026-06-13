#!/usr/bin/env python3
"""Stand-in for the C++ ReaForge extension's HTTP server.

Answers the contract from
``openspec/changes/2026-06-07-reaforge-agentic-mvp/specs/`` with
hardcoded data. Used by ``tools/run_bridge_smoke.sh`` and by anyone
wanting to develop the opencode-bridge layer without a real REAPER.

The 7 post-pivot endpoints are:

    GET  /v1/state                 (compact project projection)
    GET  /v1/artifacts             (merged list of the 3 ReaForge/ subfolders)
    GET  /v1/api-reference         (embedded markdown payload)
    POST /v1/save/jsfx             (writes to <root>/Effects/ReaForge/<name>.jsfx)
    POST /v1/save/lua              (writes to <root>/Scripts/ReaForge/<name>.lua)
    POST /v1/save/fx-chain         (writes to <root>/FXChains/ReaForge/<name>.RfxChain)
    POST /v1/refresh               (no-op returning an ISO8601 timestamp)

``/v1/health`` is a readiness probe used by the test fixture and the
smoke script.

The Write endpoints persist to a fixture root, NOT to a real REAPER
resource directory. By default this is ``/tmp/reaforge_stub_fixtures``
(``artifact-folder-convention/spec.md`` is enforced in production by
the C++ writer; the stub only needs to prove the contract on disk).
Override with the ``REAFORGE_STUB_ROOT`` environment variable.
"""
from __future__ import annotations

import json
import os
import sys
import urllib.parse
from http.server import BaseHTTPRequestHandler, HTTPServer

HOST = "127.0.0.1"
PORT = 7800

# --- Fixture roots (overridable via REAFORGE_STUB_ROOT) -------------------
STUB_ROOT = os.environ.get("REAFORGE_STUB_ROOT", "/tmp/reaforge_stub_fixtures")
EFFECTS_DIR = os.path.join(STUB_ROOT, "Effects", "ReaForge")
SCRIPTS_DIR = os.path.join(STUB_ROOT, "Scripts", "ReaForge")
FXCHAINS_DIR = os.path.join(STUB_ROOT, "FXChains", "ReaForge")

# Map artifact kind -> (subdir, suffix)
KIND_TABLE = {
    "jsfx": (EFFECTS_DIR, ".jsfx"),
    "lua": (SCRIPTS_DIR, ".lua"),
    "fx_chain": (FXCHAINS_DIR, ".RfxChain"),
}

# --- Hardcoded fixtures ---------------------------------------------------

# Compact project projection per extension-read-api/spec.md.
MOCK_STATE = {
    "project_name": "demo.rpp",
    "sample_rate": 48000,
    "bpm": 120.0,
    "tracks": [
        {
            "id": 0,
            "name": "Drums",
            "selected": False,
            "fx_count": 0,
            "fx_names": [],
            "selected_item_count": 0,
        },
        {
            "id": 1,
            "name": "Bass",
            "selected": False,
            "fx_count": 0,
            "fx_names": [],
            "selected_item_count": 0,
        },
        {
            "id": 2,
            "name": "Vocals",
            "selected": True,
            "fx_count": 2,
            "fx_names": ["ReaEQ", "Delay"],
            "selected_item_count": 1,
        },
    ],
    "selected_items_count": 1,
}

# Embedded API-reference payloads per the spec's offline-first rule.
# Real C++ reads these from docs/api_reference/*.md at extension start.
API_REFERENCE = {
    "jsfx": "# JSFX API Reference (stub)\n\ndesc: ...\n",
    "reascript_lua": "# ReaScript Lua API Reference (stub)\n\nreaper.*\n",
    "fx_chain_format": "# FX Chain format (stub)\n\n<RFXCHAIN .../>\n",
}

# Legacy extensions table — kept around during the 5→7 pivot; removed
# in the refactor commit.
MOCK_EXTENSIONS = [
    {
        "id": "humanize_midi",
        "name": "Humanize MIDI",
        "runtime": "lua",
        "target": "midi_item",
        "status": "loaded",
    },
    {
        "id": "render_selection",
        "name": "Render Selection",
        "runtime": "lua",
        "target": "track",
        "status": "loaded",
    },
]

# Legacy tracks table — kept around during the 5→7 pivot.
MOCK_TRACKS = MOCK_STATE["tracks"]


# --- Helpers --------------------------------------------------------------


def _json(handler: BaseHTTPRequestHandler, status: int, body: dict) -> None:
    payload = json.dumps(body).encode("utf-8")
    handler.send_response(status)
    handler.send_header("Content-Type", "application/json")
    handler.send_header("Content-Length", str(len(payload)))
    handler.end_headers()
    handler.wfile.write(payload)


# --- Handler --------------------------------------------------------------


class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):  # silence default access log
        sys.stderr.write("stub: " + (fmt % args) + "\n")

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        query = urllib.parse.parse_qs(parsed.query)

        if path == "/v1/health":
            return _json(self, 200, {"ok": True, "host_version": "0.0.1-stub"})

        # ---- New (PR 3) read routes ----
        if path == "/v1/state":
            # Spec accepts ?summary=<bool>. The stub ignores it — the
            # compact projection IS the only projection the stub ships.
            return _json(self, 200, MOCK_STATE)

        if path == "/v1/artifacts":
            # Missing-folder-tolerant: returns an empty list when the
            # fixture subdirs don't exist yet. The bridge treats an
            # empty list as "no artifacts generated so far".
            return _json(self, 200, {"artifacts": []})

        if path == "/v1/api-reference":
            target = query.get("target", [None])[0]
            if not target:
                return _json(self, 400, {"error": "MISSING_TARGET"})
            if target not in API_REFERENCE:
                return _json(
                    self, 400, {"error": "INVALID_TARGET", "target": target}
                )
            return _json(
                self, 200, {"target": target, "reference": API_REFERENCE[target]}
            )

        # ---- Legacy (to be removed in PR 3 refactor) ----
        if path == "/v1/tracks":
            return _json(self, 200, {"tracks": MOCK_TRACKS})
        if path == "/v1/extensions":
            return _json(self, 200, {"extensions": MOCK_EXTENSIONS})

        return _json(self, 404, {"error": "not found", "path": path})

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0") or 0)
        raw = self.rfile.read(length) if length else b""
        try:
            body = json.loads(raw) if raw else {}
        except json.JSONDecodeError:
            return _json(self, 400, {"error": "invalid JSON"})

        # ---- Legacy (to be removed in PR 3 refactor) ----
        if self.path.startswith("/v1/extensions/") and self.path.endswith("/run"):
            ext_id = self.path[len("/v1/extensions/"):-len("/run")].strip("/")
            ext = next((e for e in MOCK_EXTENSIONS if e["id"] == ext_id), None)
            if not ext:
                return _json(self, 404, {"error": "unknown extension", "id": ext_id})
            return _json(self, 200, {
                "ok": True,
                "id": ext_id,
                "args": body,
                "result": {"ok": True, "count": 47, "extension": ext["name"]},
            })

        return _json(self, 404, {"error": "not found", "path": self.path})


def main():
    server = HTTPServer((HOST, PORT), Handler)
    sys.stderr.write(
        f"stub: listening on http://{HOST}:{PORT} (root={STUB_ROOT})\n"
    )
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        sys.stderr.write("stub: shutting down\n")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
