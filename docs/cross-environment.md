# ReaForge — Cross-Environment Communication

REAPER lives on Windows. opencode lives on WSL. The ReaForge extension is a C++ `.dll` that REAPER loads. The first-party `opencode-bridge` is a Python process that lets opencode talk to the running REAPER instance. This document explains how those four actors communicate.

## Topology

```
┌──────────────── Windows host ──────────────────────────────┐
│                                                            │
│  REAPER.exe                                                │
│  ┌──────────────────────────────────────────────────────┐  │
│  │ reaper_reaforge_host.dll                              │  │
│  │   ├─ ReaImGui Extensions Manager panel                │  │
│  │   ├─ Context menu hooks (HookCustomMenu)              │  │
│  │   ├─ Extension loader (filesystem scan)               │  │
│  │   └─ REAPER bridge: HTTP server on 0.0.0.0:7800      │  │
│  │       (small JSON-over-HTTP API, see protocol.md)     │  │
│  └──────────────────────────────────────────────────────┘  │
│              │                                             │
│              │ http://<wsl-ip>:7800/v1/...                 │
│              │                                            │
│  ┌──────── %APPDATA%\REAPER\ReaForge\wsl-bridge.txt ────┐  │
│  │    contains: <wsl-ip>:<port>                         │  │
│  │    written by: opencode-bridge on every startup      │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                            │
└────────────────────────────────────────────────────────────┘
                           │
                           │  TCP/HTTP (cross-env via WSL IP)
                           │
┌──────────────── WSL2 ──────────────────────────────────────┐
│                                                            │
│  opencode (CLI, terminal)                                  │
│   ├─ has an MCP client                                     │
│   └─ connects to the opencode-bridge MCP server            │
│                                                            │
│  opencode-bridge (Python, MCP server)                      │
│   ├─ talks MCP to opencode (stdio)                         │
│   ├─ talks HTTP to the REAPER extension                    │
│   ├─ on startup:                                          │
│   │     1. discover WSL IP: `hostname -I | awk '{print $1}'│
│   │     2. write %APPDATA%/REAPER/ReaForge/wsl-bridge.txt │
│   │     3. probe http://<wsl-ip>:7800/health              │
│   │     4. exit non-zero if REAPER is not running         │
│   └─ tools exposed to opencode:                            │
│        reaforge_list_tracks, reaforge_run_extension,       │
│        reaforge_get_state, reaforge_set_*, ...             │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

## Why this design

| Option | Verdict |
|---|---|
| **WSL2 IP + HTTP/JSON** (chosen) | Cross-env, simple, debuggable with `curl`, no extra daemons |
| Named pipe (Windows-only) | opencode is in WSL, can't reach a Windows-only pipe |
| TCP on 127.0.0.1 with port proxy | Requires `netsh` setup; opaque to debug |
| mDNS / service discovery | Cross-env but adds a daemon and a non-trivial failure mode |
| Filesystem watch | Simple but no real-time; good for batch but not for live control |
| ReaPack with REAPER as IPC daemon | REAPER is not a CLI; the extension would have to spawn a process per call |

HTTP/JSON is the boring choice. It also lets you `curl http://<wsl-ip>:7800/v1/state` to debug without involving opencode at all.

## The WSL IP problem

WSL2 has its own network namespace. `127.0.0.1` inside WSL is **not** the same as `127.0.0.1` on the Windows host. They are on different networks.

- WSL2's eth0 IP is typically `172.x.x.x` (e.g., `172.21.32.45`).
- Windows sees that IP directly because the WSL2 virtual NIC is bridged.
- The IP changes every time WSL2 restarts.

**Resolution**: the opencode-bridge writes the current WSL IP to a file that the extension reads. The flow:

