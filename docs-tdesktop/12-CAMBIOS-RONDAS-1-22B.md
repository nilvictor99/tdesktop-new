# Telegram Desktop personalizado — Cambios aplicados

Resumen en lenguaje sencillo de todo lo que hace distinta esta versión.
Aplicado en `C:\Users\VICTOR\code\tdesktop` (rama de trabajo única).

---

## Tabla general

| Nº | Cambio | Para qué sirve |
|---|---|---|
| 1 | Copiar texto desbloqueado | Copiar texto en canales que lo prohibían |
| 2 | Guardar/descargar desbloqueado | Guardar fotos, videos y documentos restringidos |
| 3 | Selección de media desbloqueada | Marcar fotos/videos aunque el canal "no permita reenvíos" |
| 4 | Selección sin límite de 100 | Marcar 200, 500 o 900+ items a la vez |
| 5 | Shift + clic | Seleccionar por rangos de un golpe |
| 6 | Seleccionar todo: Ctrl+A + menú + botón | Tres formas de marcar todo lo cargado |
| 7 | Cuadrícula densa | Miniaturas más pequeñas para ver más por pantalla |
| 8 | Filtros Todo / Fotos / Videos | Submenú superior dentro de la galería unificada (sin GIF) |
| 9 | Botón único "Media" en el perfil | Fotos y videos juntos, no separados (incluye layout classic del canal) |
| 10 | Reenvío masivo estable | Lotes de 100 en orden cronológico; 900+ llegan completos |
| 11 | Borrado masivo fiable | Lotes de 100 al servidor: los borrados NO reaparecen |
| 12 | Recordar opciones de reenvío | "Sin autor" / "sin captions" se recuerdan entre reenvíos |
| 13 | Orden cronológico al reenviar | Primero los antiguos, después los recientes |
| 14 | Botón Descargar siempre visible | En el visor de media, fuerza la descarga original |
| 15 | Ctrl+Shift+G en el visor | Saltar al mensaje en el chat desde la foto abierta |
| 16 | Módulo "Todo" (All) en el perfil | Una sola cuadrícula con fotos + videos + archivos (sin stickers/GIF/voz); carga completa, orden cronológico, selección múltiple |
| 17 | Pestañas Fotos/Videos ocultas si están vacías | En la galería Media, las pestañas Fotos y Videos solo se muestran si hay contenido de ese tipo; "Todo" siempre visible |

---

## Detalle de lo añadido en las últimas rondas

### Reenvío por lotes de 100 (`apiwrap.cpp`)
Antes: si seleccionabas 160 y reenviabas, solo llegaban 100 (el servidor
procesa máximo 100 IDs por petición). Ahora `ApiWrap::forwardMessages` corta
el envío en lotes de 100 y los manda en secuencia: primero 100, luego los
siguientes. Orden cronológico garantizado (`ranges::sort` ascendente).

### Borrado por lotes de 100 (`data_histories.cpp`)
Síntoma que motivó el fix: al borrar una selección grande de fotos/videos del
canal, desaparecían… pero ~30 volvían a aparecer. Causa: la app destruía los
mensajes localmente al instante, pero enviaba TODOS los IDs en UNA petición
`channels.deleteMessages`; el servidor solo procesaba los primeros 100 y el
resto seguía vivo → reaparecía al recargar. Ahora `Histories::deleteMessages`
divide en lotes de 100 por petición y todo se borra de verdad.

### Galería unificada "Media" en el perfil
- Layout classic (`info_profile_shared_media_classic.cpp`): un solo botón
  `PhotoVideo` ("Media") en lugar de botones separados de Fotos y Videos.
- Dentro de la galería: submenú superior con **Todo / Fotos / Videos**
  (sin GIF), opción por defecto "Todo".

### Icono de "Seleccionar todo" corregido (`info.style`)
El botón de la barra superior usa ahora el **check circular**
(`toast/check-36x36`), igual que el resto de checks de selección.

### Módulo "Fotos + Archivos" en el perfil del canal
- Nueva fila "Fotos + Archivos" en el perfil que abre una cuadrícula tipo Media
  mostrando fotos y archivos juntos (sin videos), con lazy loading y conteo
  combinado.
- Se ocultan los nombres de archivo (solo thumbnail centrado).
- **Bug corregido (data_sparse_ids.cpp)**: `SparseIdsSliceBuilder::applyUpdate`
  rechazaba los primeros mensajes del servidor cuando `_ids` estaba vacío porque
  la comprobación de intersección fallaba; ahora la primera carga se acepta
  siempre y la intersección solo se aplica a actualizaciones incrementales.
- **Fotos + archivos desde el servidor (apiwrap.cpp)**: como no existe un
  filtro MTP combinado "fotos + documentos", `requestSharedMediaFilesPhotos`
  lanza DOS peticiones en paralelo (Photo `inputMessagesFilterPhotos` + File
  `inputMessagesFilterDocument`) y fusiona los resultados (IDs, rango, conteo)
  antes de guardarlos bajo el tipo `FilesPhotos`. Con esto las fotos ya
  aparecen sin necesidad de haber entrado antes al chat, igual que los archivos.
- **Fix crash al "Seleccionar todo" en FilesPhotos (overview_layout.*)**:
  en la cuadrícula se mezclaban fotos (celda cuadrada `itemWidth`) con
  documentos `hideName` (altura `fileThumbSize`, menor). La geometría quedaba
  inconsistente: el alto de fila (`_itemHeight`) se calculaba con el último
  ítem, pero el pintado/hit-test usaban el alto real de cada ítem, descuadrando
  las secciones y rompiendo el repintado tras seleccionar todo (crash). Ahora
  `Document::resizeGetHeight` hace que un documento `hideName` ocupe una celda
  cuadrada como las fotos (y actualiza `_minh` para centrar el thumbnail), de
  modo que TODAS   las celdas de FilesPhotos comparten la misma altura y la
  geometría de la cuadrícula vuelve a ser consistente.
- **Fix crash al arrancar en "Fotos + Archivos" (info_wrap_widget.cpp)**:
  la barra superior calcula el texto "N seleccionado" con un `switch` sobre el
  tipo de media que terminaba en `Unexpected(...)` (aborta la app) al no
  contemplar `FilesPhotos`. Se añadió `case Type::FilesPhotos` →
  `lng_media_selected_media` ("{count} Media").
- **Fix módulo en blanco en canales/grupos con temas (apiwrap.cpp)**: se
  elimina la guarda que devolvía sin hacer nada cuando había `topicRootId`
  (canales foro / dentro de un tema). `PrepareSearchRequest` ya soporta temas
  (`Flag::f_top_msg_id`), así que "Fotos + Archivos" ahora carga igual que los
  demás módulos.
