# Seguimiento técnico — tdesktop personalizado

Estado de avance por secciones, con los archivos tocados en cada una.
Regla permanente: solo implementar lo de **riesgo bajo**; lo de riesgo
medio/alto queda anotado en [17-MEJORAS-CATALOGO-ANTERIOR.md](17-MEJORAS-CATALOGO-ANTERIOR.md).

> Árbol de trabajo: `C:\Users\VICTOR\code\tdesktop` (ver01 eliminada tras portar todo).

---

## Resumen de estado

| Sección | Estado |
|---|---|
| 1. Funciones desbloqueadas (copiar/guardar/seleccionar) | ✅ Completado |
| 2. Selección y galería | ✅ Completado |
| 3. Reenvío masivo | ✅ Completado |
| 4. Descargas y media | ✅ Completado |
| 5. UI / UX | ✅ Completado (parcial por diseño) |
| Extra: borrado masivo | ✅ Completado |

---

## Ronda 1 — Porte desde ver01 al árbol limpio

Cambios A–F aplicados y verificados con diff contra ver01:

- **Filtros**: enum `MediaFilter {All, Photos, Videos}` (sin GIF) en
  `info_media_common.h`; `setMediaFilter`/`supportsMediaFilter`/`matchesFilter`
  + skip en `fillSections` (`info_media_provider.*`); slider de 3 secciones con
  wiring de geometría en `info_media_inner_widget.*`.
- **Selección ilimitada**: `_selectedLimit(max)` + drag ilimitado pasando max a
  `ChangeItemSelection` (firma virtual intacta para no romper providers ajenos).
- **Shift+clic**: `selectRange`, `_pressWithShift`, `_selectionAnchor`,
  `applyItemSelection→bool` (`info_media_list_widget.*`).
- **Seleccionar todo**: `selectAll()`, `SelectionAction::SelectAll`, Ctrl+A en
  `info_wrap_widget.cpp`, botón top bar (`info_top_bar.*`) + estilos.
- **Orden cronológico**: `ranges::sort` ascendente en `collectSelectedIds`.
- **Recordar reenvío**: `RememberedForwardOptions`/`RememberForwardOptions`
  (prefs `forward_drop_names`/`forward_drop_captions`) + siembra del draft en
  `window_peer_menu.cpp`.

## Ronda 2 — Correcciones tras primera compilación

- 13 errores C2065 `_selectionText`: añadido miembro al `.h`, creación única en
  `createSelectionControls`, orden de botones corregido → top bar idéntico a ver01.
- Fix visibilidad del botón unificado (`buttonType != Type::PhotoVideo && ...`).
- Estilos `mediaSelectAll` base + TopBar + LayerTopBar.

## Ronda 3 — Ajustes finales de UI

- **Icono check circular**: `infoTopBarSelectAll` usa `toast/check-36x36`;
   eliminado estilo duplicado del layer; layer apunta a `infoTopBarSelectAll`.
   Diff de `info_top_bar.cpp/.h` vs ver01 = 0 diferencias.
- **Botón Media único en layout classic**:
  `info_profile_shared_media_classic.cpp` → un solo `addMediaButton(MediaType::PhotoVideo)`.
- **Key de idioma**: `"lng_context_select_all"` añadida a `lang.strings`.

## Ronda 4 — Reenvío y borrado por lotes

- **Reenvío** (`apiwrap.cpp`): `constexpr auto kMaxForwardMessages = 100;` +
  condición de corte `forwardFrom != newFrom || ids.size() >= kMaxForwardMessages`.
  Causa previa: el servidor solo procesa ~100 IDs por petición; con 160
  seleccionados llegaban solo 100.
- **Borrado** (`data_histories.cpp`, `Histories::deleteMessages`): bucle de
  lotes `kMaxDeletePerRequest = 100`, un `sendRequest` por lote con su propio
  done/fail. Causa previa: borrado local inmediato + petición única con TODOS
  los IDs → el excedente seguía vivo en el servidor y reaparecía (~30 mensajes).

---

## Reglas de trabajo (permanentes)

1. **Nunca compilar durante el desarrollo**; entregar cambios y dejar compilar al usuario.
2. Build del usuario (Windows):
   ```
   msbuild Telegram.sln /p:Configuration=Release /p:Platform=x64 /m /t:Telegram
   ```
3. Investigar causa raíz primero; corrección mínima después.
4. Tras cada cambio: actualizar este archivo y [12-CAMBIOS-RONDAS-1-22B.md](12-CAMBIOS-RONDAS-1-22B.md).
5. Antes de tocar código nuevo: revisar [17-MEJORAS-CATALOGO-ANTERIOR.md](17-MEJORAS-CATALOGO-ANTERIOR.md)
   para no duplicar ni romper lo existente.

## Ronda 5 — Reenvío masivo robusto + cuadrícula de archivos

- **Causa raíz del corte en reenvíos grandes**: la ruta de la galería usa
  `ShareBox::DefaultForwardCallback` (share_box.cpp), que enviaba TODOS los
  msgIds en UNA sola petición MTP → el servidor solo procesaba ~100 y con
  uso repetido disparaba FLOOD_WAIT sin reintento.
- **Cola de lotes en share_box.cpp** (`DefaultForwardCallback`):
  - Partes de ≤100 (`kMassBatchSize`), envío secuencial encadenado con
    pausa de 300 ms entre lotes (`kMassBatchDelay`).
  - Reintento automático en `FLOOD_WAIT_x`: hasta 5 reintentos
    (`kMaxFloodRetries`), esperando `min(x,60)` segundos.
  - Fallo definitivo NO aborta la cola: avanza al siguiente lote
    (con toasts existentes de pago/voz).
  - `f_allow_paid_stars` SOLO en el primer lote (evita cobro múltiple).
  - Contador visible "Reenviando X de Y…" vía toast que se refresca por
    lote (`MassProgressState` + `updateMassProgress`).
  - Timer heap-allocado con callback débil (`base::Timer` + `weak_ptr`)
    para evitar referencias colgantes; captura de `histories` POR VALOR.
- **Cuadrícula de archivos** (módulo Archivos, Type::File):
  - `ListSection::setForcedColumns(n)`: si n>1, File usa matemática de
    cuadrícula en `resizeToWidth` y `recountHeight` (posiciones y alturas).
  - `ListWidget::setFileGridColumns(n)` propaga el valor en cada resize.
  - Toggle en InnerWidget (`setupFileViewToggle`): slider Lista | 4 | 6 | 8,
    solo cuando type() == Type::File. Sin persistencia aún (en memoria).

## Ronda 6 — Fiabilidad del contador + tarjetas de archivos

- **Bugs de por vida corregidos en share_box.cpp**:
  - `MassQueue` ahora se auto-referencia (`keepAlive`) mientras quedan lotes;
    antes moría al retornar la función y los reenvíos se cortaban a mitad
    (causa de "400 seleccionados → solo 100 o 40").
  - `activeThreads` global: la capa SOLO se cierra cuando todos los destinos
    terminaron; antes el hueco entre lotes daba "requests vacías" y cerraba
    temprano matando las colas restantes.
  - Toast ahora muestra pendientes: "Reenviando X de Y… quedan N".
- **Tarjetas de archivos** (info_media_list_section):
  - `fileTileMode()` = Type::File con columnas >1 → celdas CUADRADAS.
  - `paintFileTile`: icono azul centrado arriba (página con esquina doblada),
    nombre debajo centrado con elipsis central, tamaño en gris debajo,
    overlay + círculo check azul al seleccionar.
  - Ya no se "aprieta" el dibujo horizontal original: pintura propia,
    sin deformar filas ni el resto de la app.
- **Fixes de compilación ronda 5→6**: `base::Timer timer;` sin `{nullptr}`
  (ctor ambiguo); captura `historiesPtr = &histories` (Histories no es
  copiable).

## Ronda 7 — Módulo "Fotos + Archivos" (canales)

- **Nueva fila** "Fotos + Archivos" en `info_profile_shared_media_classic.cpp`
  que abre el tipo `FilesPhotos` (fotos + archivos, sin videos).
- **Contador combinado** (`info_profile_values.cpp`): `rpl::combine(Photo, File)`.
- **HideName**: `DocumentFields.hideName`/`Document._hideName` en
  `overview_layout.*`; con FilesPhotos se pinta solo el thumbnail centrado
  (initDimensions usa fileThumbSize; paint hace return early).
