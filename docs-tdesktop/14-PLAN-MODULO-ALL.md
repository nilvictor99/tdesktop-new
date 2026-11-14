# Plan de implementación — Módulo "Todo" (All): una sola cuadrícula con todo

> **ESTADO: ✅ IMPLEMENTADO (a la espera de compilación y verificación por el
> usuario).** Este documento es el registro del diseño; los cambios ya aplicados
> se detallan en [11-DIARIO-RONDAS.md](11-DIARIO-RONDAS.md), ronda 9.

**Repositorio:** `C:\Users\VICTOR\code\tdesktop` · Rama de trabajo actual:
perfil de canales/grupos con los módulos Media, Fotos+Archivos (FilesPhotos), etc.

---

## 1. Objetivo

Añadir al perfil de canales y grupos (layout classic y, si aplica, tabs) un
nuevo módulo **"Todo"** (`All`) que muestre en una sola cuadrícula tipo Media:

- **Fotos** (MediaPhoto)
- **Videos** (documentos `isVideoFile`)
- **Archivos** (documentos no especiales, `fileDoc`)

**Excluidos:** stickers, GIF animados, notas de voz, videomensajes (round),
música, enlaces/webpages, fotos de perfil y vistas previas.

Comportamiento esperado (igual que Media/FilesPhotos):
- Carga completa desde servidor, paginada, orden cronológico.
- Agrupación por mes (`sectionTitle`). Sin fecha → header por mes.
- Selección múltiple, reenvío por lotes y borrado masivo ya soportados por el
  framework (se reutilizan tal cual).
- Botón en el perfil con contador propio (fotos + videos + archivos).

**No romper:** Media (PhotoVideo), FilesPhotos, Archivos (File), ni la vista
de archivos. Cada tipo mantiene su `SparseIdsList` independiente.

---

## 2. Hallazgos clave de la investigación (verificados)

1. **`Info::Media::Type` ≡ `Storage::SharedMediaType`** (alias; `info_media_common.h:22`).
   Un enum compartido alimenta botones, títulos, provider, API y storage.
   Añadir `All` al enum basta para que exista una **nueva lista de storage
   independiente** por (peer, topicRootId, monoforumPeerId):
   `storage_shared_media.h:281` (`std::array<SparseIdsList, kSharedMediaTypeCount>`),
   indexada por `static_cast<int>(type)`. `kSharedMediaTypeCount` deriva de `kCount`.

2. **La máscara `base::enum_mask` aguanta 32 bits**; con `FilesPhotos=13` y `All=14`
   salen 15 valores → sin overflow.

3. **FilesPhotos es el patrón a copiar.** Inventado en esta rama ya:
   - Botón en el perfil: `info_profile_shared_media_classic.cpp:350`.
   - API de doble búsqueda paralela: `apiwrap.cpp:3686` (`requestSharedMediaFilesPhotos`).
   - Títulos, contador combinado, layout híbrido foto/documento.

4. **Diferencias clave entre FilesPhotos y All:**
   - FilesPhotos = `Photo` + `File` → busca con `inputMessagesFilterPhotos` +
     `inputMessagesFilterDocument` (2 sub-búsquedas), y **excluye videos**.
   - All = `Photo` + `Video` + `File` → necesita 3 sub-búsquedas:
     `inputMessagesFilterPhotos` + `inputMessagesFilterVideo` +
     `inputMessagesFilterDocument`. (No existe un único filtro MTP para
     foto+video+documento; `PrepareSearchFilter` devuelve filtro vacío y
     `PrepareSearchRequest` devuelve `nullopt` para un tipo sin mapeo.)
   - **Los stickers nunca llegan a las listas** porque `MediaFile::sharedMediaTypes()`
     devuelve máscara vacía (`data_media_types.cpp:1120`). Con `IsItemGoodForType`
     añadiremos la exclusión de `document->sticker()` para coherencia en el
     visor programado.