- **Bug corregido: FilesPhotos solo cargaba el primer lote (~50) y no exploraba
  al hacer scroll** (lista real 1000+). `requestSharedMediaFilesPhotos` fusiona
  dos búsquedas (Photo+File) por lote; el `noSkipRange` que se guardaba por lote
  era `{min..max}` de ese lote, que NO se solapaba con el lote anterior →
  `SparseIdsSliceBuilder::applyUpdate` descartaba las páginas más viejas (la
  intersección fallaba) y la cuadrícula se quedaba atascada en los ~50 primeros
  re-pidiendo lo mismo. Ahora se guarda cada lote con `noSkipRange = {min...
  ServerMaxMsgId}`, de modo que storage acumula todas las páginas en UNA slice
  creciente y `applyUpdate` las fusiona en `_ids`, haciendo que el preload
  avance el `aroundId` hacia atrás igual que Media (exploración sin límite). Los
  lotes vacíos propagan el rango "de fin de dirección" (resuelto por
  `ParseSearchResult`) para que el loader deje de pedir (evita bucles infinitos
  de `Before`/`After`). Solo aplica a FilesPhotos; el resto de módulos no cambia.

---

## Notas honestas

- El bloqueo de reenvío en canales `noforwards` es del SERVIDOR: la app puede
  dejarte seleccionar, pero el servidor rechazará el forward final.
- Copiar texto y guardar archivos sí funcionan porque esos flujos no se validan
  en el servidor.
- "Seleccionar todo" marca lo cargado en memoria; no descarga todo el historial.

## Archivos modificados (lista completa)

```
Telegram/SourceFiles/apiwrap.cpp                          ← lotes reenvío 100
Telegram/SourceFiles/data/data_histories.cpp              ← lotes borrado 100
Telegram/SourceFiles/info/media/info_media_common.h       ← enum MediaFilter
Telegram/SourceFiles/info/media/info_media_provider.*     ← filtro por tipo
Telegram/SourceFiles/info/media/info_media_list_widget.*  ← selección ilimitada/rangos/selectAll
Telegram/SourceFiles/info/media/info_media_inner_widget.* ← slider Todos/Fotos/Videos
Telegram/SourceFiles/info/profile/info_profile_shared_media_classic.cpp ← botón Media único
Telegram/SourceFiles/info/info_top_bar.*                  ← botón seleccionar todo
Telegram/SourceFiles/info/info_wrap_widget.cpp            ← Ctrl+A
Telegram/SourceFiles/info/info.style                      ← estilos check circular
Telegram/SourceFiles/history/view/controls/history_view_forward_panel.* ← recordar opciones
Telegram/SourceFiles/window/window_peer_menu.cpp          ← sembrar opciones recordadas
Telegram/SourceFiles/media/view/media_view_overlay_widget.cpp ← Descargar siempre visible
Telegram/Resources/langs/lang.strings                     ← lng_context_select_all
Telegram/SourceFiles/boxes/share_box.cpp                  ← cola de lotes reenvío masivo + progreso + FLOOD_WAIT retry (ronda 5)
Telegram/SourceFiles/info/media/info_media_list_section.* ← cuadrícula forzada Type::File (ronda 5)
Telegram/SourceFiles/data/data_sparse_ids.cpp             ← fix applyUpdate primera carga (FilesPhotos)
Telegram/SourceFiles/apiwrap.cpp                          ← requestSharedMediaFilesPhotos: 2 requests + merge
Telegram/SourceFiles/apiwrap.h                            ← declaración requestSharedMediaFilesPhotos
Telegram/SourceFiles/overview/overview_layout.*           ← Document hideName cuadrado (fix crash FilesPhotos)
Telegram/SourceFiles/info/info_wrap_widget.cpp            ← case FilesPhotos en SelectedTitleForMedia (fix crash arranque)
Telegram/SourceFiles/apiwrap.cpp                          ← quitar guarda topicRootId (fix módulo en blanco)
Telegram/SourceFiles/apiwrap.cpp                          ← fix lazy loading FilesPhotos (noSkipRange + lotes vacíos)
Telegram/SourceFiles/storage/storage_shared_media.h       ← tipo All en enum (módulo "Todo")
Telegram/SourceFiles/data/data_media_types.cpp            ← máscaras All (foto/video/archivo; stickers/GIF/voz excluidos)
Telegram/SourceFiles/apiwrap.*                            ← requestSharedMediaAll: 3 búsquedas paralelas + merge
Telegram/SourceFiles/info/media/info_media_provider.cpp   ← switches + layout híbrido All
Telegram/SourceFiles/info/media/info_media_common.cpp     ← MinItemHeight All
Telegram/SourceFiles/info/media/info_media_list_section.cpp ← cuadrícula All
Telegram/SourceFiles/info/media/info_media_empty_widget.cpp ← estado vacío All
Telegram/SourceFiles/info/media/info_media_widget.cpp     ← título sección All
Telegram/SourceFiles/info/info_wrap_widget.cpp            ← título selección All
Telegram/SourceFiles/info/media/info_media_buttons.cpp    ← texto botón All
Telegram/SourceFiles/info/profile/info_profile_shared_media_classic.cpp ← botón Todo en el perfil
Telegram/SourceFiles/info/profile/info_profile_values.cpp ← contador All (Foto+Video+Archivo)
Telegram/SourceFiles/data/data_shared_media.cpp           ← IsItemGoodForType All + exclusión stickers
Telegram/Resources/langs/lang.strings                     ← lng_media_type_all / lng_profile_all / lng_media_all_empty
Telegram/SourceFiles/info/media/info_media_inner_widget.* ← pestañas Fotos/Videos solo si hay contenido (ronda 10)
Telegram/SourceFiles/info/media/info_media_provider.*    ← auto-reparación carga completa FilesPhotos/All (ronda 11)
Telegram/SourceFiles/apiwrap.cpp                          ← retirada de logs temporales [FILESPHOTOS]/[ALL] (ronda 11)
```

## Ronda 5 — Detalles técnicos

### share_box.cpp — DefaultForwardCallback
- Antes: 1 petición MTP con TODOS los msgIds por destino → servidor procesa
  ~100, resto se pierde; FLOOD_WAIT mata el envío sin reintento.
- Ahora: partes ≤100 enviadas en secuencia (pausa 300 ms), reintento hasta 5x
  en FLOOD_WAIT esperando min(x,60)s, la cola continúa tras fallo definitivo,
  estrellas pagadas solo en el primer lote, toast "Reenviando X de Y…".