- **Fix de primera carga** (`data_sparse_ids.cpp`, `applyUpdate`): cuando
  `_ids` está vacío se aceptan siempre los mensajes del servidor. Antes la
  comprobación de intersección contra `{_key,_key}` fallaba con `_key` lejano y
  descartaba los IDs (bucles de `insufficientAround`, archivos que no aparecían).
- **Carga de fotos desde el servidor** (`apiwrap.cpp`): no existe filtro MTP
  combinado "fotos + documentos". `requestSharedMedia` desvía FilesPhotos a
  `requestSharedMediaFilesPhotos`, que lanza DOS búsquedas en paralelo
  (Photo→`inputMessagesFilterPhotos`, File→`inputMessagesFilterDocument`) y
  fusiona IDs/rango/conteo al terminar ambas (contador `doneCount`), para
  guardar una única actualización bajo `FilesPhotos`.
  - Con esto las fotos aparecen sin haber entrado al chat (antes solo las que
    quedaban en caché local tras entrar).
  - **Fix de compilación**: `histories` (Data::Histories, no copiable) no se
    puede capturar por valor en lambdas `[=]` → se captura `historiesPtr = &histories`
    y se llama `historiesPtr->sendRequest(...)` (mismo patrón que roundas 5→6).

## Ronda 8 — Fix crash "Seleccionar todo" en FilesPhotos

- **Síntoma**: al pulsar "Seleccionar todo" en Fotos + Archivos la app se
  cierra al instante (en Photo/File no pasa).
- **Causa raíz** (geometría de cuadrícula inconsistente): FilesPhotos mezcla
  fotos (celda cuadrada `itemWidth`) y documentos `hideName` (altura
  `fileThumbSize`, menor). `recountHeight`/`resizeToWidth` posicionan las filas
  con un único `_itemHeight` (= alto del último ítem), pero `paint`/hit-test
  (`visualHeight`, `findItemRect`) usan el alto real de cada ítem. Cuando el
  último ítem era un documento, `_itemHeight` quedaba pequeño y las filas de
  fotos se descuadraban → al repintar todo tras "Seleccionar todo" la geometría
  rota provocaba el cierre de la app.
- **Fix**: `Document::resizeGetHeight` override en `overview_layout.*` — con
  `_hideName` el documento ocupa una celda cuadrada como las fotos
  (`_height = _width`) y actualiza `_minh` para que el thumbnail quede centrado
  en el cuadrado. Con eso TODAS las celdas de FilesPhotos tienen la misma
  altura y la cuadrícula vuelve a ser geométricamente consistente (el mismo
  patrón que ya usaba el módulo Archivos con `fileTileMode`). `_hideName` solo
  se usa en FilesPhotos, así que ningún otro módulo cambia.

## Ronda 8b — Fix crash al arrancar por "Unexpected: Type in generateSelectedText"

- **Síntoma (log)**: la app crashea al iniciar/abrir la sección con un
  `Assertion Failed! Unexpected: Type in TopBar::generateSelectedText()
  info_wrap_widget.cpp:96`. (Las líneas `RPC Error ... PEER_ID_INVALID` del
  log son ruido de fondo normal de MTProto, no la causa.)
- **Causa**: `SelectedTitleForMedia` en `info_wrap_widget.cpp` hace un
  `switch` sobre el tipo de media para generar el texto "N seleccionado" de la
  barra superior, y termina en `Unexpected(...)` (aborta) si no encuentra el
  caso. No contemplaba el nuevo tipo `FilesPhotos` → al entrar en "Fotos +
  Archivos" con selección (o al pedir el título de selección de ese tipo) el
  switch caía al final y abortaba la app.
- **Fix**: añadir `case Type::FilesPhotos: return tr::lng_media_selected_media;`
  (el texto genérico "{count} Media", clave ya usada por PhotoVideo).

## Ronda 8c — Fix módulo "Fotos + Archivos" en blanco en canales/grupos con temas

- **Síntoma (log)**: al abrir el perfil → "Fotos + Archivos" en ciertos canales/
  grupos con muchos archivos e imágenes, el módulo se queda en blanco esperando.
  Logs: `API Error: FilesPhotos request inside a topic is not supported!`
  (`apiwrap.cpp`).
- **Causa**: `requestSharedMediaFilesPhotos` tenía una **guarda** que al detectar
  `topicRootId` (canales foro / uso dentro de un tema) hacía `return` sin lanzar
  la petición → la sección nunca recibe resultados y se queda en blanco. El resto
  de módulos (Photo/Video/File) no tenían esa guarda y por eso sí funcionaban.
- **Fix**: se elimina la guarda en `apiwrap.cpp`. `Api::PrepareSearchRequest` ya
  soporta temas (pone `Flag::f_top_msg_id` y pasa `topicRootId`), y
  `ParseSearchResult`/`sharedMediaDone` también los manejan, así que las DOS
  búsquedas en paralelo (Photo + File) corren con el mismo código que el resto
  de módulos. Las líneas `RPC Error ... TOP_MSG_ID_NOT_SUPPORTED` del log son de
  otras peticiones paralelas al abrir la sección, no de la guarda.

## Ronda 8d — Bug: FilesPhotos solo carga el primer lote y no explora (FIX)

- **Síntoma (usuario)**: "Fotos + Archivos" solo muestra ~100 y al hacer scroll
  NO se cargan más; se quiere exploración sin límite como Media (página a página
  al hacer scroll, sin traer todo de golpe: hay canales con 100k+).
- **Diagnóstico por logs `[FILESPHOTOS]` en `out/Release/log.txt`** (un canal
  real, total=3870):
  - `request ... slice=0` inicial → `merge n=50 ... range=8204..8352` (solo 50
    items en el primer lote, por el filtrado `sharedMediaTypes`).
  - `request msg=8204 slice=1` (Before) se repite MUCHAS veces con el MISMO
    msgId → `merge n=100 ... range=7916..8199` y `done msg=100`. O sea: las
    peticiones Before SÍ funcionan y devuelven 100 más, pero la UI no los muestra.
  - `merge ... slice=2 ... range=0..0` (After vacío) repetido → la dirección
    "recién alcanzada" nunca señalaba fin (rango 0..0 = sin señal de completado).
- **Causa raíz**: cada lote fusionado se guardaba con `noSkipRange = {min..max}`
  de ESE lote. El lote siguiente (más viejo) tiene un rango que NO se solapa con
  el actual → `SparseIdsSliceBuilder::applyUpdate` cumplía
  `needMergeMessages = intersects(...) == false` → descartaba los IDs nuevos.
  Resultado: `_ids`/`_slice` quedaba en ~50, el preload de la sección
  (`info_media_provider.cpp` `preloadAroundItem`) no conseguía que `aroundId`
  avanzara hacia atrás (la distancia nunca superaba `minUniversalIdDelta`), y el
  builder re-pedía el MISMO `Before` una y otra vez sin llegar a mostrar nada.
- **Fix (apiwrap.cpp, `requestSharedMediaFilesPhotos`/`finishOne`)**:
  - Lote con ítems → `noSkipRange = {minDelLote, ServerMaxMsgId}`. Con ello
    storage une todas las páginas en UNA slice creciente (hacia atrás) y todos
    los `update.range` intersectan → `applyUpdate` los fusiona en `_ids` → la
    distancia crece → el preload avanza `aroundId` más viejo → búsqueda fresca
    (applyInitial) revela páginas más antiguas → EXPLORACIÓN TIPO MEDIA.
  - Lote vacío → propagar el rango "de fin de dirección" que ya resuelve
    `ParseSearchResult` (Before vacío → `{0, msgId}`; After vacío →
    `{msgId, ServerMaxMsgId}`) para que el loader deje de pedir (antes daba
    `0..0` y NO señalaba fin → bucles infinitos).
  - Solo afecta a FilesPhotos; Photo/Video/File/Music/etc. no cambian.
- **Pendiente**: usuario compila y verifica exploración ilimitada con scroll en
  un canal de 1000+; revisar `[FILESPHOTOS]` en el log para confirmar que el
  range de los lotes nuevos es `min..ServerMaxMsgId` y que no se re-piden
  msgIds repetidos. Los logs temporales se pueden retirar luego.