5. **`IsItemGoodForType` real** (no usa partición bitmask como se asumía):
   comparaciones por igualdad `type == Type::X`
   (`data_shared_media.cpp:27-71`). Para All habrá que añadir ramas explícitas
   para foto (`type == Type::All && photo`), video (`type == All && videoDoc`)
   y archivo (`type == All && (fileDoc)`).

6. **La API `messages.search` valida con `ParseSearchResult`** que
   `item->sharedMediaTypes().test(type)` (`data_search_controller.cpp:264`).
   Los 3 sub-resultados deben parsearse con **su propio sub-tipo** dentro de la
   búsqueda (Photo, Video, File) para que superen esa validación. La etiqueta
   final del slice es `All` (con `SharedMediaAddSlice` genérico,
   `apiwrap.cpp:3834`).

7. **`sharedMediaDone`** es genérico en `type`. Solo hay que añadir la rama de
   dispatch en `requestSharedMedia` (como ya se hizo con FilesPhotos en
   `apiwrap.cpp:3635`).

8. **Merge de 3 sub-lotes en `requestSharedMediaAll`:** acumular `fullCount`,
   concatenar `messageIds`, y fijar `noSkipRange` como se hace en FilesPhotos
   (lote con ítems → `{min(ids), ServerMaxMsgId}`; lote vacío → propagar
   rango de "fin de dirección"). Esto mantiene el comportamiento lazy de una
   única slice creciente (exploración hacia atrás).

9. **Contador del botón All:** `SharedMediaCountValue` ya tiene rama
   FilesPhotos combinando Photo+File (`info_profile_values.cpp:605-612`).
   All = Photo+Video+File con 3 ramas.

10. **El controlador/media source es genérico:** `mediaSourceOfType`
    (`info_controller.cpp:220`) pasa `type` a `SharedMediaMergedViewer`.
    Sin cambios una vez enum+storage+API soporten All.
    `SharedMediaAllowSearch(All)` → `false` (sin búsqueda), igual FilesPhotos.
    `SharedMediaOverviewType(All)` → `std::nullopt`.

---

## 3. Archivos y ediciones (orden de dependencia)

### A. Enum y máscaras de indexación (base)

1. **`storage/storage_shared_media.h`** — insertar `All,` entre `FilesPhotos`
   (línea 31) y `kCount` (línea 33). Nada más en storage: listas, `add`/`remove`,
   `query`, `snapshot` se adaptan solos (bucles por índice).

2. **`data/data_media_types.cpp:892-895`** — `MediaPhoto::sharedMediaTypes()`:
   añadir `.added(Type::All)` en la rama no-chat (fotos).

3. **`data/data_media_types.cpp:1128-1141`** — `MediaFile::sharedMediaTypes()`:
   - Rama `isVideoFile()` (1128-1131): añadir `.added(Type::All)`. *(videos)*
   - Rama `else` final (1139-1141): añadir `.added(Type::All)`. *(archivos)*
   - **NO** tocar sticker/ttl (1120), GIF (1126), voz (1132), música (1136),
     round/video-message (1122). Así se excluyen stickers, GIF, voz, música y round.

### B. API — búsqueda triple merge

4. **`apiwrap.h`** — declarar `requestSharedMediaAll(peer, topicRootId,
   monoforumPeerId, messageId, slice)` junto a `requestSharedMediaFilesPhotos`.

5. **`apiwrap.cpp:3635-3643`** — en `requestSharedMedia`, añadir:
   ```
   if (type == SharedMediaType::All) {
       requestSharedMediaAll(...); return;
   }
   ```

6. **`apiwrap.cpp`** — nueva `requestSharedMediaAll`, copia del patrón FilesPhotos
   (3686-3814) pero con 3 sub-búsquedas:
   - `struct State { optional<SearchResult> photo, video, file; }`
   - `doneCount` espera a 3.
   - `sendOne(SharedMediaType::Photo); sendOne(Video); sendOne(File);`
   - Merge: `fullCount` sumado, `messageIds` concatenados (foto+video+archivo),
     `noSkipRange` = como FilesPhotos.
   - Intervalo `SharedMediaType::All` en `sharedMediaDone`.

   *(Nota: el orden de ids por lote no es estrictamente cronológico entre tipos
   — igual que FilesPhotos — pero el orden global (por msgid) sí lo es para la
   exploración. Acceptable; ver sección 5.)*

