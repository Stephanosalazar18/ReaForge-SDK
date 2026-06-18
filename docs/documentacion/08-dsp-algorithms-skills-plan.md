# ReaForge — Plan Maestro: DSP Algorithms + Design Patterns + Skills

> Plan comprehensivo. Combina algoritmos DSP, patrones de diseño, y skills jerárquicas.

---

## 1. EL PROBLEMA COMPLETO

El LLM puede **copiar código** (16 primitivos embebidos) pero no puede **diseñar efectos**. Tres gaps:

| Gap | Síntoma | Causa |
|---|---|---|
| **Sin algoritmos** | No sabe hacer phase vocoder, FDN, Karplus-Strong | Solo tiene primitivos básicos, no algoritmos completos |
| **Sin criterio de diseño** | Pone sliders redundantes, no respeta gain staging | No tiene reglas de diseño |
| **Sin workflow guiado** | Saltea pasos, no consulta primitivos antes de generar | No hay skill que fuerce el proceso correcto |

---

## 2. FUENTES DE ALGORITMOS DSP

### 2.1 Fuentes primarias (código abierto, traducibles a JSFX)

| Fuente | Contenido | Licencia | Acceso | Esfuerzo de traducción |
|---|---|---|---|---|
| **FAUST libraries** | ~200 algoritmos: filtros, reverbs, osciladores, compresores, efectos | MIT | https://faustlibraries.grame.fr/ | Bajo — declarativo → JSFX |
| **Joep Van Lier JSFX** | 26 tipos de filtros (Moog, MS-20, SVF, Ladder, 303), FM, wavetables, LFOs, sequencers | MIT | context7: `/joepvanlier/jsfx` (126 snippets) | **Cero — ya es JSFX** |
| **musicdsp.org** | ~200 snippets C/C++ de efectos y síntesis | Public domain | https://www.musicdsp.org/ | Bajo — C → JSFX |
| **STK (Sound Toolkit)** | Modelado físico: cuerdas, tubos, placas. Pitch shift. | MIT | https://github.com/thestk/stk | Medio — C++ → JSFX |
| **Csound opcodes** | Miles de operadores DSP documentados | LGPL | https://csound.com/docs/manual/ | Medio — DSL → JSFX |
| **JUCE DSP modules** | Filtros, compresores, LFOs, oversampling | MIT/ISC | https://github.com/juce-framework/JUCE | Medio — C++ → JSFX |

### 2.2 Fuentes secundarias (requieren traducción manual)

| Fuente | Contenido | Acceso |
|---|---|---|
| **CCRMA Julius O. Smith** | Textbooks: Physical Audio, Spectral Audio, Filters | https://ccrma.stanford.edu/~jos/ |
| **DAFx proceedings** | Papers 1998-presente: vanguardia DSP | http://www.dafx.de/ |
| **RBJ Audio EQ Cookbook** | Fórmulas de biquad | https://www.w3.org/TR/audio-eq-cookbook/ |
| **Will Pirkle "Designing Audio Effect Plugins"** | Libro: diseño de efectos con C++ | Comercial (TOC público) |
| **Vital synthesizer** | Código fuente de sintetizador wavetable | https://github.com/mattt/vital (GPL) |
| **Surge synthesizer** | Código fuente de sintetizador híbrido | https://github.com/surge-synthesizer/surge (GPL) |

### 2.3 Fuentes de patrones de diseño (cómo organizar efectos)

| Fuente | Qué aporta | Acceso |
|---|---|---|
| **ReaPack top 50 JSFX** | Organización de sliders, naming, signal flow, @gfx | https://reapack.com/repos |
| **Joep Van Lier JSFX (context7)** | 26 tipos de filtros, LFOs con 26 formas, sequencers, FM — todo con diseño maduro | context7: `/joepvanlier/jsfx` |
| **JSFX SDK docs** | Sintaxis canónica, convenciones de REAPER | context7: `/websites/reaper_fm_sdk_js` |
| **JSFX UI Library (Geraint Luff)** | Framework de UI para JSFX: layouts, temas, controles | context7: `/geraintluff/jsfx-ui-lib` (510 snippets) |

### 2.4 Lo que extraemos de cada fuente