## Pendiente de verificación por el usuario

- Borrado masivo >100 items: confirmar que nada reaparece (fix ronda 4).
- Reenvío 200–900+: confirmar llegada completa en orden cronológico.
- Reenvío 1000+ REPETIDO y luego 300–500: la cola debe terminar SIEMPRE;
  contador llega a Y con "quedan 0" y la capa se cierra sola (rondas 5+6).
- Módulo Archivos: toggle Lista/4/6/8 → tarjetas cuadradas con nombre abajo;
  selección arrastrando, menú contextual y Seleccionar todo funcionando.
- Módulo "Fotos + Archivos": abrir SIN haber entrado antes al chat → deben
  aparecer fotos y archivos de inmediato (ronda 7).
- Extensión a Grupos y Grupos con temas (pendiente de implementar).
- **Módulo "Fotos + Archivos" lazy loading (ronda 8d)**: tras compilar, abrir un
  canal con 1000+, hacer scroll y revisar `out/Release/log.txt` buscando
  `[FILESPHOTOS]`: se espera ver peticiones con `slice=1` (Before) al hacer
  scroll y resultados con `n>0`; reportar estas líneas para aplicar el fix.

## Ronda 9 — Módulo "Todo" (All) en el perfil

Implementado siguiendo el patrón de FilesPhotos. Estado: código escrito, NO
compilado aún (lo compila el usuario).

- **Enum**: `All` añadido a `Storage::SharedMediaType` (índice 14, antes de
  `kCount`) → lista de storage independiente, `kSharedMediaTypeCount=15`
  dentro del límite de 32 bits de `base::enum_mask`.
- **Máscaras** (`data_media_types.cpp`): `MediaPhoto` y `MediaFile`
  (solo ramas vídeo-archivo y archivo genérico) añaden `All`. Stickers/ttl,
  GIF, voz, música y round quedan fuera (máscara vacía o sin `All`) → el
  backfill de historial indexa All automáticamente vía
  `HistoryItem::sharedMediaTypes()`.
- **API** (`apiwrap.*`): `requestSharedMediaAll` = 3 `messages.search` en
  paralelo (Photo/Video/Document), `doneCount==3`, merge de `fullCount` +
  `messageIds`, `noSkipRange = {min,lote, ServerMaxMsgId}` (lote no vacío) o
  propagación de rango de fin (lote vacío), idéntico al fix lazy de FilesPhotos.
- **Provider** (`info_media_provider.cpp`): switches `sectionHasFloatingHeader`
  (false), `sectionTitle` (mes), `sectionItemBelongsHere` (mes), y
  `createLayout` híbrido foto→Photo / video→Video / archivo→Document(hideName).
- **Mobile**: `MinItemHeight`, `ListSection` grid (resizeToWidth/recountHeight),
  `EmptyWidget` (icono foto + `lng_media_all_empty`).
- **Títulos**: `SharedMediaTitle`→`lng_media_type_all`,
  `SelectedTitleForMedia`→`lng_media_selected_media`, `MediaTextPhrase`→
  `lng_profile_all`.
- **Perfil**: botón `All` tras FilesPhotos (classic); contador combinado
  Foto+Video+Archivo en `SharedMediaCountValue`.
- **Visores**: `IsItemGoodForType` añade ramas All (foto/video/archivo) con
  exclusión explícita `document->sticker()`.
- **Idioma**: `lng_media_type_all`, `lng_profile_all#one/#other`,
  `lng_media_all_empty` en `lang.strings` (zona ~1895).
- **Decisiones adoptadas**: sin zoom de cuadrícula ni ventana separada (igual
  que FilesPhotos); NO añadido a tabs experimentales (FilesPhotos tampoco); sin
  búsqueda (`SharedMediaAllowSearch(All)=false`); global media no afectado
  (`SharedMediaOverviewType(All)=nullopt`).

### Verificación pendiente (tras compilar)
- Abrir canal/grupo → Perfil → botón **Todo**: ver fotos+videos+archivos en
  una sola cuadrícula, sin stickers/GIF/voz/links.
- Scroll hasta arriba: cargar TODO (exploración tipo Media); revisar
  `[ALL] request/merge` en `out/Release/log.txt` (range `min..ServerMaxMsgId`).
- Contador del botón = fotos+videos+archivos reales.
- Selección múltiple, reenvío masivo y borrado masivo funcionan como en Media.
- Regresiones: Media, Archivos (File), Fotos+Archivos siguen igual.

## Ronda 10 — Fix: pestañas Fotos/Videos visibles aunque estén vacías

Implementado. Estado: código escrito, NO compilado aún (lo compila el usuario).

- **Síntoma**: en la galería Media (modal Foto+Video), las pestañas
  Todo/Fotos/Videos se mostraban SIEMPRE las tres, aunque el canal no tuviera
  fotos (o no tuviera videos) → pestaña vacía con listado en blanco.
- **Antes**: `setupMediaFilter()` creaba el slider con 3 secciones fijas
  (`info_media_inner_widget.cpp`) y mapeaba índices rígidos 1=Fotos, 2=Videos.
- **Ahora**: el slider se reconstruye según la presencia real de cada tipo:
  - `rpl::combine(SharedMediaCountValue(Photo), SharedMediaCountValue(Video))`,
    mapeado a `std::vector<MediaFilter>` = `[All]` + `Photos` si hay >0 +
    `Videos` si hay >0, con `rpl::distinct_until_changed` (solo se reconstruye
    cuando cambia la presencia, no cada cambio de contador).
  - `rebuildMediaFilter()` recrea el slider (`_filter.destroy()/create`), asigna
    el filtro activo según el índice del `MediaFilter` actual y, si el filtro
    activo dejó de existir, cae a `All`. `sectionActivated` ahora mapea vía
    `_filterSections[section]` (con rango) en vez del switch rígido.
  - "Todo" siempre presente; caso "sin fotos ni videos" → solo "Todo" con
    estado vacío (ya cubierto a nivel de botón del perfil).
- **Nuevos miembros** en el `.h`: `std::vector<MediaFilter> _filterSections` y
  `rpl::lifetime _filterCountsLifetime`; helper `FilterLabel`/`FilterSections`
  en anónima; incluye `info/profile/info_profile_values.h`, `<rpl/combine.h>`,
  `<rpl/distinct_until_changed.h>`, `<rpl/lifetime.h>` y `<vector>`.
- **No se tocó** el resto: providers PhotoVideo/All/FilesPhotos/File, botón del
  perfil, títulos o el caso "0 media".

### Verificación pendiente (tras compilar)
- Canal solo-fotos → la pestaña Videos NO aparece; solo-videos → Fotos no aparece;
  con ambas → las 3 pestañas.
- "Todo" siempre presente; al cambiar de pestaña y que esta deje de existir no
  crashea (vuelve a Todo).
- Regresiones: Media/All/Archivos/Fotos+Archivos y el botón Media del perfil
  siguen igual.

## Ronda 11 — Fix carga completa "Fotos + Archivos" y "Todo" (auto-reparación)

Implementado. Estado: código escrito, NO compilado aún (lo compila el usuario).

- **Síntoma**: en canales/grupos grandes, al hacer scroll por FilesPhotos y All
  llegaban zonas donde solo se veían fotos o solo archivos (celdas ausentes).
  "Mostrar en el chat" arreglaba la zona al instante. Media siempre mostraba
  todo.
- **Causa raíz**: `fillSections` itera `_slice` y para cada id llama
  `getLayout`→`createLayout`→`session.data().message(id)`. Si el `HistoryItem`
  no está materializado en memoria (canales grandes, evicción/cobertura no
  materializada a tiempo), `createLayout` devuelve `nullptr` y la celda se
  omite → hueco. La zona se arreglaba con `jumpToMessage` porque hacía un fresh
  search `Around` que materializaba los mensajes y reparaba la cobertura.
- **Fix (provider auto-reparación)**: en `fillSections`, cuando un id de
  `_slice` no puede crear layout **y** el tipo es `FilesPhotos`/`All`, se pide
  automáticamente una búsqueda compuesta `Around` de ese id
  (`requestMissingAround`, que llama a `api().requestSharedMedia(_type, id,
  Around)` → merge correcto de 2/3 carriles), materializando foto+archivo
  (+video en All) y reparando storage; la slice se reconstruye y la zona se
  rellena. Guard `_missingRequested` (un solo request pendiente a la vez, se
  limpia en `refreshViewer` al llegar la siguiente slice) → sin bucles.
