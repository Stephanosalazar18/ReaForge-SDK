# ReaForge — Primitivos de REAPER: Marco Contextual

> Exploración pre-SDD. Define el problema, el alcance, y las preguntas abiertas antes de abrir un change formal.

## 1. El problema que resolvemos

### Síntoma

El LLM genera `denorm = 1 <!> e-25;` en vez de `denorm = 1e-25;`. El `<!>` es markup de documentación que se filtró al código.

### Causa raíz

El LLM tiene **sintaxis de JSFX** (nuestro cheatsheet de ~100 líneas le dice cómo escribir sliders, `@sample`, `@block`) pero NO tiene **patrones DSP**. Sabe que existe `tanh()` pero no sabe cuándo usarlo para saturación vs. compresión. Sabe que existe `biquad()` pero no tiene los coeficientes RBJ. El resultado es código que **compila pero no suena bien** — o directamente no compila.

### Lo que falta

Una **biblioteca de building blocks DSP** — fragmentos de código JSFX real y funcional que el LLM puede **copiar literalmente y combinar**. No pseudocódigo. No documentación. Código que ya funciona en REAPER.

---

## 2. El concepto de "primitivo"

Un primitivo es un **fragmento de código JSFX autocontenido** que implementa UNA técnica DSP. Tiene:

| Elemento | Ejemplo (saturation/tanh) |
|---|---|
| **Nombre** | Soft tanh saturation |
| **Código** | `spl0 = tanh(spl0 * drive);` |
| **Parámetros** | `drive` (slider, 1-10) |
| **Caso de uso** | Warm tube/tape saturation, subtle mastering saturation |
| **Precaución** | DC offset puede acumularse si se usa en serie sin high-pass |

El LLM combina primitivos como piezas de Lego:

```
Usuario: "tape saturation with wow and flutter"

LLM:
  1. get_api_reference("jsfx/saturation")  → primitivo tanh_saturation
  2. get_api_reference("jsfx/modulation")  → primitivo lfo_sine
  3. Compone: tanh(in * drive) + lfo_mod en el mismo @sample
  4. get_api_reference("jsfx/utilities")   → primitivo dc_block
  5. Agrega dc_block al final del chain
```

---

## 3. Catálogo de primitivos propuesto

### JSFX (efectos de audio)

| Categoría | Primitivos | Complejidad |
|---|---|---|
| **Saturation** | tanh soft clip, asymmetric tanh, hard clip, tube emulation, tape hysteresis | Media |
| **Filters** | RBJ lowpass/highpass/bandpass/peak/shelf, one-pole LP/HP, state variable, Moog ladder | Alta |
| **Dynamics** | RMS envelope follower, peak detector, soft knee compressor, expander, gate | Alta |
| **Delays** | Feedback delay, ping-pong, tap delay, modulated delay (chorus/flanger), allpass | Media |
| **Modulation** | Sine/triangle/saw LFO, sample-and-hold, smoothed random, envelope generator | Baja |
| **Reverb** | Schroeder, feedback delay network (FDN), early reflections, plate | Alta |
| **Pitch** | Linear interpolation pitch shift, granular, harmonizer | Alta |
| **Spectral** | FFT analysis (basic), spectral freeze, crossover (2-band, 3-band) | Muy alta |
| **Utilities** | DC blocking, stereo width, M/S encode/decode, gain staging, denormal prevention | Baja |

### ReaScript Lua (flujos de trabajo)

| Categoría | Primitivos |
|---|---|
| **MIDI** | Select notes by velocity range, scale velocity, quantize timing, transpose, humanize |
| **Items** | Split at cursor, glue items, normalize, reverse, duplicate |
| **Tracks** | Create track from template, color tracks, route audio, solo/mute selected |
| **Automation** | Create envelope point, linear ramp, LFO to envelope, show/hide envelope |
| **Project** | Get BPM, get time signature, get selected region, render queue |

### FX Chains (combinaciones)

| Categoría | Primitivos |
|---|---|
| **Vocal** | Vocal slap, de-esser chain, doubling chain, telephone effect |
| **Mastering** | Limiter + EQ + multiband, loudness measurement chain |
| **Creative** | Parallel distortion, shimmer reverb, reverse reverb |

---

## 4. Arquitectura de distribución

Los primitivos se **embeeben en la DLL** igual que los 3 cheatsheets actuales, vía `tools/gen_api_reference_header.py`. Cada primitivo es un archivo `.md` con código JSFX/Lua real.

### Jerarquía de archivos

```
docs/api_reference/
├── jsfx.md                          # Sintaxis general (ya existe)
├── jsfx-primitives/
│   ├── 00-index.md                  # Índice navegable (el LLM consulta esto primero)
│   ├── saturation.md                # ~80 líneas de código JSFX real
│   ├── filters.md
│   ├── dynamics.md
│   ├── delays.md
│   ├── modulation.md
│   ├── reverb.md
│   ├── pitch.md
│   ├── spectral.md
│   └── utilities.md
├── reascript_lua.md                 # Sintaxis general (ya existe)
├── reascript_lua-primitives/
│   ├── 00-index.md
│   ├── midi.md
│   ├── items.md
│   ├── tracks.md
│   ├── automation.md
│   └── project.md
└── fx_chain-primitives/             # (ya existe: template + vocal-slap)
    ├── chain-builder-template.md
    └── recipes/
        ├── vocal-slap.md
        ├── mastering.md
        └── creative.md
```