| Categoría | Algoritmos específicos | Fuente primaria |
|---|---|---|
| **Filtros avanzados** | Moog Ladder, MS-20, SVF no-lineal, Diode Ladder, 303, Steiner | Joep Van Lier (ya JSFX) |
| **Reverb avanzado** | Convolution, Griesinger, plate emulation | FAUST reverb.lib |
| **Síntesis** | Karplus-Strong, waveguide, FM, wavetable, granular | STK + Joep Van Lier |
| **Spectral** | Phase vocoder, cross-synthesis, spectral freeze, FFT NR | FAUST fft.lib + Csound pvs* |
| **Dinámica avanzada** | Multiband compressor, lookahead limiter, transient designer | FAUST compressor.lib |
| **Modulación** | Ring mod, AM, FM, phase distortion, wavefolding | musicdsp.org |
| **Tape emulation** | Wow/flutter, hysteresis, tape saturation | DAFx papers + ReaPack |
| **Lo-fi/degradation** | Bitcrush, sample rate reduction, tape hiss | musicdsp.org |

---

## 3. ARQUITECTURA DE SKILLS JERÁRQUICA

### 3.1 Concepto: "Plugin Designer" como meta-skill

```
Usuario: "generá un delay con modos dotted/triplet"
         │
         ▼
┌─────────────────────────────┐
│  jsfx-plugin-designer       │  ← META-SKILL (orquesta todo)
│  "Sé que esto es un delay"  │
│  "Necesito: design patterns │
│   + delay primitive +       │
│   parameter design rules"   │
└──────────┬──────────────────┘
           │
     ┌─────┼─────┬─────────────┐
     ▼     ▼     ▼             ▼
┌────────┐┌────────┐┌──────────┐┌──────────────┐
│dsp-    ││effect- ││reapack-  ││jsfx-syntax   │
│patterns││design  ││analyzer  ││reference     │
│        ││        ││          ││              │
│Lee:    ││Lee:    ││Lee:      ││Lee:          │
│00-index││manifesto│Top 50    ││JSFX SDK      │
│matrix  ││param   │effects   ││via context7  │
│        ││signal  │          ││              │
│        ││gain    │          ││              │
└────────┘└────────┘└──────────┘└──────────────┘
     │         │         │             │
     └────┬────┴────┬────┴─────────────┘
          ▼         ▼
    reaforge_get_api_reference()  +  context7_query-docs()
```

### 3.2 Skills propuestas

| Skill | Tipo | Trigger | Sub-skills que usa |
|---|---|---|---|
| `jsfx-plugin-designer` | **Meta-skill** | "generate JSFX", "create effect", "make a delay/reverb/synth" | Todas las siguientes |
| `dsp-patterns` | Sub-skill | Llamada por plugin-designer | Lee `00-index.md` + matriz de compatibilidad |
| `effect-design` | Sub-skill | Llamada por plugin-designer | Lee `00-manifesto.md` + `01-parameter-design.md` |
| `reapack-analyzer` | Sub-skill | "analyze effect", "study pattern" | Analiza JSFX de ReaPack via context7 |
| `jsfx-syntax-reference` | Sub-skill | Siempre (validación de sintaxis) | Query a context7 `/websites/reaper_fm_sdk_js` |
| `dsp-algorithm-translator` | Sub-skill | "translate FAUST to JSFX", "port algorithm" | Lee FAUST/Csound/musicdsp via context7 |

### 3.3 Skill `jsfx-plugin-designer` (SKILL.md)