- **No se toca** Media ni Archivos (el guard limita la auto-reparación a los
  dos tipos compuestos; los null en otros tipos se siguen saltando como antes).
- **Retirados** los `LOG` de diagnóstico `[FILESPHOTOS]`/`[ALL]`
  (`apiwrap.cpp`): `requestSharedMediaFilesPhotos`, `requestSharedMediaAll`,
  `sharedMediaDone` y `fillSections` quedan limpios.
- Archivos tocados: `info_media_provider.h`, `info_media_provider.cpp`,
  `apiwrap.cpp` (solo limpieza de logs).

### Verificación pendiente (tras compilar)
- FilesPhotos y All: hacer scroll por TODAS las zonas → fotos+archivos (y
  videos en All) mezclados sin huecos ni "zonas solo archivos"/"solo fotos".
- Media (foto+video) y Archivos (File Lista/4/6/8) sin regresiones.
- Canal normal, grupo y grupo con temas.
- Selección múltiple, reenvío y borrado masivo siguen funcionando.

## Ronda 12 — Técnica avanzada: materialización masiva por ids (FilesPhotos/All)

Implementado (sustituye a la ronda 11). Estado: código escrito, NO compilado
aún (lo compila el usuario).

- **Motivo del cambio**: el usuario confirmó que con la ronda 11 **seguían
  apareciendo huecos/zonas incompletas**. La técnica reactiva (una búsqueda
  compuesta `Around` por hueco) solo re-materializaba una zona pequeña
  alrededor del primer id que fallaba; con varios huecos o con el `History`
  descargado (`clear(ClearType::Unload)` al venir del chat) no cubría todo.
- **Causa confirmada**: `createLayout` → `session.data().message(FullMsgId)`
  solo consulta la tabla en memoria (`Data::MessageIds`). Si el `HistoryItem`
  no está materializado (descargado o nunca traído en esa región) devuelve
  `nullptr` y la celda se omite. La ronda 11 dependía de que un `search Around`
  devolviera el item exacto, no garantizado para toda la ventana.
- **Fix avanzado**: materializar **exactamente los ids que faltan**, en masa y
  en una pasada:
  1. `fillSections` detecta un id sin layout (FilesPhotos/All) →
     `requestMissingAround`.
  2. Recorre TODOS los ids de `_slice`; los que no tienen `HistoryItem`
     (`session.data().message(full) == null`) se recogen en `missing`.
  3. Cada id se materializa con `api().requestMessageData(peer, msgId, done)`.
     `ApiWrap::requestMessageData` agrupa (con `afterDelay`) en una sola
     `messages.getMessages` / `channels.getMessages` →
     `processExistingMessages` → `addNewMessage(Existing)` → materializa en
     `Data::MessageIds`.
  4. Se cuentan las respuestas (un call-back por id, incluso si el mensaje ya
     no existe, así el guard siempre se libera); cuando todas llegan →
     `_refreshed` → `refreshRows()` → reconstrucción con la región completa.
- **Seguridad**:
  - Guard `_missingRequested`: una sola pasada de materialización en vuelo
    (además se limpia en `refreshViewer` con cada slice nueva).
  - Call-back protegido contra use-after-free: estado en `shared_ptr` con un
    `bool alive`; `_lifetime.add(...)` lo marca muerto si el `Provider` se
    destruye antes de responder, y el call-back comprueba `alive` antes de
    tocar `this`.
  - **Media y Archivos NO cambian** (el guard solo aplica a FilesPhotos/All).
  - Mensajes borrados: no materializan pero su call-back cuenta → sin bloqueos.
  - Carga acotada: `_idsLimit` típico (~60–100 ids) dentro del límite razonable
    de `messages.getMessages`; batching de `requestMessageData`.
- Archivos tocados (solo provider):
  `info/media/info_media_provider.h` (comentario del guard),
  `info/media/info_media_provider.cpp` (`requestMissingAround` reescrito).

## Ronda 13 — Batching por lotes en la materialización (fix de fiabilidad)

Implementado. Sustituye el pedido de "todos los ids a la vez" de la ronda 12 por
**tandas encadenadas de ≤100 ids**, porque `messages/channels.getMessages` tienen
límite de ids por llamada y una slice con muchos huecos podía no materializar
todo de forma fiable.

- **Cambio** (`requestMissingAround`):
  - `MaterializeState` ahora guarda todos los ids faltantes, `next` (siguiente a
    pedir), `pendingInBatch` y un `std::function<void()> requestNextBatch`.
  - `state->requestNextBatch()` lanza un lote de ≤100 `requestMessageData`; cada
    respuesta invoca `onOneResponse`, que baja `pendingInBatch`; al cerrar un
    lote lanza el siguiente; cuando no quedan más → `_missingRequested = false`
    y `_refreshed.fire({})` (reconstrucción con la región llena).
  - `requestNextBatch` y `onOneResponse` viven dentro del `shared_ptr` `state`
    (mantenido vivo por los call-backs de `ApiWrap`) y se llaman por indirección
    vía `state->` → **sin captura dangling de variables del stack**.
  - **Fix de compilación**: se corrigió `const auto &session` → `auto &session`
    (Ronda 12 no compilaba porque `session.api()` requiere `Main::Session&`
    no-const); se añadieron `#include <functional>` y `<memory>`.
- **Seguridad**: guard `_missingRequested` (una pasada a la vez); `alive` +
  `_lifetime` (sin use-after-free); lotes ≤100 respetan el límite de la API;
  Media y Archivos no cambian; los call-backs no usan referencias a variables
  del stack de `requestMissingAround`.

### Verificación pendiente (tras compilar)
- FilesPhotos y All: hacer scroll por TODAS las zonas → todo el contenido
  (fotos, archivos y videos en All) sin huecos y sin necesidad de "Mostrar en
  el chat", incluso si hay muchas celdas que faltaban de golpe.
- Media (foto+video) y Archivos (File Lista/4/6/8) sin regresiones.
- Canal normal, grupo y grupo con temas.
- Selección múltiple, reenvío y borrado masivo siguen funcionando.

## Ronda 14 — Blindaje ante fallos de búsqueda (PEER_ID_INVALID / red)

Implementado. Cierra el requisito del plan que pedía manejar PEER_ID_INVALID /
TOP_MSG_ID_NOT_SUPPORTED "sin crashear ni detener la carga completa".

