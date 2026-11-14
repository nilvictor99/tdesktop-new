# Plan de implementación — Fix carga completa "Fotos + Archivos" y "Todo"

> **ESTADO: ✅ IMPLEMENTADO — Técnica AVANZADA (ronda 12), a la espera de
> compilación y verificación por el usuario.** Reemplaza la técnica reactiva
> de la ronda 11 por una **materialización masiva dirigida por ids concretos**.
> Registro del diseño; los cambios aplicados se detallan en
> [11-DIARIO-RONDAS.md](11-DIARIO-RONDAS.md), ronda 12.

**Repositorio:** `C:\Users\VICTOR\code\tdesktop`

---

## 1. Problema

En canales/grupos grandes, los módulos **"Fotos + Archivos"** (`FilesPhotos`) y
**"Todo"** (`All`) al hacer scroll llegaban a **zonas donde solo se ven fotos o
solo se ven archivos** (faltan elementos que sí existen en el historial). El
módulo **Media** (PhotoVideo) siempre muestra todo de forma continua.

### 1.1 Síntomas exactos
- Zonas con celdas ausentes (hueco visual) sin placeholders.
- "Mostrar en el chat" (jumpToMessage) "arreglaba" la zona al instante.
- Los logs `[FILESPHOTOS] fillSections` mostraban `noItem=0` en la apertura
  (todos los ítems recién materializados), pero el problema aparece en zonas
  profundas al desplazarse.

### 1.2 Causa raíz (determinada)
- La cuadrícula se construye en `Provider::fillSections` iterando `_slice`.
- Para cada id llama `getLayout` → `createLayout` →
  `session.data().message(computeFullId(id))`.
- **Si ese `HistoryItem` no está materializado en memoria**, `createLayout`
  devuelve `nullptr` y la celda se **omite** → hueco.
- En canales grandes, los ids del slice pueden no estar materializados (el
  `HistoryItem` fue evictado o la cobertura de la búsqueda no se materializó a
  tiempo en esa región). Media no lo sufre de forma visible porque usa UNA sola
  secuencia continua y todos sus ítems están recién buscados al construir.
- El mecanismo que "arreglaba" la zona (`jumpToMessage`) hace un **fresh search
  `Around`** que **materializa** los mensajes y **repara la cobertura** de
  storage, y luego `refreshViewer` reconstruye la slice mostrando el contenido.

## 2. Solución (cambio mínimo y localizado)

**Auto-reparación (self-healing):** cuando `fillSections` encuentra un id en
`_slice` cuyo layout no se puede crear (mensaje no materializado) para los
tipos compuestos `FilesPhotos`/`All`, se pide automáticamente una búsqueda
compuesta `Around` de ese id (exactamente la lógica que ya funciona en
"Mostrar en el chat"), lo que **materializa** los mensajes de foto+archivo
(+video en All) y **repara la cobertura** de storage. Luego la slice se
reconstruye y la zona se rellena.

### 2.1 Archivos tocados
- `info/media/info_media_provider.h`
- `info/media/info_media_provider.cpp`
- `apiwrap.cpp` (solo retirada de logs temporales `[FILESPHOTOS]`/`[ALL]`)

### 2.2 Cambios
1. **`info_media_provider.h`**: nuevo miembro `bool _missingRequested = false;`
   (guarda antiloop: un solo request pendiente de auto-reparación) y
   declaración `requestMissingAround(UniversalMsgId)`.
2. **`info_media_provider.cpp`**:
   - `fillSections`: al encontrar un id sin layout **y** siendo
     `FilesPhotos`/`All`, llamar `requestMissingAround(universalId)` y
     continuar (no renderiza hueco; la zona se rellena al reconstruir).
     Media/Archivos: sin cambio (los null se siguen saltando como antes).
   - `requestMissingAround`: si `_missingRequested` está activo, no hace nada;
     si no, lo activa y llama
     `session().api().requestSharedMedia(peer, topicRootId, monoforumPeerId,
     _type, id, Around)` (despacha al merge compuesto correcto: 2 carriles para
     FilesPhotos, 3 para All) → materializa + repara storage.
   - `refreshViewer`: al recibir cada slice nueva se limpia `_missingRequested`
     (un solo request por zona; se reactiva en la siguiente zona con fallos).
3. **`apiwrap.cpp`**: se retiran los `LOG` de diagnóstico `[FILESPHOTOS]` y
   `[ALL]` de `requestSharedMediaFilesPhotos`, `requestSharedMediaAll` y
   `sharedMediaDone` (ya cumplieron su propósito).

## 3. Por qué es seguro
- **Media y Archivos NO cambian**: el guard `(_type == FilesPhotos || All)`
  limita la auto-reparación a los dos módulos compuestos.
- **No hay bucle infinito**: `_missingRequested` impide más de un request
  simultáneo; se limpia en la siguiente actualización de slice. Si el servidor
  no devuelve el ítem (mensaje borrado), la actualización de slice también
  llega (con count), limpiando el flag.
- **Orden cronológico / agrupación por mes / selección múltiple**: no se tocan.
- **Rendimiento**: se pide solo la zona que falla (límite por página), no todo
  el historial de golpe.

## 4. Verificación (tras compilar con el comando habitual)
1. Abrir canal/grupo grande → Perfil → **"Fotos + Archivos"**.
2. Hacer scroll por TODAS las zonas: fotos y archivos deben verse mezclados,
   sin huecos ni "zonas solo archivos"/"solo fotos".
