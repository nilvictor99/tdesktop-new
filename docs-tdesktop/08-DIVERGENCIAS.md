# 08 – Divergencias reales frente al cliente oficial

Informe del **diff real entre el fork y el Telegram Desktop oficial**, con la
base oficial identificada y verificada. Complementa a `07-ESTADO-ACTUAL.md`.

Fecha del análisis: 10 de septiembre de 2026.

---

## 1. Resumen ejecutivo

- La **base oficial del fork es la tag `v7.0.9`** de
  `telegramdesktop/tdesktop` (commit `a1e89e1f64`, "Version 7.0.9.",
  2026-08-06). Se identificó por comparación de blobs: version, changelog,
  README, CMakeLists, main.cpp, storage_shared_media.h y .gitmodules son
  idénticos entre `eef2d48` (primer commit del fork) y `v7.0.9`.
- **Divergencia total del fork (HEAD `41286e02`) frente a `v7.0.9`:**
  **60 archivos** en el diff, de los cuales **41 tienen contenido real
  (+2203 −220)** y **19 son solo cambios de modo/atributos** (scripts con el
  bit de ejecución perdido al vivir en `/mnt/c`).
- Los 41 archivos con contenido se reparten en código de la app
  (`Telegram/SourceFiles` + `Resources/langs`), `10-INVENTARIO-ANTERIOR.md`, y un Dockerfile.
  **No se tocó la versión**: `Telegram/build/version` y `changelog.txt`
  siguen siendo los de `v7.0.9`.
- Los **dependencias (submodulos, ~40 directorios `lib_*`/`ThirdParty`/`cmake`)**
  se importaron en el fork como **árboles completos dentro del commit**
  (modo `040000 tree`) en lugar de gitlinks (modo `160000 commit`), así que el
  diff se computa **excluyéndolos** (y su contenido no se ha verificado contra
  los repos de dependencias; fuera de alcance).

## 2. Base oficial identificada

| Item | Valor |
|---|---|
| Tag | `v7.0.9` |
| Commit | `a1e89e1f64f08cb058caf1c61ff43f319f98a6ec` |
| Fecha | 2026-08-06 11:23:28 +0400 ("Version 7.0.9.") |
| Tag anterior | `v7.0.8` (`8ed7b58848`, 2026-08-03) |
| Primer commit del fork | `eef2d48b89` (2026-08-22 08:57:48 −0500) |

Método de comprobación: blobs idénticos entre `eef2d48` y `v7.0.9` en
`Telegram/build/version`, `changelog.txt`, `10-INVENTARIO-ANTERIOR.md`,
`Telegram/CMakeLists.txt`, `docs/building-win.md`,
`Telegram/SourceFiles/main.cpp`, `Telegram/SourceFiles/storage/storage_shared_media.h`
y `.gitmodules`. La única divergencia en el commit inicial del fork fuera de
submodulos es el conjunto de "Ronda 0" descrito en §5.

## 3. Marco de comparación

- Los submodulos del fork están **vendoreados** (están como árboles, no como
  gitlinks), por lo que el árbol raíz nunca coincidirá con el de `v7.0.9`.
  El diff se limita a `-- .` excluyendo explícitamente sus rutas.
- Dependencias vendoreadas (fuera del diff): `Telegram/ThirdParty/*`,
  `Telegram/lib_crl`, `lib_rpl`, `lib_base`, `lib_ui`, `lib_lottie`, `lib_tl`,
  `lib_spellcheck`, `lib_storage`, `lib_qr`, `lib_webrtc`, `lib_webview`,
  `lib_translate`, `Telegram/codegen`, `cmake`.
- Los 19 archivos `0/0` (solo modo) son los scripts de build (`refresh.sh`,
  `build/*.sh`, `configure.sh`, `Dockerfile` **no**, ese aporta 1 línea real)
  y `.agents/`, `.claude/`: identicales en contenido; git los marca porque el
  bit de ejecución difiere (WSL sobre `/mnt/c` no lo conserva).
- El commit `v7.0.9` se trajo con **fetch blob-less** (`--filter=blob:none`);
  los blobs se descargan bajo demanda al hacer el diff. Para trabajos
  repetidos se recomienda un fetch completo o un clon aparte.

### Cómo reproducir

```bash
git fetch upstream --tags --filter=blob:none            # ya hecho: upstream añadido
EXCL='. :(exclude)Telegram/ThirdParty :(exclude)Telegram/lib_crl \
      :(exclude)Telegram/lib_rpl :(exclude)Telegram/lib_base \
      :(exclude)Telegram/codegen :(exclude)Telegram/lib_ui \
      :(exclude)Telegram/lib_lottie :(exclude)Telegram/lib_tl \
      :(exclude)Telegram/lib_spellcheck :(exclude)Telegram/lib_storage \
      :(exclude)cmake :(exclude)Telegram/lib_qr \
      :(exclude)Telegram/lib_webrtc :(exclude)Telegram/lib_webview \
      :(exclude)Telegram/lib_translate'
git diff --numstat v7.0.9 HEAD -- $EXCL                 # divergencia total
git diff --numstat v7.0.9 eef2d48 -- $EXCL              # solo "Ronda 0" del primer commit
git diff eef2d48 HEAD --stat                             # Rondas commiteadas (≈06)
```

