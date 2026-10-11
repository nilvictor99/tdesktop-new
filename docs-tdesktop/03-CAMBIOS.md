# Cambios aplicados — registro por Ronda

Registro cronológico de TODO lo que hace distinta esta build frente al Telegram
Desktop original. Cada Ronda: problema → causa → fix → archivos. El índice de
funciones está en `00-INDICE.md`; los diseños previos en `02-PLANES.md`.

---

## Ronda 1 — Porte desde ver01 al árbol limpio
- **Filtros**: enum `MediaFilter {All, Photos, Videos}` (sin GIF) +
  `setMediaFilter`/`supportsMediaFilter`/`matchesFilter` en `info_media_provider.*`; slider de 3 secciones en `info_media_inner_widget.*`.
- **Selección ilimitada**: `_selectedLimit(max)` (firma virtual intacta).
- **Shift+clic**: `selectRange`/`_selectionAnchor` en `info_media_list_widget.*`.
- **Seleccionar todo**: `selectAll()` + Ctrl+A (`info_wrap_widget.cpp`) + botón top bar (`info_top_bar.*`).
- **Orden cronológico**: `ranges::sort` ascendente en `collectSelectedIds`.
- **Recordar reenvío**: `RememberedForwardOptions` (prefs `forward_drop_names`/`_captions`) sembradas en `window_peer_menu.cpp`.

## Ronda 2 — Fixes primera compilación
- 13 errores C2065 `_selectionText`: miembro añadido al `.h`, creación en `createSelectionControls`. Fix visibilidad botón único; estilos `mediaSelectAll`.

## Ronda 3 — Ajustes UI
- Icono "Seleccionar todo" con **check circular** (`toast/check-36x36`); botón **Media único** en layout classic (`info_profile_shared_media_classic.cpp`); key `lng_context_select_all`.

## Ronda 4 — Reenvío y borrado por lotes de 100
- **Reenvío** (`apiwrap.cpp`): `kMaxForwardMessages = 100` + corte por cambio de peer o tamaño.
- **Borrado** (`data_histories.cpp`): `Histories::deleteMessages` divide en lotes de 100. Antes: petición única con TODOS los ids → el excedente seguía vivo y reaparecía (~30).

## Ronda 5 — Reenvío masivo robusto + cuadrícula de archivos
- **Causa del corte en reenvíos grandes**: `ShareBox::DefaultForwardCallback` enviaba TODOS los ids en una petición (servidor procesa ~100) + FLOOD_WAIT sin retry.
- **Cola de lotes** (`share_box.cpp`): partes ≤100 secuenciales (pausa 300 ms), retry FLOOD_WAIT hasta 5x (esperando min(x,60)s), fallo que no aborta la cola, `f_allow_paid_stars` solo en el primer lote, toast "Reenviando X de Y…". Timer heap + weak_ptr.
- **Cuadrícula de archivos** (Type::File): `ListSection::setForcedColumns(n)` → File usa matemática de cuadrícula; slider Lista | 4 | 6 | 8 en InnerWidget (sin persistencia).

## Ronda 6 — Fiabilidad del contador + tarjetas
- `MassQueue.keepAlive`: la cola se sostiene a sí misma (antes moría al retornar y los reenvíos se cortaban). Cierre de capa cuando TODOS los destinos terminaron (`activeThreads`).
- Tarjetas cuadradas de archivos (`paintFileTile`): icono azul, nombre centrado, tamaño en gris, check azul.

## Ronda 7 — Módulo "Fotos + Archivos" (FilesPhotos)
- Fila nueva en el perfil (classic) + contador combinado (`rpl::combine(Photo,File)` en `info_profile_values.cpp`).
- `hideName` en `overview_layout.*`: solo thumbnail centrado.
- **Fix primera carga** (`data_sparse_ids.cpp::applyUpdate`): con `_ids` vacío se acepta siempre la página (antes la intersección fallaba y descartaba ids).
- **Carga desde servidor** (`apiwrap.cpp`): no existe filtro MTP combinado → `requestSharedMediaFilesPhotos` lanza DOS búsquedas paralelas (Photo + File) y fusiona ids/rango/conteo (`doneCount`). Compile fix: `histories` no copiable → captura `historiesPtr`.

## Ronda 8 — Fix crash "Seleccionar todo" en FilesPhotos
- **Causa**: geometría inconsistente — fotos (celda cuadrada) + docs `hideName` (menor). `recountHeight` usaba un único `_itemHeight` y `paint`/hit-test usaban la altura real → filas descuadradas → crash al repintar todo.
- **Fix** (`overview_layout.*`): `Document::resizeGetHeight` con `_hideName` → celda cuadrada (thumb centrado). Todas las celdas comparten altura.

## Ronda 8b — Fix crash arranque "Unexpected: Type"
- `SelectedTitleForMedia` (`info_wrap_widget.cpp`) abortaba con `Unexpected(...)` para el tipo nuevo. Se añadió `case Type::FilesPhotos → lng_media_selected_media`.

## Ronda 8c — Fix módulo en blanco en canales con temas
- `requestSharedMediaFilesPhotos` tenía una guarda que con `topicRootId` hacía `return` sin lanzar. Eliminada; `PrepareSearchRequest` ya soporta temas.

## Ronda 8d — Fix lazy loading FilesPhotos (exploración tipo Media)
- **Causa**: cada lote se guardaba con `noSkipRange = {min..max}` → el lote siguiente (más viejo) no intersectaba → `applyUpdate` descartaba los ids y el builder re-pedía el mismo `Before` (lista clavada en ~50).
- **Fix** (`apiwrap.cpp`): lote con ítems → `noSkipRange = {min, ServerMaxMsgId}` (una slice creciente, todo intersecta); lote vacío → propagar el rango de fin de `ParseSearchResult` (evita bucles). No afecta a Photo/Video/File.