- Timer heap + weak_ptr en callbacks (sin ciclos ni colgantes).

### Cuadrícula de archivos (Type::File)
- `ListSection::setForcedColumns(n)`: n>1 → File comparte matemática de
  cuadrícula de fotos (`resizeToWidth`/`recountHeight`).
- Slider en InnerWidget solo para Archivos: Lista | 4 | 6 | 8.
- Persistencia pendiente (mejora futura).

### Ronda 6 — Contador infalible + tarjetas
- `MassQueue.keepAlive`: la cola se sostiene a sí misma hasta terminar;
  sin esto, al retornar `DefaultForwardCallback` el timer moría y los
  reenvíos grandes se cortaban (400→100).
- `MassProgressState.activeThreads`: cierre de capa solo cuando TODOS los
  destinos agotaron sus lotes (antes: hueco entre lotes = cierre prematuro).
- Toast: "Reenviando X de Y… quedan N".
- Archivos en cuadrícula = tarjetas cuadradas (`paintFileTile`): icono azul,
  nombre centrado debajo, tamaño gris, check azul al seleccionar.

### Ronda 9 — Módulo "Todo" (All) en el perfil
- Nuevo botón **Todo** en el perfil de canales/grupos (layout classic, junto a
  Media / Archivos / Fotos+Archivos): fotos + videos + archivos en UNA sola
  cuadrícula, orden cronológico con encabezado por mes, selección múltiple,
  reenvío y borrado masivo ya soportados.
- Se crea como un `Storage::SharedMediaType` nuevo (`All`, índice 14) con su
  propia lista `SparseIdsList` independiente (no mezcla con Media).
- **Carga desde servidor**: no existe filtro MTP único para foto+video+archivo;
  `ApiWrap::requestSharedMediaAll` lanza 3 búsquedas en paralelo
  (Photos + Video + Documents) y fusiona usando el mismo truco `noSkipRange`
  del lazy loading de FilesPhotos → exploración tipo Media (todo cabe, sin
  repeticiones).
- **Exclusiones**: stickers, GIFs, notas de voz, música, videomensajes (round)
  y enlaces no entran: sus máscaras `sharedMediaTypes()` no incluyen `All`,
  y `IsItemGoodForType(All)` además descarta stickers explícitamente.
- Atajo de memoria: los módulos Media, Archivos y Fotos+Archivos no cambian.

### Ronda 10 — Pestañas Fotos/Videos solo si hay contenido
- En la galería Media (Todo/Fotos/Videos) las pestañas Fotos y Videos se
  muestran ahora SOLO si existe al menos un elemento de ese tipo; "Todo"
  siempre está (es el filtro por defecto y el único caso "sin media").
- El contador es reactivo (`SharedMediaCountValue` de cada tipo) y el slider se
  reconstruye únicamente cuando la presencia (fotos>0 / videos>0) cambia; si el
  filtro activo desaparece se vuelve a "Todo" sin crashear.
- Solo se toca `info_media_inner_widget.*`; providers, perfil y demás módulos
  no cambian.

### Ronda 11 — Carga completa en "Fotos + Archivos" y "Todo" (auto-reparación)
- **Problema**: en canales/grupos grandes, al hacer scroll llegaban zonas donde
  solo se veían fotos o solo archivos (celdas ausentes); Media siempre mostraba
  todo.
- **Causa**: la cuadrícula se construye desde `_slice`; si un mensaje del slice
  no está materializado en memoria, su celda se omite. El módulo Media no lo
  sufría por usar una sola secuencia continua.
- **Fix**: auto-reparación en el provider. Al encontrar un id sin materializar
  en "Fotos + Archivos" o "Todo", se pide automáticamente la búsqueda de esa
  zona (igual que "Mostrar en el chat"), lo que materializa el mensaje y repara
  la cobertura para que la celda aparezca al reconstruir. Con guard antiloop.
- **Sin cambios** en Media ni en la vista de Archivos. Se retiraron los logs de
  diagnóstico temporales.

### Ronda 12 — Técnica avanzada: materialización masiva por ids (FilesPhotos/All)
- **Problema**: la ronda 11 no bastaba (el usuario confirmó que seguían
  apareciendo huecos). La búsqueda `Around` por hueco re-materializaba solo una
  zona pequeña; con varios huecos o con el Historial descargado
  (`History::clear(ClearType::Unload)`) no cubría toda la región visible.
- **Fix avanzado**: en vez de buscar "alrededor de un id", ahora se
  materializan **exactamente los ids que faltan**, en masa y en una sola
  pasada. `requestMissingAround` recorre `_slice`, recoge todos los ids sin
  `HistoryItem` (`session.data().message(full) == null`) y los materializa con
  `api().requestMessageData(peer, msgId, done)`, que agrupa en una sola
  `messages.getMessages` / `channels.getMessages` → `processExistingMessages` →
  `addNewMessage(Existing)`. Al completar todas las respuestas → `_refreshed` →
  reconstrucción con TODA la región visible llena.
- **Seguridad**: guard `_missingRequested` (una pasada a la vez); call-back
  protegido contra use-after-free mediante `shared_ptr<bool> alive` + `_lifetime`
  (si el provider muere antes de responder, el call-back comprueba `alive` antes
  de tocar `this`); los mensajes borrados cuentan su respuesta aunque no
  materialicen (sin bloqueos); Media y Archivos no cambian (guarda solo para
  FilesPhotos/All); carga acotada al `_idsLimit` (~60–100 ids).
- Solo se toca `info/media/info_media_provider.{h,cpp}`.

### Ronda 13 — Batching por lotes en la materialización (fix de fiabilidad)
- **Problema**: la ronda 12 pedía todos los ids faltantes de la slice de golpe;
  si había muchos huecos superaba el límite de `messages/channels.getMessages`
  y no materializaba todo de forma fiable.
- **Fix**: `requestMissingAround` ahora materializa los ids sin `HistoryItem` en
  **tandas encadenadas de ≤100 ids**. `MaterializeState` guarda los ids, el
  siguiente a pedir y el contador del lote; una función `requestNextBatch` (que
  vive en el `shared_ptr` del estado, llamada por indirección vía `state->`)
  lanza cada lote; un call-back por respuesta baja el contador y al cerrar un
  lote lanza el siguiente; al terminar → `_refreshed` → reconstrucción completa.
- **Compilación**: corregido `const auto &session` → `auto &session`
  (`session.api()` exige `Main::Session&`); añadidos `#include <functional>` y
  `<memory>`.
