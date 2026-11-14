# Cambios aplicados — detalle por archivo (diff eef2d48..HEAD)

**Repositorio:** Telegram Desktop (C++/Qt) — `/mnt/c/Users/VICTOR/code/tdesktop`
**Rango de commits:** `eef2d48` ("first commit") → `HEAD` (`41286e02`)
**Metrica global del diff:** 33 archivos, 1887 inserciones (+) y 218 borrados (−).

> **Nota de alcance.** Este documento describe SOLO lo que está commiteado en git
> (`eef2d48..HEAD`). Las **Rondas 22c–23** (reintentos con mismos `randomIds`,
> watchdog de 2 h, pacing adaptativo 30–300 s, cierre de oleada anti-flood con
> pausa honesta, `sendForwardAsCopy` para contenido restringido y las trazas
> `[FWD]`/`[SBX]`/`[MTP]`/`[DEPUR]`) viven en el **working tree sin commitear**
> y por lo tanto NO aparecen aquí; están registradas en `03-CAMBIOS.md` y
> `05-REGISTRO-REENVIO-MASIVO.md`.

---

## Lista de commits

```
41286e02 Merge pull request #6 from nilvictor99/3-agregar-all-a-la-interfas
          "3 agregar all a la interfas" — añade el módulo "All" (Todo) a la
          interfaz (galería/perfil), junto con el botón y la vista de Todo.
aeaa8af7 cambio yea de mejora            — ajustes de pulido sobre las rondas previas.
317c2bb3 mejora                           — mejoras intermedias (catalogación parcial).
0f4d534d a                                — commit de trabajo (parte de la serie).
b56415f8 a                                — commit de trabajo (parte de la serie).
```

La serie construye, por capas: (1) desbloqueos de selección/descarga, (2) tipos de media compuestos `FilesPhotos` y `All` con su carga unificada, (3) reenvío y borrado masivos estables, y (4) estabilidad de carga (ventana grande + materialización + anti-bucles) con un depurador propio.

---

## Resumen por archivo

| Archivo | Cambios (+/−) | Tema / por qué |
|---|---|---|
| `10-INVENTARIO-ANTERIOR.md` | +54 / −0 | Documenta el fork: funcionalidades, estabilidad de carga y crashes corregidos. |
| `Telegram/Resources/langs/lang.strings` | +8 / −0 | Nuevas claves i18n para los módulos `FilesPhotos` y `All`. |
| `Telegram/SourceFiles/apiwrap.cpp` | +581 / −59 | Núcleo: unión de búsquedas `Photo`+`File` (FilesPhotos) y `Photo`+`Video`+`File` (All), y cadena de reenvío masivo secuencial. |
| `Telegram/SourceFiles/apiwrap.h` | +12 / −0 | Declara `requestSharedMediaFilesPhotos` / `requestSharedMediaAll`. |
| `Telegram/SourceFiles/boxes/share_box.cpp` | +214 / −88 | Reenvío desde la caja Compartir: lotes de 100, cola con `base::Timer`, toast de progreso y anti-flood (`FLOOD_WAIT`). |
| `Telegram/SourceFiles/config.h` | +6 / −1 | `MaxSelectedItems` subido de 100 a 10000. |
| `Telegram/SourceFiles/data/data_histories.cpp` | +23 / −19 | Borrado masivo en lotes de ≤100 por petición. |
| `Telegram/SourceFiles/data/data_media_types.cpp` | +9 / −3 | Máscaras `sharedMediaTypes()` incluyen `FilesPhotos`/`All`. |
| `Telegram/SourceFiles/data/data_search_controller.cpp` | +2 / −0 | Filtro MTP para `FilesPhotos`. |
| `Telegram/SourceFiles/data/data_shared_media.cpp` | +8 / −3 | `IsItemGoodForType` reconoce fotos/archivos en tipos compuestos; excluye stickers de `All`. |
| `Telegram/SourceFiles/data/data_sparse_ids.cpp` | +33 / −4 | Fuerza el merge en slices compuestas (evita bucle infinito) + traza `Depur`. |
| `Telegram/SourceFiles/debug_depurador.h` | +52 / −0 | **Nuevo archivo**: depurador `depurador.txt` (self-debug de la carga). |
| `Telegram/SourceFiles/info/info_controller.cpp` | +10 / −2 | Factoriza `mediaSourceOfType(type,…)` reutilizable. |
| `Telegram/SourceFiles/info/info_controller.h` | +7 / −0 | Declaración de `mediaSourceOfType` + include de tipos. |
| `Telegram/SourceFiles/info/info_wrap_widget.cpp` | +2 / −0 | Texto de selección en barra superior para `FilesPhotos`/`All`. |
| `Telegram/SourceFiles/info/media/info_media_buttons.cpp` | +2 / −0 | Frases de los botones de perfil para `FilesPhotos`/`All`. |
| `Telegram/SourceFiles/info/media/info_media_common.cpp` | +2 / −0 | `MinItemHeight` trata `FilesPhotos`/`All` como malla de fotos. |
| `Telegram/SourceFiles/info/media/info_media_common.h` | +4 / −0 | `MediaFilter::Files` nuevo + getter `mediaFilter()`. |
| `Telegram/SourceFiles/info/media/info_media_empty_widget.cpp` | +9 / −1 | Icono y mensajes "vacío" para los módulos compuestos. |
| `Telegram/SourceFiles/info/media/info_media_inner_widget.cpp` | +126 / −11 | Filtros superiores dinámicos (se ocultan si vacíos) + toggle vista Lista / 4 columnas para Archivos. |
| `Telegram/SourceFiles/info/media/info_media_inner_widget.h` | +9 / −0 | Miembros nuevos del widget interior. |
| `Telegram/SourceFiles/info/media/info_media_list_section.cpp` | +144 / −6 | Modo "tile" cuadrado para archivos (4 columnas) con painter dedicado. |
| `Telegram/SourceFiles/info/media/info_media_list_section.h` | +10 / −0 | Declaraciones del modo tile. |
| `Telegram/SourceFiles/info/media/info_media_list_widget.cpp` | +69 / −1 | `setFileGridColumns` + autoscroll al seleccionar con arrastre. |
| `Telegram/SourceFiles/info/media/info_media_list_widget.h` | +9 / −0 | Campos y métodos del autoscroll / columnas. |
| `Telegram/SourceFiles/info/media/info_media_provider.cpp` | +342 / −17 | Ventana grande de entrada, materialización proactiva y creación de layouts de los tipos compuestos. |
| `Telegram/SourceFiles/info/media/info_media_provider.h` | +32 / −0 | Constantes y estado del auto-reparado de la slice. |
| `Telegram/SourceFiles/info/media/info_media_widget.cpp` | +4 / −0 | Títulos de sección para `FilesPhotos`/`All`. |
| `Telegram/SourceFiles/info/profile/info_profile_shared_media_classic.cpp` | +2 / −0 | Botones `FilesPhotos`/`All` en el perfil clásico del canal. |
| `Telegram/SourceFiles/info/profile/info_profile_values.cpp` | +18 / −0 | Conteos combinados (suma de subconteos) para la etiqueta del perfil. |
| `Telegram/SourceFiles/overview/overview_layout.cpp` | +79 / −3 | `Document` en modo celda cuadrada sin nombre (tile). |
| `Telegram/SourceFiles/overview/overview_layout.h` | +3 / −0 | Campo `hideName` y override de `resizeGetHeight`. |
| `Telegram/SourceFiles/storage/storage_shared_media.h` | +2 / −0 | Nuevos valores `FilesPhotos` y `All` en `SharedMediaType`. |

---

## 10-INVENTARIO-ANTERIOR.md

La raíz `Telegram/SourceFiles/info/` contiene la lógica de la pestaña "Media" del perfil: lista media por tipo, filtros y búsqueda.

- **Funcionalidad**: README oficial del proyecto; explica qué es Telegram Desktop, licencia y cómo compilarlo.
- **Qué hacía antes** (baseline `eef2d48`): README oficial sin ninguna referencia a esta build personalizada; solo cabecera, licencia y sistemas soportados.
- **Cambios aplicados (por partes)**:
### Documentación del fork (funcionalidades + estabilidad + crashes)
    - **Qué se hizo**: se añade un bloque `> Build personalizada (fork)` con enlace a `C:\Users\VICTOR\code\docs-tdesktop`, seguido de tablas: "Gestión de contenido (desbloqueos)"; "Galería y perfiles"; "Reenvío y borrado masivos"; una lista "Estabilidad de carga"; y "Correcciones de crash destacadas". El módulo compuesto queda descrito explícitamente: *"se obtiene fusionando dos búsquedas MTP (Photo + File)"*.
    - **Con qué fin / por qué se cambió**: dejar constancia en el propio árbol de qué hace distinta esta build; es la fuente canónica de las funcionalidades añadidas (selección sin límite, cuadrícula densa, módulos `Todo`/`Fotos+Archivos`, reenvío masivo por lotes, borrado en lotes, ventana grande de carga, materialización con paralelismo acotado, fin-de-lista honesto, anti-bucle de reintentos) y de los crashes ya corregidos.
    - **Lógica aplicada**: solo documentación; no hay código. La corrección de crash citada (`std::bad_function_call` → `abort` en la cadena de reenvío) es exactamente la implementada en `apiwrap.cpp` (ver más abajo).

---

## Telegram/Resources/langs/lang.strings

- **Funcionalidad**: diccionario de traducciones de Telegram Desktop (claves usadas por el sistema `tr::lng_*`).
- **Qué hacía antes** (baseline `eef2d48`): sin claves para los nuevos tipos.
- **Cambios aplicados (por partes)**:
### Claves i18n para FilesPhotos / All
    - **Qué se hizo**: 8 claves nuevas junto a las de media:
      ```cpp
      "lng_media_type_photos_files"  = "Photos & Files";
      "lng_media_type_all"           = "All";
      "lng_profile_all#one"          = "{count} media item";
      "lng_profile_all#other"        = "{count} media items";
      "lng_media_all_empty"          = "No media here yet";
      "lng_profile_photos_files#one" = "{count} photo/file";
      "lng_profile_photos_files#other" = "{count} photos/files";
      "lng_media_photos_files_empty" = "No photos or files here yet";
      ```
    - **Con qué fin / por qué se cambió**: los nuevos tipos `FilesPhotos` y `All` necesitan: título de sección (`lng_media_type_*`), contador de perfil con plurales (`lng_profile_*#one/#other`) y mensaje de "sin contenido" (`lng_media_*_empty`).
    - **Lógica aplicada**: el resto del código referencia estas claves vía `tr::lng_profile_all(...)`, `tr::lng_media_all_empty(...)`, etc. Se reutilizan para los sufijos `#one`/`#other` los plurales estándar de Qt/Telegram.