## Ronda 9 — Módulo "Todo" (All)
- `All` en `Storage::SharedMediaType` (índice 14) con lista independiente. Máscaras foto/video/archivo añaden `All` (`data_media_types.cpp`); stickers/GIF/voz fuera.
- `requestSharedMediaAll` = 3 búsquedas en paralelo (Photo/Video/File) + merge con `noSkipRange` lazy.
- Provider: switches + layout híbrido (foto→Photo, video→Video, archivo→Document hideName). Títulos `lng_media_type_all` / `lng_profile_all` / `lng_media_all_empty`; botón y contador (Photo+Video+File) en el perfil. `IsItemGoodForType(All)` descarta stickers. Sin zoom/búsqueda/tabs (decisión).

## Ronda 10 — Pestañas solo si hay contenido
- En la galería Media, las pestañas Fotos/Videos se muestran solo si el contador del tipo es >0; "Todo" siempre presente. Slider reconstruido reactivamente (`rpl::combine` + `distinct_until_changed`); si el filtro activo desaparece → vuelve a "Todo". Solo `info_media_inner_widget.*`.

## Ronda 11 — Carga completa FilesPhotos/All (auto-reparación)
- **Causa**: `fillSections` omite la celda si el `HistoryItem` no está materializado ("Mostrar en el chat" la arreglaba con un fresh search).
- **Fix**: en `fillSections`, ante un id sin layout (solo FilesPhotos/All) → `requestMissingAround` (búsqueda compuesta `Around`) que materializa y repara storage; guard `_missingRequested`. Seed logs `[FILESPHOTOS]`/`[ALL]` retirados.

## Ronda 12 — Materialización masiva por ids
- Ronda 11 no bastaba (seguían huecos con varios huecos o Historial descargado). Ahora se materializan **exactamente los ids faltantes**: `requestMissingAround` recoge los ids sin `HistoryItem` y los pide con `requestMessageData` (agrupa en una sola `messages.getMessages`). Seguridad: guard `_missingRequested` + `shared_ptr<bool> alive` + `_lifetime` (sin use-after-free); mensajes borrados cuentan su respuesta.

## Ronda 13 — Batching por lotes (≤100)
- Pedir todos los ids a la vez superaba el límite de la API. `MaterializeState` + `requestNextBatch` encadenada (`pendingInBatch`), call-backs por indirección `state->` (sin dangling). Fix compile: `const auto &session` → `auto &session`; includes `<functional>/<memory>`.

## Ronda 14 — Blindaje ante fallos (PEER_ID_INVALID / red)
- **Problema**: si TODAS las sub-búsquedas fallaban, el merge de lote vacío daba `noSkipRange={0,0}` → "fin de lista" falso → exploración congelada.
- **Fix** (`apiwrap.cpp`): flag de éxito por sub-búsqueda (`photoOk/fileOk/videoOk`); lote vacío con fallo → **omitir `sharedMediaDone` sin tocar estado** (el loader re-pide al próximo disparo); solo con todos OK se propaga el fin real.

## Ronda 15 — Instrumentación del depurador
- Nuevo `debug_depurador.h` (raíz SourceFiles): trazas con timestamp en `depurador.txt` (junto a `log.txt`), limpio por arranque, thread-safe.
- Trazas en `apiwrap.cpp` (FP/ALL REQUEST/SEND/DONE/FAIL/APPLY/MERGE-SKIP — el `.fail` ahora captura `(const MTP::Error&, mtpRequestId)`) y en `info_media_provider.cpp` (PROVIDER ENTER/PRELOAD/REFRESH/FILL/MATER).

## Ronda 16 — Fin de lista honesto (quitar el "tope fantasma")
- **Síntoma**: lista cortada pese a `missing=0`; `noSkipRange={min,ServerMaxMsgId}` hacía `skippedAfter=0` → el loader dejaba de pedir hacia abajo.
- **Fix** (`apiwrap.cpp`): rango real `{min,max}` de los ids del lote; lote vacío con mínimo >2 → **MERGE-SKIP** (no marcar `skippedBefore=0` falso); lote vacío con inicio alcanzado → propaga `{federalFrom, federalTill}` sin recortar `fullCount`.

## Ronda 17 — Fix del bucle de carga
- **Síntoma**: `FP REQUEST msgId=16859 slice=1` repetido sin avanzar → bucle infinito.
- **Causa**: `applyUpdate` solo fusiona la página si intersecta `_ids` (`needMergeMessages`) → en las uniones las páginas anchas no intersectan → ids descartados, `_ids.front()` no baja.
- **Fix** (`data_sparse_ids.cpp`): `needMergeMessages = needMergeMessages || (update.messages != nullptr)` → `base::flat_set::merge` idempotente acumula todas las páginas. Media/Archivos ya intersectan (sin cambio de comportamiento).

## Ronda 18 — Escala en la materialización (getMessages por lotes propios)
- **Causa**: `requestMessageData` acumula todos los ids en una sola `getMessages` → en canales grandes superaba el límite y fallaba (`PEER_ID_INVALID`/`delayed resend`) → huecos.
- **Fix** (`info_media_provider.cpp`): materialización con **`messages/channels.getMessages` propios en lotes de ≤100** (`kMaxMaterializeBatch`), procesados con `processExistingMessages`; re-dispara `_refreshed`; reintentos limitados (`_missingRetries` / `kMaxMissingRetries=3`).

## Ronda 18b — Ventana de entrada grande
- FilesPhotos/All arrancan con `_idsLimit = kFullIdsLimit = 400` (≈800 ids) en constructor, `restart()` y `jumpToMessage()` (antes `kMinimalIdsLimit=16`). El módulo se llena de una vez; Media no cambia.

## Ronda 18c — Reset del tope de reintentos al scrollear
- **Causa**: `_missingRetries` nunca se reseteaba; al llegar a 3 la materialización paraba para siempre → huecos permanentes en toda zona nueva.
- **Fix**: `_missingRetries = 0` + `_missingRequested = false` en `refreshViewer` al llegar cada slice nueva.

