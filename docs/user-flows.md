# ReaForge — User Flows

This document clarifies the three distinct ways a user can interact with ReaForge. Each is a separate milestone; they are NOT alternatives, they are sequential phases of the same product.

## The mental model

ReaForge is, first, a **platform for REAPER extensions** (like Ableton's Extensions SDK). Everything else — including AI integration — is layered on top of that platform.

```
                    ┌─────────────────────────┐
                    │   REAPER (the host)     │
                    │                         │
                    │   ReaForge extension    │
                    │   (.dll / .dylib / .so) │
                    └────────────┬────────────┘
                                 │
                ┌────────────────┼────────────────┐
                │                │                │
        ┌───────▼──────┐  ┌──────▼─────┐  ┌───────▼────────┐
        │ Lua ext      │  │ QuickJS    │  │ JSFX effect    │
        │ (control)    │  │ ext (UI/   │  │ (audio DSP)    │
        │              │  │  logic)    │  │                │
        └──────────────┘  └────────────┘  └────────────────┘
```

Every ReaForge capability hangs off this platform. The three user flows below are different **clients** of the same platform.

---

## Flow 1 — ReaForge native (Fase 1a)

**Status**: in development. Deliverable of the [`2026-06-07-reaforge-fase1a-host`](../changes/2026-06-07-reaforge-fase1a-host/) change.

The user interacts with ReaForge **entirely from inside REAPER**. The extension ships a dock panel and a context menu; both are populated by extensions the user has installed (or that ship with ReaForge).

```
┌────────────────────────────────────────────────────┐
│  REAPER                                            │
│                                                    │
│  ┌─ Extensions Manager (dock) ──────────────────┐  │
│  │  id           runtime   status               │  │
│  │  humanize-midi  lua     loaded   [Run] [↻]  │  │
│  │  render-sel     lua     loaded   [Run] [↻]  │  │
│  │  …                                       +   │  │
│  └──────────────────────────────────────────────┘  │
│                                                    │
│  Track 1 [Drums]  Track 2 [Bass]  Track 3 [Vox]    │
│      │                                  │          │
│      └─ click derecho ──►               │          │
│         "ReaForge →"  ──►               │          │
│            humanize-midi ✓             │          │
│            render-sel                  │          │
│                                        │          │
│      (también click en item MIDI) ─────┘          │
│                                                    │
└────────────────────────────────────────────────────┘
```

**Requirements**: REAPER 7+, the ReaForge extension installed in `UserPlugins/`.

**What the user does NOT need**: opencode, terminal, any external process, network access.

**Best for**: producers who want a stable, click-driven workflow. This is the Ableton Extensions experience for REAPER.

---

## Flow 2 — opencode as external CLI (Fase 5)

**Status**: deferred. Will land as a separate change, `opencode-bridge`.

The user runs `opencode` in a terminal (or in an editor that embeds opencode). opencode has a ReaForge adapter that exposes the platform's operations as tools. The user types natural-language requests; opencode invokes them against the running REAPER instance via MCP or IPC.

```
┌──────────────────────┐    MCP / IPC    ┌───────────────────────┐
│  opencode (CLI)      │◄───────────────►│  REAPER + ReaForge    │
│                      │                 │                       │
│  $ opencode          │                 │  - reads project      │
│  > humaniza el midi  │                 │  - mutates tracks     │
│    de la pista 2     │                 │  - invokes extensions │
│  < listo, 47 notas   │                 │                       │
│    modificadas       │                 │                       │
│                      │                 │                       │
│  > y ahora sumá un   │                 │                       │
│    reverb al master  │                 │                       │
│  < reverb cargado    │                 │                       │
└──────────────────────┘                 └───────────────────────┘
```

**Requirements**: REAPER 7+ with ReaForge, plus `opencode` (or any MCP-compatible client) running on the same machine.

**What the user does NOT need**: a ReaForge dock panel (Flow 1 is technically still required because it ships the bridge, but the user never opens it).

**Best for**: scripted / batch workflows; power users who prefer a terminal; automations triggered by file watchers or schedulers.

---

## Flow 3 — opencode embedded in REAPER (Fase 5+)

**Status**: deferred to after Flow 2 ships. Build on top of Flow 2 by adding a ReaImGui dock that hosts the opencode chat.

The user opens a second ReaImGui dock inside REAPER — the "AI Assistant". It is a chat surface; messages are routed to a local `opencode` process which acts on REAPER through the same bridge Flow 2 uses.

```
┌─────────────────────────────────────────────────────┐
│  REAPER                                             │
│                                                     │
│  ┌─ Extensions Manager ─┐  ┌─ AI Assistant ────────┐ │
│  │  humanize-midi  [Run]│  │ > humaniza pista 2   │ │
│  │  render-sel     [Run]│  │ < listo, 47 notas    │ │
│  └──────────────────────┘  │                       │ │
│                             │ > sumá reverb al     │ │
│  Track 1 [Drums]            │   master              │ │
│  Track 2 [Bass]             │ < reverb cargado     │ │
│  Track 3 [Vox]              │                       │ │
│                             │  [type a message]    │ │
│                             └───────────────────────┘ │
└─────────────────────────────────────────────────────┘
```

**Requirements**: REAPER 7+ with ReaForge, plus a local `opencode` process (started automatically by the dock or manually by the user).

**What the user does NOT need**: a terminal, an external editor. The chat is in REAPER.

**Best for**: producers who want AI assistance without leaving REAPER. This is the "killer" UX, but it requires Flow 1 (the platform) and Flow 2 (the bridge) to exist first.

---

## Roadmap ordering

```
Fase 1a  ──►  Fase 1b  ──►  Fase 3  ──►  Fase 5  ──►  Fase 5+
(platform,     (more        (ReaPack    (Flow 2:     (Flow 3:
 Flow 1)       extensions)  + registry)  external    embedded
                                          opencode)   opencode)
```

Each layer builds on the previous one. Skipping ahead to Flow 3 before Flow 1 works is impossible — the AI has nothing to call into.

---

## Why this order

1. **Flow 1 alone is already a useful product.** The platform ships value the moment a user installs the extension. The AI is a multiplier, not the product.
2. **Flow 2 reuses Flow 1's bridge.** The bridge functions are the same; only the client changes. So Flow 2 is mostly a packaging exercise plus an `opencode` tool definition file.
3. **Flow 3 is a UI change on top of Flow 2.** The chat dock is a ReaImGui window that wraps a websocket to the local `opencode` process. The hard part is Flow 2; the chat itself is small.
