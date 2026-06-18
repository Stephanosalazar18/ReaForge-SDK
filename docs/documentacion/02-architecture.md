# ReaForge — Architecture (post-pivot)

## Component diagram

```
┌───────────────────────── User's machine ─────────────────────────┐
│                                                                  │
│   opencode desktop                                               │
│   ┌─────────────────────────────────────────────────────────┐    │
│   │ Chat UI · conversation history · multi-model · tool     │    │
│   │ approval · MCP client                                   │    │
│   └─────────────────────┬───────────────────────────────────┘    │
│                         │ MCP (stdio)                            │
│                         ▼                                        │
│   ┌─────────────────────────────────────────────────────────┐    │
│   │ tools/opencode_bridge.py    (Python · mcp + httpx)      │    │
│   │ Exposes 7 tools. Translates each to HTTP.               │    │
│   └─────────────────────┬───────────────────────────────────┘    │
│                         │ HTTP (localhost or WSL→Windows IP)     │
│                         ▼                                        │
│   ┌─────────────────────────────────────────────────────────┐    │
│   │ REAPER.exe                                              │    │
│   │ ┌─────────────────────────────────────────────────┐     │    │
│   │ │ reaper_reaforge_host.dll   (C++ extension)      │     │    │
│   │ │   HTTP server  ·  filesystem writes  ·  refresh │     │    │
│   │ └─────────────────────────────────────────────────┘     │    │
│   │ Native runtimes: JSFX engine, ReaScript Lua, FX chains  │    │
│   └─────────────────────┬───────────────────────────────────┘    │
│                         │ filesystem                             │
│                         ▼                                        │
│   <REAPER resource>/                                             │
│      Effects/ReaForge/*.jsfx                                     │
│      Scripts/ReaForge/*.lua                                      │
│      FXChains/ReaForge/*.RfxChain                                │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

## Components

| Component | Lives in | Language | Role |
|---|---|---|---|
| Agent | `opencode desktop` | Multi-model (Claude / GPT / Gemini / local) | Talks to user, decides what to generate, calls bridge tools |
| Bridge | `tools/opencode_bridge.py` | Python (MCP server) | Exposes 7 tools to opencode. Translates to HTTP to the extension. |
| Extension | `src/host/` → `reaper_reaforge_host.dll` | C++ | HTTP server inside REAPER. Reads project state, writes artifact files, refreshes REAPER's caches. |
| REAPER | DAW | Native | Runs the project. Loads `.jsfx`, `.lua`, `.RfxChain` natively when the user uses them. |

## Data flow — generating a JSFX

1. User in opencode: *"Generate a JSFX that does a soft tape saturation"*
2. opencode calls `reaforge_get_state` → bridge → extension → returns project context (selected items, active FX, etc.)
3. opencode (LLM) generates the JSFX code, using `reaforge_get_api_reference("jsfx")` if needed to ground the syntax
4. opencode shows the user the generated code and asks for approval (built into opencode's MCP tool approval)
5. On approval, opencode calls `reaforge_save_jsfx(name, code)` → bridge → extension writes `<REAPER>/Effects/ReaForge/tape_saturation.jsfx`
6. opencode calls `reaforge_refresh()` → bridge → extension triggers REAPER's FX list rescan
7. User goes to REAPER, drops the new FX on a track, evaluates the result

## Key technical decisions

### 1. Agent lives outside REAPER (in opencode desktop)

| Reason | Detail |
|---|---|
| Reuse | opencode already ships chat UI, history, multi-model, tool approval, MCP. Embedding all of that in C++ would take months and bring zero unique value. |
| Multi-model for free | User configures their API key once in opencode; switching between Claude/GPT/Gemini/local is a setting, not a release. |
| Distribution | The user installs two pieces (REAPER + opencode). Acceptable trade-off for the target audience. |
| Bridge is the contract | The C++ extension only has to speak HTTP. Any agent (opencode today, anything else tomorrow) can drive it. |

### 2. Extension does NOT embed runtimes

REAPER already includes a JSFX engine and ReaScript Lua. Embedding Lua/QuickJS/JSFX inside our DLL is **duplicate work**. The extension only needs to write files into REAPER's standard folders. REAPER picks them up natively.

### 3. Extension exposes HTTP, not direct REAPER API to the bridge

| Reason | Detail |
|---|---|
| Cross-environment | The user runs REAPER on Windows and may run opencode on WSL or Windows. HTTP works in both. |
| Validated | The bridge spike (`bridge-spike-results.md`) proves the HTTP + MCP layer works end-to-end against a Python stub. |
| Replaceable | If we later want a stdio or IPC transport, the bridge isolates the transport choice. |

The HTTP discovery mechanism is documented in `docs/cross-environment.md`: the extension writes its base URL to `%APPDATA%\REAPER\ReaForge\wsl-bridge.txt`, and the bridge reads it. Falls back to `REAPER_BASE_URL` env var or `http://127.0.0.1:7800`.

### 4. Folder convention — everything under `ReaForge/` subfolders

| Artifact type | Path | REAPER picks it up via |
|---|---|---|
| JSFX effects | `<REAPER resource>/Effects/ReaForge/*.jsfx` | FX browser refresh |
| Lua scripts | `<REAPER resource>/Scripts/ReaForge/*.lua` | Action list refresh + optional auto-register |
| FX chains | `<REAPER resource>/FXChains/ReaForge/*.RfxChain` | FX chain browser |

Keeping ReaForge-generated artifacts in a dedicated subfolder:

- Prevents namespace collisions with the user's own files
- Makes "what did the agent generate?" answerable by `ls ReaForge/`
- Lets the user wipe everything ReaForge created with one `rm -rf`

**Open question** for the user: confirm the `ReaForge/` subfolder convention before implementing. Default is yes.

### 5. opencode's tool approval is our review gate

The MCP protocol lets opencode show the user *"the agent wants to call `reaforge_save_jsfx` with this content. Approve?"* before any write. We do not build our own approval UI.

## What this architecture does NOT include

| Excluded | Why |
|---|---|
| Live state mutation tools (`open_automation`, `set_parameter`, `play`, …) | User explicitly rejected this scope. Agent only generates. |
| Internal chat panel inside REAPER | Deferred to last phase. MVP is opencode desktop only. |
| Context menu hooks (right-click → "Generate spectral morph") | Deferred. Last phase. |
| Authentication, multi-user, cloud sync | Single-user local tool. |
| Versioning of generated artifacts | Defer. User can use git on their REAPER resource folder if they care. |