---

## Telegram/SourceFiles/apiwrap.cpp

- **Funcionalidad**: `ApiWrap` es la fachada de llamadas al MTProto (mensajería, media, búsquedas, reenvíos). Aquí viven `requestSharedMedia`, `forwardMessages`, `sharedMediaDone` y la gestión de `_sharedMediaRequests`.
- **Qué hacía antes** (baseline `eef2d48`):
  - `requestSharedMedia` preparaba directamente una búsqueda única con `Api::PrepareSearchRequest` (un solo filtro MTP) y la registraba en `_sharedMediaRequests`.
  - `forwardMessages` acumulaba ids en un bucle y, cada vez que cambiaba el peer origen o se llegaba a `kMaxForwardMessages` (100), invocaba `sendAccumulated`, que **enviaba el lote inmediatamente**. Es decir: todos los lotes salían a la vez (paralelo), y la cuenta de finalización era un simple `shared->requestsLeft` decrementado en cada `done`/`fail`; el `successCallback` solo se disparaba cuando la cuenta llegaba a cero.

- **Cambios aplicados (por partes)**:

### Interceptación de `requestSharedMedia` para los tipos compuestos
    - **Qué se hizo**: al inicio del flujo normal, se añaden dos ramas:
      ```cpp
      if (type == SharedMediaType::FilesPhotos) { …; requestSharedMediaFilesPhotos(peer, topicRootId, monoforumPeerId, messageId, slice); return; }
      if (type == SharedMediaType::All) { …; requestSharedMediaAll(peer, topicRootId, monoforumPeerId, messageId, slice); return; }
      ```
      Cada rama emite una traza `[DEPUR] FP REQUEST` / `[DEPUR] ALL REQUEST` con peer, `msgId` y `slice`, y delega en las nuevas funciones.
    - **Con qué fin / por qué se cambió**: `FilesPhotos` y `All` son **uniones de varias secuencias de servidor** (Photo+File y Photo+Video+File) que ningún filtro MTP único representa; por eso el envío ya no puede pasar por el camino mono-tipo.
    - **Lógica aplicada**: `return` temprano dentro del `requestSharedMedia` original; el resto del camino (búsqueda clásica) queda intacto para los tipos individuales.

### `requestSharedMediaFilesPhotos` — fusión Photo + File
    - **Qué se hizo**: nueva función que registra la clave de petición, construye un `State` compartido con dos `std::optional<Api::SearchResult>` (`photo`, `file`) y sus flags `photoOk/fileOk`, y lanza **dos búsquedas en paralelo** (`SharedMediaType::Photo` y `SharedMediaType::File`). Cada una pasa por `histories->sendRequest(...)` con `RequestType::History` y cierra con `Api::ParseSearchResult`. Cuando ambas terminan (`doneCount == 2`, patrón `finishOne`) se hace el merge en un único `Api::SearchResult`.
    - **Con qué fin / por qué se cambió**: el módulo "Fotos + Archivos" no existe como tipo MTP nativo; la única forma fiable de provocar que el sparse-loader acumule todo es unir las dos secuencias del servidor. El punto delicado era el `noSkipRange` de la fusión.
    - **Lógica aplicada**:
      - El merge calcula `merged.fullCount` como suma de sub-conteos y concatena `messageIds`.
      - `noSkipRange` se fija al **rango real** de los ids recibidos: `minmax_element` sobre los ids fusionados.
      - Casos de borde:
        - Lote vacío y al menos una búsqueda fallida (`!allOk`): se **omite la actualización** y se loguea `[DEPUR] FP MERGE-SKIP items=0 allOk=0`, para que el sparse-loader no congele la exploración con un falso "fin de lista" (`{0,0}`) y siga re-pidiendo en el próximo disparo de scroll.
        - Lote vacío con búsquedas OK: se distinguen dos subcasos mediante un "tope honesto": si el `from` federal (mínimo de los `noSkipRange.from` de Photo y File) es `> kMinServerMsgId` (`constexpr auto kMinServerMsgId = 2;`, los ids de mensaje de servidor parten de ~2), da igual: aún no se alcanzó el inicio del historial, así que se **omite** la actualización para que el loader siga pidiendo las páginas anteriores (evita `skippedBefore=0` prematuro). Solo si `from <= 2` se propaga el rango federal `{federalFrom, federalTill}` como fin real.
      - Se loguea `[DEPUR] FP APPLY items=… count=… nsk=…..  allOk=…` y se llama a `sharedMediaDone(...)` con el resultado fusionado.
      - Los sub-envíos usan lambdas `sendOne(type)` con punteros capturados al slot correcto del `State` (vía `[&]` sobre el `State` compartido), nombres `QString` por tipo para las trazas, y ocupan la cadena `done`→`ParseSearchResult`+`okFlag=true`→`finishOne`+`finish`, y `fail`→`SearchResult()` vacío→`finishOne`+`finish`.
    - **Nota de diseño**: el `State` es `std::shared_ptr` (dos sub-peticiones con distinta vida); el `doneCount` también es `shared_ptr<int>` para que el cierre solo ocurra cuando ambos hayan terminado, manteniendo `_sharedMediaRequests.emplace(key)` desde el primer momento (el loader ya sabe que hay una petición en vuelo).

### `requestSharedMediaAll` — fusión Photo + Video + File
    - **Qué se hizo**: el mismo patrón que `FilesPhotos` pero con **tres** secuencias (`Photo`, `Video`, `File`), un `State` con tres slots y `doneCount` esperando a `3`. El merge suma `fullCount`, concatena los tres `messageIds` y calcula `noSkipRange` con `minmax_element` de la unión.
    - **Con qué fin / por qué se cambió**: el módulo "Todo" junta fotos, videos y archivos (excluidas voz/stickers/GIF/link) en una sola cuadrícula con orden cronológico global.
    - **Lógica aplicada**: idéntica a `FilesPhotos` con los tres sub-tipos: `federalFrom = qMin(photo.from, qMin(video.from, file.from))`, `federalTill = qMax(…)`, mismo `kMinServerMsgId = 2` para decidir el "fin real" y mismas rutas de *skip* (todo-OK pero sin llegar al inicio, o alguna sub-búsqueda fallida) y trazas `[DEPUR] ALL …`. Por último `sharedMediaDone(..., SharedMediaType::All, ...)`.

### Reescritura de `forwardMessages` — reenvío masivo en cadena
    - **Qué se hizo**: sustituye el bucle+`sendAccumulated` paralelo por una **construcción previa de lotes** y una **cadena secuencial**:
      1. **Troceado**: se define `struct ForwardChunk { not_null<PeerData*> from; QVector<MTPint> ids; QVector<MTPlong> randomIds; std::shared_ptr<base::flat_map<uint64, FullMsgId>> localIds; };` y se rellena `std::vector<ForwardChunk> chunks` con `chunks.reserve(1 + count / kMaxForwardMessages)`. La lambda interna `flush` empuja el chunk actual cuando `ids.isEmpty()`; se corta al cambiar de peer origen o al llegar a 100 ids. El `randomId` (`base::RandomValue<uint64>`) y el mensaje local (si `genClientSideMessage`) se generan antes del troceado real.
      2. **Estado de la cadena**: `scheduled`, `starsLeft = shared_ptr<int>(action.options.starsApproved)`, `nextIndex = shared_ptr<int>(0)`.
      3. **Callable recursivo** guardado en `std::shared_ptr<std::function<void()>>`:
         ```cpp
         const auto sendNextChunk = std::make_shared<std::function<void()>>();
         *sendNextChunk = [=, &histories] { … };
         ```
         En cada invocación toma el índice `(*nextIndex)++`, y si `index >= chunks.size()` retorna sin hacer nada (final natural de la cadena).
      4. **`starsPaid` acumulado**: `std::min(*starsLeft, int(ids.size() * peer->starsPerMessageChecked()))`, restando del total disponible.
      5. **`settle` defensivo**:
         ```cpp
         const auto settle = [=] {
             if (shared && !--shared->requestsLeft && shared->callback) { shared->callback(); }
             if (*nextIndex < int(chunks.size()) && *sendNextChunk) { (*sendNextChunk)(); }
         };
         ```
         Solo invoca un handler si existe (invocar un `std::function` vacío lanza `std::bad_function_call` → `abort` y cierra la app).
      6. **Salto de lotes inválidos**: si `ids.isEmpty() || fromInput.type() == mtpc_inputPeerEmpty` (peer de origen aún no resuelto → la API rechazaría con `INPUT_PEERS_EMPTY`), se hace `settle()` y se continúa.
      7. Los `done`/`fail` terminan con `settle()` siempre (ya no se quedan colgados si un lote falla) y el `done` usa además `ProcessRecentSelfForwards(_session, result, peer->id, fromId)` cuando `peer->isSelf() && _session->premium()`.
      8. Al final: `if (shared) shared->requestsLeft = int(chunks.size());` y `(*sendNextChunk)()` arranca la cadena.
    - **Con qué fin / por qué se cambió**:
      - *Flood/desorden*: con reenvíos de 900+ o 1000+ mensajes, enviar todos los lotes a la vez disparaba decenas de peticiones simultáneas (flood del API y mensajes entregados desordenados). Ahora solo hay **una petición en vuelo**, en orden cronológico.
      - *Cuelgues*: un fallo de lote dejaba la cuenta pendiente para siempre (nunca se llegaba a `requestsLeft == 0`); ahora el fallo se reporta y la cadena continúa y el `successCallback` se dispara SIEMPRE al terminar el último lote.
      - *Crash*: el callable recursivo auto-capturado por valor copiaba la versión vacía inicial y al invocar esa copia lanzaba `std::bad_function_call` → `abort`. El `shared_ptr` mantiene vivo el estado mientras la cadena está en vuelo y evita el problema; el `settle` cortocircuita si el callable no existe.
    - **Lógica aplicada**: *estado compartido vía `shared_ptr`* (contadores sin copias), *cadenas con `finish`* de `Histories::sendPreparedMessage`, envío de a un chunk por tick (sin timer, la propia finalización de la petición encadena la siguiente), y protección explícita de peer vacío. Las trazas `[DEPUR] FP / ALL …` van por `Depur::append` (ver `debug_depurador.h`).

