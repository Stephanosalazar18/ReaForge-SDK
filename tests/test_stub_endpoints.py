#!/usr/bin/env python3
"""Test the stub REAPER server's HTTP endpoint surface.

Boots ``tools/stub_reaper_server.py`` as a subprocess on port 7800 and
verifies the 7 post-pivot endpoints respond with the expected shape
(see ``openspec/changes/2026-06-07-reaforge-agentic-mvp/specs/`` for
the wire contract).

The 5 pre-pivot endpoints (``/v1/tracks``, ``/v1/extensions`` GET,
``/v1/extensions/{id}/run`` POST) MUST return 404 — the 5→7 swap is
clean, not a 5+2 hybrid. ``/v1/health`` is the readiness probe and
stays. ``/v1/state`` returns the compact projection per the
read-API spec.
"""
from __future__ import annotations

import json
import os
import socket
import subprocess
import sys
import time
import urllib.parse
import urllib.request
from pathlib import Path

import pytest

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
STUB = REPO / "tools" / "stub_reaper_server.py"

# POST routes the stub MUST answer with 2xx for the MVP harness.
WRITE_BODY_KIND = {
    "jsfx": {"name": "smoke_jsfx", "code": "// smoke", "overwrite": False},
    "lua": {
        "name": "smoke_lua",
        "code": "-- smoke",
        "register_action": False,
        "overwrite": False,
    },
    "fx_chain": {"name": "smoke_chain", "content": "<smoke/>", "overwrite": False},
}

# Pre-pivot paths that MUST 404 (the legacy 5, minus /v1/health which is
# the readiness probe and stays).
PRE_PIVOT_PATHS = [
    "/v1/tracks",
    "/v1/extensions",
    "/v1/extensions/humanize_midi/run",
]