## Ronda 19 — Traza SPARSE (sparse builder)
- `data_sparse_ids.cpp::applyUpdate`: traza `SPARSE apply updateRange=from..till items=N ids=M ... skipB/skipA` para comparar Media (cierra 0/0) con FilesPhotos/All.

## Ronda 19.1 — Diagnóstico + traza post-merge
- Hallazgos (canal JAV, fullCount=7709): materialización perfecta (`missing=0`) pero `skipBefore=7552` que no baja → el corte está en el sparse-loader de la unión; solo llega `range.from=min`, nunca 0.
- Traza corregida: `SPARSE applied` se imprime DESPUÉS del merge.

## Ronda 19.2 — Ventana grande 1000
- `kFullIdsLimit` 400 → 1000 (≈2000 ids). El front avanza mucho más por página Before y cubre el contenido antiguo.

## Rondas 19.3 / 19.4 — Validadas (JAV)
- **FilesPhotos (type=13)** y **All (type=14)** quedan completos con la ventana 1000: walkback hasta `skipBefore=0` (7709 y 13399 items), `missing=0` en todas las slices, sin huecos ni bucles. Los `PEER_ID_INVALID`/`delayed resend` de fondo son de carga de thumbnails, no del walkback.

## Ronda 19.5 — Mantener la ventana grande al scrollear
- **Causa del "congelamiento"**: `checkPreload` hacía `_idsLimit = preloadIdsLimit` (~128) al scrollear → encogía la ventana → zonas sin celdas ("mostrar en el chat" usaba kFullIdsLimit y por eso sí mostraba).
- **Fix**: para FilesPhotos/All `checkPreload` conserva `kFullIdsLimit` durante el scroll. El resto de módulos no cambia.

## Ronda 19.6 — Materialización proactiva
- La materialización era REACTIVA (desde el render). Ahora `refreshViewer`, para FilesPhotos/All, llama `requestMissingAround()` **antes** de `_refreshed.fire({})` → toda la ventana grande se rellena de inmediato. `kMaxMissingRetries` 3 → 8.

## Ronda 19.7 — Paralelismo acotado
- La cola era serial (~10 pasadas por zona 1000) → la cuadrícula parecía congelada. `requestMissingAround` lanza hasta `kMaxMaterializeParallel = 3` lotes en vuelo (≈3x más rápido, máx. 3 getMessages concurrentes); fin único con `stillMissing` + tope; se rompe el ciclo `state->startMore->batchSettled` (memoria).

## Ronda 20 — Reenvío masivo: cadena estable, sin límites, cronología
- **Problemas**: (1) lotes enviados en paralelo → flood y desorden con 1000+ mensajes; (2) el handler de fallo no decrementaba `requestsLeft` → successCallback nunca se disparaba y la app quedaba "malograda"; (3) `MaxSelectedItems=100`.
- **Fixes**: `forwardMessages` → **cadena secuencial** (`ForwardChunk` + `sendNextChunk`/`settle`): un lote a la vez, el fallo también hace `settle()` (reporta y continúa), successCallback siempre al final. `config.h`: `MaxSelectedItems` 100 → 10000.
- Fix compile: lambda `sendNextChunk` capturaba `histories` (no copiable) por valor → captura `[=, &histories]` (referencia no-const).

## Ronda 21 — Robustez (crash al reenviar varios archivos)
- **Causa raíz** (dump WER → `abort()`/`fastfail(7)` en `_Xbad_function_call`): el callable recursivo `sendNextChunk` se auto-capturaba **por valor**, copiando el `std::function` VACÍO pre-asignación; al terminar el lote, `settle` lo invocaba → `bad_function_call` → terminate.
- **Fix**: `std::make_shared<std::function<void()>>` + llamar `(*sendNextChunk)()`. Refuerzos: `settle` solo invoca callbacks si el `std::function` existe; un lote con origen vacío (`mtpc_inputPeerEmpty`) o `ids` vacío se **salta** (evita 400 `INPUT_PEERS_EMPTY` sin perder el conteo).

## Ronda 22 — Desbloqueo de copiar/reenviar/seleccionar (contenido protegido)
- **Objetivo**: en canales/grupos `noforwards`, dejar seleccionar (incl. "Seleccionar todos"), copiar y reenviar, y quitar el aviso. Enfoque del enunciado: neutralizar SOLO las comprobaciones de UI/acción locales (el flag se sigue detectando y protege permisos legítimos).
- **Archivos (4)**:
  1. `history_item.cpp` — `allowsForward()` ya no exige `!forbidsForward()` ni `allowsForwarding()`; mantiene `!isService() && isRegular() && media->allowsForward()`.
  2. `history_inner_widget.cpp` — `hasSelectRestriction()` → false (salvo sesión congelada/reporte); `hasCopyRestriction()`/`hasCopyMediaRestriction()`/`hasCopyRestrictionForSelected()`/`showCopyRestrictionForSelected()` → false.
  3. `history_view_list_widget.cpp` — `CopyRestrictionTypeFor()`/`CopyMediaRestrictionTypeFor()`/`SelectRestrictionTypeFor()` → `CopyRestrictionType::None`; métodos "ForSelected" → false.
  4. `history_view_context_menu.cpp` — `AddSelectRestrictionAction()` no-op.
- Sin restricción → comportamiento idéntico; no toca Media/FilesPhotos/All ni el envío.