---

## Telegram/SourceFiles/apiwrap.h

- **Funcionalidad**: cabecera de `ApiWrap` que expone los servicios de MTProto de la sesión.
- **Qué hacía antes** (baseline `eef2d48`): declaraba `requestSharedMedia(...)` y `requestGlobalMedia(...)` pero no las rutas de unión.
- **Cambios aplicados (por partes)**:
### Declaración de las peticiones compuestas
    - **Qué se hizo**: dos declaraciones públicas con la misma firma que el resto de peticiones de media:
      ```cpp
      void requestSharedMediaFilesPhotos(not_null<PeerData*> peer, MsgId topicRootId,
          PeerId monoforumPeerId, MsgId messageId, SliceType slice);
      void requestSharedMediaAll(not_null<PeerData*> peer, MsgId topicRootId,
          PeerId monoforumPeerId, MsgId messageId, SliceType slice);
      ```
    - **Con qué fin / por qué se cambió**: `Provider`/`AbstractController` disparan la carga de `FilesPhotos`/`All` a través de `sharedMediaDone` y estas rutas son el punto de entrada desde `requestSharedMedia`.
    - **Lógica aplicada**: interfaz pasiva; el comportamiento está en el `.cpp`. Los parámetros replican la firma de `requestSharedMedia` para encajar en los mismos call-sites.

---

## Telegram/SourceFiles/boxes/share_box.cpp

- **Funcionalidad**: caja de "Reenviar / Compartir" de Telegram Desktop; aquí `DefaultForwardCallback` orquesta el envío de `messages.forwardMessages` a los destinos elegidos.
- **Qué hacía antes** (baseline `eef2d48`): por cada hilo de destino construía **una única** petición con TODOS los ids (`mtpMsgIds`), un `buildMessage` (que fabricaba `randomIds` para `msgCount`), y registraba una sola `requestKey`. El `requestDone/requestFail` quitaban la clave y, cuando `state->requests.empty()`, cerraban la capa y mostraban el toast de reenvío. No había progreso, ni troceado, ni manejo de flood.
- **Cambios aplicados (por partes)**:

### Toast de progreso de reenvío masivo
    - **Qué se hizo**: nueva struct y lambda compartidas:
      ```cpp
      struct MassProgressState { int sent = 0; int total = 0; int activeThreads = 0;
                                base::weak_ptr<Ui::Toast::Instance> toast; };
      const auto progress = std::make_shared<MassProgressState>();
      const auto updateMassProgress = [=] { … };
      ```
      Si queda trabajo muestra un toast `"Reenviando %1 de %2… quedan %3"` con `duration = 4000`; si `total <= sent` oculta el toast anterior (`hideAnimated`).
    - **Con qué fin / por qué se cambió**: con reenvíos de cientos de mensajes el usuario necesitaba feedback del avance y cuánto quedaba por enviar.
    - **Lógica aplicada**: estado compartido `shared_ptr`, `sent`/`total` acumulados por todos los hilos de destino y un nuevo toast por cada incremento (se oculta el anterior antes de mostrar el siguiente).

### Troceado en lotes de 100 y cola por hilo (`MassQueue`)
    - **Qué se hizo**: constantes del reenvío masivo:
      ```cpp
      constexpr auto kMassBatchSize = 100;
      constexpr auto kMassBatchDelay = crl::time(300);
      constexpr auto kMaxFloodRetries = 5;
      constexpr auto kMaxFloodWaitSec = 60;
      ```
      Se trocea `mtpMsgIds` en `parts` (100 cada uno), se acumula `progress->total += msgCount`, y por hilo con lotes se incrementa `progress->activeThreads`. Cada hilo crea un `MassQueue`:
      ```cpp
      struct MassQueue { base::Timer timer; std::function<void()> step;
                         std::shared_ptr<MassQueue> keepAlive; };
      ```
      El `timer` invoca `step` si existe (con `weak_ptr` para no retener después de destruirse); el `keepAlive` mantiene viva la cola mientras haya work. La lambda `queue->step` captura por valor `parts`, `idx` (`shared_ptr<int>`), `retries`, `state`, la referencia `&histories`, etc., y **envía un lote por invocación**, programando el siguiente con `s->timer.callOnce(kMassBatchDelay)` (300 ms de pausa anti-flood entre lotes).
    - **Con qué fin / por qué se cambió**: la API limita `messages.forwardMessages` a 100 ids; una petición única con 900+ se truncaba o fallaba. Además, serializar con pausa evita rápido-fuego de peticiones y preserva el orden de entrega.
    - **Lógica aplicada**: *cola secuencial con `base::Timer` y `keepAlive`* — una petición en vuelo por hilo; `step` se autoprograma con `callOnce(kMassBatchDelay)`; `keepAlive` se suelta (`nullptr`) solo cuando `state->requests.empty() && activeThreads == 0` (fin completo), liberando el ciclo de referencias.

### Reintentos ante `FLOOD_WAIT`
    - **Qué se hizo**: en el `fail` de la petición, si `error.type().startsWith(u"FLOOD_WAIT_"_q)` y `*retries < kMaxFloodRetries`:
      - se parsea el número de segundos (`base::StringViewMid(type, prefix.size()).toInt()`), se acota con `std::clamp(…, 1, kMaxFloodWaitSec)`;
      - se quita la `requestKey` del mapa (para no contar como pendiente), se incrementa `retries` y se vuelve a lanzar el mismo lote con `s->timer.callOnce(secs * crl::time(1000))`.
    - **Con qué fin / por qué se cambió**: en reenvíos masivos Telegram responde frecuentemente `FLOOD_WAIT_N`; antes eso mataba el envío con un fallo y el usuario veía el lote incompleto. Ahora el lote se reintenta hasta 5 veces con la espera indicada por el servidor.
    - **Lógica aplicada**: reintento con backoff terciario (el servidor manda el segundo), contador compartido en `shared_ptr<int>`, pausa vía el timer de la propia `MassQueue`, y el lote retringit recomienza desde el mismo `idx` (no se avanza `idx` en la rama flood).

### Cierre correcto de la cuenta y estrellas solo en el primer lote
    - **Qué se hizo**: en el `done` de cada lote se acumula `progress->sent += part.size()` y se actualiza el toast; si era el último lote del hilo se hace `threadDone()` (decrementa `activeThreads`), si no se programa el siguiente; y siempre se llama a `finishRequest(requestKey)`. `finishRequest` quita la clave y, si `state->requests.empty() && progress->activeThreads == 0`, libera el `keepAlive`, oculta la capa (`show->hideLayer()`) y muestra `ShowForwardedMessageToast`. Además, las estrellas se pagan solo en el primer lote: `const auto paid = (*idx == 0) ? starsPaid : 0;` y `partFlags = sendFlags | (paid ? Flag::f_allow_paid_stars : Flag())`.
    - **Con qué fin / por qué se cambió**: garantizar que el cierre del diálogo y el toast solo ocurren cuando **todos** los hilos terminaron (antes dependía solo de un contador de peticiones simples), y no cobrar estrellas por mensaje en cada lote de un mismo reenvío multi-lote.
    - **Lógica aplicada**: cuenta en dos niveles (`state->requests` por petición + `activeThreads` por hilo); `buildMessage` se construye dentro del `step` con el lote concreto (`MTP_vector<MTPint>(part)`), `randomIds` con `base::RandomValue` por elemento y `MTP_long(paid)`.

---

## Telegram/SourceFiles/config.h

- **Funcionalidad**: constantes globales de configuración compilada del cliente (límites de selección, cifrado local, etc.).
- **Qué hacía antes** (baseline `eef2d48`): `MaxSelectedItems = 100`.
- **Cambios aplicados (por partes)**:
### Límite de selección ampliado
    - **Qué se hizo**:
      ```cpp
      MaxSelectedItems = 10000,
      ```
      con un comentario que explica la subida.
    - **Con qué fin / por qué se cambió**: los reenvíos masivos (900+, 5000+) requerían seleccionar más de 100 ítems a la vez; la UI ya trocea todo lo seleccionado en lotes de 100 dentro de `ApiWrap::forwardMessages`, así que el tope solo protege frente a selecciones patológicas.
    - **Lógica aplicada**: constante única en un `enum`; el resto de la app (selección de media, `_selectedLimit`) la usa de forma centralizada.

---

## Telegram/SourceFiles/data/data_histories.cpp

- **Funcionalidad**: `Histories` gestiona las colas de peticiones por historial (borrar, limpiar, marcar leído, etc.) con semáforo de una petición por historial.
- **Qué hacía antes** (baseline `eef2d48`): `deleteMessages` enviaba **una sola** petición `messages.deleteMessages` / `channels.deleteMessages` con TODOS los ids.
- **Cambios aplicados (por partes)**:
### Borrado masivo en lotes
    - **Qué se hizo**:
      ```cpp
      constexpr auto kMaxDeletePerRequest = 100;
      for (auto offset = 0; offset < ids.size(); offset += kMaxDeletePerRequest) {
          const auto part = ids.mid(offset, kMaxDeletePerRequest);
          sendRequest(history, RequestType::Delete, [=](Fn<void()> finish) { … });
      }
      ```
      El cuerpo interno es el mismo que antes (canal → `channels.deleteMessages`, normal → `messages.deleteMessages` con flag `revoke`), pero ahora por sub-lote.
    - **Con qué fin / por qué se cambió**: la API limita `deleteMessages` a 100 ids. Un borrado de 900+ se truncaba silenciosamente y parte de los mensajes "reaparecían" (nunca llegaron a borrarse). Con lotes de 100 todos se borran de verdad y los `applyAffectedMessages` y `history->requestChatListMessage()` se hacen por lote.
    - **Lógica aplicada**: reutiliza el mecanismo existente `sendRequest(history, RequestType::Delete, cb)` — el propio `Histories` serializa por historial, de modo que los lotes se envían en orden y cada `finish` desbloquea el siguiente.

---