3. Repetir con **"Todo"** (fotos+videos+archivos).
4. Confirmar que **Media** (foto+video) sigue funcionando idéntico.
5. Confirmar que **Archivos** (File) con cuadrícula Lista/4/6/8 no cambia.
6. Probar en canal normal, grupo, y grupo con temas.
7. Selección múltiple, reenvío y borrado masivo deben seguir funcionando.

---

## APÉNDICE — Ronda 12: técnica avanzada (materialización masiva por ids)

### Por qué la ronda 11 no bastó
La técnica reactiva (una búsqueda compuesta `Around` por hueco, con guard de un
solo request) re-materializaba una pequeña zona alrededor del primer id que
fallaba. Si la ventana visible tenía varios huecos esparcidos —o el `History`
se había descargado (`History::clear(ClearType::Unload)`, p. ej. al venir del
chat)—, una sola búsqueda no cubría todos y los huecos persistían hasta que la
zona se volvía a tocar.

### ¿Por qué persisten los huecos? (causa confirmada)
`createLayout` → `session.data().message(FullMsgId)` solo consulta la tabla en
memoria (`Data::MessageIds`). Si el `HistoryItem` no está materializado
(descargado o nunca traído en esa región), devuelve `nullptr` y la celda se
omite. La ronda 11 dependía de que un `search Around` a través del merge
compuesto devolviera el item exacto, lo que no está garantizado para toda la
ventana.

### Técnica avanzada: `messages/channels.getMessages` por ids concretos
En lugar de buscar "alrededor de un id", ahora se **materializan exactamente
los ids que faltan**, en masa y en una sola pasada:

1. `fillSections` detecta un id sin layout (FilesPhotos/All) →
   `requestMissingAround(universalId)`.
2. Se recorren TODOS los ids de `_slice`; los que no tienen `HistoryItem`
   (`session.data().message(full) == null`) se recogen en `missing`.
3. Cada id se materializa vía `api().requestMessageData(peer, msgId, done)`.
   `ApiWrap::requestMessageData` agrupa los ids (con `afterDelay`) en una sola
   llamada `messages.getMessages` / `channels.getMessages` →
   `Session::processExistingMessages` → `addNewMessage(Existing)` que los
   materializa en `Data::MessageIds`.
4. Se cuentan las respuestas (un call-back por id, incluso si el mensaje ya no
   existe → el guard siempre se libera); cuando todas llegan se lanza
   `_refreshed` → `refreshRows()` → reconstrucción con las celdas presentes.

Así una sola reconstrucción rellena TODA la región visible, donde antes harían
falta muchas búsquedas `Around` de una en una.

### Seguridad de la ronda 12
- **Guard `_missingRequested`**: una sola pasada de materialización en vuelo.
- **Callback contra use-after-free**: el estado se guarda en un `shared_ptr`
  con un `bool alive`; `_lifetime.add(...)` lo marca muerto si el `Provider` se
  destruye antes de que respondan, y el call-back comprueba `alive` antes de
  tocar `this`.
- **Media y Archivos NO cambian** (el guard solo aplica a FilesPhotos/All).
- **Mensajes borrados**: no materializan, pero su call-back cuenta igual →
  sin bloqueos ni bucles.
- **Carga acotada**: el `_idsLimit` típico (≈60–100 ids) queda dentro del
  límite razonable de `messages.getMessages`; el `RequestType::History` y el
  batching de `requestMessageData` evitan saturación.

---

## APÉNDICE — Ronda 13: batching por lotes (refinamiento de la materialización)

`messages.getMessages` / `channels.getMessages` tienen un límite de ids por
llamada (≈100–200). El enfoque de la ronda 12 pedía todos los ids faltantes de
la slice de golpe; si una slice tiene varios cientos de huecos (scroll rápido
o Historial muy descargado) la llamada podía superar el límite y no materializar
todo de forma fiable.

### Cambio
`requestMissingAround` ahora materializa los ids sin `HistoryItem` en **tandas
(en lotes) de ≤100 ids encadenadas**:
- `state->requestNextBatch()` lanza el siguiente lote de ids
  (`requestMessageData` por cada uno, que se agrupan en una sola
  `getMessages`).
- `onOneResponse` decrementa `pendingInBatch`; cuando un lote termina
  completamente, llama a `state->requestNextBatch()` para lanzar el siguiente.
- Cuando no quedan más ids → `_missingRequested = false` + `_refreshed.fire({})`
  → reconstrucción con TODA la región visible llena.

### Seguridad de la ronda 13
- **Sin auto-referencia dangling**: `requestNextBatch` y `onOneResponse` viven
  dentro de `MaterializeState` (un `shared_ptr` mantenido vivo por los
  call-backs de `ApiWrap`), y se llaman por indirección vía `state->` — nunca
  se capturan referencias a variables del stack de `requestMissingAround`.
- **Guard `alive` + `_lifetime`**: mismo patrón que la ronda 12 (sin
  use-after-free).
- Cada lote es ≤100 ids → se respeta el límite de `channels.getMessages` y se
  materializan DE FORMA FIABLE todos los huecos, sean pocos o muchos.
- El usuario compila y verifica que al hacer scroll ya no quedan huecos ni hace
  falta "Mostrar en el chat".