```markdown
---
name: jsfx-plugin-designer
description: >
  Generate JSFX audio effects following ReaForge design patterns.
  Orchestrates DSP primitives, design rules, and syntax validation.
  Trigger: "generate JSFX", "create effect", "make a delay/reverb/
  saturator/compressor/synth", "build a plugin"
---

## WORKFLOW (follow in order, do not skip steps)

### Step 1: CLASSIFY the request
Identify what the user wants:
- Effect type: delay, reverb, saturation, compressor, filter, chorus, etc.
- Complexity: simple (1-2 primitives) or complex (3+ primitives composed)
- Special modes: dotted/triplet, tempo-sync, MIDI-controlled, etc.

### Step 2: READ design rules (MANDATORY)
Call `reaforge_get_api_reference("jsfx-design-patterns/00-manifesto")`
Read ALL rules. These are non-negotiable:
- ONE concept, ONE slider
- Respect signal chain order
- Gain stage every stage
- Name sliders for musicians
- Start simple

### Step 3: SELECT primitives
Call `reaforge_get_api_reference("00-index")`
From the catalog, select the primitives you'll compose.
Check the compatibility matrix — verify ✓ or ✓✓, never ✗.

### Step 4: FETCH each primitive's code
For each selected primitive, call:
`reaforge_get_api_reference("jsfx-primitives/<category>/<name>")`
Read the Code block, Parameters, Compatibility, and Source.

### Step 5: VALIDATE syntax
If unsure about a JSFX function or variable, query context7:
- `context7_query-docs("/websites/reaper_fm_sdk_js", "<function name>")`
- `context7_query-docs("/justinfrankel/jsfx", "<pattern>")`

### Step 6: DESIGN the parameter layout
Apply rules from Step 2:
- Decide: mode selector vs. separate sliders (NEVER both for same concept)
- Group: input section → processing → output
- Name: musician-friendly ("Warmth" not "Lowpass Freq")
- Hide: advanced parameters with `sliderX:-Hidden`

### Step 7: COMPOSE the JSFX
Merge primitives into a single JSFX:
- @init: all state variables + denorm prevention
- @slider: all coefficient calculations
- @sample: signal chain in canonical order
- Include DC blocking after any saturation with drive > 2.0

### Step 8: SELF-CHECK before saving
- [ ] No redundant sliders (same concept, different controls)
- [ ] Signal chain follows: utilities → filters → saturation → dynamics → delays → reverb → modulation
- [ ] Every stage has gain staging (output ≈ input)
- [ ] denorm = 1e-25 in @init + +=/-= pattern in @sample
- [ ] DC blocking after saturation
- [ ] Slider names are musician-friendly
- [ ] No more than 8 visible sliders for a simple effect

### Step 9: SAVE
Call `reaforge_save_jsfx(name, code)` or `reaforge_save_lua(name, code, register_action)`.
Call `reaforge_refresh()` to make REAPER pick it up.

## SUB-SKILLS

This skill coordinates the following sub-skills:
- `dsp-patterns`: primitive selection and compatibility checking
- `effect-design`: parameter layout, UI conventions, gain staging
- `jsfx-syntax-reference`: syntax validation via context7
```

### 3.4 Skill `dsp-algorithm-translator` (para algoritmos complejos)

```markdown
---
name: dsp-algorithm-translator
description: >
  Translate DSP algorithms from FAUST/Csound/musicdsp to idiomatic JSFX.
  Trigger: "translate FAUST", "port algorithm", "implement <algorithm name>"
---

## WORKFLOW

### Step 1: Identify the source algorithm
- Name: e.g., "FDN reverb", "phase vocoder", "Karplus-Strong"
- Source: FAUST library, Csound opcode, musicdsp snippet, STK class

### Step 2: Fetch the source code
- FAUST: query context7 or fetch from https://faustlibraries.grame.fr/
- Csound: fetch from https://csound.com/docs/manual/
- musicdsp: fetch from https://www.musicdsp.org/
- JSFX (Joep Van Lier): `context7_query-docs("/joepvanlier/jsfx", "<filter type>")`

### Step 3: Validate JSFX syntax
Query context7 for any JSFX function you're unsure about:
- `context7_query-docs("/websites/reaper_fm_sdk_js", "<function>")`

### Step 4: Translate
- Map C++ classes → JSFX @init state + @sample processing
- Map FAUST declarations → JSFX slider definitions + coefficient calc
- Map Csound opcodes → JSFX function calls
- Preserve the DSP algorithm's signal flow exactly

### Step 5: Add JSFX conventions
- denorm = 1e-25 in @init
- Anti-denormal pattern in @sample
- DC blocking if the algorithm has nonlinear elements
- Musician-friendly slider names

### Step 6: Test mentally
- What happens at silence? (denormals?)
- What happens at max drive? (clipping? DC?)
- What happens at min freq? (stability?)
```

---

## 4. CONTENIDO A CREAR