## Telegram/SourceFiles/data/data_media_types.cpp

- **Funcionalidad**: implementa los tipos de media (`MediaPhoto`, `MediaFile`, …) y sus máscaras `sharedMediaTypes()` — los "cajones" del historial donde cae cada mensaje.
- **Qué hacía antes** (baseline `eef2d48`): una foto solo aparecía en `Photo`/`PhotoVideo`; un vídeo en `Video`/`PhotoVideo`; un documento genérico solo en `File`.
- **Cambios aplicados (por partes)**:
### Fotos también en `FilesPhotos` y `All`
    - **Qué se hizo** (`MediaPhoto::sharedMediaTypes`):
      ```cpp
      return Storage::SharedMediaTypesMask{}
          .added(Type::Photo)
          .added(Type::PhotoVideo)
          .added(Type::FilesPhotos)
          .added(Type::All);
      ```
    - **Con qué fin / por qué se cambió**: la unión `FilesPhotos` (fotos+archivos) y `All` (fotos+vídeo+archivos) deben indexar también los mensajes de foto.
    - **Lógica aplicada**: la máscara es un `flat_set` de bits; añadir los tipos sigue el patrón de `.added(...)` en cadena.
### Vídeos y archivos en los tipos compuestos
    - **Qué se hizo** (`MediaFile::sharedMediaTypes`): para `_document->isVideoFile()` se añade `.added(Type::All)`; y el retorno final pasa de `return Type::File;` a:
      ```cpp
      return Storage::SharedMediaTypesMask{}
          .added(Type::File)
          .added(Type::FilesPhotos)
          .added(Type::All);
      ```
    - **Con qué fin / por qué se cambió**: un vídeo debe aparecer en `All` (pero no en `FilesPhotos`, que es "sin videos"); un documento genérico (tema, imagen, o no-streamable) debe aparecer en `File`, `FilesPhotos` y `All`. Los GIF/música/voz no se tocan (quedan fuera de los compuestos).
    - **Lógica aplicada**: se mantienen las ramas especiales (voice, music) y solo se amplía la rama genérica y la de vídeo.

---

## Telegram/SourceFiles/data/data_search_controller.cpp

- **Funcionalidad**: convierte `Storage::SharedMediaType` en el filtro MTP de búsqueda (`MTP messagesFilter`).
- **Qué hacía antes** (baseline `eef2d48`): sin `case FilesPhotos`; un `Type::FilesPhotos`/`All` habría caído en `MTP_inputMessagesFilterEmpty()` (búsqueda sin filtrar → resultados incorrectos).
- **Cambios aplicados (por partes)**:
### Filtro MTP document para `FilesPhotos`
    - **Qué se hizo**:
      ```cpp
      case Type::FilesPhotos:
          return MTP_inputMessagesFilterDocument();
      ```
    - **Con qué fin / por qué se cambió**: `ApiWrap::requestSharedMediaFilesPhotos` nunca pasa directamente `Type::FilesPhotos` a `PrepareSearchRequest` (hace la unión Photo+File), por lo que este case cubre cualquier call-site que sí lo haga y sirve como filtro base de "documentos" en la unión.
    - **Lógica aplicada**: añadido al final del `switch`; `Type::All` no aparece aquí porque `requestSharedMediaAll` lo descompone en `Photo`/`Video`/`File` antes de preparar la búsqueda.

---

## Telegram/SourceFiles/data/data_shared_media.cpp

- **Funcionalidad**: `IsItemGoodForType` valida qué mensajes pertenecen a cada tipo de media compartida (usado por el indexado local y la renderización de los tipos).
- **Qué hacía antes** (baseline `eef2d48`): solo `Photo`/`PhotoVideo` aceptaban fotos; `File` aceptaba documentos; no había tipos compuestos.
- **Cambios aplicados (por partes)**:
### Fotos válidas en `FilesPhotos`/`All`
    - **Qué se hizo**:
      ```cpp
      const auto filesPhotosType = (type == Type::FilesPhotos);
      const auto allType = (type == Type::All);
      if ((photoType || photoVideoType || filesPhotosType || allType) && photo) return true;
      ```
    - **Con qué fin / por qué se cambió**: un mensaje de foto debe ser "bueno" para los módulos Fotos+Archivos y Todo.
    - **Lógica aplicada**: ampliación del predicado existente sin tocar el resto.
### Stickers excluidos, vídeos incluidos y documentos genéricos validados
    - **Qué se hizo**:
      ```cpp
      if (allType && document->sticker()) return false;   // sticker NO entra en Todo
      …
      || ((videoType || photoVideoType || allType) && videoDoc)
      || ((fileType || filesPhotosType || allType) && (document->isTheme()
          || document->isImage() || !document->canBeStreamed()));
      ```
    - **Con qué fin / por qué se cambió**: reglas de contenido de cada módulo compuesto: los stickers quedan fuera de `All` (ruido visual); los vídeos entran en `All`; los documentos genéricos (tema/imagen/no-streamable) entran en `File`, `FilesPhotos` y `All`.
    - **Lógica aplicada**: predicados booleanos reutilizando los campos del documento; el `return false` del sticker se coloca justo después de comprobar `!document` para no afectar a los tipos individuales.

---

## Telegram/SourceFiles/data/data_sparse_ids.cpp

- **Funcionalidad**: `SparseIdsSliceBuilder` — la máquina que acumula los resultados de páginas de búsqueda hasta cubrir el historial y decide `skippedBefore/skippedAfter` (los "huecos" que el loader debe seguir rellenando).
- **Qué hacía antes** (baseline `eef2d48`): calculaba `needMergeMessages` solo si el rango de la actualización intersectaba el rango actual de `_ids`; con un `_ids` vacío usaba `_key` como rango.
- **Cambios aplicados (por partes)**:
### Forzar el merge de las actualizaciones en slices compuestas
    - **Qué se hizo**:
      ```cpp
      auto needMergeMessages = (update.messages != nullptr)
          && (_ids.empty() || intersects(update.range, {_ids.front(), _ids.back()}));
      // … el forzado …
      needMergeMessages = needMergeMessages || (update.messages != nullptr);
      ```
    - **Con qué fin / por qué se cambió**: en `FilesPhotos`/`All` cada actualización trae la **unión** de varias secuencias de servidor; la primera página ya cubre un rango ancho y las páginas siguientes no intersectan el estado visible, por lo que el builder descartaría sus ids (bucle infinito: el `msgId` del "Before" nunca avanzaba y se re-pedía la misma página). Forzando el merge, `_ids` crece, el loader avanza y explora todo el historial. El `base::flat_set::merge` es idempotente (no duplica), y no afecta a los tipos contiguos (Media/Archivos) que ya intersectaban.
    - **Lógica aplicada**: solo afecta cuando hay `update.messages` (viene de búsqueda de media), no al camino de conteos; el resto del método (skips, `_count`, rango) queda igual.
### Traza `[DEPUR] SPARSE` del borde de la slice
    - **Qué se hizo**: tras el merge se añade:
      ```cpp
      Depur::append("[DEPUR] SPARSE applied range=%1..%2 items=%3 ids=%4 f=%5..%6 skipB=%7 skipA=%8 count=%9" …);
      ```
      con `#include "debug_depurador.h"`.
    - **Con qué fin / por qué se cambió**: diagnosticar por qué `FilesPhotos`/`All` "no cierran" frente a `Media` (que cierra con `skipBefore/skipAfter` en 0/0): muestra el rango aplicado, el nº de ids y los skips para comparar comportamientos.
    - **Lógica aplicada**: saneado de valores opcionales (`QStringLiteral("n/a")` / `"-"` / `"?"`) para los campos sin dato en la cadena de log.

---

## Telegram/SourceFiles/debug_depurador.h

**Archivo nuevo** (no existía en `eef2d48`).
- **Funcionalidad**: helper de depuración propia del fork, no presente en el tdesktop oficial.
- **Cambios aplicados (por partes)**:
### Namespace `Depur` — trazas de FilesPhotos/All
    - **Qué se hizo**: cabecera `#pragma once` con un namespace `Depur` e inline funcs:
      - `filePath()` → `cWorkingDir() + "depurador.txt"` (junto a `log.txt`);
      - `append(text)` — escribe una línea con timestamp `yyyy-MM-dd HH:mm:ss.zzz`; **thread-safe** (`static QMutex` + `QMutexLocker`); la primera vez de cada proceso borra el archivo previo (log limpio por ejecución) y luego abre en modo `Append|Text` con `QTextStream`;
      - `num(qlonglong)` → `QString::number` (formato de ids).
    - **Con qué fin / por qué se cambió**: los módulos `FilesPhotos`/`All` generan trazas `[DEPUR] FP / ALL / SPARSE / PROVIDER / MATER / PRELOAD / REFRESH / FILL / RESTART` desde `apiwrap.cpp`, `data_sparse_ids.cpp` e `info_media_provider.cpp`; este archivo centraliza la escritura sin interferir con `Logs` oficiales.
    - **Lógica aplicada**: todo *inline* en cabecera (sin unidad de compilación extra), estado estático local para la inicialización, y API mínima `append`/`num` que las otras TU llaman.

---

## Telegram/SourceFiles/info/info_controller.cpp

- **Funcionalidad**: `AbstractController` proporciona la fuente de la slice media (`mediaSource`) para el perfil y la galería, junto con la navegación por secciones.
- **Qué hacía antes** (baseline `eef2d48`): `mediaSource(...)` construía el `SparseIdsMergedSlice` usando siempre `section().mediaType()`.
- **Cambios aplicados (por partes)**:
### `mediaSourceOfType` parametrizado por tipo
    - **Qué se hizo**: se renombra el cuerpo original a `mediaSourceOfType(Storage::SharedMediaType type, …)` (recibe el tipo como argumento), y `mediaSource(...)` pasa a ser un delegado:
      ```cpp
      rpl::producer<SparseIdsMergedSlice> mediaSource(type? no:)
      return mediaSourceOfType(section().mediaType(), aroundId, limitBefore, limitAfter);
      ```
    - **Con qué fin / por qué se cambió**: `Provider::refreshViewer` necesita pedir la slice de un tipo concreto (`section().mediaType()`) y el resto de la lógica (migrated, sublist, monoforum) sigue siendo común. La factorización evita duplicar el montaje de la clave MTP del slice.
    - **Lógica aplicada**: la cadena `SharedMediaMergedViewer(...)` se pasa el `type` en lugar de derivarlo de la sección; comportamiento idéntico para los tipos clásicos.

