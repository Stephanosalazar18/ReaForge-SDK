# ReaForge — Plan Arquitectónico Completo

Inspirado en el [Ableton Extensions SDK](https://www.ableton.com/en/live/extensions/). Última actualización: 2026-06-07.

---

## 1. Filosofía (lo que aprendimos de Ableton)

Ableton Extensions son herramientas JavaScript que viven dentro de Live, se disparan desde el menú contextual, leen y reescriben el Set, y están diseñadas para que **un asistente de IA las genere sin que el usuario sepa programar**. Cita textual de su FAQ:

> *"The SDK is built on standard web technologies that AI coding assistants handle well. If you can clearly describe your idea for an Extension, you may be able to build a working Extension with AI assistance, without any coding experience."*

**ReaForge toma ese mismo concepto y lo lleva a REAPER**, con una diferencia fundamental: en vez de Node.js/JS, usamos los lenguajes **nativos de REAPER** (JSFX para audio DSP, Lua/ReaScript para flujos y automatización) porque REAPER ya los ejecuta sin necesidad de embeber un runtime nuevo.

---

## 2. Los tres actores y cómo se conectan

```
┌─── Windows ───────────────────────────────────────────────────┐
│                                                               │
│  REAPER.exe                                                   │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ reaper_reaforge_host.dll  (C++ extension)               │  │
│  │                                                         │  │
│  │  ┌─────────────────┐  ┌──────────────────────────────┐  │  │
│  │  │ HTTP Server      │  │ Artifact Manager             │  │  │
│  │  │ (0.0.0.0:7800)  │  │  ├─ Write .jsfx → Effects/   │  │  │
│  │  │                  │  │  ├─ Write .lua  → Scripts/   │  │  │
│  │  │ GET  /v1/state   │  │  ├─ Write .RfxChain → Chains/│  │  │
│  │  │ GET  /v1/artifacts│  │  └─ Refresh() → REAPER API  │  │  │
│  │  │ POST /v1/jsx      │  │                              │  │  │
│  │  │ POST /v1/lua      │  │                              │  │  │
│  │  │ POST /v1/fxchain  │  │                              │  │  │
│  │  │ POST /v1/refresh  │  │                              │  │  │
│  │  └────────┬──────────┘  └──────────────────────────────┘  │  │
│  │           │                                               │  │
│  │  ┌────────┴──────────┐  ┌──────────────────────────────┐  │  │
│  │  │ Project Reader    │  │ Chat Panel (fase +3, defer)  │  │  │
│  │  │ (REAPER API)      │  │ ReaImGui, Lua-based          │  │  │
│  │  └───────────────────┘  │ shells out to opencode       │  │  │
│  │                         └──────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────┘  │
│                         │ HTTP (localhost:7800)               │
│                         │ o WSL IP si opencode en WSL         │
└─────────────────────────┼────────────────────────────────────┘
                          │
┌─── WSL2 ───────────────┼────────────────────────────────────┐
│                         ▼                                    │
│  opencode desktop / opencode serve --port 4096               │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │  LLM backend · conversation history · tool approval     │ │
│  │  MCP client (stdio)                                     │ │
│  └──────────────────────┬──────────────────────────────────┘ │
│                         │ stdio (MCP)                        │
│                         ▼                                    │
│  tools/opencode_bridge.py  (Python · mcp + httpx)           │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │ 7 tools: 3 Read / 3 Write / 1 Refresh                   │ │
│  │ Traduce MCP ↔ HTTP                                      │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                               │
│  // Para escrituras DIRECTAS desde WSL (opcional, más rápido)│
│  /mnt/c/Users/.../AppData/Roaming/REAPER/                    │
│       Effects/ReaForge/                                       │
│       Scripts/ReaForge/                                       │
│       FXChains/ReaForge/                                      │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

### Puntos clave de la conexión

| Capa | Protocolo | Justificación |
|---|---|---|
| opencode ↔ bridge | MCP (JSON-RPC sobre stdio) | Es el protocolo nativo de opencode. El bridge se registra como MCP server. |
| bridge ↔ extension | HTTP (localhost o WSL IP) | Validado por el bridge spike (5/5 GO). Funciona en local y cross-env. |
| extension ↔ REAPER | REAPER C++ API + filesystem | La extension vive dentro de REAPER, tiene acceso total a la API y al disco. |
| bridge ↔ disco REAPER (alternativo) | `/mnt/c/` desde WSL | Escrituras directas sin pasar por HTTP. Opción complementaria, no reemplaza la extension. |

---

## 3. Evaluación del mini-scope propuesto

El mini-scope que proveíste (escrito por otro agente o conversación previa) es **sólido en concepto, con algunos ajustes técnicos necesarios**.

### ✅ Lo que está BIEN

| Punto | Por qué |
|---|---|
| **ReaImGui como chat UI** | Es la forma correcta de poner UI dentro de REAPER sin compilar C++ para cada widget. Ya lo tenemos en Fase 1a. |
| **opencode como cerebro externo** | Coincide con nuestra decisión de arquitectura. opencode resuelve UI, history, multi-model, tool approval. No reinventamos. |
| **Artefactos se guardan en las carpetas nativas de REAPER** | `Effects/` para JSFX, `Scripts/` para Lua, `FXChains/` para cadenas. REAPER los carga sin reiniciar. |
| **`reaper.AddRemoveReaScript()` para registro automático** | El script aparece en la Action List instantáneamente. |
| **Metáfora "cerebro y cuerpo"** | Correcta: opencode (cerebro) decide qué generar, REAPER (cuerpo) ejecuta. |
| **WSL tiene acceso a `/mnt/c/`** | Las escrituras directas desde WSL a las carpetas de REAPER en Windows funcionan y son más rápidas que HTTP para payloads grandes. |
| **JSFX se compila JIT** | REAPER compila JSFX al vuelo. El archivo toca el disco y ya está disponible en el FX browser. |
| **Un solo entry point de chat** | El usuario escribe en UN lugar. El agente decide si es JSFX, Lua, o FX chain. |

### ❌ Lo que hay que AJUSTAR

| Punto del mini-scope | Problema | Solución |
|---|---|---|
| **"Usar la Web Interface de REAPER para leer estado"** | La web interface de REAPER es MUY limitada: transport, mixer básico, markers. No expone: items seleccionados, FX por track, parámetros de automation, estructura del proyecto. No sirve para darle contexto rico al agente. | **Usar nuestro C++ extension** con acceso total a la REAPER API (`GetSelectedMediaItem`, `TrackFX_GetCount`, etc.). Ya tenemos el esqueleto. La web interface de REAPER es un callejón sin salida para nuestro caso de uso. |
| **"El chat manda HTTP directo a opencode"** | opencode habla MCP (JSON-RPC sobre stdio), no HTTP para tool calling. El chat no puede "hablarle" a opencode como si fuera un endpoint REST cualquiera. | **Dos opciones**: (1) El chat shellea `opencode --prompt "..."`. (2) El chat usa la HTTP API de opencode serve (`localhost:4096`). La opción 1 es más simple para MVP. |
| **"Dos canales de comunicación separados"** (web + archivos + proxy) | Agrega complejidad innecesaria. El agente termina hablando por 3 caminos distintos. | **Un solo canal**: MCP bridge → HTTP → extension. Las escrituras a archivos pueden hacerse via HTTP (la extension escribe) o via `/mnt/c/` directo (más rápido). Pero el protocolo es UNO. |
| **"El chat de Lua hace `dofile()` sobre el código generado"** | `dofile()` ejecuta en el contexto del script del chat, no como acción independiente de REAPER. Pierde undo points, no aparece en Action List, no es reutilizable. | **Usar `reaper.AddRemoveReaScript()` + `reaper.Main_OnCommand()`** para registrar y ejecutar como acción nativa de REAPER, con undo points y visibilidad completa. |
| **No menciona MCP ni el bridge** | El mini-scope asume que opencode acepta HTTP crudo. opencode Desktop usa MCP para tool calling. | **Mantener y pivotar el bridge Python existente**. Ya validado 5/5 GO. Cambiar las tools de 5 a 7. |

---

## 4. Arquitectura definitiva (lo que construimos)

### 4.1 Componentes

| Componente | Lenguaje | Vive en | Responsabilidad |
|---|---|---|---|
| **Extension C++** | C++ (REAPER plugin API) | `src/host/` → `.dll` dentro de REAPER | HTTP server, lectura de estado del proyecto, escritura de archivos, refresh de caches de REAPER |
| **Bridge Python** | Python (mcp + httpx) | `tools/opencode_bridge.py` → subproceso de opencode | Traduce MCP ↔ HTTP. 7 tools. |
| **Stub de testing** | Python (http.server) | `tools/stub_reaper_server.py` | Mock de la extension para tests offline |
| **Chat UI** | Lua + ReaImGui | `chat/reaforge_chat.lua` (fase +3) | Panel de chat dentro de REAPER. Postergado. |
| **Agente** | LLM via opencode | opencode Desktop (WSL o Windows) | Decide qué generar, escribe el código, aprueba con el usuario |

### 4.2 Las 7 tools del bridge (MVP)

#### Read — 3 tools

| Tool | Qué devuelve | Para qué sirve |
|---|---|---|
| `reaforge_get_state` | Proyecto: tracks, items seleccionados, FX por track, BPM, sample rate | El agente entiende el contexto del proyecto antes de generar |
| `reaforge_list_artifacts` | Archivos bajo `Effects/ReaForge/`, `Scripts/ReaForge/`, `FXChains/ReaForge/` | Evitar duplicados, soportar "regenerate" |
| `reaforge_get_api_reference(target)` | Documentación embebida de JSFX / ReaScript Lua / FX Chain format | Reduce alucinaciones en el código generado |

#### Write — 3 tools

| Tool | Efecto | Retorna |
|---|---|---|
| `reaforge_save_jsfx(name, code)` | Escribe `Effects/ReaForge/<name>.jsfx` | Path absoluto |
| `reaforge_save_lua(name, code, register_action)` | Escribe `Scripts/ReaForge/<name>.lua`. Si `register_action=true`, registra en Action List. | Path + action ID |
| `reaforge_save_fx_chain(name, content)` | Escribe `FXChains/ReaForge/<name>.RfxChain` | Path absoluto |

#### Refresh — 1 tool

| Tool | Efecto |
|---|---|
| `reaforge_refresh()` | Dispara rescan de FX list y Action List en REAPER |

### 4.3 Flujo de usuario (MVP, sin chat interno)

```
1. Usuario abre opencode Desktop (Windows o WSL)
2. Usuario escribe: "Generate a JSFX tape saturation, I have a vocal on track 1"
3. opencode llama reaforge_get_state → ve track 1, tipo de item, FX actuales
4. opencode (LLM) genera el código JSFX
5. opencode muestra el código al usuario para aprobación (tool approval nativo)
6. Usuario aprueba → opencode llama reaforge_save_jsfx("tape_saturation", code)
7. opencode llama reaforge_refresh()
8. Usuario va a REAPER, busca "tape_saturation" en el FX browser, lo agrega al track
9. Evalúa si funciona
```

### 4.4 Flujo futuro (fase +3, con chat interno)

```
1. Usuario tiene el panel de chat abierto dentro de REAPER (ReaImGui dock)
2. Escribe: "Haceme un script que duplique la velocidad de las notas MIDI seleccionadas"
3. El chat shellea: opencode --prompt "Write a Lua script that doubles MIDI velocity..."
4. opencode procesa, genera, guarda via bridge, registra como action
5. El chat recibe confirmación y muestra: "Listo. Buscá 'double_velocity' en tu Action List"
```

---

## 5. Lo que NO hacemos (y por qué)

| No hacemos | Razón |
|---|---|
| Embeber Lua/QuickJS/JSFX en la DLL | REAPER ya los corre nativo. La extension solo escribe archivos. |
| Usar la web interface de REAPER | Demasiado limitada. No expone selección, FX, automation. |
| Hacer que el agente mute estado vivo de REAPER (abrir automation, cambiar params) | El usuario dijo que NO. El agente solo genera artefactos. |
| Construir el chat interno primero | MVP valida con opencode Desktop. El chat es mejora de UX, no requisito de validación. |
| Múltiples canales de comunicación | Un solo bridge MCP → HTTP. Menos superficie de fallo. |
| Usar `dofile()` para ejecutar código generado | Pierde undo points y registro. `AddRemoveReaScript` es el camino correcto. |

---

## 6. Paralelos con Ableton Extensions SDK

| Ableton | ReaForge (REAPER) |
|---|---|
| JavaScript / Node.js runtime | JSFX (audio DSP), Lua/ReaScript (flujos), FX Chains (combinaciones) |
| Context menu → extension aparece | Context menu → ReaForge → "Generate X" (fase +2) |
| Lee y reescribe el Set | Lee estado del proyecto, escribe artefactos en disco |
| Run-once, aplica cambios, stop | El agente genera, el usuario ejecuta cuando quiere |
| SDK para devs + AI-friendly | El agente ES el "dev". El usuario solo describe. |
| Extensiones instaladas vía Settings | Artefactos generados aparecen en FX browser / Action List automáticamente |
| Sandboxed runtime | REAPER es el sandbox (los artefactos son archivos nativos) |

---

## 7. Reestructuración del proyecto

### Estado actual → Estado objetivo

| Path actual | Acción |
|---|---|
| `src/host/` (Fase 1a: panel + context menu + extension loader + sample) | **Simplificar**: dejar HTTP server + project reader + artifact writer + refresh. Panel y context menu se mueven a `chat/` (defer). Extension loader y sample se archivan. |
| `src/host/extensions/humanize_midi/` | **Archivar**. El patrón "ship a sample" es reemplazado por "agente genera on demand". |
| `tools/opencode_bridge.py` (5 tools viejas) | **Pivotar** a 7 tools nuevas (3 Read + 3 Write + 1 Refresh). |
| `tools/stub_reaper_server.py` | **Extender** para mockear los 7 endpoints nuevos. |
| `tools/run_bridge_smoke.sh` | **Actualizar** para las 7 tools nuevas. |
| `README.md` | **Reescribir** con la nueva visión (agente generador de artefactos). |
| `docs/user-flows.md` | **Archivar** como histórico. Reemplazado por `docs/documentacion/`. |
| `openspec/specs/{multi-runtime,extension-execution,runtime-bridge}/` | **Mover a `openspec/specs/archive/`** con `REJECTED.md`. |
| `openspec/changes/2026-06-07-reaforge-fase1a-host/` | **Archivar** después de decidir si verificamos o no. |
| `openspec/changes/2026-06-07-reaforge-bridge-spike/` | **Archivar** cuando el MVP agentic lo reemplace. |

### Nueva estructura propuesta

```
ReaForge-SDK/
├── src/
│   └── host/                     # C++ extension (SIMPLIFICADO)
│       ├── host.cpp/h            # REAPER plugin entry, registra HTTP server
│       ├── http_server.cpp/h     # Endpoints para las 7 tools
│       ├── project_reader.cpp/h  # Lectura de estado (REAPER API)
│       ├── artifact_writer.cpp/h # Escritura de archivos (JSFX/Lua/RfxChain)
│       └── refresh.cpp/h         # Refresh de FX list y Action List
├── tools/
│   ├── opencode_bridge.py        # MCP bridge (7 tools)
│   ├── stub_reaper_server.py     # HTTP stub para testing offline
│   ├── run_bridge_smoke.sh       # Smoke test end-to-end
│   └── interactive_bridge_test.py # REPL interactivo
├── chat/                         # DEFERRED (fase +3)
│   └── reaforge_chat.lua         # Panel ReaImGui, shellea a opencode
├── scripts/                      # Build helpers
│   ├── install-build-deps.sh
│   ├── build-and-load-linux.sh
│   └── build-windows.sh
├── docs/
│   ├── documentacion/            # Handoff + plan (ESTA CARPETA)
│   │   ├── README.md
│   │   ├── 01-vision-and-pivot.md
│   │   ├── 02-architecture.md
│   │   ├── 03-current-state.md
│   │   ├── 04-mvp-and-handoff.md
│   │   └── 05-plan-arquitectonico.md
│   └── cross-environment.md      # Transporte cross-env (vigente)
├── openspec/
│   ├── specs/
│   │   └── archive/              # Specs rechazadas por el pivot
│   └── changes/
│       └── <date>-reaforge-agentic-mvp/  # PRÓXIMO SDD
├── meson.build
└── README.md                     # Reescrito
```

---

## 8. Roadmap de fases

| Fase | Qué | Entry point del usuario | Estado |
|---|---|---|---|
| **MVP** (ahora) | 7 tools del bridge. Extension C++ con HTTP server + artifact writer + refresh. | opencode Desktop | **Por arrancar** |
| **+1 Polish** | Mejor contexto (FX params, automation refs). Nombres de artefactos más inteligentes. | opencode Desktop | — |
| **+2 Context menu** | Click derecho en track/item → "ReaForge → Generate X". Shellea `opencode --prompt`. | REAPER context menu | — |
| **+3 Chat interno** | Panel ReaImGui dockeable. Chat completo dentro de REAPER. | REAPER dock panel | — |
| **+4 Distribución** | ReaPack package. Instalador. Builds signed para Windows/Mac/Linux. | ReaPack | — |

---

## 9. Próximo paso concreto

Abrir un SDD change: `openspec/changes/2026-06-XX-reaforge-agentic-mvp/`

**PRs sugeridos (7, uno por cada paso lógico):**

| PR | Scope | Tamaño |
|---|---|---|
| 1 | `README.md` + docs rewrite | small |
| 2 | Pivot bridge tools (5→7) en `opencode_bridge.py` | medium |
| 3 | Extender stub server para 7 endpoints + smoke update | medium |
| 4 | C++: implementar artifact writer (3 Write tools) | medium |
| 5 | C++: implementar project reader (3 Read tools) | medium |
| 6 | C++: implementar refresh hook (`reaforge_refresh`) | small |
| 7 | Integración end-to-end en REAPER Windows | verify |