### 4.1 Design Patterns (Nivel 3 del encyclopedia)

| Archivo | Contenido | Fuente |
|---|---|---|
| `00-manifesto.md` | 6 reglas obligatorias antes de generar | Síntesis de ReaPack + Will Pirkle |
| `01-parameter-design.md` | Modos vs sliders, agrupación, presets, hidden | ReaPack top 50 + Joep Van Lier |
| `02-signal-flow.md` | Orden canónico, parallel/serial, feedback, sidechain | DSP textbooks + ReaPack |
| `03-ui-conventions.md` | Slider ordering, naming, @gfx, metering | JSFX UI Library (Geraint Luff) |
| `04-gain-staging.md` | Makeup gain, dry/wet, nivel constante, headroom | Mastering engineering practice |
| `05-anti-patterns.md` | 15 errores comunes con ejemplos de qué NO hacer | Observación del LLM actual |

### 4.2 Algoritmos DSP (Nivel 3 expandido)

| Algoritmo | Categoría | Líneas (est.) | Fuente | Prioridad |
|---|---|---|---|---|
| **Moog Ladder Filter** | Filters | ~80 | Joep Van Lier (JSFX) | Alta |
| **MS-20 Filter** | Filters | ~60 | Joep Van Lier (JSFX) | Alta |
| **SVF Non-linear** | Filters | ~70 | Joep Van Lier (JSFX) | Alta |
| **Convolution Reverb** | Reverb | ~150 | FAUST reverb.lib | Media |
| **Karplus-Strong** | Synthesis | ~50 | STK | Alta |
| **Waveguide** | Synthesis | ~100 | CCRMA Smith | Media |
| **FM Synthesis** | Synthesis | ~80 | Joep Van Lier (JSFX) | Alta |
| **Wavetable Oscillator** | Synthesis | ~120 | Joep Van Lier (JSFX) | Media |
| **Phase Vocoder** | Spectral | ~300 | FAUST fft.lib | Baja (Phase 2) |
| **Cross-synthesis (CDP8)** | Spectral | ~400 | Csound pvs* | Baja (Phase 2) |
| **Lookahead Limiter** | Dynamics | ~100 | FAUST compressor.lib | Media |
| **Multiband Compressor** | Dynamics | ~200 | FAUST + JUCE | Media |
| **Ring Modulator** | Modulation | ~30 | musicdsp.org | Alta |
| **Bitcrusher** | Lo-fi | ~40 | musicdsp.org | Alta |
| **Tape Wow/Flutter** | Tape | ~80 | DAFx papers | Media |

### 4.3 Skills (3 nuevas)

| Skill | Archivo | Dónde |
|---|---|---|
| `jsfx-plugin-designer` | SKILL.md | `~/.config/opencode/skills/jsfx-plugin-designer/` |
| `dsp-algorithm-translator` | SKILL.md | `~/.config/opencode/skills/dsp-algorithm-translator/` |
| `reapack-analyzer` | SKILL.md | `~/.config/opencode/skills/reapack-analyzer/` |

### 4.4 Engram feeding

| Topic key | Contenido | Cómo |
|---|---|---|
| `dsp/filters-moog-ladder` | Código JSFX + diseño del filtro Moog | Extraer de Joep Van Lier via context7 |
| `dsp/filters-ms20` | Código JSFX + diseño del filtro MS-20 | Extraer de Joep Van Lier via context7 |
| `dsp/synthesis-karplus-strong` | Algoritmo + implementación JSFX | Traducir de STK |
| `dsp/synthesis-fm` | Algoritmo + implementación JSFX | Extraer de Joep Van Lier via context7 |
| `dsp/design-parameter-patterns` | 10 patrones de diseño de parámetros | Sintetizar de ReaPack top 50 |
| `dsp/design-gain-staging` | Reglas de gain staging con ejemplos | Mastering practice |
| `dsp/design-anti-patterns` | 15 anti-patrones con ejemplos | Observación del LLM |

---

## 5. PLAN DE IMPLEMENTACIÓN

### Fase 1: Design Patterns + Skills (3-4 horas)