---

## Telegram/SourceFiles/info/info_controller.h

- **Funcionalidad**: declaraciones de `AbstractController` y el memento de la pestaña de información.
- **Qué hacía antes** (baseline `eef2d48`): solo `mediaSource(...)` público.
- **Cambios aplicados (por partes)**:
### Declaración de `mediaSourceOfType` e include del tipo
    - **Qué se hizo**: `#include "storage/storage_shared_media.h"` y la declaración `virtual rpl::producer<SparseIdsMergedSlice> mediaSourceOfType(Storage::SharedMediaType type, SparseIdsMergedSlice::UniversalMsgId aroundId, int limitBefore, int limitAfter) const;`
    - **Con qué fin / por qué se cambió**: exponer a los `Provider` la carga por tipo concreto sin depender del include accidental.
    - **Lógica aplicada**: include directo del header de tipos para que el parámetro quede completo en la firma.

---

## Telegram/SourceFiles/info/info_wrap_widget.cpp

- **Funcionalidad**: el widget "envoltorio" de la pestaña Media; `generateSelectedText` produce el texto de la barra superior cuando hay ítems seleccionados.
- **Qué hacía antes** (baseline `eef2d48`): `switch` de tipos sin casos para `FilesPhotos`/`All` → habría caído en `Unexpected(...)` (crash en barra superior al seleccionar en esos módulos).
- **Cambios aplicados (por partes)**:
### Casos de barra superior para tipos compuestos
    - **Qué se hizo**:
      ```cpp
      case Type::FilesPhotos: return tr::lng_media_selected_media;
      case Type::All: return tr::lng_media_selected_media;
      ```
    - **Con qué fin / por qué se cambió**: el crash "Seleccionar todo en Fotos + Archivos" venía precisamente de aquí (switch exhaustivo sin caso → `Unexpected` → abort). Con el caso se muestra la etiqueta genérica de media seleccionada.
    - **Lógica aplicada**: reutilización de la frase existente `lng_media_selected_media` en lugar de crear una nueva clave.

---

## Telegram/SourceFiles/info/media/info_media_buttons.cpp

- **Funcionalidad**: botones de la cabecera/perfil que abren cada pestaña media (`MediaTextPhrase` devuelve la frase con contador).
- **Qué hacía antes** (baseline `eef2d48`): sin casos compuestos → `Unexpected` si se pulsara.
- **Cambios aplicados (por partes)**:
### Frases de botón para los módulos compuestos
    - **Qué se hizo**:
      ```cpp
      case Type::FilesPhotos: return tr::lng_profile_photos_files;
      case Type::All: return tr::lng_profile_all;
      ```
    - **Con qué fin / por qué se cambió**: mostrar "N photos/files" y "N media items" en los botones del perfil (las frases con plural `#one/#other` definidas en `lang.strings`).
    - **Lógica aplicada**: ampliación pura del `switch`.

---

## Telegram/SourceFiles/info/media/info_media_common.cpp

- **Funcionalidad**: utilidades comunes de la pestaña Media, entre ellas `MinItemHeight(type, width)`.
- **Qué hacía antes** (baseline `eef2d48`): tipos compuestos sin caso → altura por defecto corporativa.
- **Cambios aplicados (por partes)**:
### Altura mínima de ítem para módulos compuestos
    - **Qué se hizo**:
      ```cpp
      case Type::FilesPhotos:
      case Type::All:
      case Type::Photo: … (misma rama que fotos)
      ```
    - **Con qué fin / por qué se cambió**: `FilesPhotos`/`All` usan el layout de malla de fotos; necesitan la misma altura mínima para que la cuadrícula calcule bien las filas.
    - **Lógica aplicada**: `switch` con `case` agrupado (fall-through) junto a `Photo`/`GIF`/`Video`.

---

## Telegram/SourceFiles/info/media/info_media_common.h

- **Funcionalidad**: tipos y contratos comunes de Media (enum `MediaFilter`, `AbstractSection`, delegado, memento).
- **Qué hacía antes** (baseline `eef2d48`): `MediaFilter {All, Photos, Videos}` y solo setter virtual `setMediaFilter` (el getter no existía).
- **Cambios aplicados (por partes)**:
### Nuevo filtro `Files`
    - **Qué se hizo**:
      ```cpp
      enum class MediaFilter { All, Photos, Videos, Files };
      ```
    - **Con qué fin / por qué se cambió**: la vista de archivos permite filtrar por "Todos los archivos" genéricos; el toggle superior (Todo/Fotos/Videos) se complementa en el módulo de Archivos.
    - **Lógica aplicada**: ampliación del enum — no rompe switches existentes porque se añade al final.
### Getter virtual `mediaFilter()`
    - **Qué se hizo**:
      ```cpp
      [[nodiscard]] virtual MediaFilter mediaFilter() const { return MediaFilter::All; }
      ```
      junto al setter existente `virtual void setMediaFilter(MediaFilter filter) {}`.
    - **Con qué fin / por qué se cambió**: `InnerWidget::rebuildMediaFilter` necesita leer el filtro actual del provider para restaurar la sección activa tras una reconstrucción dinámica de las pestañas.
    - **Lógica aplicada**: default `All` permite override en `Provider` (`mediaFilter()` devuelve `_filter`).

---

## Telegram/SourceFiles/info/media/info_media_empty_widget.cpp

- **Funcionalidad**: widget de "no hay contenido" (icono + mensaje) cuando la pestaña media está vacía.
- **Qué hacía antes** (baseline `eef2d48`): sin casos compuestos → icono/mensaje por defecto.
- **Cambios aplicados (por partes)**:
### Icono vacío por tipo compuesto
    - **Qué se hizo** en `setType`:
      ```cpp
      case Type::All: (junto a Photo/PhotoVideo/GIF) → &st::infoEmptyPhoto;
      case Type::FilesPhotos: (junto a File) → &st::infoEmptyFile;
      ```
    - **Con qué fin / por qué se cambió**: visual apropiado (cámara para Todo, papel/archivo para Fotos+Archivos).
    - **Lógica aplicada**: fall-through agrupado en el switch de iconos.
### Mensajes vacíos
    - **Qué se hizo** en `setSearchQuery`:
      ```cpp
      case Type::All: return tr::lng_media_all_empty(tr::now);
      case Type::FilesPhotos:
          return query.isEmpty()
              ? tr::lng_media_photos_files_empty(tr::now)
              : tr::lng_media_file_empty_search(tr::now);
      ```
      (con una indentación irregular en el código añadido — quirk estético del parche).
    - **Con qué fin / por qué se cambió**: mensajes específicos ("No media here yet", "No photos or files here yet") frente a las claves nuevas; en búsqueda se reutiliza el mensaje de archivos.
    - **Lógica aplicada**: dos ramas nuevas; `All` no distingue query (usa un único mensaje).

---

## Telegram/SourceFiles/info/media/info_media_inner_widget.cpp

- **Funcionalidad**: `InnerWidget` — el panel interior de la pestaña Media: filtros superiores, lista de ítems y widget vacío; `setupMediaFilter` monta las pestañas y `MediaFilter` maneja la segmentación.
- **Qué hacía antes** (baseline `eef2d48`): `setupMediaFilter` creaba SIEMPRE tres pestañas fijas (`Todo`, `Fotos`, `Videos`) con `setActiveSectionFast(0)`, y el `sectionActivated` hacía un switch 0/1/2 → `All/Photos/Videos`.
- **Cambios aplicados (por partes)**:

### Filtros dinámicos (se ocultan si no hay contenido)
    - **Qué se hizo**: helper local:
      ```cpp
      [[nodiscard]] std::vector<MediaFilter> FilterSections(int photos, int videos) {
          auto result = {All}; if (photos>0) push(Photos); if (videos>0) push(Videos);
      }
      [[nodiscard]] QString FilterLabel(MediaFilter filter); // All/Photos/Videos/Files
      ```
      En `setupMediaFilter`, en lugar de crear las pestañas fijas, se suscribe un `rpl::combine(...)` de los **conteos reales** del peer:
      ```cpp
      rpl::combine(
          Profile::SharedMediaCountValue(peer, topicRootId, monoforumPeerId, migrated, SharedMediaType::Photo),
          Profile::SharedMediaCountValue(peer, topicRootId, monoforumPeerId, migrated, SharedMediaType::Video))
        | rpl::map(FilterSections) | rpl::distinct_until_changed()
        | rpl::on_next([=](const std::vector<MediaFilter> &sections) { rebuildMediaFilter(sections); },
                       _filterCountsLifetime);
      ```
      `rebuildMediaFilter` compara con `_filterSections`, destina `_filter.destroy()` + recrea, reselecciona el filtro activo actual del `_list` (reiniciando a `All` si el filtro activo ya no existe), enlaza `sectionActivated` → `_list->setMediaFilter(...)`, y ajusta `resizeToWidth(width())` + `refreshHeight()`.
    - **Con qué fin / por qué se cambió**: "Tabs ocultas si vacías" — un chat sin vídeos no debe mostrar la pestaña "Videos" (comportamiento más limpio y evita cambiar a una pestaña vacía). Al trastear en `All`/`FilesPhotos` se descubrió que el conteo de `Photo/Video` permitía decidir dinámicamente.
    - **Lógica aplicada**: reactivo (`rpl`), con `distinct_until_changed` para no reconstruir el slider en cada actualización de conteo; `rebuildMediaFilter` es idempotente (retorna si el vector no cambió); se conserva la selección del usuario cuando el filtro sigue existiendo.

### Toggle de vista Lista / 4 columnas para Archivos
    - **Qué se hizo**: `setupFileViewToggle()` (solo si `type() == Type::File`) crea un segundo slider `_rows` con dos secciones `"Lista"` y `"4"`, activa por defecto `setActiveSectionFast(0)` y captura `sectionActivated`:
      ```cpp
      switch (index) { case 0: return 0; case 1: return 4; }
      _list->setFileGridColumns(columns);
      ```
    - **Con qué fin / por qué se cambió**: "Cuadrícula densa" / vista de archivos en malla: permitir alternar entre la fila clásica de documento (con nombre) y una malla cuadrada de 4 columnas con miniaturas sin nombre.
    - **Lógica aplicada**: segundo `Ui::SettingsSlider` colocado bajo el filtro; el valor se propaga al `ListWidget` (que a su vez lo pasa a cada `ListSection`).
