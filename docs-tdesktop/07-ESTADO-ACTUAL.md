# 07 – Estado actual de la app

Informe del estado del repositorio y de cómo funciona la aplicación hoy, en el
punto tras la última reversión y cambio de rama. Complementa a
`00-INDICE.md`–`06-CAMBIOS-GIT.md` pero **refleja el código real**, no la
documentación histórica.

Fecha del análisis: 10 de septiembre de 2026.

---

## 1. Resumen ejecutivo

- El trabajar en el repositorio está en `C:\Users\VICTOR\code\tdesktop`
  (Telegram Desktop, fork del proyecto oficial).
- **Rama actual:** `7-mejorar-interfas-de-al-app`.
- **HEAD:** `41286e02` (merge del PR #6 de la rama `3-agregar-all-a-la-interfas`).
- **Árbol de trabajo:** NO limpio desde el 11-sep-2026: el reenvío masivo
  robusto (Rondas 22e/23) está **implementado pero sin commitear** en 3 archivos
  (`apiwrap.cpp` +489, `apiwrap.h` +4, `share_box.cpp` +34). Ver secciones
  4.6, 6 (filas 22-23) y 9.
- **Base local "original":** `eef2d48 first commit`. El diff commiteado
  `eef2d48..HEAD` es exactamente **33 archivos, +1887 −218** (coincide con
  `06-CAMBIOS-GIT.md`).
- **Qué hay hoy en el código:** todos los cambios **commiteados** de las Rondas
  1–22b: módulos compuestos `FilesPhotos`/`All`, carga completa con
  materialización proactiva, reenvío masivo por lotes con cola por hilo y toast
  de progreso, borrado masivo en lotes, selección ampliada
  (`MaxSelectedItems = 10000`), filtros de galería, cuadrícula densa,
  `hideName` para tiles de archivos y el depurador propio `depurador.txt`.
- **Qué NO está en el código (documentado pero ausente):** Rondas **22
  (desbloqueos noforwards), 22b (reenvío como copia), 22c (reintentos 3x +
  gasto único de estrellas)**. Corresponden a trabajo que
  vivía sin commitear en el working tree de la rama 5 y se perdió con los
  resets/cambios de rama; hoy no hay ni rastro de marcadores en el código.
- **Ronda 22e/23 SÍ: reenviada como nueva implantación (11-sep-2026):**
  pacing adaptativo AIMD + watchdog de inactividad + toasts de espera +
  persistencia/reanudación en disco (`reenvio_reanudable.dat`), en
  `ApiWrap::forwardMessages` y en la cola masiva de `share_box.cpp`.
  Sin commitear (pendiente build Windows + pruebas).
- **Ronda 24 (11-sep-2026): recuperación completa de `FilesPhotos`/`All`.**
  También **sin commitear**, en 6 archivos (`data_search_controller.*`,
  `data_sparse_ids.*`, `apiwrap.cpp`, `data_shared_media.cpp`): las
  sub-búsquedas compuestas aceptan toda foto/documento (no-sticker) y un modo
  "caminata" del `SparseIdsSliceBuilder` recorre el historial hacia atrás en
  segundo plano hasta demostrar el inicio, con conteo exacto al cierre.
  Con la Ronda 24a (13-sep) todo quedó SÓLO en `FilesPhotos` (`All` revierte:
  sin acceptAllMedia, sin caminata), pacing 1500 ms y materialización por
  zona visible. **La Ronda 25 (13-sep) la sustituyó por la paridad con
  `Fotos + Vídeo`**: FP pagina por scroll con ventana pequeña (sin caminata,
  sin ventana ±400), lo que elimina los huecos con icono de descarga que
  dejaba la ventana grande. **La Ronda 26 (17-sep)** ataca el defecto
  estructural restante: FP **pagina cada sub-búsqueda (Photo y File) desde su
  PROPIO frente** (no un ancla global), de modo que ninguna secuencia salta
  franjas; cada lado se cierra al llegar a su inicio y el conteo ya no se
  infravalora si una sub-búsqueda falla. Pendiente build Windows + prueba.

## 2. Identidad y versión de la aplicación

| Campo | Valor | Fuente |
|---|---|---|
| App | Telegram Desktop (cliente oficial, API de Telegram + MTProto) | `10-INVENTARIO-ANTERIOR.md:1-3` |
| Versión | **7.0.9** (`AppVersion 7000009`) | `Telegram/build/version` |
| Changelog | `7.0.9 (05.08.26)` | `changelog.txt:1` |
| Entrada | `main()` → `Core::Launcher::Create()->exec()` | `Telegram/SourceFiles/main.cpp` |
| Plataforma | Windows (win64), CMake + MSBuild | `docs/building-win.md` |

## 3. Estado Git y ramas

Estado verificado con comandos ligeros (en este entorno `git status`/`git diff`
globales se cuelgan por `out/` ~38 GB y `/mnt/c`; los blobs no mienten).

- Remoto único: `origin = https://github.com/nilvictor99/tdesktop-new.git`
  (fork propio; **no hay** remoto `upstream` del `telegramdesktop/tdesktop`).
- Historial lineal del fork (hasta HEAD):

```
eef2d48  first commit
b56415f  a
0f4d534  a
317c2bb  mejora
aeaa8af  cambio yea de mejora
41286e0  Merge PR #6 (3-agregar-all-a-la-interfas)  ← HEAD
```

- Ramas relevantes:
  - `7-mejorar-interfas-de-al-app` (actual) → `41286e02`
  - `main` → `41286e02`
  - `origin/3-agregar-all-a-la-interfas` → `aeaa8af7`
  - `origin/5-agregar-funcionalidad-de-rembio-ye` → `0f4d534d`
  - `origin/main` → `41286e02`
- Reflog reciente muestra el baile de ramas/resets de los últimos días
  (`checkout` ramas, `pull origin main`, `reset moving to HEAD~3`,
  `reset moving to origin/5-agregar...`), que es la causa de que el trabajador
  sin commitear de las Rondas 22c–23 desapareciera del árbol.
- **Limpiado:** blob-hash de los 33 archivos del diff commiteado == `HEAD`
  (todos `OK`). No hay cambios sin commitear pendientes.

## 4. Cómo funciona la aplicación actualmente

### 4.1 Arranque y plataforma Windows

- `main.cpp` crea `Core::Launcher` → `exec()` → `executeApplication()`
  (procesa argumentos, working dir portable, instancia/hash).
- Windows: `platform/win/launcher_win.cpp`, `main_window_win.cpp`,
  `specific_win.cpp`, `integration_win.cpp`, `tray_win.cpp`.
- Aplicación/sesión: `core/application.cpp`, `mainwindow.cpp`,
  `mainwidget.cpp`, `main/main_domain.cpp` (dominio multi-cuenta),
  `main/main_account.cpp`, `main/main_session.cpp`.

### 4.2 Sesión, cuenta y almacenamiento

- Autenticación y persistencia: `storage/storage_domain.cpp`,
  `storage/storage_account.cpp`, `storage/localstorage.cpp` (settings,
  backgrounds, cuenta cifrada con `LocalEncryptIterCount=4000`,
  `config.h:20-22`).
- Datos en memoria: `data/data_session.cpp`, `storage/storage_facade.cpp`.
- Caché/DB cifrada en `Telegram/lib_storage/storage/`.

### 4.3 Red, MTProto y API

- Capa MTProto: `mtproto/session.cpp`, `mtproto/session_private.cpp`,
  `mtproto/mtp_instance.cpp`, `mtproto/sender.h`.
- Fachada de alto nivel: `apiwrap.h/.cpp` (`requestHistory`,
  `requestSharedMedia`, `requestGlobalMedia`, `forwardMessages`, etc.).
- **Particularidad del fork:** rutas compuestas de media
  `requestSharedMediaFilesPhotos` (`apiwrap.h:326`) y
  `requestSharedMediaAll` (`apiwrap.h:332`) que fusionan varias
  búsquedas MTP (Photo + File / Photo + Video + File).

### 4.4 Datos, historiales y medios compartidos

- Enums de media: `storage/storage_shared_media.h:17-35` — además de los tipos
  clásicos existen **`FilesPhotos`** y **`All`** (tipos compuestos, valor 13/14
  cerca de `kCount`).
- `data/data_media_types.cpp`: las máscaras `sharedMediaTypes()` añaden
  `FilesPhotos`/`All` para fotos y documentos genéricos.
- `data/data_search_controller.cpp`: `Type::FilesPhotos` → filtro
  `MTP_inputMessagesFilterDocument` (`case` añadido).
- `data/data_shared_media.cpp`: viewer/merge de slices para los tipos
  compuestos.
- `data/data_sparse_ids.cpp`: forzado del merge de ids para las slices
  compuestas (evita el bucle de páginas repetidas). Aquí también vive la traza
  `[DEPUR] SPARSE` instrumentada con `debug_depurador.h`.
- `data/data_histories.cpp:793-795`: borrado masivo en **lotes de 100**
  (`kMaxDeletePerRequest`).

### 4.5 Galería, perfiles y visor multimedia

- Módulos de media: `info/media/info_media_provider.cpp/.h`,
  `info_media_widget.cpp`, `info_media_inner_widget.cpp`,
  `info_media_list_widget.cpp`, `info_media_list_section.*`,
  `info_media_buttons.cpp`, `info_media_empty_widget.cpp`,
  `info_media_common.*`.
- **Ventana de carga grande:** `info_media_provider.h:98`
  `kFullIdsLimit = 1000` (ventana ≈ 2000 ids) y estado de auto-reparado:
  `_missingRequested`, `_missingRetries`, `kMaxMissingRetries=8`,
  `requestMissingAround()` (materialización proactiva en lotes de ≤100 vía
  `messages/channels.getMessages`).
- **Filtros de galería:** `info_media_common.h:25-29` `MediaFilter {All,
  Photos, Videos, Files}` + getters/setters virtuales (`mediaFilter()`,
  `setMediaFilter`, `supportsMediaFilter`).
- **Cuadrícula densa:** `info/media/info_media_grid_zoom.cpp` (paso de zoom de
  la cuadrícula persistido en settings).
- **Tiles de archivos sin nombre:** `overview/overview_layout.h:433,490`
  `hideName` + `resizeGetHeight` → celdas cuadradas en la malla.
- **Visor:** `media/view/media_view_overlay_widget.cpp` (descargar,
  guardar como…, atajos; `Ctrl+Shift+G` de salto al mensaje NO presente: el
  patrón documentado en Ronda 1 no está en el código).

### 4.6 Selección, reenvío y borrado

- Selección ampliada: `config.h:18` **`MaxSelectedItems = 10000`** (antes 100).
- Reenvío masivo (`ApiWrap::forwardMessages`, `apiwrap.cpp:4311`):
  - Trocea el seleccionado en lotes de ≤100 (`kMaxForwardMessages=100`).
  - Construye `ForwardChunk` con `randomIds` y `localIds` por lote.
  - **Pipeline AIMD (Ronda 22e/23, 11-sep, sin commitear):**
    - Cadena secuencial con un solo lote en vuelo (`ForwardPipeline` +
      `base::Timer`, `apiwrap.cpp:4520-4932`).
    - **Pacing por latencia**: fast-start de 15 lotes a 5 s (~1500 msgs),
      luego pausa sostenible mínima 100 s (≈2000/h, la cuota real del
      servidor), máx. 300 s; si una respuesta tarda más de `pausa + 5 s`
      (spike → flood invisible, la capa MTProto traga los `FLOOD_WAIT`),
      la pausa se duplica; si responde al vuelo, se recupera de 10 s en 10 s.
    - `FLOOD_WAIT_<seg>`: espera real (`clamp` a 2 h) y **reintenta el MISMO
      lote**, nunca se salta.
    - Watchdog: aborta limpio tras inactividad > 2× muro (4 h) sin flood.
    - Toasts: "Reenvío completo: %1 mensajes", "Esperando al servidor… %1 s"
      (actualizado cada 3 s), "Flood: faltan %1s para reanudar…",
      "Reenvío terminado (con errores): %1 mensajes",
      "Reenvío abortado por timeout".
    - **Reanudación persistida**: cada 5 lotes escribe `reenvio_reanudable.dat`
      (magic "FWDR", peer destino, ids ya enviados); al reenviar la misma
      selección al mismo destino tras un corte se saltan los ids confirmados
      y se continúa; se borra al completar.
    - Guard de vida: `_session->lifetime().add(...)` cancela las timers si la
      sesión se destruye (logout) y conserva el archivo de reanudación.
  - El callable de la cadena se guarda en `shared_ptr` (elimina el
    `std::bad_function_call` de la build).
  - Estrellas: se cobran por el costo real del lote (`starsPaid`).
- `boxes/share_box.cpp` (capa de UI del reenvío):
  - `MassProgressState` + toast `"Reenviando %1 de %2… quedan %3"`
    (`share_box.cpp:1858-1884`).
  - Lotes de 100 (`kMassBatchSize=100`), **pacing adaptativo**:
    pausa mínima 5 s que se acerca a sí misma, duplica en `FLOOD_WAIT`
    (espera real, clamp 2 h) y reintenta el mismo lote — nunca se salta
    (cambios del 11-sep, sin commitear); `MassQueue`/`keepAlive` por hilo
    destino, cierre correcto cuando `requests.empty() &&
    activeThreads == 0`.
- Borrado: lotes de 100 por petición (`data/data_histories.cpp:793`).

### 4.7 Depurador propio

- `Telegram/SourceFiles/debug_depurador.h` (nuevo, 1490 B): escribe
  `depurador.txt` junto a `log.txt`, limpiando el archivo una vez por
  lanzamiento; helper `Depur::append`/`Depur::num`, thread-safe con `QMutex`.
- Se usa en: `apiwrap.cpp`, `data/data_sparse_ids.cpp`
  (`[DEPUR] SPARSE ...`), `info/media/info_media_provider.cpp`.

### 4.8 Compilación

- Windows: `docs/building-win.md` — Visual Studio 2026, SDK 10.0.26100.0;
  `Telegram/configure.bat` → `configure.py`; salida `out/.../Telegram.exe`.
- Comando habitual de la doc local (`docs/00-INDICE.md:66-71`):
  `msbuild Telegram.sln /p:Configuration=Release /p:Platform=x64 /m /t:Telegram`.

## 5. Comparación: base original frente a estado actual

| Aspecto | Base `eef2d48` (first commit) | Estado actual `41286e02` | Diferencia |
|---|---|---|---|
| Enum media | sin `FilesPhotos`/`All` | con `FilesPhotos`/`All` | +2 valores |
| `MaxSelectedItems` | 100 | 10000 | x100 |
| Reenvío | petición única (<=100) | lotes 100 + cadena + toast | funcional |
| Borrado | petición única | lotes de 100 | funcional |
| Módulos media | solo clásicos | `FilesPhotos`/`All` + ventana 2000 ids | funcional |
| `hideName` (tiles) | no | sí | funcional |
| Depurador | no | `debug_depurador.h` + trazas | nuevo |
| Frases i18n | sin lng_media_type_all etc. | con frases compuestas | +strings |

Nota honesta sobre la "base original": `eef2d48` **no es el upstream puro**;
es el primer commit del fork (ya contiene `.agents/`, config local, etc.). La
base estricta sería el tag de `telegramdesktop/tdesktop` correspondiente a lo
importado (requiere red/clonado, no disponible ahora). Por eso el informe usa
`eef2d48` como base local verificable y marca la comparación con upstream como
pendiente.

## 6. Matriz de funcionalidades: documentado vs código real

Leyenda: ✅ presente y verificado / ❌ ausente (marcadores = 0) / ⚠️ parcial o
discrepante.

| # | Función (docs) | Ronda | Código actual | Evidencia / nota |
|---|---|---|---|---|
| 1 | Copiar texto desbloqueado | 22 | ❌ | `history_item.cpp:2999-3005` sigue exigiendo `!forbidsForward()`; `forbidsForward()` devuelve `flag NoForwards` (`:3064-3066`) |
| 2 | Guardar/descargar desbloqueado | 22 | ❌ | `history_item.cpp:3068-3075` `forbidsSaving()` activo |
| 3 | Selección de media desbloqueada | 22 | ❌ | `history_view_list_widget.cpp:1660,1674` `hasSelectRestriction()` bloquea `selectItem` |
| 4 | Selección sin límite de 100 | 1 | ✅ | `config.h:18` `MaxSelectedItems = 10000` (⚠️ docs dicen 50000) |
| 5 | Shift + clic (rangos) | 1 | ✅ | selección por rangos en list/overview (commiteado) |
| 6 | Seleccionar todo (Ctrl+A / menú / botón) | 1, 3 | ✅ | en lista de media y overview (commiteado) |
| 7 | Cuadrícula densa | 5 | ✅ | `info/media/info_media_grid_zoom.cpp` |
| 8 | Filtros Todo/Fotos/Videos | 1, 10 | ✅ | `info_media_common.h:25-29` `MediaFilter` + getters |
| 9 | Botón único "Media" (incl. classic) | 3 | ✅ | clásico añade `FilesPhotos`/`All` (`info_profile_shared_media_classic.cpp:348-351`) |
| 10 | Reenvío masivo estable (lotes 100) | 4,5,6,20,21 | ✅ | `apiwrap.cpp:4233,4343`, `share_box.cpp:1933` |
| 11 | Cadena secuencial, 1 en vuelo, fallo no rompe | 20, 21 | ✅ | `apiwrap.cpp:4520+` (pipeline AIMD) |
| 12 | Sin límite práctico | 20 | ✅/⚠️ | real `10000`, docs `50000` |
| 13 | Opciones "Sin autor"/"sin captions" recordadas | 1 | ✅ | `NormalizeForwardOptions` (`apiwrap.cpp:4252`) |
| 14 | Borrado masivo fiable | 4 | ✅ | `data_histories.cpp:793-795` lotes de 100 |
| 15 | Descargar siempre visible / Ctrl+Shift+G | 1 | ⚠️ | visor sí expone descargar (`media_view_overlay_widget.cpp:947-948`); atajo Ctrl+Shift+G no verificado en código |
| 16 | Módulo "Todo" (All) | 9 | ✅ | `storage_shared_media.h:32`, `requestSharedMediaAll` |
| 17 | Módulo "Fotos + Archivos" (FilesPhotos) | 7 | ✅ | `storage_shared_media.h:31`, `requestSharedMediaFilesPhotos` |
| 18 | Carga completa FilesPhotos/All sin huecos | 11–19.7 | ✅ | `info_media_provider.h:98,121,149,155-156` |
| 19 | Reenvío "como copia" ante `CHAT_FORWARDS_RESTRICTED` | 22b | ❌ | `sendForwardAsCopy` = 0 coincidencias |
| 20 | Reintentos 3x + gasto único de estrellas | 22c | ❌ | `kMaxForward*` = 0 coincidencias; estrellas solo en código de `starsPaid` por lote |
| 21 | Contador en vivo "Reenviados: X de N" | 22d | ⚠️ | toast `MassProgress` R20-21 + toast de espera "Esperando al servidor… %1 s" (22e) |
| 22 | Watchdog anti-cuelgue (2 h) | 22e | ✅ | `apiwrap.cpp:4888-4889` (inactividad > 2× muro aborta, con `depurador.txt` de la traza) |
| 23 | Pacing adaptativo 5 s→100 s→300 s | 23 | ✅ | `apiwrap.cpp:4209-4217` (constantes `kForwardChunk*`), pipeline AIMD + reintento del mismo lote en FLOOD_WAIT; `share_box.cpp:1933-1936` (pausa 5 s→300 s) |
| 24 | Iconos nuevos (FilesPhotos→video, All→all) | 23 | ❌ | `infoIconMediaAll` = 0; clásico usa `st::infoIconMediaPhoto` (`:350-351`) |
| 25 | Depurador `depurador.txt` | – | ✅ | `debug_depurador.h` + trazas en 3 archivos |
| 26 | Carga COMPLETA FilesPhotos/All (caminata al inicio, conteo exacto) | 24 | ⚠️ | superada: la Ronda 25 (paridad scroll) y la **Ronda 26** (paginado por lado + merge encadenado + conteo protegido, `apiwrap.h/.cpp`) + **Ronda 26.1** (conteo fiable `reliableCount`, ventanas descartadas `FP DROP`, guard `anyContinues`) reemplazan el modo caminata; `All` queda revertido (ventana grande, sin acceptAllMedia). Pendiente build/prueba Windows |

## 7. Documentación histórica frente a la realidad

- **`00-INDICE.md` / `03-CAMBIOS.md` / `05-REGISTRO-REENVIO-MASIVO.md`**:
  describen Rondas **22–23** que ya **no están en el código** (ver matriz).
  Leerlas como estado actual sería engañoso; son histórico de lo que se hizo.
- **`MaxSelectedItems`**: `00`/`03`/`05` dicen 50000; `10-INVENTARIO-ANTERIOR.md` del repo dice
  10000; **config.h:18 = 10000 (real)**. Discordancia entre fuentes documentales.
- **`10-INVENTARIO-ANTERIOR.md` del repositorio** (commiteado): líneas 27-29 describen los
  desbloqueos R22 (copiar/guardar/selección) y 61-67 describe reenvío con
  reintentos/watchdog que **no están** — README está sobre-descrito frente al
  código.
- **`06-CAMBIOS-GIT.md`**: su "Nota de alcance" ya avisaba de que Rondas
  22c–23 vivían sin commitear. Ese aviso sigue vigente y además esos cambios
  tampoco están hoy en el working tree (árbol limpio).
- **`01-ENTORNO.md`**: vigente en lo esencial (toolchain, usar `Libraries`),
  aunque fechas/tamaños (126 GB, Qt 5.15.19) son de agosto.

## 8. Riesgos y notas honestas

1. **No confundir docs con código.** El código actual es el de `41286e02`
   (commit del fork). Cualquier plan debe partir de este estado, no de `00`/`05`.
2. **El límite de selección real es 10000**, no 50000: pruebas masivas de
   10k+ a la vez pueden quedarse en 10000.
3. **Desbloqueos `noforwards` no existen en el código.** Intentar reenviar
   contenido de canales restringidos hoy dará `CHAT_FORWARDS_RESTRICTED` del
   servidor; la app no tiene el *fallback* "reenviar como copia".
4. **Reenvío masivo robusto (11-sep)** está **implementado (sin commitear)**:
   pipeline AIMD + watchdog + reanudación en disco. Pendiente compilar en
   Windows y probar con una selección grande (fast-start ~1500 pesados →
   ritmo sostenible ~2000/hora; espera real en flood; reanudación tras
   reinicio reenviando la misma selección al mismo destino).
5. **Recuperación completa de `FilesPhotos` (Ronda 24, 11-sep / Ronda 25,
    13-sep / Ronda 26, 17-sep) también **sin commitear**. La Ronda 25 adoptó
    la **paridad con `Fotos + Vídeo`**: FP pagina por scroll con ventana
    pequeña (sin caminata ni materialización por zona visible), conservando
    `acceptAllMedia` y el merge MERGE-SKIP; el conteo vuelve a ser la
    estimación del servidor. La **Ronda 26** añade el paginado por lado
    (frentes propios Photo/File), merge encadenado (slice contigua única) y
    conteo protegido ante fallos parciales.
    **`All`** quedó revertido a su comportamiento previo (ventana grande,
    sin acceptAllMedia, sin caminata), y `File`/módulos clásicos no cambian.
6. `git status`/`git diff` globales cuelgan en este entorno (`out/` ~38 GB
   sobre `/mnt/c`). Para verificar limpieza usar `git rev-parse HEAD:<ruta>` +
   `git hash-object <ruta>` por archivo, como se hizo aquí.
7. Para una comparación estricta contra el cliente oficial haría falta
   clonar `telegramdesktop/tdesktop` en el tag/commit correspondiente
   (pendiente, requiere red).

## 9. Próximos pasos recomendados

1. **Compilar en Windows** (Release x64, `msbuild Telegram.sln
   /p:Configuration=Release /p:Platform=x64 /m /t:Telegram`) y probar el
   reenvío robusto: selección grande (~10.000) → fast-start + ritmo
   sostenible, toast "Esperando al servidor…" durante flood invisible, y
   reanudación tras reiniciar la app con la misma selección al mismo destino
   (verificar creación/borrado de `reenvio_reanudable.dat`).
2. En la MISMA build, probar la **Ronda 25 + 26**: abrir `Fotos + Archivos`
   en un chat con >1000 media → la carga inicial es rápida, el **scroll hacia
   arriba/abajo rellena progresivamente sin celdas vacías ni iconos de
   descarga colgados** y sin congelamientos; `depurador.txt` muestra
   `FP APPLY`/`MATER` pequeños por zona, `FP CLOSE` al final de cada lado y
   **cero líneas `WALK`**. Evidencia esperada de la Ronda 26: los `FP SEND`
   de Photo y File divergen en `msgId` (cada lado a su ritmo) y el rango
   aplicado baja hasta ~2 (sin franjas huérfanas). `Todo` y `Fotos + Vídeo`
   deben comportarse como antes.
3. Tras la validación, **commitear** los 3 archivos del 11-sep
   (`apiwrap.cpp`, `apiwrap.h`, `share_box.cpp`) y actualizar
   `06-CAMBIOS-GIT.md`.
4. Decidir si procede reconstruir las Rondas 22–22c restantes (desbloqueos
   `noforwards` con fallback "reenviar como copia").

## 10. Apéndice de evidencia

### Comandos y verificaciones (10-sep-2026)

- `git rev-parse --short HEAD` → `41286e0`
- `git branch --show-current` → `7-mejorar-interfas-de-al-app`
- `git remote -v` → `origin https://github.com/nilvictor99/tdesktop-new.git`
- `git log --reverse --oneline` → 6 commits (eef2d48…41286e02)
- `git diff --shortstat eef2d48 HEAD` → `33 files changed, 1887 insertions(+), 218 deletions(-)`
- `git reflog --oneline` → muestra checkouts/pulls/resets recientes
- Blob-hash de **33/33** archivos del diff == `HEAD` (working tree limpio)

### Verificaciones del reenvío robusto (11-sep-2026, sin commitear)

- `git diff --stat` (con `git config gc.auto 0` y `.git/gc.log` eliminado,
  el diff ya no cuelga) →
  `apiwrap.cpp | 489 ++++++++++---`, `apiwrap.h | 4 +-`,
  `share_box.cpp | 34 +-` → **458 inserciones, 69 borrados**.
- Balance de llaves = 0 en los 3 archivos; includes sin duplicados; sin
  emojis en toasts (regla del proyecto).
- Tipos verificados: `PeerId.value` (no `.bare`); **`MsgId` es una clase**
  (`struct MsgId { int64 bare; }`, `data/data_msg_id.h:19-49`) — por eso el
  set de reanudación es `QSet<qint64>` alimentado por `rawIds` (`item->id.bare`
  sin truncar) y el `quint32(item->id)` del primer intento falló (C2440).
  `MTP_int(MsgId)` trunca a int32 solo para el MTP, el archivo de reanudación
  guarda el int64 completo.
- Semántica de inicializadores designados (`.text = QString` para
  `TextWithEntities`): aceptada por MSVC/GCC (la usa el propio
  `share_box.cpp:1876`); clang la rechaza (irrelevante: build = MSVC).
- Entradas del reenvío verificadas: `click_handler_types.cpp:675-695`,
  `window_peer_menu.cpp:4525-4545` y la cola masiva de `share_box.cpp` —
  no hay un tercer motor de reenvío.
- **Pendiente:** compilar en Windows y probar ritmo/espera/reanudación.

### Verificaciones de la Ronda 25 (13-sep-2026, sin commitear)

- Archivos tocados en esta ronda: `data/data_shared_media.cpp`,
  `info/media/info_media_provider.h/.cpp` (sobre la Ronda 24a ya sin commitear).
- Balance de llaves y paréntesis = 0 en los archivos tocados (verificado).
- Ventana grande (`kFullIdsLimit`) **solo para `All`**: ctor, `restart`,
  `preloadAroundItem` y `jumpToMessage` de `info_media_provider.cpp` dejan de
  incluir `Type::FilesPhotos` → FP usa `kMinimalIdsLimit`/`preloadIdsLimit`
  y pagina por scroll como `Fotos + Vídeo`.
- Caminata: el `setAlwaysPageBefore(true)` de FP se eliminó en
  `data_shared_media.cpp`; el código queda dormido en `data_sparse_ids.*`
  (reversible). `grep` confirma cero activaciones de la caminata.
- Materialización simplificada (solo queda la red de seguridad): sin
  `kMaterializeWindow`, sin `_lastBudgetAroundBare`, sin `findSliceIndex`
  (declaración en `.h`, implementación en `.cpp`); `requestMissingAround`
  vuelve a juntar TODA la slice (1-3 lotes con ventana pequeña), con reset
  de presupuesto por refresh y `_refreshed.fire` incondicional al terminar.
- `acceptAllMedia=true` sigue solo en FP DONE (`apiwrap.cpp:3876`); ALL DONE
  sigue sin él.
- Sin emojis ni toasts nuevos (solo trazas `[DEPUR]` existentes).
- **Pendiente:** build MSBuild en Windows + prueba de scroll sin huecos.

### Verificaciones de la Ronda 26 (17-sep-2026, sin commitear)

- Archivos tocados: `apiwrap.h` (estado de paginación por lado
  `FilesPhotosPagingKey/State` + `_filesPhotosPaging`), `apiwrap.cpp`
  (reescritura de `requestSharedMediaFilesPhotos`, `sharedMediaDone` acepta
  `std::optional<int> fullCount`): frentes/topes propios Photo/File,
  cierre por lado (vacío + rango completo), merge encadenado al ancla,
  conteo protegido (no se sobrescribe si alguna sub-búsqueda falla),
  reinicio por `Around` con `epoch`.
- Balance de llaves/paréntesis = 0 en `apiwrap.h` y `apiwrap.cpp` (verificado
  con script sobre ambos archivos).
- Include `data/data_msg_id.h` añadido en `apiwrap.cpp` (para
  `ServerMaxMsgId` en el cierre por `After`).
- `All` y módulos clásicos: intactos (la lógica solo aplica a
  `SharedMediaType::FilesPhotos`; `sharedMediaDone` conserva el mismo
  comportamiento con el conteo explícito).
- Trazas nuevas: `FP RESET`, `FP SEND ... CERRADO`, `FP CLOSE type=… dir=…`;
  `FP SEND` muestra el ancla real usado por cada lado.
- **Pendiente:** build MSBuild en Windows + sesión de scroll larga; se espera
  ver `FP CLOSE` en ambos lados, rangos que bajan hasta ~2 y sin franjas
  permanentes.

### Verificaciones de la Ronda 26.1 (20-sep-2026, sin commitear)

- **Objetivo**: cerrar los dos defectos restantes que podían dejar la lista
  incompleta aunque R26 paginase bien: (1) `fullCount` degradado a tamaño de
  página en la página de cierre (`Api::SearchResult::reliableCount` nuevo,
  `data_search_controller.{h,cpp}`), que provocaba una "corona" `{0,ServerMax}`
  prematura en `storage_sparse_ids_list.cpp:115-123`; y (2) cierre falso de un
  lado por ventana descartada (mensajes que `acceptAllMedia` no acepta →
  sello `from==0` en `data_search_controller.cpp:282-293`).
- Cambios: `reliableCount` solo con `vcount` real; `updatePagingFrom` guarda
  el fullCount por lado solo si fiable; `finishOne` arma el total con el conteo
  fiable (o último conocido) y `fullCountToSave` exige `allOk && countsOk`;
  `.done` detecta ventana descartada (`FP DROP`, anula el `{0,...}` del sello,
  avanza frente sin cerrar); guard `anyContinues` (el fin real exige que ningún
  lado vivo siga explorando).
- Trazas nuevas: `FP DROP` y `reliable=%6` en `FP DONE`.
- Balance de llaves = 0 en `apiwrap.cpp` y `data_search_controller.{h,cpp}`.
- **Pendiente:** build MSBuild en Windows (verificar `Telegram.exe` con fecha
  HOY, el anterior era 16-sep = R25) + sesión de scroll larga; se espera
  `FP APPLY count=7709` constante, `FP CLOSE` en ambos lados y lista completa.

### Rutas clave citadas

- `config.h:18` (MaxSelectedItems 10000), `config.h:20-22` (cifrado local)
- `storage/storage_shared_media.h:17-35` (enum con FilesPhotos/All)
- `apiwrap.h:314-333` (rutas media), `apiwrap.cpp:4227-4486` (forwardMessages)
- `boxes/share_box.cpp:1858-1971` (MassProgress/MassQueue/toast/lotes)
- `data/data_histories.cpp:793-795` (borrado por lotes)
- `info/media/info_media_provider.h:98-156` (ventana + auto-reparado)
- `info/media/info_media_common.h:25-29,187-192` (filtros)
- `overview/overview_layout.h:433,490` (hideName)
- `history/history_item.cpp:2999-3075` (restricciones noforwards activas)
- `history/view/history_view_list_widget.cpp:1654-1686,1775` (restricción de selección/copia)
- `Telegram/SourceFiles/debug_depurador.h` (depurador)

### Lecturas recomendadas del paquete de docs

- Para entender cómo se construyeron las funcionalidades commiteadas:
  `06-CAMBIOS-GIT.md` (dif-commit por archivo).
- Para el contexto técnico de los módulos compuestos: `02-PLANES.md`,
  `03-CAMBIOS.md` (Rondas 1–19.7 ✅ diseñadas/implantadas).
- Para la historia del reenvío masivo: `05-REGISTRO-REENVIO-MASIVO.md`
  (leer como histórico, no como estado actual).