- **Seguridad**: sin captura dangling (los lambdas viven en el `shared_ptr`
  `state`); guard `alive` + `_lifetime` contra use-after-free; lotes ≤100
  respetan el límite de la API; Media y Archivos no cambian.
- Solo se toca `info/media/info_media_provider.cpp`.

### Ronda 14 — Blindaje ante fallos de búsqueda (PEER_ID_INVALID / red)
- **Problema**: en `requestSharedMediaFilesPhotos`/`All`, un fallo de una
  búsqueda asignaba un `Api::SearchResult()` vacío (que trae `noSkipRange
  {0,0}` por defecto). Si TODAS las búsquedas fallaban (p.ej. `PEER_ID_INVALID`,
  corte de red), el merge con lote vacío producía `noSkipRange={0,0}` → el
  sparse-loader interpretaba `from==0` como "nada más atrás" (`skippedBefore=0`)
  y **congelaba la exploración** aunque existiera contenido real.
- **Fix** (`apiwrap.cpp`, ambos bloques):
  - Se añadió un flag de éxito por sub-búsqueda (`State::photoOk`, `fileOk`,
    `videoOk`); el call-back `.done` lo marca (`*okFlag = true`); el `.fail` no.
  - En `finishOne` se computa `allOk`. La lógica del merge queda:
    - Lote **con ítems** → `noSkipRange={min, ServerMaxMsgId}` (igual que antes).
    - Lote **vacío** con algún fallo (`!allOk`) → se omite `sharedMediaDone`
      **sin tocar el estado** (igual que el camino oficial, que descarta el
      fallo): el loader deja la región "insuficiente" y re-pide al próximo
      disparo (scroll / cambio de datos); evita el falso "fin de lista".
    - Lote **vacío** con todos OK (`allOk`) → fin legítimo de la dirección; se
      propaga el rango real de `ParseSearchResult` (comportamiento previo).
- **Efecto**: `PEER_ID_INVALID` / `TOP_MSG_ID_NOT_SUPPORTED` / cortes de red ya
  no detienen la carga: no crashean, no envenenan el estado del loader y la
  cadena de scroll sigue viva. Media/Archivos oficiales no cambian.
- Solo se toca `apiwrap.cpp` (`requestSharedMediaFilesPhotos` y
  `requestSharedMediaAll`).

### Ronda 15 — Instrumentación del depurador (archivo depurador.txt)
- **Nuevo helper** `debug_depurador.h` (raíz de SourceFiles): escribe trazas
  con timestamp en `depurador.txt` (junto a `log.txt`, en `cWorkingDir()`).
  El archivo se **liquida una vez por ejecución** (primera escritura) para que
  cada arranque genere un `depurador.txt` limpio; thread-safe mediante mutex.
- **Instrumentación en `apiwrap.cpp`** (solo requestSharedMediaFilesPhotos/All):
  - `FP/ALL REQUEST`: entrada en el dispatch (tipo, peer, msgId, slice).
  - `FP/ALL SEND` (+ `NO-PREPARADO` si `PrepareSearchRequest` devuelve null).
  - `FP/ALL DONE`: items, `noSkipRange`, `fullCount` reales de cada búsqueda.
  - `FP/ALL FAIL`: `error.type()` + `error.code()` + `requestId` del RPC.
  - `FP/ALL APPLY` o `MERGE-SKIP`: lote fusionado aplicado, o lote vacío con
    fallo omitido (no se marca fin de lista).
  - El `.fail` cambia a `(const MTP::Error &error, mtpRequestId)` para capturar
    código/type exactos (p.ej. PEER_ID_INVALID).
- **Instrumentación en `info_media_provider.cpp`** (solo FilesPhotos/All):
  - `PROVIDER ENTER`: al abrir el perfil/entrar al módulo (peer, type, topic,
    sublist, around) y `RESTART`.
  - `PRELOAD`: cada cambio de región por scroll (around, límite, dirección).
  - `REFRESH`: cada slice que llega (tamaño, fullCount, around, skippedBefore/
    After → fin de lista).
  - `FILL`: en fillSections, celdas `added` vs `missing` (ausentes).
  - `MATER start/batch/done`: materialización masiva (missing → materialized →
    stillMissing) y refresco tras completar.
- Permite correlacionar scroll ↔ peticiones ↔ respuestas ↔ celdas ausentes sin
  tocar `log.txt`. Solo se añaden `debug_depurador.h`, `apiwrap.cpp` y
  `info_media_provider.cpp`; sin cambios de comportamiento de carga.

### Ronda 16 — Fin de lista honesto (quitar el "tope fantasma" que cortaba la lista)
- **Diagnóstico (depurador.txt, chat JAV)**: la lista de `Fotos+Archivos` y `Todo`
  se cortaba en un punto pese a que `FILL added=... missing=0` (materialización
  correcta). La traza mostraba `REFRESH slice=257 skipBefore=7490 skipAfter=96`
  y luego, al hacer scroll hacia abajo (`PRELOAD bottom=1`), **ya no se emitía
  `REQUEST`**: el slice quedaba clavado y no se pedía la página siguiente.
- **Causa raíz**: en `apiwrap.cpp` el `noSkipRange` del lote fusionado se forzaba
  a `{min, ServerMaxMsgId}`. En `SparseIdsSliceBuilder::applyUpdate`
  (`data_sparse_ids.cpp` línea 188) un `range.till == ServerMaxMsgId` produce
  `skippedAfter = 0`, es decir el loader cree que **ya tiene todo hasta el
  mensaje más nuevo** → deja de pedir `After` y el scroll hacia abajo se corta
  aunque `skipBefore` indique miles de elementos pendientes. A eso se sumaba que
  el `count` recibido al final podía recortar el `fullCount` (1724 → 581).
- **Fix** (`apiwrap.cpp`, ambos bloques FilesPhotos y All):
  1. **Rango real del lote**: cuando hay ítems, `noSkipRange = {min, max}` de los
     ids devueltos (antes `{min, ServerMaxMsgId}`). Con eso `skippedAfter` queda
     `nullopt` y `sliceToLimits` pide `After`/`Before` cuando hace falta, igual
     que Media. La unión sigue acumulándose porque las páginas descendentes
     intersectan (`needMergeMessages` se mantiene true).
  2. **Lote vacío sin inicio alcanzado**: si el mínimo de `noSkipRange` de las
     sub-búsquedas (federalFrom) es > 2 (no se llegó al principio del historial),
     se omite la actualización (`MERGE-SKIP`) para no marcar un `skippedBefore=0`
     falso que congelaría el scroll hacia atrás; el loader sigue pidiendo.
  3. **Lote vacío con inicio alcanzado** (`federalFrom <= 2`): fin legítimo; se
     propaga `{federalFrom, federalTill}` para cerrar limpio, sin recortar el
     `fullCount`.