## 4. Divergencia total (HEAD `41286e02` vs `v7.0.9`)

**60 archivos** = **41 con contenido real (+2203 −220)** + **19 solo-modo**.

### 4.1 Código de la app (archivos con contenido)

| Archivo | +/− | Quiéres |
|---|---|---|
| `Telegram/SourceFiles/apiwrap.cpp` | +583 −58 | `requestSharedMediaFilesPhotos`/`All`, `forwardMessages` por lotes/cadena |
| `Telegram/SourceFiles/apiwrap.h` | +12 | nuevas rutas de media |
| `Telegram/SourceFiles/boxes/share_box.cpp` | +214 −88 | reenvío masivo: MassProgress/MassQueue, lotes, toast |
| `Telegram/SourceFiles/info/media/info_media_provider.cpp` | +368 −12 | ventana 2000 ids, materialización proactiva, auto-reparado |
| `Telegram/SourceFiles/info/media/info_media_provider.h` | +38 | `kFullIdsLimit`, `_missingRequested`, `kMaxMissingRetries` |
| `Telegram/SourceFiles/info/media/info_media_list_widget.cpp` | +209 −12 | módulos FilesPhotos/All, selección, filtros |
| `Telegram/SourceFiles/info/media/info_media_list_widget.h` | +21 | — |
| `Telegram/SourceFiles/info/media/info_media_inner_widget.cpp` | +153 −3 | filtros y tabs de "Media" |
| `Telegram/SourceFiles/info/media/info_media_list_section.cpp` | +144 −6 | secciones/headers de la lista |
| `Telegram/SourceFiles/info/media/info_media_list_section.h` | +10 | — |
| `Telegram/SourceFiles/info/media/info_media_common.h` | +16 | `MediaFilter` + getters/setters |
| `Telegram/SourceFiles/info/media/info_media_inner_widget.h` | +14 | — |
| `Telegram/SourceFiles/info/media/info_media_empty_widget.cpp` | +9 −1 | estado vacío de módulos compuestos |
| `Telegram/SourceFiles/info/media/info_media_buttons.cpp` | +2 | botones |
| `Telegram/SourceFiles/info/media/info_media_common.cpp` | +2 | — |
| `Telegram/SourceFiles/info/media/info_media_widget.cpp` | +4 | — |
| `Telegram/SourceFiles/info/info_controller.cpp` | +10 −2 | controladores de módulo compuestos |
| `Telegram/SourceFiles/info/info_controller.h` | +7 | — |
| `Telegram/SourceFiles/info/info_top_bar.cpp` | +16 | barra superior |
| `Telegram/SourceFiles/info/info_top_bar.h` | +1 | — |
| `Telegram/SourceFiles/info/info_wrap_widget.cpp` | +11 | contenedor del perfil |
| `Telegram/SourceFiles/info/info_wrap_widget.h` | +1 | — |
| `Telegram/SourceFiles/info/info.style` | +7 | estilos |
| `Telegram/SourceFiles/info/profile/info_profile_values.cpp` | +18 | valores del perfil |
| `Telegram/SourceFiles/info/profile/info_profile_shared_media_classic.cpp` | +3 −2 | botones FilesPhotos/All en perfil clásico |
| `Telegram/SourceFiles/overview/overview_layout.cpp` | +79 −3 | `hideName` (tiles de archivos) |
| `Telegram/SourceFiles/overview/overview_layout.h` | +3 | — |
| `Telegram/SourceFiles/data/data_sparse_ids.cpp` | +33 −4 | merge forzado de slices compuestas + traza |
| `Telegram/SourceFiles/data/data_histories.cpp` | +23 −19 | borrado masivo en lotes de 100 |
| `Telegram/SourceFiles/data/data_media_types.cpp` | +9 −3 | máscaras compuestas |
| `Telegram/SourceFiles/data/data_shared_media.cpp` | +8 −3 | viewer de slices compuestos |
| `Telegram/SourceFiles/data/data_search_controller.cpp` | +2 | filtro FilesPhotos |
| `Telegram/SourceFiles/storage/storage_shared_media.h` | +2 | enum `FilesPhotos`/`All` |
| `Telegram/SourceFiles/config.h` | +6 −1 | `MaxSelectedItems = 10000` |
| `Telegram/SourceFiles/debug_depurador.h` | **A (+52)** | depurador `depurador.txt` |
| `Telegram/SourceFiles/history/view/controls/history_view_forward_panel.cpp` | +36 | opciones de reenvío recordadas |
| `Telegram/SourceFiles/history/view/controls/history_view_forward_panel.h` | +3 | — |
| `Telegram/SourceFiles/window/window_peer_menu.cpp` | +10 −1 | recordar opciones desde el box |
| `Telegram/Resources/langs/lang.strings` | +9 | `lng_media_type_all`, `lng_profile_all`, `lng_media_photos_files_empty` |