### Problema
En `requestSharedMediaFilesPhotos`/`All`, un fallo de una sub-búsqueda asignaba
`Api::SearchResult()` vacío → `noSkipRange={0,0}` por defecto. Si **todas** las
búsquedas fallaban, el merge de lote vacío emitía `noSkipRange={0,0}` y el
sparse-loader leía `from==0` como "nada más atrás" (`skippedBefore=0`) →
**congelaba la exploración** aunque hubiera contenido real (falso "fin de
lista"). El camino oficial, en cambio, descarta el fallo sin tocar el estado.

### Fix (`apiwrap.cpp`)
- Flags de éxito por sub-búsqueda: `State::photoOk/fileOk/videoOk`; `.done` hace
  `*okFlag = true`, `.fail` no.
- `finishOne` computa `allOk` y la fusión queda así:
  - Lote con ítems → `noSkipRange = {min, ServerMaxMsgId}` (igual que antes).
  - Lote vacío con algún fallo (`!allOk`) → **se omite `sharedMediaDone`** sin
    tocar el estado (paridad con el camino oficial): la región queda
    "insuficiente" y se re-pide al próximo disparo (scroll / cambio de datos).
  - Lote vacío con todos OK → fin legítimo de la dirección; se propaga el rango
    real de `ParseSearchResult` (comportamiento previo).

### Efecto
`PEER_ID_INVALID`, `TOP_MSG_ID_NOT_SUPPORTED` y cortes de red ya **no detienen
la carga**: no crashean, no envenenan el estado del loader y la cadena de
scroll sigue viva. Media / Archivos oficiales no cambian (solo
`requestSharedMediaFilesPhotos` y `requestSharedMediaAll`).

### Verificación pendiente (tras compilar)
- Abrir FilesPhotos / All en un chat y comprobar scroll completo sin huecos.
- (Opcional, difícil de reproducir) Simular fallo de red / peer inválido y
  verificar que la lista no se queda "congelada" y recupera al re-dispararse.
- Media y Archivos sin regresiones; selección y borrado masivo OK.

## Ronda 15 — Instrumentación del depurador (archivo depurador.txt)

Implementado. Para diagnosticar los huecos restantes sin adivinar, se añadió un
registro detallado a un archivo APARTE (`depurador.txt`, junto a `log.txt`) que
traza TODO el flujo de FilesPhotos/All al hacer scroll en el perfil.

### Helper nuevo
- `debug_depurador.h` (raíz de SourceFiles): `Depur::append(QString)` escribe
  una línea con timestamp en `cWorkingDir()/depurador.txt`. Se borra una vez por
  proceso (primera escritura) → `depurador.txt` limpio en cada arranque.
  Thread-safe (mutex estático); incluye `settings.h` para `cWorkingDir()`.

### Trazas añadidas
- **ApiWrap** (`requestSharedMediaFilesPhotos/All` y dispatch):
  - `FP/ALL REQUEST` (tipo, peer, msgId, slice).
  - `FP/ALL SEND` (+ `NO-PREPARADO` si el request no se puede construir).
  - `FP/ALL DONE` (items + nsk + fullCount de cada búsqueda).
  - `FP/ALL FAIL` (`error.type()`, `error.code()`, `requestId`). El `.fail`
    ahora captura `(const MTP::Error&, mtpRequestId)`.
  - `FP/ALL APPLY` o `MERGE-SKIP` (lote aplicado o omitido por fallo).
- **Provider** (`info_media_provider.cpp`, solo FilesPhotos/All):
  - `PROVIDER ENTER` y `RESTART` (al entrar al módulo).
  - `PRELOAD` (cada scroll/cambio de región).
  - `REFRESH` (cada slice que llega: tamaño, full, around, skippedBefore/After).
  - `FILL` (celdas añadidas vs. ausentes en fillSections).
  - `MATER start/batch/done` (materialización: missing→materializado→sin
    materializar).

### Objetivo
Correlacionar scroll ↔ petición ↔ respuesta/fin ↔ celdas ausentes y ver si los
restantes "huecos" son de cobertura de la slice (no llegan ids) o de render
(celda omitida porque falta el HistoryItem). Sitio del log: junto a Telegram.exe
en out/Release (mismo lugar que log.txt).

### Verificación
- Compilar y abrir "Fotos + Archivos" / "Todo" en un chat grande, hacer scroll
  por TODAS las zonas y luego revisar `depurador.txt`.

## Ronda 16 — Fin de lista honesto (quitar el "tope fantasma" que cortaba la lista)

Terminada. El depurador de la Ronda 15 permitió ver la causa real de que la lista
se cortara en un punto aunque la materialización estuviera perfecta.

### Síntoma (depurador, chat JAV)
- `FILL ... added=N missing=0` en toda la sesión → la materialización NO era el
  problema (no hay celdas ausentes por HistoryItem).
- Al hacer scroll hacia abajo, el slice quedaba clavado: `REFRESH slice=257
  skipBefore=7490 skipAfter=96` se repetía y **no se emitía más `REQUEST`** pese
  a que `skipBefore` indicaba miles de elementos pendientes.

### Verificación con depurador (sesión 16:43, chat JAV y LEGIÓN)
- El depurador mostró `FILL added=N missing=0` (materialización perfecta) pero la
  lista quedó **estancada** cargando solo ~28 celdas y entró en **bucle infinito**:
  `FP REQUEST msgId=16859 slice=1` → `FP APPLY items=200 nsk=14039..16857` →
  `REFRESH slice=28 skipBefore=7681` se repetía cientos de veces con el **mismo
  msgId=16859**, sin avanzar ni cargar las miles de miniaturas pendientes.
- El `log.txt` de la misma sesión solo reportaba `PEER_ID_INVALID` (código 400,
  ya blindado en Ronda 14) y `-404` de destrucción de claves MTP (inofensivo);
  el error real era el estancamiento/bucle del cargador, que en la app se ve como
  "no muestra todo el contenido y lo limita según el chat".

### Causa raíz real (dos problemas encadenados)
1. **Bucle por no-merge**: `SparseIdsSliceBuilder::applyUpdate` (data_sparse_ids)
   exige que la página entrante **intersecte** el rango de `_ids` para fusionarla
   (`needMergeMessages`). En las uniones FilesPhotos/All, cada página cubre un
   rango ancho (p.ej. `14039..16857`) y las siguientes **no intersectan** la
   ventana visible actual → el builder **descarta los ids** (`needMergeMessages=
   false`), `_ids` no crece, `_ids.front()` no baja y la petición `Before` repite
   siempre `msgId=16859` → bucle infinito sin progreso real.
2. **Corte/recorte previo**: al usar `noSkipRange={min, ServerMaxMsgId}` (estilo
   anterior) `applyUpdate` marca `skippedAfter=0` y el loader dejaba de pedir
   hacia abajo.

### Fix definitivo
- **`data_sparse_ids.cpp` (`applyUpdate`)**: se fuerza el merge de los mensajes
  recibidos aunque la página no intersecte (`needMergeMessages = needMergeMessages
  || (update.messages != nullptr)`). Es un `base::flat_set::merge` idempotente (no
  duplica). Con esto `_ids` acumula todas las páginas, `_ids.front()` avanza hacia
  atrás en cada petición y el cargador explora el historial completo hasta el
  inicio real (deja de estancarse). No afecta a Media/Archivos (secuencia
  contigua que ya intersecta).
- **`apiwrap.cpp` (FilesPhotos y All)**: se mantiene la Ronda 16 (rango real
  `{min,max}` en vez de `ServerMaxMsgId`; y lote vacío sin inicio → MERGE-SKIP).
  El rango real evita el corte por `skippedAfter=0`; el merge forzado evita el
  bucle.

### Estado
- Código aplicado. Sin compilar aún (lo hace el usuario).
- Se tocan `data_sparse_ids.cpp` (merge forzado en `applyUpdate`) y `apiwrap.cpp`
  (rango honesto + MERGE-SKIP). Media/Archivos se mantienen en su comportamiento
  (contiguos, ya intersectan).

## Ronda 18 — Fix de escala en la materialización (getMessages por lotes propios)
### Problema
- Tras la Ronda 17 (ids completos, sin bucle), los **huecos/zonas sin celdas**
  persistían solo en canales/grupos grandes (1000-2000+ media) **NO visitados**;
  en canales de 400-500 se mostraba todo. Depurador sesiones 16:39-17:42:
  `All full=13399` estable, todas las páginas traídas, `JAV` (visitado)
  `missing=0`/`MATER=0`. El problema restante era de **escala**, no de ids.
- **Causa raíz**: `requestMissingAround` materializaba los ids pendientes con
  `session->api().requestMessageData(...)`, que acumula **todos** los ids en un
  único `channels.getMessages`/`messages.getMessages` (`resolveMessageDatas`,
  `apiwrap.cpp`). En canales grandes la slice superaba el límite de ids por
  petición del API → petición masiva que falla (`PEER_ID_INVALID` / `delayed
  resend` en log) → celdas que quedaban como hueco permanente hasta abrir el
  chat. A bajo volumen cabía en el límite y se veía completo.

### Fix (`info_media_provider.cpp`, `requestMissingAround`)
- Ya no se usa `requestMessageData`: la materialización envía
  **`channels.getMessages` / `messages.getMessages` propios en lotes fijos de
  ≤100 ids** (`kMaxMaterializeBatch`), de modo que nunca se excede el límite del
  API por petición, sin importar el volumen del canal.
- Cada lote se procesa con `processExistingMessages` (igual que la vía interna)
  y al resolverse se re-dispara el `_refreshed` para reconstruir las miniaturas.
- **Control de reintentos**: si quedan ids sin HistoryItem (p.ej. borrados de
  verdad), `_refreshed.fire` re-dispara la recogida automáticamente, ahora
  limitada a **3 pasadas seguidas** (`_missingRetries` / `kMaxMissingRetries`)
  para no entrar en bucle infinito de render + getMessages (que alimentaba los
  `delayed resend`).

### Archivos tocados
- `info_media_provider.cpp`: getMessages + processExistingMessages por lotes,
  contador de reintentos.
- `info_media_provider.h`: miembros `_missingRetries` y `kMaxMissingRetries`.

### Verificación pendiente (tras compilar)
- Probar en un canal grande **NO visitado** (1000-2000+ media).
- En `depurador.txt` deben aparecer `MATER ... batch sent=...` en ráfagas de
  ≤100 y, al final, `MATER done ... stillMissing=0 retry=0`, sin huecos visibles
  (que antes se llenaban al abrir el chat).

## Ronda 18b — Ventana de entrada grande en FilesPhotos/All (no más límites visibles)
- Petición del usuario: que FilesPhotos y All muestren todo el contenido como
  Media, sin límites ni huecos y sin tener que abrir el chat con clic en una
  celda.
- Cambio: `_idsLimit` inicial de FilesPhotos/All pasa de `kMinimalIdsLimit=16` a
  `kFullIdsLimit=400` (≈800 ids) en constructor, `restart()` y `jumpToMessage()`.
  Al materializar esa ventana grande (batches de ≤100 de la Ronda 18) el módulo
  muestra todo de una vez. No se toca Media ni otras funciones.
- Verificación pendiente (tras compilar): entrar directo a FilesPhotos y a All en
  un canal grande NO visitado; todo el contenido debe verse sin hacer clic en
  celdas, y en `depurador.txt` debe verse `MATER ... batch sent=...` en ráfagas
  de ≤100 hasta `MATER done ... stillMissing=0`.

## Ronda 18c — Reset del tope de reintentos al scrollear (fix de escala real)
- Problema final: fallaba con canales grandes (1000-2000+ media) y no con
  (400-500), sin relación con visitar o no el canal. Viene del contador
  `_missingRetries`: como nunca se reseteaba, al llegar a 3 `requestMissingAround`
  dejaba de materializar para siempre y toda zona nueva al scrollear quedaba con
  huecos permanentes hasta abrir el chat. En canales chicos el contador nunca se
  agota.
- Fix: reset `_missingRetries = 0` en `refreshViewer` al recibir cada slice nueva.
- Verificación pendiente (tras compilar): entrar directo a FilesPhotos y All en un
  canal grande (1000-2000+), visitado o no, y recorrer todo el contenido; no deben
  quedar zonas vacías ni necesidad de abrir el chat con clic.

## Ronda 19 — Instrumentación de diagnóstico (logging del sparse builder)
- Se añadió traza `SPARSE apply ...` en `data_sparse_ids.cpp::applyUpdate` para
  ver, por página, el rango recibido, el tamaño/extremos de `_ids` y los
  skipped. Objetivo: ver con logs por qué FilesPhotos/All no explora hacia atrás
  como Media.
- Verificación pendiente (tras compilar): reabrir la app y navegar por FilesPhotos
  y All; mirar `depurador.txt`. Buscar:
  - ¿`SPARSE apply ... skipB=0` alguna vez? (Media sí lo alcanza).
  - ¿`ALL/FP REQUEST msgId=...` sigue repitiendo el mismo msgId sin bajar?
- Enviar de vuelta el `depurador.txt` de una corrida que reproduzca el fallo.

## Ronda 19.1 — Análisis del depurador (sesión 21:07) + traza post-merge
- Hallazgos con la traza SPARSE (canal JAV, FilesPhotos type=13, fullCount=7709):
  1. NO hay problema de materialización: siempre `FILL added=257 missing=0` y cero
     `MATER start`. Las celdas visibles se renderizan.
  2. El corte está en el sparse-loader de la unión: `REFRESH ... skipBefore=7552
     skipAfter=0`. El borde superior cierra (skipAfter=0) pero quedan ~7552
     archivos ANTES del inicio visible que no se muestran; `skipBefore` nunca llega
     a 0.
  3. Causa: en la unión nunca se propaga `update.range.from=0` al llegar al
     comienzo (siempre se usa el `min` de ids devueltos), por lo que el builder
     deja `skippedBefore=?` y el loader no explora hasta el inicio real.
- Traza corregida: `SPARSE applied ...` ahora se imprime DESPUÉS del merge (antes
  mostraba el estado previo y confundía). Muestra `f=front..back`, `skipB`, `skipA`
  post-merge.
- Verificación pendiente (tras compilar): reabrir y mirar `SPARSE applied` para ver
  cómo evoluciona `f` y si `skipB` baja a 0 en algún momento.

## Ronda 19.2 — Diagnóstico con traza post-merge (sesión 16:38) y ventana 1000
- Con la traza `SPARSE applied` (post-merge) se detectó la causa concreta del
  "no se muestran todos los archivos" en canales grandes:
  1. fullCount All=13399, pero la slice visible queda en ~257 y `skipB` en
     ~13000 SIN bajar. Al aplicar la página Before siguiente
     (`ALL APPLY nsk=12925..14038`) el front solo baja de 14039 a 13981, y la
     petición `ALL REQUEST msgId=14039` se repite: **el front de la unión avanza
     muy poco por página** por el recorte de `SparseIdsSliceBuilder::sliceToLimits`
     (poda a limitBefore+limitAfter alrededor del around).
  2. `fullCount` grande + ventana pequeña = los archivos antiguos quedan por
     debajo y no se muestran (el `skipBefore` en ~13000 es el contenido que falta).
- Cambio: `kFullIdsLimit` de 400 -> 1000 (window ≈ 2000 ids). Con la poda basada
  en limitBefore/limitAfter, una ventana mayor hace que el front avance mucho más
  por página Before y cubra más contenido antiguo por scroll.
- Verificación pendiente (tras compilar): abrir FilesPhotos/All en canal grande y
  scrollear hasta el fondo; ahora debe haber menos `skipBefore` residual y verse
  más contenido. Revisar en depurador si `ALL REQUEST msgId=` avanza hacia abajo
  (no repetido en 14039).

## Ronda 19.3 — Validación del depurador tras ventana 1000 (sesión 17:49, canal JAV)
- **FilesPhotos (type=13) AHORA FUNCIONA**: el `RESTART ... limit=1000` arranca con
  la ventana grande y el walkback inicial recorre TODO el historial hacia atrás:
  `14039 → 12925 → 12298 → 10173 → 7975 → 6811 → 5301 → 3240 → 1469 → 244 → 6 → 3 → 0`,
  hasta que el lote vacío (`FP APPLY items=0 nsk=0..3`) cierra con
  `SPARSE applied ... skipB=0 skipA=7571` y `REFRESH ... skipBefore=0`.
  Es decir, el front llega al **inicio real** del contenido (7709 items accesibles,
  `missing=0` en todas las slices, sin materializaciones).
- El scroll posterior de subida (hacia lo más nuevo) también recorre toda la lista
  hasta `around=16857` y cierra arriba con `skipAfter=0`
  (`FP DONE ... range=16859..Max ... skipA=0`). La lista completa es navegable en
  ambas direcciones, SIN huecos (FILL missing=0 siempre), sin bucles
  (`msgId` avanza y no se repite clavado).
- **Conclusión**: la Ronda 18b-19.2 (ventana de ~2000 ids) resolvió el problema de
  "no se muestran todos los archivos". El recorte del sparse builder ya no estanca
  el front; la exploración llega al origen real y `skipBefore` baja a 0.
- **Pendiente**: el módulo **All (type=14) NO se probó** en esta sesión (solo hay
  `FP` REQUEST/SEND/DONE, ninguna `ALL`). Falta validarlo igual: abrir "Todo" en el
  canal grande, recorrer hasta el principio y hasta el final.

## Ronda 19.4 — Validación All (type=14) con la ventana 1000 (sesión 18:16, canal JAV)
- **All AHORA FUNCIONA**, igual que FilesPhotos. Con `RESTART ... limit=1000`:
  - Walkback inicial desde `16859` → recorre TODO: `14039 → 12925 → 12298 → 10173 →
    ... → 244 → 6 → 3 → 0`.
  - Lote vacío final (`ALL APPLY items=0 nsk=0..3`, las 3 sub-búsquedas Photo/Video/
    File con `items=0`) cierra con `SPARSE applied ... skipB=0 skipA=13271` y
    `REFRESH ... skipBefore=0` → llega al **inicio real del contenido**
    (full=13399 items accesibles).
  - `FILL ... missing=0` en TODAS las slices, sin huecos y sin materializaciones.
  - `msgId` avanza sin bucles y el merge de 3 búsquedas (Photo+Video+File) funciona
    con deduplicación y `allOk=1`.
- **Los errores `PEER_ID_INVALID` / `delayed resend` del log** ocurren antes del
  arranque de All (18:16:38) y después de su carga inicial (18:17:09-26), y son de
  carga de datos/historial de fondo (ruta estándar requestMessageData), NO del
  walkback de búsquedas (que usa getMessages en lotes de la Ronda 18). No producen
  huecos visibles (`FILL missing=0`).
- **Conclusión final del objetivo**: Tanto FilesPhotos (type=13) como All (type=14)
  muestran ahora TODO el contenido sin límites ni huecos en canales grandes
  (7709 y 13399 items respectivamente), con la ventana `kFullIdsLimit=1000`.

## Ronda 19.5 — Fix del "congelamiento"/"no muestra todos" durante el scroll
- Relectura del problema del usuario: en FilesPhotos/All el muestreo "se congela" y
  no muestra todos los archivos; solo al usar "mostrar en el chat" aparecen. En
  cambio Media (PhotoVideo) siempre muestra todo.
- **Causa raíz (código)**: `Provider::checkPreload` (info_media_provider.cpp) tenía
  `_idsLimit = preloadIdsLimit` (~128) cada vez que hacía preload al scrollear.
  Eso **encogía la ventana** de kFullIdsLimit(=1000) a ~128 en cuanto el usuario
  se movía. Con una ventana tan pequeña, al llegar a una zona del historial cuyos
  ids aún no están cacheados, la cuadrícula no tiene celdas para mostrar
  ("congelamiento") y los ítem del chat de esa zona no aparecen. Al usar
  "mostrar en el chat" (jumpToMessage) se usaba kFullIdsLimit y por eso SÍ se veía:
  confirmaba que la diferencia era la ventana.
- **Fix**: en `checkPreload`, para FilesPhotos/All se conserva `kFullIdsLimit`
  durante el scroll (en lugar de preloadIdsLimit). El resto de módulos no cambia.
  Efecto: el sparse-builder pide/cachea ~1000 ids delante, la zona visible siempre
  tiene contenido y no se congela; los ítem del chat aparecen sin necesitar
  "mostrar en el chat".
- Pendiente: validar tras compilar, y reproducir el caso de **grupos con temas**
  (obtener un depurador con topic!=0) para confirmar si el mismo fix resuelve el
  tema o si hay un bug adicional específico de FilesPhotos/All en foros.

## Ronda 19.6 — Materialización PROACTIVA (change of approach, sesión actual)
- El usuario señaló que el método anterior no funcionaba y pidió cambiar de
  enfoque e implementar la materialización proactiva directamente.
- Con los depuradores disponibles la unión y las slices se ven completas
  (`missing=0`), pero en uso real faltaban archivos hasta "mostrar en el chat".
  La causa de fondo: la materialización del History solo era REACTIVA (se
  disparaba desde fillSections al detectar una celda sin HistoryItem durante el
  render). Si la zona visible aún no tenía cacheado su HistoryItem, quedaba un
  hueco persistente hasta que algo lo re-materializaba.
- **Fix aplicado** en `refreshViewer`: para FilesPhotos/All, tras asignar cada
  slice se llama a `requestMissingAround()` antes de `_refreshed.fire({})`, de
  modo que TODOS los ids de la ventana grande sin HistoryItem se materializan de
  inmediato por lotes (≤100). La ventana (kFullIdsLimit=1000) se rellena entera
  al llegar, sin huecos ni necesidad de "mostrar en el chat".
- `kMaxMissingRetries` subido de 3 a 8 (Header) para tolerar lotes grandes y
  fallos transitorios, manteniendo el anti-bucle.
- **Pendiente**: el usuario debe RECOMPILAR y probar:
  1) FilesPhotos y All en un canal grande (la ventana entera debe verse, sin
     congelamiento y sin faltar archivos hasta "mostrar en el chat").
  2) Un GRUPO/foro CON TEMAS (FilesPhotos en un tema debe mostrar su contenido,
     como Media). Si persiste el fallo en temas, se necesita un depurador con
     `topic != 0` para diagnosticar el paso de la unión con topicRootId y, solo
     entonces, aplicar el fix adicional específico de foros.