| Paso | Qué | Tiempo |
|---|---|---|
| 1.1 | Escribir 6 archivos de design patterns | 2h |
| 1.2 | Escribir 3 SKILL.md | 1h |
| 1.3 | Registrar skills en `~/.config/opencode/` | 15min |
| 1.4 | Embedder patterns en DLL + regen | 30min |
| 1.5 | Test: pedir "delay con dotted/triplet" → verificar que no duplica sliders | 15min |

### Fase 2: Algoritmos DSP de Joep Van Lier (2-3 horas)

| Paso | Qué | Tiempo |
|---|---|---|
| 2.1 | Query context7 `/joepvanlier/jsfx` para Moog, MS-20, SVF, FM | 30min |
| 2.2 | Extraer y formatear como primitivos (formato spec) | 1h |
| 2.3 | Escribir Ring Mod + Bitcrusher desde musicdsp | 30min |
| 2.4 | Embedder en DLL + regen | 30min |
| 2.5 | Guardar en Engram como observaciones DSP | 30min |

### Fase 3: Algoritmos de FAUST/STK (3-4 horas)

| Paso | Qué | Tiempo |
|---|---|---|
| 3.1 | Traducir Karplus-Strong de STK a JSFX | 1h |
| 3.2 | Traducir Convolution Reverb de FAUST | 1h |
| 3.3 | Traducir Lookahead Limiter de FAUST | 1h |
| 3.4 | Traducir Multiband Compressor de FAUST | 1h |
| 3.5 | Embedder + regen + Engram | 30min |

### Fase 4: Spectral/CDP8 (Phase 2, post-validación)

| Paso | Qué | Tiempo |
|---|---|---|
| 4.1 | Phase vocoder desde FAUST fft.lib | 3h |
| 4.2 | Cross-synthesis desde Csound pvs* | 3h |
| 4.3 | Spectral freeze | 1h |
| 4.4 | FFT noise reduction | 2h |

### Fase 5: Engram feeding continuo (ongoing)

Cada vez que el LLM genera un efecto y el usuario detecta un error de diseño, se guarda como observación en Engram con topic_key `dsp/design-lesson-<date>`. Esto alimenta la memoria acumulativa.

**Total Fases 1-3: 8-11 horas** (una sesión larga o 2-3 sesiones cortas)

---

## 6. CÓMO EL MCP SABE CUÁNDO USAR CADA SKILL

### Trigger matching

El skill registry de opencode usa el campo `description` para matching. Cuando el usuario escribe algo que matchea el trigger, opencode carga el SKILL.md.

| Trigger del usuario | Skill que se activa |
|---|---|
| "generate JSFX", "create effect", "make a delay" | `jsfx-plugin-designer` |
| "translate FAUST", "port algorithm" | `dsp-algorithm-translator` |
| "analyze effect", "study ReaPack pattern" | `reapack-analyzer` |

### Dentro de `jsfx-plugin-designer`, el workflow fuerza el uso de sub-skills:

1. El meta-skill dice "lee el manifesto" → `reaforge_get_api_reference("jsfx-design-patterns/00-manifesto")`
2. El meta-skill dice "consulta los primitivos" → `reaforge_get_api_reference("00-index")`
3. El meta-skill dice "valida sintaxis" → `context7_query-docs("/websites/reaper_fm_sdk_js", ...)`
4. El meta-skill dice "self-check antes de guardar" → 8 checkboxes obligatorios

**El LLM no puede saltear pasos porque el SKILL.md los enumera en orden y dice "MANDATORY" y "do not skip steps".**

---

## 7. PRÓXIMO PASO

Abrir SDD change: `2026-06-17-reaforge-dsp-design-patterns` que cubre:
- Fase 1: Design patterns + Skills (prioridad — resuelve el gap de diseño HOY)
- Fase 2: Algoritmos de Joep Van Lier (ya JSFX, cero traducción)
- Fase 3: Algoritmos de FAUST/STK (traducción)
- Fase 4: Spectral (deferred)
- Fase 5: Engram feeding (ongoing)

3 PRs:
- PR 1: 6 design pattern files + 3 SKILL.md + embed + regen
- PR 2: 5 algoritmos de Joep Van Lier + 2 de musicdsp + embed
- PR 3: 4 algoritmos de FAUST/STK traducidos + embed