Subtotal código de la app: **38 archivos**.

### 4.2 Otros (con contenido)

| Archivo | +/− | Nota |
|---|---|---|
| `10-INVENTARIO-ANTERIOR.md` | +54 | README del fork (sobre-describe funciones: ver §7 de 07) |
| `Telegram/build/docker/centos_env/Dockerfile` | +1 | 1 línea añadida |

### 4.3 Archivos solo-modo (19, sin contenido real)

`.agents/skills/process-inbox/scripts/workspace.py`, `.claude/grab_clipboard.sh`,
`Telegram/Resources/langs/refresh.sh`, `Telegram/build/build.sh`,
`Telegram/build/changelog2appstream.py`, `Telegram/build/deploy.sh`,
`Telegram/build/docker/build.sh`,
`Telegram/build/docker/centos_env/{build,build_debug,gen_dockerfile,run}.sh`,
`Telegram/build/mac_store_upload.sh`, `Telegram/build/minidebug.sh`,
`Telegram/build/prepare/{linux,mac}.sh`, `Telegram/build/release.sh`,
`Telegram/build/set_version.sh`, `Telegram/build/updates.sh`,
`Telegram/configure.sh`.

Idénticos en contenido; git los marca solo por el bit de ejecución.

## 5. Lo que el fork ya traía en el primer commit (`eef2d48` vs `v7.0.9`)

El commit inicial del fork **ya divergía** del oficial: **38 archivos,
+336 −22**, de los cuales ~22 con contenido real. Corresponde a una
"Ronda 0" preexistente, repartida por las mismas zonas que luego construyeron
las Rondas documentadas:

- **Opciones de reenvío recordadas** — `history_view_forward_panel.cpp/.h`,
  `window_peer_menu.cpp`: secciones `RememberedForwardOptions()` /
  `RememberForwardOptions()` con preferencias `forward_drop_names` /
  `forward_drop_captions` persistentes (`ForwardPanel::update` y
  `ShowForwardMessagesBox`).
- **Esqueleto de `MediaFilter`** — `info_media_common.h`: enum
  `MediaFilter {All, Photos, Videos}` (sin `Files` aún) y hooks virtuales
  `setMediaFilter`/`supportsMediaFilter` (esqueleto, sin `Files`).
- **Galería/perfil tempranos** — `info_top_bar`, `info_wrap_widget`,
  `info_media_inner_widget`, `info_media_list_widget`, `info_media_provider`,
  `info_media_common.h`, `info.style`, `window_peer_menu`, `apiwrap.cpp`.
- Otros: `lang.strings` (+1), Dockerfile (+1).

> Es decir: el fork **no arrancó de `v7.0.9` virgen**; `eef2d48` ya llevaba
> ese trabajo previo. `06-CAMBIOS-GIT.md` solo documenta lo que vino después
> de `eef2d48`; el primer commit quedaba fuera de su comparación y ahora está
> cuantificado aquí.

## 6. Qué añadieron las Rondas commiteadas encima de la "Ronda 0"

Las Rondas documentadas (`eef2d48`→`HEAD` = **33 archivos, +1887 −218**)
completaron la divergencia hasta la lista de §4: módulos compuestos
FilesPhotos/All funcionales, carga masiva sin huecos, reenvío masivo por
lotes con cadena y toast, borrado por lotes, selección 10000, filtros
funcionales, `hideName`, depurador, strings. Ver `06` para el detalle
archivo por archivo.

## 7. Notas y limitaciones

1. El diff **excluye las dependencias vendoreadas**; sus contenidos no se han
   verificado contra los repos de dependencias (requeriría clonar lib_* y
   ThirdParty). La sospecha razonable es que son un snapshot fiel de los que
   apuntaba `v7.0.9`, pero no está probado.
2. El fetch de upstream es **blob-less**; repetir `git diff` grandes puede
   disparar descargas bajo demanda. Para auditorías recurrentes conviene un
   clon completo aparte (`git clone --recursive ...`).
3. Los `0/0` (modo) son ruido de `/mnt/c`; los blobs son idénticos.
4. La **versión oficial NO cambió**: el fork se presenta como 7.0.9 idéntico
   al upstream. Las features son comportamiento interno, no version bump.
5. `git status`/`git diff` globales siguen colgando por `out/` (~38 GB);
   usar los comandos de §3.

## 8. Conclusión

La única divergencia real del fork frente al Telegram Desktop oficial 7.0.9 es
la lista de §4: **41 archivos con contenido (+2203 −220)** — 38 de código de
la app, 2 de documentación/build y 1 nuevo (depurador) — más 19 cambios de
bit de ejecución irrelevantes. Con `v7.0.9` como base oficial verificada, un
futuro rebase/actualización a versiones oficiales posteriores (v7.1.x
disponibles en `upstream/*`) puede calcularse con total certeza usando los
comandos de §3.