### C. Controlador / media source

7. **`info/info_controller.cpp`** — `mediaSourceOfType` ya es genérico; sin
   cambios. (Los switches restantes sobre `SharedMediaType` en este archivo
   tienen `default` o son por tipo con `Unexpected` inexistente.)

### D. Provider — grid, títulos, layout

8. **`info/media/info_media_provider.cpp`** — añadir `case All` en:
   - `sectionHasFloatingHeader()` (136-141): junto a FilesPhotos → `false`.
   - `sectionTitle()` (152-158): mismo grupo que FilesPhotos → `langMonthFull`.
   - `sectionItemBelongsHere()` (178-184): agrupación por mes.
   - `createLayout()` (561-577): **híbrido** foto→`Photo`, video→`Video`,
     archivo→`Document(hideName)` (rama de FilesPhotos para docs, con
     `Layout::Video` para videos).
   - `supportsMediaFilter()` (297-299): `false` para All (sin chips).

9. **`info/media/info_media_common.cpp:66`** — `MinItemHeight`: `case All`
   junto a PhotoVideo (grid, altura de galería).

10. **`info/media/info_media_list_section.cpp`** — `resizeToWidth` (~450) y
    `recountHeight` (~507): `case All` igual que PhotoVideo (grid/tile).

11. **`info/media/info_media_grid_zoom.cpp`** — `isZoomable` (All → true) y
    `minGridSizeForStep` (All → galería), si se quiere zoom de cuadrícula.

12. **`info/media/info_media_empty_widget.cpp`** — `setType` (icono) y
    `setSearchQuery` (texto): `case All` → `st::infoEmptyPhoto` + nueva key
    `lng_media_all_empty`.

### E. Títulos y botón del perfil

13. **`info/media/info_media_widget.cpp:60`** — `SharedMediaTitle`: `case All`
    → `tr::lng_media_type_all`.

14. **`info/info_wrap_widget.cpp:94`** — `SelectedTitleForMedia`: `case All`
    → `tr::lng_media_selected_media` (como FilesPhotos).

15. **`info/media/info_media_buttons.cpp:94`** — `MediaTextPhrase`: `case All`
    → `tr::lng_profile_all`.

16. **`info/profile/info_profile_shared_media_classic.cpp:348-355`** — añadir
    `addMediaButton(MediaType::All, st::infoIconMediaPhoto);` (botón del perfil).
    Icono reutilizado de Media (sin SVG nuevo) — o `st::infoIconMediaFile` si se
    prefiere distinción.

17. **`info/profile/info_profile_values.cpp:605-612`** — `SharedMediaCountValue`:
    rama `type == Type::All` → `rpl::combine` de Photo+Video+File (suma).

### F. Modo tabs (si está activo en esta rama)

18. **`info/profile/tabs/adapters/info_profile_tab_media.cpp`** — `MediaTabGrid(All)`
    → true; `MediaTabSearchable(All)` → false; `MediaTabTitle(All)` →
    `lng_media_type_all`.
19. **`info/profile/tabs/info_profile_tab_skeleton.cpp`** (~98) — `case All` →
    skeleton grid.

### G. Visores / overview (coherencia, no bloquea)

20. **`data/data_shared_media.cpp:27-71`** — `IsItemGoodForType`: añadir a la
    rama de foto `(type == Type::All || ...)`; en la rama de documentos:
    `((videoType || ... || type == Type::All) && videoDoc)` y
    `((fileType || filesPhotosType || type == Type::All) && ...)`, excluyendo
    `document->sticker()` para All. (`SharedMediaOverviewType` y
    `SharedMediaAllowSearch` ya degradan seguros con `nullopt`/`false`.)

21. **`data/data_search_controller.cpp:29-62`** — NO añadir `All` a
    `PrepareSearchFilter` (no debe buscarse directo); el filtro vacío de fallback
    ya es seguro. Opcional: `case All: return MTP_inputMessagesFilterEmpty()`.