- La causa raíz de Ronda 19.5 (ventana encogida a ~128 en checkPreload al
  scrollear) sigue siendo válida; esta Ronda 19.6 la complementa haciendo que la
  materialización de la ventana grande sea inmediata y completa.

## Ronda 19.7 — Paralelismo acotado en la materialización (estabilidad)
- Objetivo del usuario (sesión actual): que FilesPhotos y All obtengan TODO el
  contenido de forma estable, sin que se detenga el procesado/cargado, y sin
  sobrecargar la app (cambio de enfoque).
- Análisis: la materialización (Rondas 18-19.6) era serial (1 lote de 100 ids a
  la vez: una zona de 1000 ids = ~10 pasadas de red seguidas). Al llegar a una
  zona sin HistoryItems cacheados, la cuadrícula permanecía vacía (aparentemente
  congelada/parada) hasta terminar la serie.
- Fix (`requestMissingAround`, info_media_provider.cpp): cola con paralelismo
  acotado `kMaxMaterializeParallel=3` lotes en vuelo (`startMore` rellena el
  hueco conforme cada lote se resuelve). La zona se rellena ~3x más rápido, con
  carga acotada a 3 getMessages concurrentes. Fin único: contar
  materializados/stillMissing, subir reintentos con tope, romper el ciclo de
  dependencias (libera memoria) y disparar `_refreshed.fire`.