### Layout del widget con el nuevo slider
    - **Qué se hizo**: `resizeGetHeight` y `recountHeight` añaden el bloque de `_rows` (resize a ancho y `moveToLeft(0, top)` + `top += heightNoMargins()`) igual que el de `_filter`.
    - **Con qué fin / por qué se cambió**: que el segundo slider ocupe su fila sin solapar la lista ni el filtro.
    - **Lógica aplicada**: mismos patrones de geometría que `_filter`; los sliders NO participantes se saltan por `if (_filter)` / `if (_rows)`.

---

## Telegram/SourceFiles/info/media/info_media_inner_widget.h

- **Funcionalidad**: cabecera del widget interior (estado del filtro, lista, sliders).
- **Qué hacía antes** (baseline `eef2d48`): solo `_filter`, `_list`, `_empty` y `setupMediaFilter`.
- **Cambios aplicados (por partes)**:
### Campos nuevos
    - **Qué se hizo**: includes nuevos (`info_media_common.h`, `<rpl/lifetime.h>`, `<vector>`), los métodos `void rebuildMediaFilter(const std::vector<MediaFilter> &sections);` y `void setupFileViewToggle();`, y los miembros:
      ```cpp
      object_ptr<Ui::SettingsSlider> _rows = { nullptr };
      std::vector<MediaFilter> _filterSections;
      rpl::lifetime _filterCountsLifetime;
      ```
    - **Con qué fin / por qué se cambió**: dar soporte al filtrado dinámico (guardar el estado para comparar y la lifetime de la suscripción reactiva) y al segundo slider de columnas.
    - **Lógica aplicada**: `object_ptr` para la destrucción descuidada del Qt parent; `rpl::lifetime` para cancelar el `rpl::combine` al destruir el widget.

---

## Telegram/SourceFiles/info/media/info_media_list_section.cpp

- **Funcionalidad**: `ListSection` agrupa los ítems de un bloque (malla de fotos o columna de archivos) y calcula geometría/hit-testing/pintado dentro del `ListWidget`. Incluye `MosaicLayout` para las mallas de fotos.
- **Qué hacía antes** (baseline `eef2d48`): los archivos se renderizaban SIEMPRE en una sola columna (`resizeOneColumn`) con la fila clásica de documento (miniatura + nombre + info + botón). El switch de `resizeToWidth`/`recountHeight` no contemplaba modos de cuadrícula para `File`, ni casos `FilesPhotos`/`All`.
- **Cambios aplicados (por partes)**:

### Modo "tile" de archivos (cuadrícula cuadrada)
    - **Qué se hizo**:
      ```cpp
      void ListSection::setForcedColumns(int value) { _forcedColumns = value; }
      bool ListSection::fileTileMode() const { return (_type == Type::File) && (_forcedColumns > 1); }
      ```
      En `resizeToWidth` la rama `Type::File` ahora comprueba el modo tile: con `_forcedColumns > 1` calcula `_itemsInRow = _forcedColumns`, `_itemWidth` distribuyendo el ancho (`newWidth` ajustado con `st::infoMediaSkip`), centra la cuadrícula (`_itemsLeft` re-calculado) y `_itemHeight = _itemWidth` (celdas cuadradas). En `recountHeight` la rama `File/MusicFile/Link` añade la sub-rama tile: calcula `result += _itemsTop`, posiciona por filas (`item->setPosition(_itemsInRow * result + index)`) y suma `itemHeight = _itemHeight + st::infoMediaSkip` por fila, con manejo de fila final incompleta para `_rowsCount`.
    - **Con qué fin / por qué se cambió**: el toggle "Lista/4" del `InnerWidget` necesita una malla de archivos con celdas uniformes (los tiles pintan un icono de documento centrado).
    - **Lógica aplicada**: la geometría reusa el patrón de la malla de fotos pero con el `_forcedColumns` como constante; cada layout se redimensiona a `_itemWidth` con `item->resizeGetHeight(_itemWidth)`.
### Altura visual uniforme en el modo tile
    - **Qué se hizo**: `visualHeight(item)` devuelve `fileTileMode() ? _itemHeight : item->height()`, y se usa en `findItemAfterTop`, `findItemAfterBottom` y `findItemRect` (los rects devueltos usan `_itemHeight` en tile mode).
    - **Con qué fin / por qué se cambió**: el hit-testing y el scroll necesitan conocer la altura de celda real (uniforme) aunque el layout interno del documento sea más corto/alto; sin esto el scroll "saltaba" filas en la cuadrícula.
    - **Lógica aplicada**: centralización del cálculo en `visualHeight` para no duplicar la condición.
### Pintado de tiles de archivo (`paintFileTile`)
    - **Qué se hizo**: en `paint`, si `fileTileMode()` se recorren los ítems visibles (saltando el arrastrado) y se llama a la nueva función `paintFileTile`, que dibuja:
      - fondo `st::windowBgRipple` con radio `st::roundRadiusLarge`;
      - un rectángulo "improvisado" de documento: lado `qMin(rect.width()*55/100, rect.height()*60/100)`, azul `QColor(0x33,0x90,0xEC)`, redondeado 4px;
      - la esquina plegada (triángulo blanco translúcido con `QPainterPath`);
      - tres "líneas de texto" de documento (blanco 140 de alpha);
      - si el ítem está seleccionado: overlay `QColor(0,0,0,110)` sobre toda la celda y un check circular azul con tilde blanca (`kCheck=22`, línea con `QPen` blanco de 2).
    - **Con qué fin / por qué se cambió**: los archivos sin nombre (`hideName`) en la cuadrícula necesitaban una representación cuadrada consistente (el layout `Document` en tile mode no dibuja nombre ni fila).
    - **Lógica aplicada**: pintado directo con `Painter`/`QPainterPath`; el check es una convención visual propia (no el checkbox estándar) para legibilidad sobre la malla.
### Tipos compuestos como malla de fotos
    - **Qué se hizo**: en `resizeToWidth` y `recountHeight`:
      ```cpp
      case Type::FilesPhotos:
      case Type::All:
      case Type::Photo: … (misma rama de malla de fotos / mosaico)…
      ```
    - **Con qué fin / por qué se cambió**: `FilesPhotos`/`All` deben verse como cuadrícula de miniaturas (no como columnas de archivo); el mosaico reusado les da el aspecto de galería.
    - **Lógica aplicada**: fall-through de casos agrupados que comparten la implementación de `Photo`/`Video`/`PhotoVideo`.
### Quirks de estilo en el código añadido
    - La rama `case Type::File: case Type::Link:` y el `paintFileTile` modifican el layout del switch con funciones añadidas al final; el miembro `_forcedColumns` se inicializa a `0` (modo no-tile).

---

## Telegram/SourceFiles/info/media/info_media_list_section.h

- **Funcionalidad**: declaración de `ListSection` y `ListSectionDelegate`.
- **Qué hacía antes** (baseline `eef2d48`): solo geometría de columna/cols fijos de fotos y `setMinGridSize`.
- **Cambios aplicados (por partes)**:
### Contrato del modo tile
    - **Qué se hizo**:
      ```cpp
      void setForcedColumns(int value);
      [[nodiscard]] bool fileTileMode() const;
      [[nodiscard]] int visualHeight(not_null<const BaseLayout*> item) const;
      void paintFileTile(Painter &p, not_null<BaseLayout*> item, const QRect &rect, const ListContext &context) const;
      …
      int _forcedColumns = 0;
      ```
    - **Con qué fin / por qué se cambió**: `ListWidget::setFileGridColumns` propaga el valor del toggle y el modo tile necesita estas rutinas en la interfaz.
    - **Lógica aplicada**: miembros privados con métodos públicos `setForcedColumns` (el único punto de entrada desde el `ListWidget`).

---

## Telegram/SourceFiles/info/media/info_media_list_widget.cpp

- **Funcionalidad**: `ListWidget` — el widget de scroll de la pestaña Media: mantiene las secciones, la selección (con `_selectedLimit`), el zoom y los eventos de ratón (incluido arrastre de selección).
- **Qué hacía antes** (baseline `eef2d48`): sin modo de columnas para archivos y sin autoscroll de selección (al arrastrar con el ratón cerca del borde la lista no avanzaba).
- **Cambios aplicados (por partes)**:
### Propagación de columnas forzadas
    - **Qué se hizo**: en `resizeGetHeight` se añade `section.setForcedColumns(_fileGridColumns);` en el loop de secciones, y el nuevo método:
      ```cpp
      void ListWidget::setFileGridColumns(int columns) {
          if (_fileGridColumns == columns) return;
          clearSelected();
          _fileGridColumns = columns;
          resizeGetHeight(width());
          update();
      }
      ```
    - **Con qué fin / por qué se cambió**: el toggle Lista/4 del `InnerWidget` cambia la geometría de toda la lista; al cambiar de layout se limpia la selección (evita rects obsoletos) y se re-layout complete.
    - **Lógica aplicada**: flag interno `_fileGridColumns` (0 = lista normal), `resizeGetHeight` idempotente y `update()` para repintar.
### Autoscroll durante la selección por arrastre
    - **Qué se hizo**: en `mouseMoveEvent` se añade `updateAutoScroll();`; se crean dos métodos y el `base::Timer _autoScrollTimer`:
      - `updateAutoScroll()`: si `_mouseAction != Selecting` desactiva; si el puntero está a menos de `kEdge = 40` px del tope visible (`_visibleTop`/`_visibleBottom`) activa la flag y programa `callOnce(15)`;
      - `autoScrollStep()`: calcula `delta` por distancia al borde, `speed = std::clamp(delta/2, 6, 30)` con signo, mueve `_visibleTop` con `std::clamp(…, 0, maxTop)` (donde `maxTop = max(0, height()-visibleH)`), dispara `_scrollToRequests.fire_copy(newTop)`, actualiza `mouseActionUpdate` y se reprograma (`updateAutoScroll`).
    - **Con qué fin / por qué se cambió**: al seleccionar muchos ítems arrastrando, la lista debe desplazarse sola al acercarse a los bordes (selección masiva sin soltar el ratón).
    - **Lógica aplicada**: timer de `base/timer.h` reutilizado del proyecto; la velocidad es proporcional a la profundidad del puntero en el borde con límites 6..30; se desactiva en `mouseActionFinish` (`_autoScrollActive=false; _autoScrollTimer.cancel();`).