## Ronda 22b — Fallback "reenviar como copia" ante `CHAT_FORWARDS_RESTRICTED`
- **Decisión**: el servidor rechaza el forward "puro" de contenido protegido; esa ronda implementa el reenvío como mensajes NUEVOS (copia).
- **`forwardMessages`** (versión secuencial): cada `ForwardChunk` guarda `items` (vector<not_null<HistoryItem*>>) en paralelo a `ids`/`randomIds`; `sendNextChunk` expone `const auto chunkItems = chunk.items;`; el error handler detecta `CHAT_FORWARDS_RESTRICTED` + lote con ítems y delega en `sendForwardAsCopy`; al terminar → `settle()` (misma contabilidad `requestsLeft`); un ítem que falla → `sendMessageFail` y sigue.
- **Función libre** `Api::sendForwardAsCopy(history, items, action, done)` (declarada en `apiwrap.h` en `namespace Api`, definida en `apiwrap.cpp` tras `forwardMessages`): envía cada ítem vía `sendPreparedMessage` + `PrepareMessage`.
  - Texto (y media sin photo/document, p.ej. webpage) → `MTPmessages_SendMessage` (texto + entidades).
  - Foto → `MTPmessages_SendMedia` + `MTP_inputMediaPhoto` (spoiler si `media->hasSpoiler()`).
  - Documento/video/audio → `MTPmessages_SendMedia` + `MTP_inputMediaDocument` (spoiler si procede).
  - Replica flags de `SendAction`: reply_to, silent, schedule_date/repeat, shortcut, efecto, suggested, send_as, `allow_paid_stars` (contador compartido `shared_ptr<int>` que decae por ítem para no sobresuscribir); fin contado con `shared_ptr<int>` compartido + `shared_ptr<Fn<void()>>` que dispara el `settle()` de la cadena.
