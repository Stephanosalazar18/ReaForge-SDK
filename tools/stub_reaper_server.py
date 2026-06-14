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

import datetime
import json
import os
import re
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

# The bridge calls /v1/save/<kind>, not /v1/<kind>. The C++ spec
# documents the latter; the stub keeps both forms as a courtesy for
# the test fixture (some tests poke the canonical spec paths).
SPEC_WRITE_PATHS = {
    "jsfx": "/v1/jsfx",
    "lua": "/v1/lua",
    "fx_chain": "/v1/fxchain",
}

NAME_RE = re.compile(r"^[A-Za-z0-9_\-]{1,64}$")

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

# --- Helpers --------------------------------------------------------------


def _json(handler: BaseHTTPRequestHandler, status: int, body: dict) -> None:
    payload = json.dumps(body).encode("utf-8")
    handler.send_response(status)
    handler.send_header("Content-Type", "application/json")
    handler.send_header("Content-Length", str(len(payload)))
    handler.end_headers()
    handler.wfile.write(payload)


def _resolve_kind_from_path(path: str) -> str | None:
    """Map a POST path to its artifact kind, or None.

    Accepts both the bridge form (/v1/save/<kind>) and the canonical
    spec form (/v1/<kind> or /v1/fxchain). The bridge uses the
    human-friendly form (``fx-chain`` with a hyphen) while the spec
    uses the underscored form (``fx_chain``); we normalize here.
    """
    raw: str | None = None
    if path.startswith("/v1/save/"):
        raw = path[len("/v1/save/"):].strip("/")
    else:
        for kind, spec_path in SPEC_WRITE_PATHS.items():
            if path == spec_path:
                raw = kind
                break
    if not raw:
        return None
    # Normalize bridge form (hyphen) to internal form (underscore).
    return raw.replace("-", "_")


def _write_artifact(kind: str, name: str, content: str, overwrite: bool) -> dict:
    """Shared write logic for the 3 POST /v1/save/* routes.

    Returns the success body on 200, or an error envelope dict (with
    a separate ``_status`` key) on failure. The handler is responsible
    for promoting the envelope to the right HTTP code.
    """
    if not isinstance(name, str) or not NAME_RE.match(name):
        return {
            "_status": 400,
            "error": "INVALID_NAME",
            "message": "name must match ^[A-Za-z0-9_\\-]{1,64}$",
        }
    subdir, suffix = KIND_TABLE[kind]
    target = os.path.join(subdir, name + suffix)
    if os.path.exists(target) and not overwrite:
        return {
            "_status": 409,
            "error": "FILE_EXISTS",
            "path": target.replace(os.sep, "/"),
        }
    try:
        os.makedirs(subdir, exist_ok=True)
        tmp = target + ".tmp"
        with open(tmp, "w", encoding="utf-8") as f:
            f.write(content)
        os.replace(tmp, target)
    except OSError as exc:
        return {
            "_status": 500,
            "error": "WRITE_FAILED",
            "message": str(exc),
        }
    return {
        "path": target.replace(os.sep, "/"),
        "bytes_written": len(content.encode("utf-8")),
    }


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

        return _json(self, 404, {"error": "not found", "path": path})

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0") or 0)
        raw = self.rfile.read(length) if length else b""
        try:
            body = json.loads(raw) if raw else {}
        except json.JSONDecodeError:
            return _json(self, 400, {"error": "INVALID_JSON"})

        # ---- Refresh route ----
        if self.path == "/v1/refresh":
            return _json(
                self,
                200,
                {
                    "refreshed_at": datetime.datetime.now(
                        datetime.timezone.utc
                    ).strftime("%Y-%m-%dT%H:%M:%SZ"),
                    "warnings": [],
                },
            )

        # ---- Write routes ----
        kind = _resolve_kind_from_path(self.path)
        if kind is not None:
            if kind not in KIND_TABLE:
                return _json(self, 404, {"error": "not found", "path": self.path})
            name = body.get("name", "")
            overwrite = bool(body.get("overwrite", False))
            # jsfx and lua send `code`; fx-chain sends `content`.
            content = (
                body.get("code")
                if kind in ("jsfx", "lua")
                else body.get("content", "")
            )
            result = _write_artifact(kind, name, content or "", overwrite)
            status = result.pop("_status", 200)
            return _json(self, status, result)

        return _json(self, 404, {"error": "not found", "path": self.path})


def main():
    # Make sure the fixture roots exist on startup so missing-folder
    # tolerance can be tested without a prior write.
    for d in (EFFECTS_DIR, SCRIPTS_DIR, FXCHAINS_DIR):
        os.makedirs(d, exist_ok=True)

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
