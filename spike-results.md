# ReaForge Spike Results

## Throughput (lower is better)

| Runtime | p50 (us) | p95 (us) | p99 (us) | Budget (us) | Verdict |
|---|---|---|---|---|---|
| Lua 5.4 | 3.9 | 4.4 | 18.9 | 5000 | PASS |
| QuickJS-ng | 11.2 | 11.6 | 48.2 | 5000 | PASS |
| JSFX (stub) | 0.4 | 0.5 | 0.5 | 5000 | PASS |

## Integration Findings

- Lua 5.4 linked via system liblua5.4-dev.
- QuickJS-ng vendored as a git submodule at `third_party/quickjs-ng/`.
- JSFX runtime is a stub; `reaper_jsfx_compile` is not linked in the spike build.
- REAPER C++ SDK headers vendored at `third_party/reaper-sdk/` but not yet linked.

## Verdict

**GO**