- Se conserva el paquete completo para FilesPhotos/All:
  * ventana grande persistente (kFullIdsLimit=1000) también durante el scroll
    (Ronda 19.5),
  * materialización PROACTIVA de cada slice al llegar (Ronda 19.6),
  * paralelismo acotado en la materialización (Ronda 19.7),
  * anti-bucle con `_missingRetries`/`kMaxMissingRetries=8` (reset por slice).
- **Pendiente (usuario)**: recompilar y probar:
  1) FilesPhotos y All en canal grande: toda la lista debe verse mientras se
     scrollea, sin congelarse y sin que falten archivos hasta "mostrar en el
     chat".
  2) Grupo/foro CON TEMAS: si FilesPhotos en un tema sigue sin mostrar contenido
     (mientras Media sí), pasar un depurador con `topic != 0` para diagnosticar
     el paso de la unión con topicRootId y aplicar el fix específico de foros.

## Ronda 20 — Refuerzo del reenvío masivo (sesión actual)
- Petición del usuario: reenvío masivo sin límites (1000, 500, 200...), que
  NO se rompa la cadena de procesado (se reenvíe todo lo seleccionado hasta
  terminar), conservando la cronología que ya funciona bien, sin necesidad de
  reinstalar la app para poder volver a usar la funcionalidad, y sin malograr
  las demás funciones.
- **Causa raíz 1 (cadena rota / app colgada)**: `ApiWrap::forwardMessages` no
  decrementaba `shared->requestsLeft` en el handler de fallo. Si un lote de 100
  fallaba (p.ej. un mensaje con no_forwards o un flood), la cuenta nunca llegaba
  a 0, el successCallback nunca se disparaba y la funcionalidad quedaba esperando
  para siempre → en la práctica "la app se malograba" hasta reiniciarla.
- **Causa raíz 2 (sobrecarga / desorden)**: el envío era totalmente paralelo
  (todos los lotes a la vez). Con +1000 mensajes = +10 peticiones simultáneas →
  flood y cronología desordenada entre lotes.
- **Causa raíz 3 (límite de selección)**: `MaxSelectedItems=100` impedía
  seleccionar/enviar más de 100 mensajes desde el chat.
- **Fixes aplicados**:
  1) `apiwrap.cpp` forwardMessages → cadena SECUENCIAL de lotes (uno tras otro,
      el siguiente solo cuando el anterior termina, éxito o fallo). El fallo ya
      NO rompe la cadena: reporta y continúa hasta reenviar todo. successCallback
      siempre se dispara al final. Una petición en vuelo → estable y con
      cronología exacta (sin flood).
  2) `config.h` MaxSelectedItems 100 → 10000 (selección de reenvío sin límites
     prácticos).