def _wait_for_port(host: str, port: int, timeout: float = 5.0) -> None:
    """Block until the stub is accepting TCP connections."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(0.2)
            try:
                s.connect((host, port))
                return
            except OSError:
                time.sleep(0.05)
    raise RuntimeError(f"stub on {host}:{port} did not come up within {timeout}s")


def _http(
    method: str,
    path: str,
    body: dict | None = None,
    *,
    timeout: float = 5.0,
) -> tuple[int, dict | str]:
    url = f"http://127.0.0.1:7800{path}"
    data = None
    headers = {"Accept": "application/json"}
    if body is not None:
        data = json.dumps(body).encode("utf-8")
        headers["Content-Type"] = "application/json"
    req = urllib.request.Request(url, data=data, method=method, headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            raw = resp.read().decode("utf-8")
            try:
                return resp.status, json.loads(raw)
            except json.JSONDecodeError:
                return resp.status, raw
    except urllib.error.HTTPError as e:
        raw = e.read().decode("utf-8")
        try:
            return e.code, json.loads(raw)
        except json.JSONDecodeError:
            return e.code, raw


@pytest.fixture(scope="module")
def stub(tmp_path_factory):
    """Start the stub server on 127.0.0.1:7800 and tear it down after the module.

    The stub is configured (via the ``REAFORGE_STUB_ROOT`` env var it
    honors) to write POST artifacts to a fixture directory, NOT to real
    REAPER folders. See artifact-folder-convention/spec.md for the
    rule the real C++ writer enforces.
    """
    fixture_root = tmp_path_factory.mktemp("reaforge_stub_fixtures")
    env = os.environ.copy()
    env["REAFORGE_STUB_ROOT"] = str(fixture_root)
    proc = subprocess.Popen(
        [sys.executable, str(STUB)],
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        _wait_for_port("127.0.0.1", 7800, timeout=5.0)
        # Health probe so the fixture root is reachable from the
        # assertion path below.
        status, _ = _http("GET", "/v1/health")
        assert status == 200, f"health probe got {status}"
        yield {"proc": proc, "fixture_root": fixture_root}
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=2.0)
        except subprocess.TimeoutExpired:
            proc.kill()


# ---------- Read routes (3) ----------


def test_get_state_returns_compact_projection(stub):
    """GET /v1/state returns the compact project projection per
    extension-read-api/spec.md (project_name, sample_rate, bpm, tracks,
    selected_items_count)."""
    status, body = _http("GET", "/v1/state")
    assert status == 200, body
    assert isinstance(body, dict)
    for key in ("project_name", "sample_rate", "bpm", "tracks", "selected_items_count"):
        assert key in body, f"missing key {key!r} in /v1/state response: {body}"
    assert isinstance(body["tracks"], list)


def test_get_state_summary_param_does_not_500(stub):
    """The stub may ignore ?summary=true (compact projection is a C++
    concern) but MUST NOT 5xx on the parameter."""
    status, body = _http("GET", "/v1/state?summary=true")
    assert 200 <= status < 300, body


def test_get_artifacts_returns_list(stub):
    """GET /v1/artifacts returns {"artifacts": [...]} per the spec.
    The stub can return an empty list — the missing-folder-tolerance
    rule means the empty case is a valid 200."""
    status, body = _http("GET", "/v1/artifacts")
    assert status == 200, body
    assert isinstance(body, dict)
    assert "artifacts" in body
    assert isinstance(body["artifacts"], list)


def test_get_artifacts_kind_filter_does_not_500(stub):
    """GET /v1/artifacts?kind=jsfx is the same 200-with-list path."""
    status, body = _http("GET", "/v1/artifacts?kind=jsfx")
    assert 200 <= status < 300, body


def test_get_api_reference_returns_markdown(stub):
    """GET /v1/api-reference?target=jsfx returns {"target":..., "reference":...}
    where `reference` is a markdown string (the stub can be a hardcoded
    placeholder per the spec's embedded-payload model)."""
    status, body = _http("GET", "/v1/api-reference?target=jsfx")
    assert status == 200, body
    assert isinstance(body, dict)
    assert body.get("target") == "jsfx"
    assert isinstance(body.get("reference"), str) and body["reference"], (
        "reference must be a non-empty markdown string"
    )


def test_get_api_reference_other_targets(stub):
    """The other two valid targets — reascript_lua, fx_chain_format —
    also return 200 with the same shape."""
    for target in ("reascript_lua", "fx_chain_format"):
        status, body = _http("GET", f"/v1/api-reference?target={target}")
        assert status == 200, f"{target}: {body}"
        assert body.get("target") == target
        assert isinstance(body.get("reference"), str) and body["reference"]


def test_get_api_reference_missing_target_is_400(stub):
    """Missing the required `target` query param yields 400 per the
    spec's MISSING_TARGET error."""
    status, body = _http("GET", "/v1/api-reference")
    assert status == 400, body


# ---------- Write routes (3) ----------


def _assert_write_ok(kind: str, body: dict, fixture_root: Path) -> None:
    """Common assertions for a successful POST /v1/save/<kind>.

    The stub writes to a fixture dir, NOT to real REAPER folders. The
    returned `path` must live under the fixture root.
    """
    assert "path" in body, f"{kind}: missing 'path' in response: {body}"
    assert body["path"].startswith(str(fixture_root)), (
        f"{kind}: returned path {body['path']!r} is outside the fixture "
        f"root {fixture_root}"
    )
    # The stub MUST have actually written the file.
    written = Path(body["path"])
    assert written.is_file(), f"{kind}: stub did not write the file at {written}"


def test_post_save_jsfx_writes_to_fixture(stub):
    """POST /v1/save/jsfx writes the file to the fixture dir, returns
    {path, bytes_written}."""
    body = dict(WRITE_BODY_KIND["jsfx"], overwrite=True)
    status, resp = _http("POST", "/v1/save/jsfx", body)
    assert status == 200, resp
    assert "bytes_written" in resp, resp
    _assert_write_ok("jsfx", resp, stub["fixture_root"])


def test_post_save_lua_writes_to_fixture(stub):
    """POST /v1/save/lua writes the file to the fixture dir, returns {path}."""
    body = dict(WRITE_BODY_KIND["lua"], overwrite=True)
    status, resp = _http("POST", "/v1/save/lua", body)
    assert status == 200, resp
    _assert_write_ok("lua", resp, stub["fixture_root"])


def test_post_save_fx_chain_writes_to_fixture(stub):
    """POST /v1/save/fx-chain writes the file to the fixture dir."""
    body = dict(WRITE_BODY_KIND["fx_chain"], overwrite=True)
    status, resp = _http("POST", "/v1/save/fx-chain", body)
    assert status == 200, resp
    _assert_write_ok("fx_chain", resp, stub["fixture_root"])


def test_post_save_jsfx_refuses_overwrite(stub):
    """The spec's FILE_EXISTS rule: second write without overwrite=true
    must return 409."""
    # First write succeeds (different fixture name to avoid clashing with
    # the previous test).
    name = "overwrite_target"
    body = {"name": name, "code": "// v1", "overwrite": True}
    status, _ = _http("POST", "/v1/save/jsfx", body)
    assert status == 200, body
    # Second write without overwrite must 409.
    status, resp = _http(
        "POST", "/v1/save/jsfx", {"name": name, "code": "// v2"}
    )
    assert status == 409, resp
    assert resp.get("error") == "FILE_EXISTS", resp


def test_post_save_jsfx_invalid_name_is_400(stub):
    """The spec's INVALID_NAME rule: name with a slash is rejected
    before any filesystem touch."""
    status, resp = _http(
        "POST", "/v1/save/jsfx", {"name": "../escape", "code": "x"}
    )
    assert status == 400, resp
    assert resp.get("error") == "INVALID_NAME", resp


# ---------- Refresh (1) ----------


def test_post_refresh_returns_timestamp(stub):
    """POST /v1/refresh returns {"refreshed_at": "<ISO8601>", "warnings": []}."""
    status, body = _http("POST", "/v1/refresh", {})
    assert status == 200, body
    assert "refreshed_at" in body, body
    assert isinstance(body["refreshed_at"], str) and body["refreshed_at"], body
    assert "warnings" in body, body
    assert isinstance(body["warnings"], list), body


# ---------- Pre-pivot: 3 legacy paths must 404 ----------


@pytest.mark.parametrize("path", PRE_PIVOT_PATHS)
def test_pre_pivot_paths_are_404(stub, path):
    """The 3 legacy endpoints (track list, extension list, extension run)
    MUST be gone — the 5→7 swap is clean, not a 5+2 hybrid."""
    method = "POST" if path.endswith("/run") else "GET"
    status, _ = _http(method, path, {} if method == "POST" else None)
    assert status == 404, f"pre-pivot path {path!r} should be 404, got {status}"


# ---------- Health probe stays ----------


def test_health_probe_still_responds(stub):
    """The stub exposes /v1/health as a readiness probe — the smoke
    and the test fixture both wait on it. Must stay."""
    status, body = _http("GET", "/v1/health")
    assert status == 200, body
    assert body.get("ok") is True, body