- No se toca el camino de éxito del forward ni la detección del flag; en chats sin restricción el fallback nunca se dispara. Se conserva `requestSharedMediaAll` y el resto de la rama.
- Entrega original (2026-09-05) integrada sobre la cadena secuencial (Rondas 20–21). REIMPLEMENTACIÓN (2026-09-07): el respaldo en `code\temp\` se perdió y el árbol quedó revertido; se reaplicó la Ronda 22 (UI, 4 archivos) + 22b (apiwrap) y se regeneró el respaldo en `temp\parches-ronda22-ui\` (patch + copias + LEEME.txt).

## Ronda 22c — Reintentos ante fallos temporales + gasto único de estrellas
- **Problema**: un corte transitorio de conexión (timeout/http5xx) marcaba el lote completo como fallido y se perdían cientos de mensajes de golpe; además, si un lote se re-enviaba, las estrellas aprobadas se descontaban dos veces del pool global.
- **Fix** (`forwardMessages`, apiwrap.cpp): `kMaxForwardAttempts = 3` y vector `attempts` por lote. El fail-handler con `MTP::IsTemporaryError` reintenta **el mismo lote con los mismos randomIds** (`--*nextIndex; (*sendNextChunk)();`) — el servidor no duplica mensajes si el primer envío llegó; agotados los intentos, `settle()` reporta y continúa.
- **Estrellas**: `starsSpent` por lote + `starsLeft` compartido: solo el primer intento de cada lote descuenta (`f_allow_paid_stars`); un reintento re-ejecuta `sendNextChunk` sin descontar de nuevo.

## Ronda 22d — Contador de reenvío en vivo
- `progressShow`/`progressDone`/`updateForwardProgress`/`finishForwardProgress`: toast infinito "Reenviados: X de N", reemplazado en tiempo real vía `showToast` (solo si `count > kMaxForwardMessages`); al cerrar el último lote, toast "Reenvío completado: X de N" con duración normal.
- Strings con `u"..."_q`: `QString(u"...")` no compila en este fork.

## Ronda 22e — Watchdog anti-cuelgue (cierre garantizado del reenvío)
- **Síntoma**: el contador "Reenviados" se quedaba clavado para siempre y la cadena no terminaba (Ronda 22d lo hacía visible: toast infinito sin cierre).
- **Causa raíz**: con `FailSkipPolicy::Simple` (mtproto/sender.h:87-90 y :194) los errores temporales de la capa MTP (timeout, http5xx, FLOOD_WAIT_*) **nunca llegan al fail() del lote**: la capa los reintenta en silencio y el request queda "en vuelo" indefinidamente. Como cada lote se encadena con `afterRequest(history->sendRequestId)` (data_histories.cpp:1159-1161), todos los lotes siguientes esperan a ese lote congelado → cookie de la cadena congelada para siempre.
- **Fix** (`forwardMessages`, apiwrap.cpp): watchdog por lote con `base::Timer` (`kForwardChunkTimeout = crl::time(10 * 60 * 1000)`, paciencia anti-flood) y estado compartido `chunkTimer/curIndex/curRequestId/curSettle/chunkSettled`. Al agotarse el timeout:
  1. `Histories::cancelRequest(*curRequestId)` (data_histories.cpp:1232 → `Instance::cancel` → `unregisterRequest`) cancela el request atascado en MTP y **desbloquea la cadena afterRequest** de los lotes siguientes;
  2. si quedan intentos, reintenta el mismo lote con los mismos randomIds (sin duplicar), recargando el timeout;
  3. si se agotaron, fuerza `(*curSettle)()` para que la cadena SIEMPRE avance y el reenvío termine.
- **Ajuste anti-flood (2026-09-08)**: con 60 s el watchdog cancelaba los requests **legítimamente retenidos por FLOOD_WAIT** (`onErrorDefault` en mtp_instance.cpp los reintenta solo tras la espera) → tras ~2000 mensajes el servidor emitía flood y el reenvío "se cortaba" perdiendo el lote. Timeout subido a **10 min**: los floods se resuelven solos con calma y el watchdog solo interviene en el atasco real (backoff eterno que nunca resuelve).
- Guarda `chunkSettled[index]`: un `settle()` tardío (done/fail del request original tras el force) no avanza la cadena ni suma progreso dos veces; `*curSettle`/`*curRequestId` se limpian en cada liquidación.
- Cierre de ciclo: el último `settle()` hace `chunkTimer->setCallback(nullptr)` (el timer captura `sendNextChunk` y éste captura el timer) para no dejar un QObject sin dueño tras cada reenvío.
- Rama `CHAT_FORWARDS_RESTRICTED`: cancela el timer y pone `*curRequestId = 0` antes de delegar en `sendForwardAsCopy` (la copia gestiona su propia cadena y su propio `settle()`).

## Ronda 22f — Diagnóstico definitivo con logs + watchdog anti-flood a 2 horas
- **Síntoma**: un reenvío de 2334 mensajes llegaba a ~2000 y "se cortaba". El usuario comparaba: "con el límite de 10000 funcionaba".
- **Instrumentación temporal** (depurador.txt, retirable): trazas `[FWD ...]` en `forwardMessages` (START/SEND/DONE/FAIL/RETRY/COPY/SKIP/SETTLE/WATCHDOG/FINISH), `[SBX ...]` en la cola masiva del share box (START/SEND/DONE/FAIL/THREAD-DONE) y `[MTP DELAY req secs]` / `[MTP CANCEL req]` en la capa MTP (`onErrorDefault` rama temp/flood y `Instance::Private::cancel`).
- **Causa raíz CONFIRMADA con los logs (2026-09-08)**: NO hay límite de 2000 en el cliente. El reenvío llegó a `count=2334 chunks=24` y avanzó sin problema hasta `index=20` (mensaje 2001-2100). Cada lote de 100 mensajes devuelve, uno por uno, `MTP DELAY req secs` de FLOOD_WAIT (5-22 s típicos, uno de 251 s) que la capa MTP espera y reenvía sola (con `FailSkipPolicy::Simple` nunca llegan al fail del lote). El lote `index=20` recibió **`FLOOD_WAIT secs=3283` (≈55 minutos)**: el servidor agota la cuota horaria de la cuenta y escala la espera abruptamente justo alrededor de los 2000 mensajes reenviados en pocos minutos. La cuenta ya venía throttled antes del reenvío (req sin relación en backoff eterno de 64 s desde 18:48:55).
- Por qué "con 10000 funcionaba": en esas pruebas la cuota de la cuenta estaba descansada; ahora, con reenvíos masivos repetidos, la cuenta/DC quedaron bajo flood global y la espera escala a decenas de minutos. No es un tope del cliente.
- **Corrección**: `kForwardChunkTimeout` subido a **`crl::time(2 * 60 * 60 * 1000)` (2 horas)**. El watchdog debe RESPETAR los FLOOD_WAIT legítimos (hasta ~55 min medidos): si cancelara el request pausado por flood, el reintento volvería a chocar con el mismo flood y no avanzaría nada. Con 2 h de presupuesto, un reenvío >2000 se reanuda solo tras la espera del servidor y termina; el watchdog sigue garantizando la terminación solo en el atasco real (backoff eterno de fallos de red que nunca resuelve), con cancel+reintento (máx. `kMaxForwardAttempts=3`) y force-settle final.

## Ronda 22g — Pacing anti-saturación: reenvío masivo sin cortes NI esperas
- **Objetivo del usuario**: que el reenvío no tenga límite, que no sature el servidor y que no exija esperas (volver al comportamiento fluido de la era del límite 10000, pero SIN límite).
- **Aclaración**: el límite de selección no era la causa (el log de la Ronda 22f probó que un reenvío de 2334 avanzó a lote por lote hasta index=20). La causa fue el `FLOOD_WAIT` del servidor por cadencia: reenviar 100 mensajes seguidos de muchos en pocos segundos agota la cuota horaria y el flood escala (~55 min). Por eso "con 10000 funcionaba": en esas pruebas la cuota estaba descansada. Se mantiene la selección SIN límite práctico (`MaxSelectedItems = 50000`).
- **Fix** (`forwardMessages`, apiwrap.cpp): **pacing suave entre lotes**. Se añade `kForwardChunkGapMs = crl::time(30 * 1000)` (≈30 s por cada 100 mensajes, ≈3 mensajes/segundo) y en la cabecera de `sendNextChunk` se mide el tiempo desde el último envío efectivo; si no ha pasado el intervalo, se espera con un `base::Timer` dedicado (`pacer`) — sin congelar la UI y sin consumir `*nextIndex` — y se reintenta la cabecera al tocar. El primer lote sale al instante.
  - El servidor nunca ve un ritmo agresivo → no tiene que frenar nada → sin FLOOD_WAIT, sin excursiones de 55 minutos; un reenvío de 10000 mensajes fluye de corrido en ~50 min.
  - Si aun así llega un flood pequeño, la capa MTP lo espera y reenvía sola (compatible con el watchdog de 2 h de la Ronda 22f).
  - El `pacer` se libera igual que el watchdog al terminar la cadena (`setCallback(nullptr)` en el último `settle()`).
- **Puntos críticos tocados SOLO en**: `forwardMessages` (constante + estado `pacer/lastSendAt` + cabecera de `sendNextChunk` + teardown final). No se tocó la cuota de selección ni la cola del share box.

## Ronda 22h — Share box: el flood jamás salta partes (cero pérdida de mensajes)
- La cola masiva del share box (chat→chat vía ShareBox::DefaultForwardCallback) tenía un hard-stop simbólico: tras `kMaxFloodRetries = 5` reintentos de `FLOOD_WAIT_*`, saltaba (descartaba) la parte en curso y seguía con la siguiente — pérdida silenciosa de mensajes. La rama era en realidad marginal (la capa MTP resuelve los floods sola y no llegan hasta ahí), pero era la única vía de perderse un grupo si un flood sí llegaba.
- **Fix** (share_box.cpp): eliminado `kMaxFloodRetries` y el gate de reintentos → ante `FLOOD_WAIT_*` se espera (`kMaxFloodWaitSec` por ronda) y se reintenta la MISMA parte todas las veces que haga falta; una parte nunca se salta por un flood transitorio. Se eliminó también la variable `retries` (sin uso real). El feedback (fail permanente, `ALLOW_PAYMENT_REQUIRED`, `VOICE_MESSAGES_FORBIDDEN`) sigue avanzando el índice y la cadena no se rompe.
- **Verificación con logs (2026-09-09)**: la sesión de las 08:11-08:13 (depurador.txt) no contiene NINGUNA traza `[FWD]`/`[SBX]` — solo SPARSE/PRELOAD de medios → en esa sesión no se ejecutó reenvío alguno. Rebarrido de código: se confirma una vez más que NO existe tope de cantidad (~2000), ni de tiempo (watchdog 2 h esperando floods), ni de estrellas/créditos (el gasto por lote se acota al saldo sin cortar la cadena) en el pipeline de reenvío.

## Ronda 23 — Pacing adaptativo + oleadas con pausa honesta (reenvíos > 2000)
- **Verificación del 22g (logs 2026-09-09 13:53-14:17)**: con pacing fijo de 30 s los lotes 0-19 (hasta ~2000 mensajes) salieron limpios cada ~30 s, PERO el lote siguiente recibió `FLOOD_WAIT secs=3000` (`[MTP DELAY req=2114]`). El pacing suaviza el ritmo pero NO evita el muro: la cuota horaria de reenvíos de la cuenta es finita (~2000) y el servidor exige la espera (3000-3283 s ≈ 50-55 min) igualmente. El usuario cerró la app ~1 min después → seguía percibiéndolo como "corte".
- **Fix** (`forwardMessages`, apiwrap.cpp):
  - **Pacing adaptativo**: `kForwardChunkMinGapMs = 30 s` / `kForwardChunkMaxGapMs = 300 s`; el gap entre lotes pasa a ser `clamp(duración del lote anterior, 30 s, 300 s)` (`lastChunkDuration`, cabecera de `sendNextChunk`, guard anti-underflow de `crl::time` uint64 intacto). Si el servidor frenó, el siguiente lote da más aire; si fue rápido, ritmo base.
  - **Cierre de oleada** (`kForwardFloodWallMs = 10 min`, `kForwardFloodPauseMs = 55 min`): si un lote tardó ≥ 10 min (muro real de la cuota), `settle()` cierra la oleada en limpio, registra `[FWD WAVE index dur rest]` y muestra un toast honesto e infinito ("Pausa anti-flood: el servidor exige una espera. Continúa solo en ~55 min. Restan X de N"); el resto se reanuda SOLO vía `pacer->callOnce(kForwardFloodPauseMs)` cuando la cuota se recupera. Reintentar inmediato sería quemar intentos a ciegas (cada lote chocaría con el mismo muro). Junto con el watchdog de 2 h (22f) y los reintentos con mismos randomIds (22c): **el servidor siempre "cede", el reenvío termina el 100% y nada se duplica**.
  - No se reordena la cadena ni se toca la cuota de selección (se mantiene `MaxSelectedItems = 50000`; los logs probaron que el corte nunca fue el límite).
- **Perfil: selección (parte B)**: auditoría — seleccionar todo / deseleccionar / reenviar / eliminar con contador ya es uniforme por módulo vía `TopBar`→`ListWidget::selectionAction` (`info_top_bar.cpp` + `info/media/info_media_list_widget.cpp:1944`); `Select All` selecciona todo lo cargado sin tope (`_selectedLimit = max int`, salvo álbum de historias). Sin cambios de código necesarios.
- **Perfil: iconos por módulo (parte C)**: `info_profile_shared_media_classic.cpp` — PhotoVideo=photo (igual), File=file (igual), **FilesPhotos=video**, **All=infoIconMediaAll (nuevo, reutiliza `menu/all_media` png)** para que cada módulo tenga icono propio.
- **Corte limpio al cerrar (parte A3, verificado)**: todo el estado del reenvío vive en `shared_ptr` locales de `forwardMessages`; al salir mueren timers y request en vuelo sin dejar estado a medias → al reabrir se reenvía en limpio (nada que reiniciar/borrar).
- Registro completo: `05-REGISTRO-REENVIO-MASIVO.md`.

## Ronda 24 — Recuperación completa de "Fotos + Archivos" y "Todo" (caminata al inicio)
- **Problema**: al abrir `FilesPhotos`/`All` en chats con mucho media, la cuadrícula se quedaba en ~1000-2000 celdas (sólo lo recuperado por scroll o por la ventana inicial). El usuario pidió que se recupere **TODO** el historial de media, sin excepción y sin exigir scroll.
- **Causa 1 — descarte silencioso por máscara**: en las sub-búsquedas compuestas, `ParseSearchResult` filtraba cada ítem con `sharedMediaTypes().test(type)`; como el servidor devuelve en una misma página fotos y documentos heterogéneos (videos, GIF, música, voz, notas de vídeo), todo lo que no casaba con la máscara exacta se descartaba → huecos en la lista.
  - **Fix** (`data_search_controller.*`): nuevo parámetro `ParseSearchResult(..., bool acceptAllMedia = false)`. Con `true` el ítem se acepta si `static_cast<bool>(item->sharedMediaTypes())` (foto o cualquier documento no-sticker/no-efímero, cuya máscara es vacía). Sólo las sub-búsquedas compuestas pasan `true` (`requestSharedMediaFilesPhotos` FP DONE y `requestSharedMediaAll` ALL DONE, `apiwrap.cpp`); el resto de llamadores mantienen el filtro exacto por defecto. Render seguro: el layout de FilesPhotos/All ya contempla Photo/Video/Document (`info_media_provider.cpp`).
- **Causa 2 — la ventana se recortaba por el inicio**: `SparseIdsSliceBuilder::sliceToLimits()` borraba los ids por delante de la ventana (`limitBefore`) y el conteo se basaba en la estimación `fullCount` del servidor (suma de los `fullCount` de cada sub-filtro, inflada o desajustada).
  - **Fix — modo "caminata completa"** (`data_sparse_ids.*`): flag `_alwaysPageBefore` (`setAlwaysPageBefore`) + `base::Timer _walkTimer` con callback en el ctor. Cuando el builder está ocioso (`!requestedSomething`) y no ha demostrado el inicio (`_skippedBefore != 0`), `scheduleWalk()` dispara `walkStep()` → `requestMessages(Before)` sobre `_ids.front()` una y otra vez en segundo plano (una petición en vuelo, pausa base 500 ms; `kMaxBackoffShift = 6` → hasta 32 s si el frente no progresa: fallos/red lenta). En modo caminata NO se recorta el inicio (`_ids` conserva todos los ids recuperados) para que el conteo exacto no se contamine con los reintegros de la unión del storage. Se detiene solo cuando `_skippedBefore == 0` (`maybeFinalizeWalk()`). Se activa únicamente para `FilesPhotos`/`All` en `SharedMediaViewer` (`data_shared_media.cpp`); `File` conserva su semántica de máscara.
- **Fix — conteo exacto** (`maybeFinalizeWalk`): al demostrar el inicio real, `_fullCount = int(_ids.size())`. Se corrige ANTES de que `fillSkippedAndSliceToLimits()` derive `skippedAfter` (si no, la estimación del servidor podía dejarlo negativo). Estado final: `fullCount == celdas`, `skippedBefore == 0`, `skippedAfter == 0` → sin peticiones añadidas.
- **Trazas** (`depurador.txt`): `[DEPUR] WALK front=… ids=…` por paso, `[DEPUR] WALK-STALL … sin-progreso=…` (1 y 6) y `[DEPUR] WALK-END ids=… fullCount=…` al cerrar.
- **Riesgo aceptado**: en chats enormes (>50k media) el modo caminata materializa todo el módulo; el pace (≥500 ms/página) mantiene la UI responsiva, a costa de memoria y de ~minutos de llenado.
- **Ajuste Ronda 24a (alcance seg�n decisión del usuario, 13-sep-2026)**: todo lo de la Ronda 24 queda **SÓLO en `FilesPhotos`**:
  - `All` **revierte** a su comportamiento previo: sin `acceptAllMedia` en ALL DONE (`apiwrap.cpp`), sin caminata (`data_shared_media.cpp`), scroll/filtro de máscara/conteo del servidor originales.
  - **Pacing**: la caminata baja a **1500 ms/página** (`data_sparse_ids.cpp` `kWalkGapMs`) para que cada reconstrucción de rejilla no congele la UI en canales de >1000 media.
  - **Materialización por zona visible** (`info_media_provider.*`): en FP solo se materializa la ventana **±400** índices alrededor del ancla (`_universalAroundId`); los ids lejos de la vista (páginas que añade la caminata) NO se bajan por anticipado ni disparan pasada (`requestMissingAround` los ignora si caen fuera de la ventana).
  - **Presupuesto de reintentos por zona**: `_missingRequested`/`_missingRetries` ya NO se reinician por refresh de slice en FP (antes eso encendía una pasada completa por cada página); `_missingRetries` solo se refresca cuando la ZONA VISIBLE cambia (el ancla se mueve al scrollear). En el resto de módulos se conserva el presupuesto por refresh original.
  - **Sin doble refresh**: al terminar una pasada que materializó 0 ítems nuevos (ids borrados/inexistentes), FP ya no vuelve a reconstruir la rejilla (`_refreshed.fire` solo si `materialized > 0`).
- **Ronda 25 — paridad con `Fotos + Vídeo` (13-sep-2026)**: el enfoque "ventana grande + caminata + materialización masiva" de la Ronda 24 no completaba la cuadrícula de FP (los ítems de una slice de 1000 ids se descargaban/evictaban y sus celdas quedaban como huecos con icono de descarga). `Fotos + Vídeo` es estable porque **pagina por scroll**: ventana pequeña → cada página pide la vecina al acercarse a un borde → `addNewMessage` materializa el bloque al vuelo → lo visible siempre existe. Adaptación a FP (todo scoped; `All` conserva su ventana grande, `Fotos + Vídeo` intacto):
  - `info_media_provider.cpp`: las 4 condiciones de ventana grande dejan de incluir a FP (ctor, `restart`, `preloadAroundItem`, `jumpToMessage`) → FP usa `kMinimalIdsLimit`/`preloadIdsLimit` como el resto y pagina por scroll.
  - `data_shared_media.cpp`: el `setAlwaysPageBefore(true)` de FP se elimina (la caminata queda dormida en `data_sparse_ids.*`, reversible).
  - `info_media_provider.*`: se eliminan la ventana de materialización ±400 (`kMaterializeWindow`), el guard de zona, `_lastBudgetAroundBare` y `findSliceIndex`; `requestMissingAround` vuelve a materializar toda la slice (1-3 lotes con ventana pequeña) con reset por refresh y `_refreshed.fire` incondicional (reintento encadenado barato).
- Se conservan `acceptAllMedia=true` en FP DONE y la lógica de merge MERGE-SKIP/rango real.
   - **Consecuencia assumida**: el conteo vuelve a ser la estimación del servidor (suma de Photo+File), como en Fotos + Vídeo.

## Ronda 26 — Fotos+Archivos: paginado por LADO + merge encadenado (17-sep-2026)
- **Problema (evidencia en `depurador.txt`)**: con la paridad de la Ronda 25, FP seguía sin mostrar franjas del historial. La causa es estructural, no de ventana: FilesPhotos es la **UNION** de DOS secuencias de servidor (Photo + File) que se paginaban con un **ancla global** único (el frente que emite `SparseIdsSliceBuilder`). Como las fotos son **densas** (100 ítems en ~250 ids) y los archivos **ralos** (100 ítems en ~2500 ids), la página Photo `Before(ancla)` solo cubre un tramo corto del fondo, mientras el lado File arrastra el frente global muy abajo. En la página siguiente el ancla global ya bajó (al frente File) y Photo vuelve a anclarse en él → **salta su propia franja** y deja fotos sin cargar jamás. En el log: página 1 Photo `[16616..16859]` (ancla 16859), página 2 Photo `[13818..14039]` (ancla 14039) → las fotos `(14039..16616)` no vinieron ni vendrán.
- **Fix — frentes por lado** (`apiwrap.*`): estado de paginación por peer/sub-lista para FP (`FilesPhotosPagingKey/State`, `_filesPhotosPaging`):
  - `photoFront`/`fileFront` (borde bajo cubierto por cada lado) y `photoTop`/`fileTop` (borde alto). En `Before` cada lado se ancla en **su propio frente**, no en el ancla global; en `After`, en su propio tope. Un lado que aún no recibió ninguna página parte del ancla que pide el builder (el borde real de la cobertura conocida, cubre el caso de copia/caché en la sesión siguiente).
  - Los frentes **solo bajan** (`qMin`/`qMax` con cada respuesta), de modo que ninguna secuencia salta tramos y ambas se agotan completas.
  - `updatePagingFrom`: cada respuesta actualiza frente/tope de su lado y **cierra** el lado que responde vacío en su inicio (Before `from==0`, After `till==ServerMaxMsgId`, Around ambos) → `FP CLOSE`.
  - Un lado cerrado deja de enviarse (participa en el merge como lote vacío correcto); cuando **ambos** están cerrados se aplica el rango final `{0..}` y el builder cierra (`skippedBefore=0`).
- **Merge encadenado**: el rango real del lote (min..max de los ids) se solapa con la cobertura previa → el storage une todas las páginas en **una slice contigua** (sin islas por página ni re-fetch por preload).
- **Fin real = sólo los lados vivos**: en el lote vacío, `federalFrom`/`federalTill` se calculan **solo** con los lados que respondieron en esa página (los ya cerrados no imponen un falso `from=0`); si el único lado activo devuelve vacío con `from>2` (hueco/no-inicio) se hace MERGE-SKIP y se sigue explorando.
- **Conteo protegido**: `sharedMediaDone` acepta `std::optional<int> fullCount`. Si una sub-búsqueda falla, el sumatorio (con un 0 por el lado caído) ya **no sobrescribe** el total almacenado (evita "fines de lista" prematuros por infravaloración); el conteo solo se escribe cuando **ambos** lados responden OK.
- **Sesiones**: un `Around` reinicia el estado al ancla nueva (`FP RESET`) y sube un `epoch`; las páginas en vuelo de una sesión anterior siguen mergéandose (sus ids son válidos) pero **no avanzan** los frentes de la sesión nueva.
- **Alcance**: únicamente `FilesPhotos`. `All` (con el mismo defecto estructural y 3 lados) y `Fotos + Vídeo` intactos.
- **Trazas nuevas**: `FP RESET`, `FP SEND ... CERRADO`, `FP CLOSE type=… dir=…`; `FP SEND` ahora muestra el ancla real usado por cada lado.
- **Verificación esperada** en sesión de scroll larga: los `FP SEND` de Photo y File divergen en `msgId` (cada lado a su ritmo), aparecen `FP CLOSE` para ambos lados, el rango aplicado baja hasta `~2`, y `REFUSE/FILL` cubren todo el contenido (unión contigua sin huecos permanentes).

## Ronda 26.1 — Conteo fiable + ventanas descartadas (20-sep-2026)
- **Defecto A — `fullCount` degradado a "tamaño de página"**: en la página de cierre (y chat pequeños) el servidor responde con el constructor `messages.messages` **sin `vcount`** (`data_search_controller.cpp:210-216`), y `ParseSearchResult` ponía `fullCount = tamaño de página`. `updatePagingFrom` lo sobrescribía sobre el total real del lado (`photoFullCount`/`fileFullCount`), corrompiendo la estimación de la unión: con un lado ya cerrado, el sumatorio infravalorado llegaba al storage y la "corona" (`storage_sparse_ids_list.cpp:115-123`) forzaba `{0, ServerMaxMsgId}` → **la lista se congelaba con menos celdas de las reales**.
  - Fix: campo nuevo `Api::SearchResult::reliableCount` — `true` solo con `vcount` real (`messagesSlice`/`channelMessages`); `updatePagingFrom` conserva el fullCount por lado **solo si `reliableCount`**; `finishOne` arma el total con los conteos fiables de esta ronda o, en su defecto, con el último fiable conocido (nunca con el tamaño de página); `fullCountToSave` exige `allOk && countsOk`.
- **Defecto B — ventana descartada (pseudo-vacío) cierra un lado en falso**: `ParseSearchResult` sella `from==0` en cualquier respuesta sin ids (`data_search_controller.cpp:282-293`), aunque el servidor devolviera mensajes que `acceptAllMedia` descartó (máscara vacía). Eso cerraba el lado permanentemente y podía arrastrar a la unión a un cierre prematuro.
  - Fix: en `.done` de `sendOne` se registran los mensajes **brutos** (rawCount/rawMin/rawMax); si `rawCount>0 && messageIds.empty()` → `FP DROP`: el rango bruto cuenta como cobertura (son no-matches), se anula el `{0,...}` del sello, `updatePagingFrom` avanza frente sin cerrar.
  - Guard `anyContinues` en `finishOne`: el fin real solo se sella cuando **ningún lado vivo** sigue explorando (Before: `from>2`; After: `till<ServerMax`; Around: cualquiera). Un lado que llega a su inicio hoy se cierra, pero no arrastra a un lado vivo con una ventana descartada por encima.
- **Trazas nuevas**: `FP DROP`, y `FP DONE` ahora loguea `reliable=%6`.
- **Alcance**: solo `FilesPhotos`; `All` y el camino simple siguen usando `fullCount` como antes (`reliableCount` se ignora ahí).
- **Verificación esperada**: `FP APPLY count=7709` constante hasta el cierre (nunca degradado), `FP DROP` en chats con ventanas de stickers/efímeros sin romper la exploración, y la unión completa sin congelarse por conteo bajo.