---

## Telegram/SourceFiles/info/media/info_media_list_widget.h

- **Funcionalidad**: cabecera del `ListWidget` (estado de selección, secciones, scroll, zoom).
- **Qué hacía antes** (baseline `eef2d48`): sin getter de filtro, sin columnas y sin campos de autoscroll.
- **Cambios aplicados (por partes)**:
### Interfaz ampliada
    - **Qué se hizo**:
      ```cpp
      [[nodiscard]] MediaFilter mediaFilter() const { return _provider->mediaFilter(); }
      void setMediaFilter(MediaFilter filter);
      void setFileGridColumns(int columns);
      …
      int _fileGridColumns = 0;
      base::Timer _autoScrollTimer;
      bool _autoScrollActive = false;
      // y las declaraciones privadas updateAutoScroll(); autoScrollStep();
      ```
    - **Con qué fin / por qué se cambió**: `InnerWidget::rebuildMediaFilter` leía `_list->mediaFilter()`; el toggle de columnas necesita el setter; el autoscroll necesita su timer/flag.
    - **Lógica aplicada**: getter delegando al provider; estado mínimo (`int` + `bool`).

---

## Telegram/SourceFiles/info/media/info_media_provider.cpp

- **Funcionalidad**: `Provider` — fábrica de layout y fuente de datos de la lista de media; decide qué ítems de la slice materializar, en qué orden, y construye `BaseLayout` (Photo/Video/Document) para cada uno.
- **Qué hacía antes** (baseline `eef2d48`): arrancaba con `_idsLimit = kMinimalIdsLimit` (16), en `checkPreload` encogía a `preloadIdsLimit` (~128), y en `fillSections` saltaba los ítems sin layout (`if (auto layout = getLayout(...)) { … }`); no había materialización ni rutas de tipos compuestos.
- **Cambios aplicados (por partes)**:

### Ventana grande de entrada para FilesPhotos/All
    - **Qué se hizo** en el constructor y en `restart()`:
      ```cpp
      if (_type == Type::FilesPhotos || _type == Type::All) { _idsLimit = kFullIdsLimit; }
      ```
      con la constante en el `.h` (`kFullIdsLimit = 1000`; ventana ≈ 2000 ids alrededor del `around`).
    - **Con qué fin / por qué se cambió**: arrancar en 16 ids truncaba la galería compuesta: `SparseIdsSliceBuilder` poda la slice a `limitBefore+limitAfter` alrededor del `around`, así que el `front` se recortaba y la petición "Before" no avanzaba hacia el inicio real (en el log se veía `skipBefore≈13000` sin bajar, dejando archivos antiguos sin mostrar). Con la ventana grande el `front` avanza mucho más por página y la galería se llena.
    - **Lógica aplicada**: misma constante usada también en `checkPreload` y `jumpToMessage` — no se toca el camino de los tipos clásicos.
### `checkPreload` mantiene la ventana grande al hacer scroll
    - **Qué se hizo**:
      ```cpp
      _idsLimit = (_type == Type::FilesPhotos || _type == Type::All) ? kFullIdsLimit : preloadIdsLimit;
      ```
      y una traza `[DEPUR] PRELOAD … top/bottom`.
    - **Con qué fin / por qué se cambió**: si al scrollear la ventana se encogía a ~128, al cruzar una zona de ids aún sin cachear la cuadrícula se quedaba sin contenido ("congelada") y solo se rellenaba al saltar a un mensaje (que sí usa `kFullIdsLimit`). Mantener la ventana grande hace que el sparse-builder escuche y cachee muy por delante, como en el arranque.
    - **Lógica aplicada**: el `refreshViewer` resultante pide el doble de alrededor (`limitBefore/limitAfter = _idsLimit`).
### `refreshViewer` — slice por tipo, presupuesto de materialización y trazas
    - **Qué se hizo**:
      - `_controller->mediaSourceOfType(_controller->section().mediaType(), idForViewer, _idsLimit, _idsLimit)` (factorización de `info_controller.cpp`);
      - al recibir la nueva slice: si `_type` es compuesto, se resetea el presupuesto de materialización (`_missingRequested = false; _missingRetries = 0;`) antes de concatenar la traza `[DEPUR] REFRESH ...`;
      - y se llama a `requestMissingAround(_universalAroundId)` para materializar proactivamente TODOS los ids de la nueva región.
    - **Con qué fin / por qué se cambió**: cada nueva región de scroll debe tener un presupuesto fresco (sin este reset, en canales con mucha media el contador alcanzaba su tope y toda zona nueva quedaba con huecos permanentes hasta abrir el chat).
    - **Lógica aplicada**: la slice `SparseIdsMergedSlice` se actualiza con el cierre de la actualización de la vista; el reset del presupuesto se hace justo después del `nearest()` para no perder la posición.
### Materialización proactiva `requestMissingAround`
    - **Qué se hizo**: función nueva que:
      1. Recorre la slice al revés y recoge en `std::vector<MissingId>` todo id cuyo `HistoryItem` no exista en memoria (`session->data().message(full)` vacío), guardando `peerId+msgId` por cada uno (`computeFullId`).
      2. Si no hay missing, retorna; si `_missingRetries >= kMaxMissingRetries` (8), retorna también (ids que nunca se materializan: borrados).
      3. Marca `_missingRequested = true` y crea un `MaterializeState` (`shared_ptr`) con `missing`, `next`, `inFlight` y un `alive = make_shared<bool>(true)` que `_lifetime.add` apaga si el provider muere con peticiones en vuelo.
      4. `startMore` lanza hasta `kMaxMaterializeParallel = 3` lotes en vuelo, cada uno con `kMaxMaterializeBatch = 100` ids via `MTPchannels_GetMessages`/`MTPmessages_GetMessages`; cada lote se resuelve exactamente una vez (`done`/`fail`) llamando `batchSettled` (decrementa `inFlight` y dispara `startMore`).
      5. Cuando `inFlight == 0`: suelta `_missingRequested`, cuenta cuántos se materializaron (`processExistingMessages`), ajusta `_missingRetries` (`+1` si quedan huecos, `0` si no), loguea `[DEPUR] MATER done … retry=…`, rompe el ciclo (`state->missing.clear(); state->startMore = nullptr;`) y dispara `_refreshed.fire({})` para que `fillSections` re-renderice.
    - **Con qué fin / por qué se cambió**: los tipos compuestos pueden contener ids cuya `HistoryItem` no está materializada (canales grandes, o historial descargado en otro sitio). Antes esos ítems simplemente no aparecían hasta usar "Mostrar en el chat" (que re-materializa el historial). Ahora la ventana grande se rellena entera de una pasada.
    - **Lógica aplicada**: *cola con paralelismo acotado* (3 en vuelo) en lugar de pedir los lotes uno a uno (en una zona de 1000 ids eran ~10 pasadas de red seguidas y la cuadrícula parecía congelada); *anti-bucle* con `_missingRetries` y *guard* `_missingRequested` (una pasada en vuelo a la vez); seguridad de vida con `shared_ptr<bool> alive`.
### `fillSections` recoge los ids no materializados
    - **Qué se hizo**: el bucle de ítems:
      ```cpp
      auto layout = getLayout(universalId, delegate);
      if (layout && !matchesFilter(layout->getItem())) continue;
      if (!layout && (FilesPhotos || All)) { ++missingCount; requestMissingAround(universalId); continue; }
      if (!layout) continue;
      …
      ```
      con traza `[DEPUR] FILL type=… slice=… added=… missing=…` si hubo huecos o ítems reales.
    - **Con qué fin / por qué se cambió**: materializar bajo demanda durante el render (los ids que faltan se detectan al construir las secciones).
    - **Lógica aplicada**: `requestMissingAround` ignora llamadas repetidas dentro de la pasada gracias al guard `_missingRequested`.
### `createLayout` para los tipos compuestos
    - **Qué se hizo**: dos casos nuevos en el switch:
      ```cpp
      case Type::FilesPhotos:
          if (photo) return make_unique<Photo>(delegate, item, photo, options());
          if (file)  return make_unique<Document>(delegate, item, DocumentFields{ .document = file, .hideName = true }, songSt);
          return nullptr;
      case Type::All:
          if (photo) return make_unique<Photo>(...);
          if (file) {
              if (file->isVideoFile()) return make_unique<Video>(delegate, item, file, options());
              return make_unique<Document>(delegate, item, DocumentFields{ .document = file, .hideName = true }, songSt);
          }
          return nullptr;
      ```
    - **Con qué fin / por qué se cambió**: el módulo Fotos+Archivos muestra fotos y archivos (Documento ocultando el nombre, en celdas cuadradas), y Todo muestra fotos, vídeos y archivos con la misma regla `hideName`.
    - **Lógica aplicada**: se reutiliza `getPhoto()`/`getFile()` para decidir; los archivos usan `songSt` como estilo de overview pero con `hideName` (el pintado lo tratará como tile).
### `jumpToMessage` y setter de filtro
    - **Qué se hizo**: en `jumpToMessage`, `_idsLimit = (_type compuesto) ? kFullIdsLimit : kMinimalIdsLimit * 2;`. En `setMediaFilter` se elimina el early-return (`if (_filter == filter) return;`).
    - **Con qué fin / por qué se cambió**: al saltar a un mensaje desde el buscador, el módulo compuesto debe resolver la nueva región con la ventana grande (no la mínima). El setter sin early-return hace que cada cambio re-dispara `_refreshed` (más agresivo para re-render alto).
    - **Lógica aplicada**: se mantiene `_filter = filter;` y `_refreshed.fire({})` incondicional.
### Otras trazas / campos menores
    - `[DEPUR] PROVIDER ENTER` (constructor) y `[DEPUR] RESTART` documentan entrada y reinicio. `sectionHasFloatingHeader`, `sectionTitle` y `sectionItemBelongsHere` ganan los casos `FilesPhotos`/`All` agrupados con `Photo` (cabecera flotante y título de día igual que la galería de fotos).

---

## Telegram/SourceFiles/info/media/info_media_provider.h