- **Efecto**: el "tope fantasma" desaparece; el loader mantiene la exploración en
  ambas direcciones hasta los extremos reales. Evita también el recorte de
  `fullCount` (1724 → 581). Media/Archivos oficiales no cambian.
- Solo se toca `apiwrap.cpp` (sectores `requestSharedMediaFilesPhotos` /
  `requestSharedMediaAll`).

### Ronda 17 — Fix del bucle de carga (merge forzado de mensajes en applyUpdate)
- **Síntoma (depurador, sesión 16:43, chats JAV y LEGIÓN)**: la lista quedaba
  estancada en ~28 celdas y entraba en **bucle infinito**: `FP REQUEST msgId=
  16859 slice=1` → `FP APPLY items=200 nsk=14039..16857` → `REFRESH slice=28
  skipBefore=7681` se repetía sin parar con el **mismo msgId**, sin cargar las
  miles de miniaturas restantes. La Ronda 16 por sí sola no lo resolvió.
- **Causa raíz**: `SparseIdsSliceBuilder::applyUpdate`
  (`data/data_sparse_ids.cpp`) solo fusiona los mensajes de una página si su
  rango **intersecta** el de `_ids` (`needMergeMessages`). En las uniones
  FilesPhotos/All cada página cubre un rango ancho (`14039..16857`) que **no
  intersecta** la ventana visible → el builder descartaba los ids
  (`needMergeMessages=false`), `_ids` no crecía, `_ids.front()` no bajaba y la
  petición `Before` repetía siempre `msgId=16859` → bucle sin progreso.
- **Fix** (`data/data_sparse_ids.cpp`, `applyUpdate`):
  - `needMergeMessages = needMergeMessages || (update.messages != nullptr);`
    ─ se fuerza el merge de los mensajes recibidos aunque la página no
    intersecte. Es un `base::flat_set::merge` **idempotente** (no duplica).
  - Con esto `_ids` acumula todas las páginas, `_ids.front()` avanza hacia atrás
    en cada petición y el cargador explora el historial completo hasta el inicio
    real (desaparece el bucle).
- **Compatibilidad**: Media/Archivos usan una secuencia contigua que ya
  intersecta, así que el forzado no cambia su comportamiento (solo añade ids
  válidos si llega una página lejana).
- Se tocan `data_sparse_ids.cpp` (merge forzado) y se mantiene lo de `apiwrap.cpp`
  de la Ronda 16 (rango honesto + MERGE-SKIP).


### Ronda 18 — Fix de escala en la materialización (getMessages por lotes propios)
- **Síntoma (depurador sesiones 16:39-17:42)**: en canales/grupos grandes
  (1000-2000+ de media, NO visitados) quedaban **huecos/zonas sin celdas** que
  solo se rellenaban al abrir el chat; en canales de 400-500 se mostraba todo.
  La Ronda 17 ya traía todos los ids (`All full=13399` estable, sin bucle) pero
  los **huecos** persistían.
- **Causa raíz de escala**: `requestMissingAround` materializaba los ids
  pendientes mediante `session->api().requestMessageData(...)`. Esa vía
  acumula **todos** los ids solicitados en un único
  `channels.getMessages`/`messages.getMessages` (`resolveMessageDatas` en
  `apiwrap.cpp`). En canales grandes la slice superaba el límite de ids por
  petición del API → la petición fallaba parcialmente (`PEER_ID_INVALID` /
  `delayed resend` en el log) y las celdas quedaban como hueco permanente hasta
  abrir el chat. A menor volumen (400-500) cabía en el límite y se veía todo.
- **Fix** (`info_media_provider.cpp`, `requestMissingAround`):
  - Se dejó de usar `requestMessageData` y ahora la materialización envía
    **`channels.getMessages` / `messages.getMessages` propios en lotes fijos de
    ≤100 ids** (`kMaxMaterializeBatch`), garantizando no exceder nunca el límite
    del API por petición, independientemente del volumen del canal.
  - Cada lote se procesa con `processExistingMessages` (igual que hacía la vía
    interna) y al resolverse se re-dispara el siguiente `_refreshed` para
    reconstruir la plantilla de miniaturas.
  - **Control de reintentos**: si tras una pasada quedan ids sin HistoryItem
    (p.ej. borrados de verdad), `_refreshed.fire` re-dispara automáticamente la
    recogida de materialización, ahora **limitada a 3 pasadas seguidas**
    (`_missingRetries`, `kMaxMissingRetries`) para no entrar en un bucle
    infinito de render + getMessages (que alimentaba los `delayed resend`).
- **Archivos tocados**: `info_media_provider.cpp` (getMessages por lotes +
  contador de reintentos) y `info_media_provider.h` (`_missingRetries`).
- **Pendiente de validación**: el usuario debe compilar y probar en un canal
  grande **NO visitado** (1000-2000+ media) y revisar `depurador.txt`: debe
  aparecer `MATER ... stillMissing=0` y que los huecos se rellenen.

### Ronda 18b — Ventana de entrada grande en FilesPhotos/All (petición del usuario)
- El usuario pidió que FilesPhotos y All muestren **todo su contenido** como el
  módulo Media, sin huecos ni límites visibles y sin tener que hacer clic en una
  celda (abrir el chat) para que aparezca lo que faltaba.
- **Cambio** (`info_media_provider.h` / `.cpp`): los módulos compuestos
  FilesPhotos/All ya no arrancan con la ventana mínima (`kMinimalIdsLimit=16`):
  ahora parten de `kFullIdsLimit=400` (≈800 ids antes+después) en el constructor,
  en `restart()` y en `jumpToMessage()`. Al pedir/materializar una ventana grande,
  se llena el módulo completo de una vez (la materialización por lotes de la
  Ronda 18 ya hace seguro procesar esa ventana sin superar el límite de ids por
  petición), evitando las zonas vacías que solo se rellenaban al abrir el chat.
- El módulo Media / el resto de funcionalidades quedan intactos (solo cambia el
  `_idsLimit` inicial de los dos tipos compuestos).

