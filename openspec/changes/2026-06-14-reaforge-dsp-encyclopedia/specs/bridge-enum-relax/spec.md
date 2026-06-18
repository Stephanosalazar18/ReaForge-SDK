# Bridge Enum Relax Specification

## Purpose

Relax the JSON Schema `enum` constraint on `target` for the `reaforge_get_api_reference` tool in `tools/opencode_bridge.py` so the LLM can request sub-paths like `jsfx-primitives/saturation`, `jsfx-primitives/00-index`, `jsfx-algorithms/reverb/fdn`, and the existing `fx_chain-primitives/*` keys. The C++ lookup at `src/host/project_reader.cpp:117` already supports any string key via `api_reference_map().find(target)`, so the bridge MUST stop pre-validating.

## Current State (Pre-Change)

```python
// tools/opencode_bridge.py:107-117
"target": {
    "type": "string",
    "enum": ["jsfx", "reascript_lua", "fx_chain_format"],
    "description": "Which API reference markdown to return.",
},
```

The bridge proxies any non-empty string to `GET /v1/api-reference?target=<t>` (line 248) but the schema's enum rejects the call before it ever hits the wire. Two existing runtime-supported keys (`fx_chain-primitives/chain-builder-template`, `fx_chain-primitives/recipes/vocal-slap`) are unreachable through the tool.

## Requirements

### Requirement: Free-Form Target String (REQ-BRIDGE-01)

The `target` JSON Schema for `reaforge_get_api_reference` MUST NOT declare an `enum` constraint. The type MUST remain `string` with `minLength: 1`. The bridge MUST accept any non-empty string.

#### Scenario: sub-path key accepted by the schema

- GIVEN the bridge exposes `reaforge_get_api_reference`
- WHEN opencode calls the tool with `target="jsfx-primitives/saturation"`
- THEN the call is NOT rejected by schema validation
- AND the request reaches the C++ extension

#### Scenario: existing top-level keys still work

- GIVEN the bridge exposes `reaforge_get_api_reference`
- WHEN opencode calls the tool with `target="jsfx"`, `target="reascript_lua"`, or `target="fx_chain_format"`
- THEN each call reaches the C++ extension
- AND returns the same cheatsheet content as before

### Requirement: Pass-Through, No Validation (REQ-BRIDGE-02)

The bridge MUST forward `target` to the HTTP endpoint as a query parameter without filtering, lowercasing, or trimming beyond the existing `not target` empty-check at line 246. No client-side enum table is allowed.

#### Scenario: bridge forwards arbitrary strings

- GIVEN a request with `target="jsfx-primitives/filters/rbj-lowpass"`
- WHEN the `call_tool` handler runs
- THEN `client.get("/v1/api-reference", params={"target": "jsfx-primitives/filters/rbj-lowpass"})` is executed
- AND the string is not transformed before transmission

### Requirement: NOT_FOUND Stays In The Extension (REQ-BRIDGE-03)

The bridge MUST NOT duplicate the `INVALID_TARGET` lookup. The C++ function `lookup_api_reference` (`src/host/project_reader.cpp:117-135`) already returns `INVALID_TARGET` when `api_reference_map().find(target)` fails; the HTTP layer translates that into a 400 response (per `src/host/tests/test_http_server.cpp:430`). The bridge surfaces this error unchanged to the LLM.

#### Scenario: extension returns INVALID_TARGET for unknown key

- GIVEN `target="nonexistent-category/foo"` is NOT in the runtime map
- WHEN the bridge proxies the request
- THEN the extension returns HTTP 400 with body `{"error": "INVALID_TARGET", ...}`
- AND the bridge returns that body verbatim to the LLM (no rewriting)

#### Scenario: bridge does not pre-check against an enum

- GIVEN the schema no longer carries `enum`
- WHEN the bridge starts
- THEN no Python list or set on the bridge side lists allowed target values

### Requirement: Tool Description Mentions Sub-Paths (REQ-BRIDGE-04)

The `description` field of `reaforge_get_api_reference` MUST list the new sub-path prefixes the LLM can use: `jsfx-primitives/<cat>`, `jsfx-primitives/00-index`, `jsfx-algorithms/<cat>/<alg>`, plus the existing `fx_chain-primitives/*` keys. The description MUST clarify that `jsfx` / `reascript_lua` / `fx_chain_format` remain the top-level L1 syntax cheatsheets.

#### Scenario: tool description lists sub-path prefixes

- GIVEN the bridge tool schema is returned via `tools/list`
- WHEN opencode reads the description of `reaforge_get_api_reference`
- THEN the description includes the substring `jsfx-primitives/`
- AND it includes the substring `jsfx-algorithms/`
- AND it includes the substring `fx_chain-primitives/`

## API Contract After Change

| Field | Value |
|---|---|
| Tool | `reaforge_get_api_reference` |
| Input | `{target: string, minLength: 1}` (no `enum`) |
| Endpoint | `GET /v1/api-reference?target=<t>` (unchanged) |
| Returns | `{target, reference}` on success |
| Errors | `INVALID_TARGET` (returned by the extension, surfaced unchanged) |

## Out of Scope

- Adding a `summary=true` flag for tight context windows (deferred to Phase 2)
- Caching target responses (premature; runtime map is O(1))
- Renaming the `target` parameter (breaking schema change; rejected)
- Expanding the bridge's error vocabulary (extension errors are sufficient)

## Affected PRs

| PR | Lane | What |
|---|---|---|
| 1 | `python-tdd` | Drop the `enum` field, update description, extend `tools/run_bridge_smoke.sh` with 3 new sub-path assertions |