- **Funcionalidad**: declaración del `Provider` (constantes de límites, estado de la slice, caché de layouts).
- **Qué hacía antes** (baseline `eef2d48`): solo `kMinimalIdsLimit = 16`, `kDefaultAroundId`, `_idsLimit` y el estado de layouts; sin getter de filtro ni materialización.
- **Cambios aplicados (por partes)**:
### Constante de ventana grande y estado de auto-reparado
    - **Qué se hizo**:
      ```cpp
      static constexpr auto kFullIdsLimit = 1000;   // ventana ≈ 2000 ids
      [[nodiscard]] MediaFilter mediaFilter() const override { return _filter; }
      void requestMissingAround(UniversalMsgId universalId);
      bool _missingRequested = false;
      int _missingRetries = 0;
      static constexpr int kMaxMissingRetries = 8;
      ```
      con comentarios explicando el porqué (poda de la slice en la unión; `SparseIdsSliceBuilder` cortaba el front; `kMaxMissingRetries` evita el bucle con ids borrados).
    - **Con qué fin / por qué se cambió**: dar el contrato público al `InnerWidget` (getter de filtro) y guardar el estado de una pasada de materialización a la vez.
    - **Lógica aplicada**: constantes `constexpr` en la clase; el reset del presupuesto va en `refreshViewer` (cada región nueva).

---

## Telegram/SourceFiles/info/media/info_media_widget.cpp

- **Funcionalidad**: `Section` de Media a nivel de widget; `SharedMediaTitle(type)` da el título de la pestaña abierta.
- **Qué hacía antes** (baseline `eef2d48`): sin títulos para tipos compuestos → `Unexpected` al abrirlos.
- **Cambios aplicados (por partes)**:
### Títulos de sección para los módulos compuestos
    - **Qué se hizo**:
      ```cpp
      case Type::FilesPhotos: return tr::lng_media_type_photos_files;
      case Type::All: return tr::lng_media_type_all;
      ```
    - **Con qué fin / por qué se cambió**: la barra superior de la pestaña debe mostrar "Photos & Files" y "All" (crash evitado en el switch exhaustivo).
    - **Lógica aplicada**: claves i18n nuevas del `lang.strings`.

---

## Telegram/SourceFiles/info/profile/info_profile_shared_media_classic.cpp

- **Funcionalidad**: monta los botones de "media compartida" del perfil clásico del canal (`SetupSharedMediaClassic` + `addMediaButton`).
- **Qué hacía antes** (baseline `eef2d48`): el canal clásico mostraba `PhotoVideo`, `File`, `MusicFile`, `Link`, `Poll` (sin los módulos compuestos).
- **Cambios aplicados (por partes)**:
### Botones de acceso a FilesPhotos y All en el perfil clásico
    - **Qué se hizo**:
      ```cpp
      addMediaButton(MediaType::PhotoVideo, st::infoIconMediaPhoto);
      addMediaButton(MediaType::File, st::infoIconMediaFile);
      addMediaButton(MediaType::FilesPhotos, st::infoIconMediaPhoto);
      addMediaButton(MediaType::All, st::infoIconMediaPhoto);
      addMediaButton(MediaType::MusicFile, st::infoIconMediaAudio);
      …
      ```
      (ocultando `if (!Has(…))` remained; el orden de botones respeta la secuencia del perfil).
    - **Con qué fin / por qué se cambió**: exponer en el canal clásico los dos módulos nuevos (el layout moderno ya podía accederse por otra vía; el clásico no tenía botón).
    - **Lógica aplicada**: añade los botones reutilizando los iconos existentes (`infoIconMediaPhoto`); el resto del setup (contadores vía `SharedMediaCountValue`, navegación a la sección) queda intacto.

---

## Telegram/SourceFiles/info/profile/info_profile_values.cpp

- **Funcionalidad**: valores reactivos del perfil; `SharedMediaCountValue(...)` produce el contador de cada pestaña media del perfil.
- **Qué hacía antes** (baseline `eef2d48`): para cualquier tipo construía un viewer genérico `SharedMediaMergedViewer` y contaba.
- **Cambios aplicados (por partes)**:
### Contadores compuestos (suma de sub-tipos)
    - **Qué se hizo** al inicio de `SharedMediaCountValue`:
      ```cpp
      using Type = Storage::SharedMediaType;
      if (type == Type::FilesPhotos) {
          return rpl::combine(
              SharedMediaCountValue(peer, topicRootId, monoforumPeerId, migrated, Type::Photo),
              SharedMediaCountValue(peer, topicRootId, monoforumPeerId, migrated, Type::File))
              | rpl::map([](int photos, int files) { return photos + files; });
      }
      if (type == Type::All) {
          return rpl::combine(
              …Type::Photo, …Type::Video, …Type::File)
              | rpl::map([](int photos, int videos, int files) { return photos + videos + files; });
      }
      ```
    - **Con qué fin / por qué se cambió**: el contador del botón de perfil de `FilesPhotos` debe ser fotos+archivos, y el de `All`, fotos+vídeos+archivos. No existe un conteo MTP nativo de estas uniones; la suma reactiva de los tres subconteos es la única vía.
    - **Lógica aplicada**: `rpl::combine` re-emite cuando cualquiera de los subconteos cambia; error-free recursion (cada sub-call vuelve a entrar y usa la ruta clásica, sin bucles porque los tipos compuestos no re-entran en sí mismos).

---

## Telegram/SourceFiles/overview/overview_layout.cpp

- **Funcionalidad**: sidecar de layout de los ítems en el chat/overview; `Document` dibuja el documento (miniatura, nombre, tamaño, botón de descarga) en la fila clásica, y se reutiliza en la galería de compartidos.
- **Qué hacía antes** (baseline `eef2d48`): `Document` siempre mostraba la fila con nombre; `initDimensions` calculaba `_minh` según `songLayout()` o la fila de archivo; no había `hideName` ni `resizeGetHeight` override.
- **Cambios aplicados (por partes)**:
### Campo `hideName` en `Document`
    - **Qué se hizo**: en `DocumentFields` (vía `.h`) nuevo `bool hideName = false;`, miembro `_hideName`, inicializado en el constructor. En `initDimensions`:
      ```cpp
      if (_hideName) { _minh = _st.fileThumbSize; } else if (songLayout()) { … } else { … }
      ```
      y nuevo override:
      ```cpp
      int32 Document::resizeGetHeight(int32 width) {
          if (_hideName) { _width = qMin(width, _maxw); _minh = _height = _width; return _height; }
          return AbstractLayoutItem::resizeGetHeight(width);
      }
      ```
    - **Con qué fin / por qué se cambió**: en la cuadrícula de archivos cada celda debe ser cuadrada y de ancho uniforme (el tile se centra); con `hideName` no hay nombre y `resizeGetHeight` fuerza `_height == _width`.
    - **Lógica aplicada**: `qMin(width, _maxw)` mantiene el límite de ancho; `_minh` también se iguala para que las alturas reportadas por `Document` sean consistentes con la malla.
### Rama de pintado de tile (`paint` con `_hideName`)
    - **Qué se hizo**: nueva rama `else if (_hideName)` antes de la fila clásica que:
      1. Calcula `thumbSize = qMin(_width, _st.fileThumbSize)` centrado (`thumbX=(_width-thumbSize)/2`, `thumbY=(_minh-thumbSize)/2`);
      2. dibuja la miniatura (`paintThumbnail`), y si está seleccionado un rectángulo con `st::defaultTextPalette.selectOverlay`;
      3. si `radial || (!loaded && !activeLoading())` dibuja el círculo de progreso/descarga centrado (con `anim` de hover, `st::historyFileThumbDownload/Cancel`, y el `_radial` de progreso);
      4. `const auto checkDelta = _st.fileThumbSize - st::overviewSmallCheck.size; paintCheckbox(p, {checkDelta, checkDelta}, selected, context);` y `return;` (no pinta la fila de nombre).
    - **Con qué fin / por qué se cambió**: los tiles de archivo de la cuadrícula necesitan la miniatura centrada con su botón de descarga/progreso dentro de la celda cuadrada, sin nombre, y con el checkbox de selección en la esquina.
    - **Lógica aplicada**: todo el código reutiliza helpers ya existentes (`paintThumbnail`, `_radial`, `anim::brush`, `paintCheckbox`, los iconos `historyFileThumb*`); únicamente cambia el layout al que se aplican.

---

## Telegram/SourceFiles/overview/overview_layout.h

- **Funcionalidad**: declaraciones de layouts de overview; `DocumentFields` y `Document`.
- **Qué hacía antes** (baseline `eef2d48`): sin `hideName`.
- **Cambios aplicados (por partes)**:
### Contrato `hideName`
    - **Qué se hizo**:
      ```cpp
      bool hideName = false;                       // en DocumentFields
      int32 resizeGetHeight(int32 width) override; // público en Document
      bool _hideName = false;                      // miembro privado
      ```
    - **Con qué fin / por qué se cambió**: el `Provider` de la galería rellena `DocumentFields{ .document = file, .hideName = true }`; la clase necesita el campo y el override para comportarse como celda cuadrada.
    - **Lógica aplicada**: coincide con la implementación del `.cpp`; el default `false` mantiene intacto el camino clásico.

---

## Telegram/SourceFiles/storage/storage_shared_media.h

- **Funcionalidad**: define `Storage::SharedMediaType`, el enum de tipos de media compartida (usado por indexado, búsqueda, máscaras, y los módulos del perfil).
- **Qué hacía antes** (baseline `eef2d48`): terminaba en `Poll, kCount`.
- **Cambios aplicados (por partes)**:
### Nuevos valores de tipo
    - **Qué se hizo**:
      ```cpp
      enum class SharedMediaType : signed char {
          …
          Poll,
          FilesPhotos,
          All,
          kCount,
      };
      ```
    - **Con qué fin / por qué se cambió**: el sistema entero (máscaras, búsqueda, provider, botones) necesita poder etiquetar las dos vistas compuestas como tipos de primera clase.
    - **Lógica aplicada**: se insertan antes de `kCount` (el "contador" del enum queda intacto); el tipo firmado ya existía (`signed char`). Este es el ancla semántica de todo el resto del diff: sin estos valores, ninguna de las ramas `case Type::FilesPhotos/All` compilaría.