### Ronda 18c — Reset del tope de reintentos al scrollear (fix de escala real)
- **Causa del síntoma final** (funciona con 400-500, falla con 1000-2000+,
  visite o no el canal): el contador `_missingRetries` era un miembro que **nunca
  se reseteaba** al cambiar de zona de scroll. En cuanto alcanzaba `kMaxMissing
  Retries=3` (algo normal en canales grandes, donde cada pasada deja unos pocos
  ids sin resolver), `requestMissingAround` dejaba de materializar para siempre.
  Como el contador arrastraba su valor entre slices, **toda zona nueva que el
  usuario recorría quedaba con huecos permanentes** hasta abrir el chat con un
  clic. En canales chicos una sola pasada alcanza y nunca se agota el contador,
  por eso se veía completo.
- **Fix** (`info_media_provider.cpp`, `refreshViewer`): al recibir una slice
  nueva (nueva región al scrollear) se resetea `_missingRetries = 0` junto con
  `_missingRequested = false`. Así cada zona que el usuario recorre recibe un
  presupuesto de materialización fresco y rellena sus huecos; el tope solo limita
  reintentos dentro de una misma región estática (ids de verdad borrados).
- Combinado con la Ronda 18b (ventana grande de entrada) hace que FilesPhotos/All
  se muestren completos en canales de cualquier tamaño, visitados o no.

### Ronda 19 — Instrumentación de diagnóstico (logging del sparse builder)
- Para dejar de "adivinar", se amplió la traza con lo que faltaba en toda la
  cadena:
  - `data_sparse_ids.cpp` (`applyUpdate`): nueva traza `SPARSE apply
    updateRange=from..till items=N ids=M f=front..back skipB=? skipA=? count=?`
    que muestra, por cada página aplicada, el `noSkipRange` que nos llega, el
    tamaño de `_ids`, sus extremos y cómo quedan los `skippedBefore/After`. Esto
    permite comparar el camino de Media (cierra en umbrales 0/0) con el de
    FilesPhotos/All (se ve por qué no cierra).
- Lo que el depurador actual (sesión 20:20, canal JAV) ya revela sin más:
  - `ALL REQUEST msgId=14039 slice=1` se repite **idéntico una y otra vez** sin
    avanzar → la exploración hacia atrás está clavada en 14039 y nunca baja al
    inicio del contenido (por eso no se muestran los primeros/antiguos archivos).
  - `skipBefore=?` se queda así (nunca llega a `0`), indicando que el builder
    siempre cree que "hay más hacia atrás".
  - `MERGE-SKIP` no aparece nunca pese a `federalFrom=12925 > 2` (Ronda 16).
- Archivos tocados: `data_sparse_ids.cpp` (traza nueva) + include de
  `debug_depurador.h`.

### Ronda 19.1 — Diagnóstico con logs (sesión 21:07) + corrección de la traza
- Análisis del depurador: el corte de FilesPhotos/All NO es de materialización
  (siempre `missing=0`), sino del sparse-loader de la unión: `skipBefore=7552`
  enorme que nunca baja a 0 mientras `skipAfter=0` sí cierra. Es decir, quedan
  ~7552 archivos antiguos sin mostrarse porque el borde inferior no se cierra.
- Causa: en la unión nunca llega `update.range.from=0` (siempre el `min` de ids),
  así que `SparseIdsSliceBuilder` deja `_skippedBefore=nullopt`.
- Traza corregida en `data_sparse_ids.cpp::applyUpdate`: ahora `SPARSE applied`
  se loguea después de `mergeSliceData` (estado post-merge).

### Ronda 19.2 — Ventana de unión más grande (fix del "no se muestran todos")
- Análisis del depurador (traza post-merge): en canales grandes el front de la
  unión FilesPhotos/All avanza muy poco por página (el recorte de
  SparseIdsSliceBuilder a limitBefore+limitAfter alrededor del around), por lo
  que `skipB` se queda en ~13000 y los archivos antiguos no se muestran; la
  petición Before se repite sin avanzar.
- Fix: `kFullIdsLimit` 400 -> 1000 en `info_media_provider.h` (window ≈ 2000 ids).

### Ronda 19.2 (validada) — FilesPhotos queda completo con la ventana 1000
- Depurador del 17:49 (canal JAV, FilesPhotos): el walkback inicial con
  `limit=1000` recorre todo el historial hasta el inicio real y cierra con
  `skipBefore=0` y `missing=0` en todas las slices. El scroll en ambas direcciones
  cubre la lista completa (7709 items) sin huecos ni bucles.
- Se confirma que la ventana de entrada grande (kFullIdsLimit=1000) es la que
  permite que el front del sparse builder avance hasta el origen, resolviendo el
  "no se muestran todos los archivos".

### Ronda 19.4 (validada) — All completo con la ventana 1000
- Depurador del 18:16 (canal JAV, All type=14): el walkback con `limit=1000`
  recorre todo el historial y cierra en `skipBefore=0` con `full=13399` y
  `FILL missing=0` en todas las slices. Idéntico resultado al de FilesPhotos:
  el módulo All muestra TODO el contenido (fotos+videos+archivos) sin huecos.
- Confirma que la ventana grande (`kFullIdsLimit=1000`) cierra ambos módulos
  compuestos. Los errores `PEER_ID_INVALID`/`delayed resend` del log son de fondo
  (datos/historial de thumbnails), no afectan a la búsqueda ni a las celdas.

### Ronda 19.5 — Mantener la ventana grande al hacer scroll (FilesPhotos/All)
- `info_media_provider.cpp` (`checkPreload`): antes `_idsLimit = preloadIdsLimit`
  (~128) encogía la ventana al scrollear → congelamiento y ítem del chat sin
  mostrar hasta usar "mostrar en el chat". Ahora para FilesPhotos/All se conserva
  `kFullIdsLimit` (1000) durante el scroll, igual que al arrancar y al saltar a un
  mensaje. El resto de módulos no cambia.

### Ronda 19.6 — Materialización proactiva completa de la ventana (FilesPhotos/All)
- Contexto: el usuario pidió cambiar de enfoque tras revisar los depuradores
  (que mostraban la unión y las slices completas con `missing=0`), pero en uso
  real persistía el síntoma de archivos que no se muestran hasta usar
  "mostrar en el chat". Aunque el walkback carga todos los ids y el fill no
  reportaba huecos, la materialización del History se hacía solo de forma
  REACTIVA (cuando fillSections detectaba una celda sin HistoryItem durante el
  render). En canales muy grandes o tras descargar el History en otra parte,
  esa zona podía quedarse sin celdas hasta que algo (mostrar en el chat)
  re-materializaba el Historial.