### H. Traducciones — `Telegram/Resources/langs/lang.strings` (~zona 1895-1898)

Añadir:
```
"lng_media_type_all"          = "All";
"lng_profile_all#one"         = "{count} media item";
"lng_profile_all#other"       = "{count} media items";
"lng_media_all_empty"         = "No media here yet";
```
*(Ajustar #one/#other según plurals de cada idioma; en `lang.strings` de
Telegram los plurales se declaran en el mismo fichero con `#one`/`#other`.)*

---

## 4. Pruébalo tú mismo / verificación

Tras implementar y compilar con
`msbuild Telegram.sln /p:Configuration=Release /p:Platform=x64 /m /t:Telegram`
(desde `out\`):

1. Abrir un canal/grupo con contenido variado (fotos, videos, archivos, stray
   stickers) → Perfil → botón **Todo**.
2. Verificar:
   - La cuadrícula carga TODAS las páginas (scroll hacia arriba).
   - Se ven fotos + videos + archivos; **no** stickers/GIF/voz/links.
   - Orden cronológico con headers de mes.
   - Contador del botón = fotos+videos+archivos reales.
   - Selección múltiple → reenviar/borrar funciona como en Media.
   - Media, FilesPhotos, Archivos (File) siguen funcionando igual.
3. Reintentar con cuenta con temas/foros si aplica.
4. Revisar `out/Release/log.txt` (líneas `[FILESPHOTOS]` siguen; añadir
   `[ALL]` de diagnóstico si hace falta).

---

## 5. Decisiones adoptadas (resueltas al implementar)

- **Orden del merge por lote:** igual que FilesPhotos (`{min, ServerMaxMsgId}`
  con lote no vacío, propagación del rango de fin cuando va vacío).
- **16 - Icono del botón:** `st::infoIconMediaPhoto` (reutilizado, sin SVG nuevo).
- **11 - Zoom de cuadrícula:** NO (grid simple, como FilesPhotos).
- **F - Tabs:** NO añadido; los tabs experimentales no incluyen FilesPhotos
  (precedente); `All` sigue solo el layout classic.
- **Búsqueda:** desactivada (`SharedMediaAllowSearch=false`), como FilesPhotos.
- **Global media:** no afectado (`SharedMediaOverviewType(All)=nullopt`).

---

## 6. Resumen de archivos a tocar

| # | Archivo | Qué/e.d. |
|---|---------|----------|
| 1 | `storage/storage_shared_media.h` | enum `All` |
| 2 | `data/data_media_types.cpp` | máscaras foto/video/archivo + `All` |
| 3 | `apiwrap.h` | declarar `requestSharedMediaAll` |
| 4 | `apiwrap.cpp` | dispatch + `requestSharedMediaAll` (3 sub-búsquedas) |
| 5 | `info/media/info_media_provider.cpp` | 4 switches + filter |
| 6 | `info/media/info_media_common.cpp` | `MinItemHeight(All)` |
| 7 | `info/media/info_media_list_section.cpp` | grid/tile |
| 8 | `info/media/info_media_grid_zoom.cpp` | zoom (opcional) |
| 9 | `info/media/info_media_empty_widget.cpp` | empty states |
| 10 | `info/media/info_media_widget.cpp` | `SharedMediaTitle` |
| 11 | `info/info_wrap_widget.cpp` | `SelectedTitleForMedia` |
| 12 | `info/media/info_media_buttons.cpp` | `MediaTextPhrase` |
| 13 | `info/profile/info_profile_shared_media_classic.cpp` | botón perfil |
| 14 | `info/profile/info_profile_values.cpp` | contador combinado |
| 15 | `info/profile/tabs/*` | tabs (si activo) |
| 16 | `data/data_shared_media.cpp` | `IsItemGoodForType` |
| 17 | `Telegram/Resources/langs/lang.strings` | 4 keys nuevas |

Ningún cambio en build/CMake/scripts. El agente NO compila (el usuario sí).