### Cómo se accede desde el bridge

El LLM llama:

```
reaforge_get_api_reference("jsfx-primitives/saturation")
reaforge_get_api_reference("jsfx-primitives/00-index")
reaforge_get_api_reference("reascript_lua-primitives/midi")
```

El `api_reference_map()` en `project_reader.cpp` ya soporta sub-paths (lo arreglamos en el commit `a1e5d51`). Cada archivo nuevo se agrega a `SOURCES` en el generador y se regenera el header.

---

## 5. Preguntas abiertas (a resolver en el SDD)

### 5.1 ¿Quién escribe los primitivos?

| Opción | Ventaja | Desventaja |
|---|---|---|
| **El LLM** (le pedimos que genere los primitivos y los validamos) | Rápido, escala | Requiere validación humana (el LLM puede alucinar DSP también) |
| **Humano** (escribimos cada primitivo a mano con fuentes DSP verificadas) | 100% correcto | Lento, no escala a 50+ primitivos |
| **Híbrido** (LLM genera borrador, humano revisa y corrige) | Balance velocidad/calidad | Requiere expertise DSP del revisor |

**Recomendación**: híbrido. El LLM genera cada primitivo basado en fuentes conocidas (RBJ cookbook, Smith DSP, Zölzer DAFX). El humano verifica que el código JSFX compila y suena correctamente en REAPER.

### 5.2 ¿Cuántos primitivos en la primera iteración?

**Recomendación**: 10-15 primitivos en 5 categorías (saturation, filters, dynamics, delays, modulation). Suficiente para cubrir el 80% de los pedidos de efectos. El resto se agrega iterativamente.

### 5.3 ¿Cómo se valida un primitivo?

Cada primitivo debe:
1. **Compilar** en REAPER (copiar y pegar en un JSFX nuevo → sin errores de sintaxis)
2. **Sonar** correctamente (audio de prueba → el efecto produce el resultado esperado)
3. **Ser componible** (dos primitivos juntos en el mismo `@sample` no producen glitches)

### 5.4 ¿Los primitivos reemplazan o complementan los cheatsheets actuales?

**Complementan.** Los cheatsheets (`jsfx.md`, `reascript_lua.md`) siguen siendo la referencia de sintaxis. Los primitivos son la referencia de **diseño DSP**.

### 5.5 ¿Cómo evitamos que el LLM mezcle primitivos incompatibles?

El archivo `00-index.md` incluye una **matriz de compatibilidad**: qué primitivos se pueden combinar en serie, cuáles requieren procesamiento intermedio (DC blocking, gain staging), y cuáles son mutuamente excluyentes.

---

## 6. Esfuerzo estimado

| Fase | Qué | Quién | Tiempo |
|---|---|---|---|
| **1. Setup** | Expandir `gen_api_reference_header.py` para N archivos. Crear estructura de carpetas. | Código | 30 min |
| **2. Generación** | LLM genera 10-15 primitivos (borrador). | LLM | 2-3 horas |
| **3. Validación** | Humano prueba cada primitivo en REAPER. Corrige errores. | Humano | 3-5 horas |
| **4. Index** | Escribir `00-index.md` con matriz de compatibilidad. | Humano + LLM | 1 hora |
| **5. Regeneración** | `gen_api_reference_header.py` → regen header → rebuild DLL | Código | 15 min |
| **6. Prueba end-to-end** | Pedirle a opencode efectos complejos usando los primitivos | Humano + LLM | 1 hora |

**Total**: 8-12 horas (la mayor parte es validación humana de los primitivos).

---

## 7. Riesgos

| Riesgo | Mitigación |
|---|---|
| El LLM alucina DSP incluso con primitivos | La matriz de compatibilidad en `00-index.md` fuerza al LLM a seguir reglas. Si un primitivo dice "requiere DC blocking después", el LLM debe incluir `utilities/dc_block`. |
| Los primitivos no cubren un caso de uso común | Iterativo: cada vez que un usuario pide algo que falla, se agrega el primitivo faltante. |
| La DLL crece demasiado (cada primitivo son ~2-5 KB de markdown) | 50 primitivos × 3 KB = 150 KB. La DLL actual es 19 MB. El overhead es insignificante. |
| El LLM usa primitivos obsoletos (versión vieja de la DLL) | El bridge devuelve la versión embebida en la DLL cargada. Si el usuario no actualiza la DLL, usa los primitivos viejos. Documentar el proceso de update. |

---

## 8. Próximo paso

Abrir un SDD change: `2026-06-XX-reaforge-dsp-primitives` con:
- **explore.md** → este documento
- **proposal.md** → scope preciso: 10-15 primitivos en 5 categorías, validación híbrida
- **specs/** → formato de cada primitivo, matriz de compatibilidad, criterios de validación
- **design.md** → cómo se integra con `gen_api_reference_header.py` y el bridge
- **tasks.md** → PRs: setup, generación batch, validación, index, regen DLL, prueba