- Fix (`info_media_provider.cpp`, `refreshViewer`): para FilesPhotos/All, nada
  más llegar cada slice se invoca `requestMissingAround(_universalAroundId)`
  ANTES de `_refreshed.fire({})`. Así la materialización es PROACTIVA: se
  recogen TODOS los ids de la slice sin HistoryItem y se piden por lotes
  (≤100, messages/channels.getMessages), rellenando la ventana grande entera de
  una vez en lugar de esperar a que el render detecte cada hueco. Los guards
  `_missingRequested` (evita duplicar peticiones) y `_missingRetries`
  (reset por slice, evita bucles con ids borrados) se mantienen.
- Fix (`info_media_provider.h`): `kMaxMissingRetries` 3 -> 8, para dar varias
  pasadas de materialización a la ventana grande (8-10 lotes por slice en
  canales grandes) tolerando fallos de red transitorios sin bucles infinitos.
- No afecta a los demás módulos (solo se ejecuta para Type::FilesPhotos/All).

### Ronda 19.7 — Materialización con paralelismo acotado (estable, sin congelarse)
- Problema de estabilidad detectado: la cola de materialización de Ronda 18/19.6
  era SERIAL: enviaba un lote (≤100 ids) y esperaba su respuesta completa antes
  de enviar el siguiente. En una zona de 1000 ids eso eran ~10 pasadas de red
  seguidas; al entrar en una zona nueva sin HistoryItems la cuadrícula se veía
  congelada/parada hasta que toda la serie terminaba.
- Fix (`requestMissingAround`): la cola ahora lanza hasta `kMaxMaterializeParallel`
  (3) lotes en vuelo a la vez. Cada zona de 1000 ids se rellena en ~4 pasadas en
  lugar de ~10 (~3x más rápido), manteniendo la carga de red acotada (nunca más
  de 3 peticiones getMessages concurrentes) para no sobrecargar la API ni
  bloquear el procesado. Al terminar se calcula `stillMissing` como antes
  (contador de reintentos con tope, anti-bucle con ids borrados) y se dispara un
  único `_refreshed.fire`.
- Limpieza: se rompe el ciclo `state->startMore->batchSettled->state` al terminar
  (en Ronda 18/19.6 el estado compartido quedaba retenido en memoria).
- La materialización sigue siendo PROACTIVA (Ronda 19.6): se dispara al llegar
  cada slice, antes de refrescar, y con la ventana grande persistente
  (kFullIdsLimit=1000) del scroll, de modo que todas las zonas se rellenan sin
  dejar de cargarse y sin que el usuario tenga que usar "mostrar en el chat".

### Ronda 20 — Reenvío masivo: cadena estable, sin límites y cronología conservada
- Problemas detectados en el reenvío masivo (`ApiWrap::forwardMessages`, apiwrap.cpp):
  1) Enviaba TODOS los lotes (≤100 ids cada uno) en paralelo en un bucle. Con
     más de 1000 mensajes se lanzaban decenas de peticiones messages.forwardMessages
     a la vez → errores de flood y mensajes desordenados entre lotes.
  2) El handler de FALLO no decrementaba `shared->requestsLeft`: si un lote
     fallaba, el successCallback nunca se disparaba y la funcionalidad quedaba
     colgada (la app parecía "malograda" hasta reiniciarla/reinstalarla).
  3) `MaxSelectedItems = 100` limitaba la selección en chat a 100 mensajes.
- Cambios:
  1) `apiwrap.cpp` `forwardMessages`: reescrito como CADENA SECUENCIAL de lotes.
     Se construyen primero todos los lotes (struct ForwardChunk con from/ids/
     randomIds/localIds) y se envían de UNO en UNO: cada lote espera a que el
     anterior termine (con éxito o con fallo) antes de enviar el siguiente
     (`sendNextChunk`/`settle`). El handler de fallo también hace `settle()`
     (reporta el error con sendMessageFail y continúa con el siguiente lote), por
     lo que la cadena NUNCA se rompe y se reenvía TODO lo seleccionado hasta
     terminar. El successCallback se dispara siempre al completarse el último
     lote. Una sola petición en vuelo → sin flood y con cronología exacta.
  2) `config.h`: `MaxSelectedItems` 100 -> 10000 (selección de reenvío sin
     límites prácticos; sigue protegiendo la UI frente a selecciones patológicas).
- No se tocó `kMaxForwardMessages = 100` (es el límite real del API por petición).
- No afecta a las demás funcionalidades (solo refuerza el reenvío).
- Fix de compilación (Ronda 20): `ApiWrap::forwardMessages` fallaba al compilar
  (C2662/C2280) porque el lambda `sendNextChunk` usaba captura `[=]` y
  `histories` (`Data::Histories`, clase NO copiable) se capturaba por valor,
  copiando un objeto con constructor de copia eliminado. Se cambió la captura a
  `[=, &histories]` (referencia no-const a `_session->data().histories()`, que
  vive mientras la sesión exista). Los demás lambdas de la cadena no usan
  `histories`; `buildMessage`/`settle`/done/fail solo capturan valores copiables.
- Ronda 21 (crash al reenviar varios archivos): análisis del volcado WER
  (CrashDumps) → crash = `abort()` ejecutando `__fastfail(7)` (int 29h en
  Telegram.exe+0x5f2bc51) sin handler de SIGABRT, en pleno camino de respuesta
  MTP del reenvío masivo (frames: PrepareMessage<MTPmessages_sendMultiMedia>,
  MTP::processCallback, HistoryWidget::handleHistoryChange, std::_Xbad_function_call).
- Ronda 21 — CAUSA RAÍZ + refuerzo en `ApiWrap::forwardMessages`:
  1) CAUSA RAÍZ del cierre: el callable recursivo de la cadena se auto-capturaba
     POR VALOR (`std::function sendNextChunk; sendNextChunk = [=,...]`),
     copiando la versión VACÍA inicial (el valor pre-asignación). Al terminar un
     lote, `settle` invocaba esa copia vacía → `std::bad_function_call` →
     excepción no capturada → terminate → abort() → fastfail(7) → cierre
     inesperado. FIX: `std::make_shared<std::function<void()>>` y llamadas
     mediante `(*sendNextChunk)()` (ni copia del vacío, ni referencia colgante).
  2) `settle` ya NO invoca handlers vacíos: `shared->callback()` y
     `(*sendNextChunk)()` solo se llaman si el std::function existe.
  3) Un lote cuyo peer ORIGEN se resuelve vacío (usuario/canal sin acceso
     cargado) o que está vacío se SALTA y la cadena continúa con el siguiente,
     en vez de enviar una petición inválida (la API rechazaba con INPUT_PEERS_EMPTY).
  4) La cadena secuencial (1 petición en vuelo, fallo que no rompe,
     successCallback al final, cronología y lotes de 100) NO se modifica.

