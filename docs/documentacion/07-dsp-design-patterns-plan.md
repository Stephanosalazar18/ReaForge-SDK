# ReaForge — DSP Design Patterns: Plan de Acción

> Phase 2 del DSP Encyclopedia. Nivel 3: Conocimiento de DISEÑO, no de código.

## 1. El gap identificado

El LLM tiene **16 primitivos** (código que copia y pega) y una **matriz de compatibilidad** (qué puede combinar). Pero no sabe **DISEÑAR**. El síntoma:

| Lo que hace | Lo que debería hacer |
|---|---|
| Agrega un slider de `Time (ms)` Y un selector de modos (dotted, triplet) juntos | Elegir UNO: o slider con ms, o selector de modos con base BPM. No ambos. |
| Copia primitivos sin entender el orden de señal | Respetar el orden canónico: utilities → filters → saturation → dynamics → delays → reverb → modulation → pitch |
| Genera 15 sliders para un efecto simple | Agrupar parámetros relacionados, ocultar avanzados, simplificar la UI |
| No sabe cuándo poner makeup gain | Cada etapa de procesamiento debe mantener nivel constante: output ≈ input |

**Causa raíz**: los primitivos son código. El diseño es criterio. El LLM tiene el primero pero no el segundo.

## 2. Fuentes disponibles

| Fuente | Qué aporta | Cómo se accede |
|---|---|---|
| **[JSFX SDK docs](https://www.reaper.fm/sdk/js/js.php)** | Referencia canónica de sintaxis, funciones, variables, secciones | context7 (`/websites/reaper_fm_sdk_js`, 292 snippets) |
| **[ReaPack](https://reapack.com/repos)** | Miles de efectos JSFX reales con patrones de diseño maduros | Clonar repos, extraer `desc:` y estructura de sliders |
| **[stash.reaper.fm](https://stash.reaper.fm/)** | Efectos de la comunidad, variedad de estilos y calidades | Navegación manual, los mejores están en ReaPack |
| **DSP textbooks** | RBJ cookbook, Smith DSP (ya referenciados en primitivos) | CCRMA, W3C |
| **Nuestros 16 primitivos** | Ya embebidos en la DLL, validados | `reaforge_get_api_reference()` |

## 3. Qué construir: Nivel 3 — Design Patterns

### Estructura de archivos

```
docs/api_reference/
├── jsfx-design-patterns/           # NUEVO: Nivel 3
│   ├── 00-manifesto.md             # LECTURA OBLIGATORIA antes de generar código
│   ├── 01-parameter-design.md      # Cuándo usar modos vs sliders, agrupación, presets
│   ├── 02-signal-flow.md           # Orden canónico, parallel/serial, feedback, gain staging
│   ├── 03-ui-conventions.md        # Slider ordering, naming, @gfx, metering
│   ├── 04-gain-staging.md          # Makeup gain, dry/wet, nivel constante entre etapas
│   └── 05-anti-patterns.md         # Errores comunes: sliders redundantes, sin DC block, etc.
├── jsfx-primitives/                # Nivel 2: código (ya existe)
├── jsfx-algorithms/                # Nivel 3: algoritmos (ya existe)
└── 00-index.md                     # Nivel 2: matriz (ya existe, se actualiza)
```

### Contenido del `00-manifesto.md` (ejemplo)

```markdown
# DSP Design Manifesto

## Before you generate ANY effect, follow these rules.

### 1. One concept, one slider
Don't add a slider "just in case." Every slider must change the sound audibly.
If two controls do similar things, merge them into a mode selector.

### 2. Respect the signal chain
utilities → filters → saturation → dynamics → delays → reverb → modulation → pitch
DC blocking ALWAYS after saturation. Denormal prevention ALWAYS.

### 3. Gain stage every stage
After each processing block, output level ≈ input level.
Use makeup gain. Don't let the signal get quieter with each stage.

### 4. Group parameters logically
- Input section first (drive, threshold, input gain)
- Processing section second (the effect's core parameters)
- Output section last (makeup gain, dry/wet, output level)

### 5. Name sliders for musicians, not programmers
"Warmth" not "Lowpass Frequency"
"Character" not "Asymmetry Bias"
"Space" not "Feedback Delay Time"

### 6. Start simple, add complexity only when needed
A 4-slider effect that sounds great > a 20-slider effect that's confusing.
Hide advanced parameters. Expose only what matters.
```

### Contenido de `01-parameter-design.md` (ejemplo)

```markdown
# Parameter Design Patterns

## Pattern 1: Mode selector vs. Separate sliders

**When to use a mode selector:**
- The parameter has discrete, named options (dotted, triplet, straight)
- The options are mutually exclusive
- Example: Delay Mode {Straight, Dotted, Triplet}

**When to use a slider:**
- The parameter is continuous (0-100%, 20-20000 Hz)
- The user needs fine control
- Example: Delay Time (ms)

**ANTI-PATTERN: Don't use BOTH for the same concept.**
If you have a Mode selector, derive the time from BPM and mode.
If you have a Time slider, let the user dial it in manually.
Do NOT put both in the same effect.

## Pattern 2: Hidden vs. Visible sliders
Use `sliderX:-Hidden parameter` for:
- Internal state that the user shouldn't touch
- Parameters that are set once and forgotten
- Debug values that only matter during development
```

## 4. Plan de implementación (3 fases)

### Fase 1: Extracción de patrones (2-3 horas)
- **Fuente**: ReaPack top 50 JSFX effects
- **Método**: El LLM analiza cada efecto y extrae:
  - Cómo organiza los sliders (orden, agrupación, nombres)
  - Cómo maneja gain staging
  - Qué patrones de UI usa (@gfx, metering)
  - Qué anti-patrones evita
- **Output**: 6 archivos markdown en `jsfx-design-patterns/`

### Fase 2: Embedding y regeneración (30 min)
- Agregar los 6 archivos al auto-discover de `gen_api_reference_header.py`
- Regenerar `api_reference_data.h`
- Rebuild DLL

### Fase 3: Integración con el `00-index.md` (15 min)
- Agregar una sección "Design Patterns" en el índice
- Referenciar el `00-manifesto.md` como lectura obligatoria
- Actualizar `gen_dsp_index.py` para incluir los patterns en el índice

## 5. Cómo lo usa el LLM

**Antes** (sin patterns):
```
Usuario: "generame un delay con modos dotted/triplet"
LLM: copia feedback-delay.md, agrega slider Time Y slider Mode → 2 controles redundantes
```

**Después** (con patterns):
```
Usuario: "generame un delay con modos dotted/triplet"
LLM: lee 00-manifesto.md → "One concept, one slider"
     lee 01-parameter-design.md → "Pattern 1: Mode vs Slider — don't use both"
     Genera: SOLO un Mode selector {Straight, Dotted, Triplet}
     Deriva el tiempo del BPM vía srate y tempo
```

## 6. Esfuerzo y entregables

| Fase | Qué | Tiempo |
|---|---|---|
| 1 | Extraer 6 design patterns de ReaPack + SDK docs | 2-3h |
| 2 | Embedder en DLL + regen | 30min |
| 3 | Integrar con índice | 15min |
| **Total** | | **3-4h** |

## 7. Próximo paso

Abrir SDD change: `2026-06-17-reaforge-dsp-design-patterns` con:
- explore.md → este documento
- proposal.md → scope: 6 design pattern files + manifesto
- specs/ → formato de cada pattern, criterios de extracción de ReaPack
- design.md → integración con el auto-discover generator
- tasks.md → 3 PRs (patterns + embed + index update)