1. User starts `opencode-bridge` (manually or via opencode's MCP loader).
2. The bridge discovers its IP: `hostname -I | awk '{print $1}'` (inside WSL).
3. The bridge writes the IP to `C:\Users\<user>\AppData\Roaming\REAPER\ReaForge\wsl-bridge.txt` (which is `/mnt/c/Users/<user>/AppData/Roaming/REAPER/ReaForge/wsl-bridge.txt` from WSL's POV).
4. The bridge probes `http://<wsl-ip>:7800/health` to confirm REAPER is up.
5. REAPER extension, on `init()`, reads the same file and binds to the port referenced there (or its own default `7800` if the file is missing).

The file is the only cross-env contract. It contains a single line: `172.21.32.45:7800`.

## Why not the opencode API directly?

You asked: "or we use just the opencode API?". The answer is: **the opencode API is the wrong layer**. The opencode HTTP API is for opencode-as-server, not opencode-as-client. We want opencode to *call* REAPER, not REAPER to call opencode. The bridge is opencode's adapter to REAPER, not the other way around.

If you wanted to skip the MCP layer entirely, opencode could in principle just `curl` the REAPER extension directly. That works for the technical case but loses:
- The MCP tool surface (typed schemas, auto-discovery in opencode).
- The `wsl-bridge.txt` discovery protocol.
- The structured error handling that the MCP layer provides.

The MCP layer is ~50 LOC of Python. The HTTP layer is ~50 LOC of C++ inside the extension. Total added complexity: ~100 LOC for a much cleaner developer experience. Worth it.

## The API (HTTP/JSON)

The extension exposes a small REST surface. Examples (full spec in `docs/api.md` once Fase 5 lands):

```
GET  /v1/health                  → {"ok": true, "host_version": "0.0.1"}
GET  /v1/state                   → full project state (tracks, items, FX)
GET  /v1/extensions              → list of loaded extensions
POST /v1/extensions/{id}/run     → invoke an extension, body = JSON args
POST /v1/extensions/{id}/reload  → reload source from disk
GET  /v1/tracks                  → track list
POST /v1/tracks/{i}/fx           → add FX
```

The MCP server in `opencode-bridge` translates each of these to an MCP tool that opencode can call. Tool names are MCP-style snake_case; the underlying HTTP is kebab-case.

## Building the extension for Windows

The extension must be compiled with the **MSVC ABI** to load into REAPER Windows. From WSL2, this means cross-compiling with `x86_64-w64-mingw32` (theoretical; not widely tested for REAPER plugins) or — the practical path — building on a Windows machine with Visual Studio Build Tools.

```cmd
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" amd64
cd C:\Users\stephano\dev\audio\ReaForge-SDK
meson setup build
ninja -C build
copy build\reaper_reaforge_host.dll "%APPDATA%\REAPER\UserPlugins\"
```

The Linux build (this WSL environment) produces a `.so` that REAPER on Linux can load. It is the same code path; only the linker ABI differs. The two builds are interchangeable from opencode's perspective because they expose the same HTTP API.

## Local development loop

While developing on the WSL2 host:

1. Edit C++ in WSL.
2. `ninja -C build` produces a Linux `.so`.
3. To test the HTTP API without REAPER: write a tiny stub server in Python that listens on `127.0.0.1:7800` and answers the same JSON. opencode-bridge works against it identically.
4. When ready: copy the source to a Windows machine, build with MSVC, drop the `.dll` in `%APPDATA%\REAPER\UserPlugins\`.

This keeps the loop tight in WSL while preserving the cross-env path to real REAPER on Windows.

## Fallback: REAPER-Linux via Wine

If you can install REAPER inside WSL via Wine (the official `reaper_linux_x86_64.tar.xz` from Cockos), the WSL `127.0.0.1` and Windows `127.0.0.1` are still separate, but the WSL-bridge file approach still works. The only difference is the host running REAPER is WSL, not Windows.

This is the cleanest development setup because the build, run, and bridge all live in one filesystem. It is recommended for Fase 5 development, even if the final delivery target is REAPER-Windows.