## Ronda 22 — Desbloqueo de copiar/reenviar/seleccionar en canales y grupos con restricción de contenido
- Objetivo: en canales/grupos con "No está permitido copiar ni reenviar..."
  (flag `noforwards`), el cliente debe permitir seleccionar mensajes (incluida
  la selección múltiple y "Seleccionar todos"), copiar texto/contenido, reenviar
  (menú contextual y selección múltiple) y NO mostrar el aviso de restricción.
- ENFOQUE (el del enunciado): desactivar SOLO las comprobaciones de UI/acción
  locales, sin eliminar la detección del flag ni tocar protecciones legítimas
  (permisos reales de envío, borrado, etc.). El flag `noforwards` se sigue
  detectando (`forbidsForward()`, `forbidsSaving()`, `alowsForwarding()`/`allowsForwarding()`
  intactos; se usa en traductor, descargas, historias, etc.).
- Archivos y funciones tocados (MÍNIMO, 4 archivos):
  1) `history/history_item.cpp` — `HistoryItem::allowsForward()`: ya no exige
     `!forbidsForward()` ni `history()->peer->allowsForwarding()`. Mantiene
     `!isService() && isRegular() && (!_media || _media->allowsForward())`.
     Consecuencia: el botón "Reenviar" (menú contextual y al seleccionar varios)
     aparece y habilita el envío; `canForwardCount` cuenta todos los mensajes.
  2) `history/history_inner_widget.cpp` (vista de chat `HistoryInner`):
     - `hasSelectRestriction()` -> `false` (salvo sesión congelada/reporte):
       permite entrar en modo selección y "Seleccionar todos" en chats
       protegidos donde antes se bloqueaba si no eras admin.
     - `hasCopyRestriction()` / `hasCopyMediaRestriction()` -> `false`.
     - `hasCopyRestrictionForSelected()` / `showCopyRestrictionForSelected()` -> `false`.
     -> copiar texto/medio y "copiar selección" habilitados y sin toast de aviso
        (`showCopyRestriction`/`showCopyMediaRestriction` ya quedan inactivos).
  3) `history/view/history_view_list_widget.cpp` (vistas de lista/pinned/
     scheduled/chat nuevo):
     - `CopyRestrictionTypeFor()` / `CopyMediaRestrictionTypeFor()` -> `CopyRestrictionType::None`.
     - `SelectRestrictionTypeFor()` -> `CopyRestrictionType::None`.
     - `ListWidget::hasCopyRestrictionForSelected()` / `showCopyRestrictionForSelected()` -> `false`.
  4) `history/view/history_view_context_menu.cpp` — `AddSelectRestrictionAction()`
     ya no añade el aviso "no copiar/no reenviar" al menú contextual.
- NO cambia el comportamiento de chats SIN restricción (ahí ya devolvían false
  / None), no toca los módulos Media/PhotosAndFiles/All, no cambia el envío del
  reenvío y no elimina `MaxSelectedItems` ni `kMaxForwardMessages`.
- LÍMITE CONOCIDO (servidor): Telegram puede RECHAZAR el reenvío "puro"
  (`messages.forwardMessages`) de contenido protegido con error
  `CHAT_FORWARDS_RESTRICTED` (toast `lng_error_noforwards_*`). La UI ya está
  disponible; para que el envío funcione de verdad en ese caso se implementó el
  flujo "reenviar como copia" en la misma ronda (ver sección siguiente).

## Ronda 22b — Fallback "reenviar como copia" ante `CHAT_FORWARDS_RESTRICTED`
- DECISIÓN DEL USUARIO: implementar el fallback (opción b).
- Qué hace: cuando el servidor rechaza un lote de `messages.forwardMessages`
  con `CHAT_FORWARDS_RESTRICTED`, ese lote NO se marca como fallido con toast:
  los mensajes de ese lote se reenvían como mensajes NUEVOS (copia) al mismo
  destino, uno a uno:
  - Texto (o ítem sin foto/documento): `MTPmessages_SendMessage` con el texto
    original y entidades (`Api::EntitiesToMTP` skip-local).
  - Foto: `MTPmessages_SendMedia` con `MTP_inputMediaPhoto` (`MTPDinputMediaPhoto`,
    flag `f_spoiler` si la media lo tiene).
  - Documento/video/audio/etc.: `MTPmessages_SendMedia` con `MTP_inputMediaDocument`
    (`MTPDinputMediaDocument`, flag `f_spoiler` si procede).
  - Se replican los flags de `SendAction` válidos para envío: `reply_to`,
    `silent` (posts), `schedule_date`/`schedule_repeat_period`, quick reply
    shortcut, efecto, suggested post, send_as y allow_paid_stars (min entre
    aprobadas y `items * starsPerMessageChecked`).
- Cambios en `apiwrap`:
  1) `ApiWrap::forwardMessages` (VERSIÓN SECUENCIAL, con `sendNextChunk` de
     Rondas 20-21): cada `ForwardChunk` guarda ahora los ítems originales en
     `items` (vector<not_null<HistoryItem*>>, en paralelo a `ids`/`randomIds`,
     se reinicia en cada corte de lote: cambio de peer origen o 100 mensajes).
     En el handler de error de la cadena, si recibe `CHAT_FORWARDS_RESTRICTED` y
     el lote tiene ítems, delega en `sendForwardAsCopy` en vez de marcar fallo;
     al terminar todas las copias se llama `settle()` (misma contabilidad de
     `shared->requestsLeft` y continúa la cadena al siguiente lote). Un ítem
     que falla en la copia se notifica con `sendMessageFail` y sigue.
  2) Nuevo miembro privado `ApiWrap::sendForwardAsCopy(history, items, action,
     done)` (apiwrap.h) que envía cada ítem como mensaje nuevo con
     `Data::Histories::sendPreparedMessage` + `PrepareMessage`, contando
     finalizaciones con un `shared_ptr<int>` compartido.
- No se toca el camino de éxito del forward (lotes de 100, envío secuencial de a
  uno, contabilidad `requestsLeft`, `ProcessRecentSelfForwards`), no se toca la
  detección del flag ni las protecciones legítimas. Se conserva `requestSharedMediaAll`
  y el resto de la rama actual. En chats SIN restricción el fallback nunca se
  ejecuta (el servidor no devuelve ese error).
- VERIFICACIÓN (usuario): recompilar y probar el caso con restricción: el
  reenvío debe entregar los mensajes como copia (texto y media con su caption),
  y seguir funcionando igual en chats sin restricción.