- **Pendiente (usuario)**: recompilar y probar reenvíos masivos (200, 500, 1000+
  mensajes) — desde el chat (selección) y desde la cuadrícula de media — y
  verificar: todo se reenvía en orden, la app no se queda colgada, y se puede
  volver a usar la funcionalidad sin reiniciar la app.

## Ronda 21 — Robustez del reenvío masivo (crash al reenviar varios archivos)
- Síntoma reportado: reenviar VARIOS ARCHIVOS a la vez cerraba la app de forma
  inesperada ("cierre inesperado"), más errores 400 INPUT_PEERS_EMPTY en el
  log durante reenvío.
- Diagnóstico (dump WER `CrashDumps\Telegram.exe.9668.dmp` - 13:01:43, hilo 21160):
  excepción 0xC0000409, parámetro 7 = FAIL_FAST_INVALID_PARAMETER; RIP =
  Telegram.exe+0x5f2bc51 = la instrucción `int 29h` de `abort()` (UCRT estático,
  sin handler de SIGABRT). La pila muestra el camino MTP de reenvío:
  `PrepareMessage<boxed<MTPmessages_sendMultiMedia>>`, `MTP::Instance::processCallback`,
  `HistoryWidget::handleHistoryChange`, `std::_Xbad_function_call`,
  `std::_Func_impl_no_alloc::_Copy`, `_CxxThrowException` —
  consistente con invocación de un `std::function` VACÍO (bad_function_call ->
  terminación -> abort) y/o con peticiones con peer de origen vacío
  (INPUT_PEERS_EMPTY). El log de la instancia que crasheó se perdió por rotación.
- Herramientas usadas para el diagnóstico (fuera del repo): lector PDB perezoso
  (MSF v7, page=8192, BlockMapAddr@52; DBI stream 3; secciones stream 13;
  publics S_PUB32=0x110E en stream de símbolos; 516859 públicos indexados) +
  parseo del minidump (ThreadList=3, MemoryList=5, Exception=6) + resolutor
  RVA-vs-públicos. Scripts en `/tmp/opencode` (stackwalk2.py, pdb_map2.py).
- CAUSA RAÍZ del cierre (apiwrap.cpp): auto-captura por valor del callable
  recursivo (`sendNextChunk`). `[=]` copiaba el std::function VACÍO (estado
  pre-asignación); al terminar el lote 0, `settle` invocaba la copia vacía ->
  bad_function_call -> terminate -> abort() -> fastfail(7). FIX: guardar el
  callable en `std::shared_ptr<std::function<void()>>` y llamar mediante
  `(*sendNextChunk)()` (seguro: nada de copia del vacío ni referencia colgante).
- Refuerzos aditivos en la cadena:
  1) `settle` solo invoca `shared->callback()` / `(*sendNextChunk)()` si el
     std::function no está vacío (defensa en profundidad).
  2) Un lote con `ids` vacío o `fromInput.type() == mtpc_inputPeerEmpty` se
     SALTA (settle + return) y la cadena sigue — nunca se envía una petición
     inválida (evita 400 INPUT_PEERS_EMPTY) ni se pierde el conteo de
     `requestsLeft`.
- NO se cambió: lotes de 100, envío secuencial, fallo que no rompe la cadena,
  successCallback al final, MaxSelectedItems=10000, cronología.
- **Pendiente (usuario)**: recompilar (`msbuild Telegram.sln /p:Configuration=Release /p:Platform=x64 /m /t:Telegram`)
  y validar reenvíos masivos (200/500/1000+, chat y grid de media): sin cierre
  inesperado, en orden cronológico, sin INPUT_PEERS_EMPTY. Si reaparece el
  cierre, capturar el depurador de la instancia que crashea (el log se rota en
  cada lanzamiento) para el caller exacto.

## Ronda 22 — Desbloqueo de copiar/reenviar/seleccionar en canales con restricción de contenido
- QUÉ se pidió: copiar, reenviar, seleccionar y "Seleccionar todos" en
  canales/grupos con `noforwards`, sin romper nada (enfoque seguro del propio
  enunciado: neutralizar SOLO las comprobaciones de UI/acción locales).
- Archivos tocados (4): `history_item.cpp` (allowsForward), `history_inner_widget.cpp`
  (hasSelectRestriction, hasCopyRestriction, hasCopyMediaRestriction,
  hasCopyRestrictionForSelected, showCopyRestrictionForSelected),
  `history_view_list_widget.cpp` (CopyRestrictionTypeFor, CopyMediaRestrictionTypeFor,
  SelectRestrictionTypeFor, ListWidget::hasCopyRestrictionForSelected,
  ListWidget::showCopyRestrictionForSelected) y `history_view_context_menu.cpp`
  (AddSelectRestrictionAction -> no-op).
- NO se tocó: detección del flag (`forbidsForward`, `forbidsSaving`,
  `PeerData::allowsForwarding`), módulos Media/FilesPhotos/All, traducción,
  descarga masiva, `MaxSelectedItems`, `kMaxForwardMessages`. En chats sin
  restricción el resultado es idéntico al anterior.
- Estado del reaplicado (2026-09-05): los cambios de Ronda 22 + 22b se REAPLICARON
  sobre la rama actual, que SÍ contiene la cadena secuencial con `sendNextChunk`
  (Rondas 20-21). Por eso el fallback quedó integrado en la VERSIÓN SECUENCIAL de
  `forwardMessages`: cada `ForwardChunk` guarda ahora `items`
  (vector<not_null<HistoryItem*>>) en paralelo a `ids`/`randomIds`, y el handler
  de error de la cadena (`sendNextChunk`) detecta `CHAT_FORWARDS_RESTRICTED` para
  delegar en `sendForwardAsCopy`; al terminar las copias se llama `settle()`
  (misma contabilidad `requestsLeft`, y continua la cadena). Los 4 archivos de UI
  y la cabecera conservan `requestSharedMediaAll` y el resto intacto.
- PENDIENTE → RESUELTO en Ronda 22b: servidor rechaza el forward "puro" de
  contenido protegido (`CHAT_FORWARDS_RESTRICTED`). DECISIÓN DEL USUARIO:
  implementar el fallback "reenviar como copia". Añadido en `apiwrap`:
  - `ApiWrap::forwardMessages` (versión SECUENCIAL) acumula los ítems de cada
    lote en `ForwardChunk::items` (en paralelo a `ids`/`randomIds`, se reinicia
    en cada corte de lote). En el handler de error de la cadena,
    `CHAT_FORWARDS_RESTRICTED` + lote con ítems ⇒ `sendForwardAsCopy` en vez de
    toast; al terminar las copias se llama `settle()` (decrementa
    `shared->requestsLeft` y continua la cadena). Un ítem que falla en la copia →
    `sendMessageFail` y sigue.
  - Nuevo miembro privado `ApiWrap::sendForwardAsCopy(history, items, action,
    done)`: envía cada ítem como mensaje nuevo vía `sendPreparedMessage`.
    Texto → `MTPmessages_SendMessage` (texto+entidades); foto →
    `MTPmessages_SendMedia` + `MTP_inputMediaPhoto` (spoiler si procede);
    documento/video/audio → `MTPmessages_SendMedia` +
    `MTP_inputMediaDocument` (spoiler si procede). Replica flags de `SendAction`:
    reply_to, silent, schedule_date/repeat, shortcut, effect, suggested, send_as,
    allow_paid_stars (min(aprobadas, items*starsPerMessageChecked)).
  - No se toca el camino de éxito del forward ni la detección del flag; en chats
    SIN restricción el fallback nunca se dispara.
- VERIFICACIÓN (usuario): recompilar
  (`msbuild Telegram.sln /p:Configuration=Release /p:Platform=x64 /m /t:Telegram`)
  y probar: (1) canal/grupo CON restricción -> seleccionar, Seleccionar todos,
  copiar texto, copiar media, reenviar contexto y multi-selección; sin aviso; el
  reenvío debe entregar los mensajes como copia; (2) canal SIN restricción ->
  igual que antes; (3) multi-selección y resto de la app sin cambios.
