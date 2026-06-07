#!/usr/bin/env python3
"""Stand-in for the C++ ReaForge extension's HTTP server.

Answers the contract from docs/cross-environment.md with hardcoded
data. Used by tools/run_bridge_smoke.sh and by anyone wanting to
develop the opencode-bridge layer without a real REAPER.
"""
from __future__ import annotations

import json
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer

HOST = "127.0.0.1"
PORT = 7800

MOCK_STATE = {
    "host_version": "0.0.1-stub",
    "project": "demo.rpp",
    "tracks": [
        {"index": 0, "name": "Drums",  "volume": 0.85, "pan": 0.0,  "muted": False},
        {"index": 1, "name": "Bass",   "volume": 0.78, "pan": 0.0,  "muted": False},
        {"index": 2, "name": "Keys",   "volume": 0.70, "pan": -0.2, "muted": False},
        {"index": 3, "name": "Vocals", "volume": 0.92, "pan": 0.1,  "muted": False},
    ],
    "tempo_bpm": 120.0,
    "cursor_seconds": 12.5,
}

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


def _json(handler: BaseHTTPRequestHandler, status: int, body: dict) -> None:
    payload = json.dumps(body).encode("utf-8")
    handler.send_response(status)
    handler.send_header("Content-Type", "application/json")
    handler.send_header("Content-Length", str(len(payload)))
    handler.end_headers()
    handler.wfile.write(payload)


class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):  # silence default access log
        sys.stderr.write("stub: " + (fmt % args) + "\n")

    def do_GET(self):
        if self.path == "/v1/health":
            return _json(self, 200, {"ok": True, "host_version": MOCK_STATE["host_version"]})
        if self.path == "/v1/state":
            return _json(self, 200, MOCK_STATE)
        if self.path == "/v1/tracks":
            return _json(self, 200, {"tracks": MOCK_STATE["tracks"]})
        if self.path == "/v1/extensions":
            return _json(self, 200, {"extensions": MOCK_EXTENSIONS})
        return _json(self, 404, {"error": "not found", "path": self.path})

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0") or 0)
        raw = self.rfile.read(length) if length else b""
        try:
            body = json.loads(raw) if raw else {}
        except json.JSONDecodeError:
            return _json(self, 400, {"error": "invalid JSON"})

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
    sys.stderr.write(f"stub: listening on http://{HOST}:{PORT}\n")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        sys.stderr.write("stub: shutting down\n")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
