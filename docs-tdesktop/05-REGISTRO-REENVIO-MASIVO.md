# Registro — Reenvío masivo sin cortes

Historia completa del reenvío masivo (>1000 mensajes): qué se intentó, qué
funcionó, qué no funcionó y por qué, más el estado de la solución actual
(Ronda 23). Complementa el registro por Ronda de `03-CAMBIOS.md`.

---

## Objetivo del usuario
Reenviar lotes grandes de mensajes de un chat a otro (y desde el perfil,
seleccionando todo) SIN que el reenvío se corte alrededor de los ~2000
mensajes, SIN perder mensajes, SIN saturar el servidor, y SIN exigir que la
persona esté pendiente todo el tiempo. Dicen que "con el límite de selección de
10000 funcionaba".

## Qué se intentó (cronológico) y resultado

### Primer intento — todo de golpe (Rondas 4-5)
- **Qué**: una sola solicitud con TODOS los ids seleccionados.
- **Resultado**: el servidor solo procesa ~100 mensajes por petición; el resto
  quedaba vivo y reaparecía, o la petición fallaba por flood.
- **Aprendizaje**: hay que dividir en lotes de ≤100 **y** encadenarlos.

### Cola masiva del share box (Ronda 5 → 6 → 22h)
- **Qué**: `share_box.cpp` con cola de partes ≤100 secuenciales (pausa 300 ms),
  reintentos de FLOOD_WAIT, fallo que no aborta la cola y un keep-alive para
  que la cola no muera a mitad.
- **Funcionó**: el reenvío por el share box no se cuelga ni salta partes
  (Ronda 22h: ante flood reintenta la MISMA parte todas las veces, nunca la
  descarta).
- **No bastó**: el corte masivo que el usuario ve ocurre por la OTRA vía de
  reenvío (el botón Reenviar de la vista de chats → `forwardMessages`).

### Cadena secuencial con watchdog (Rondas 22c-22f)
- **Qué**: `forwardMessages` encadena lotes de 100 de UNO en UNO
  (`afterRequest`), reintenta errores temporales con los mismos randomIds
  (sin duplicar), cuenta en vivo "Reenviados: X de N" y un watchdog con
  timeout por lote para que la cadena SIEMPRE termine.
- **Problema encontrado**: el watchdog inicial (60 s) cancelaba la petición
  **legítimamente pausada por FLOOD_WAIT** (la capa MTP espera y reenvía sola
  los floods, nunca llegan al fail del lote). A partir de ~2000 mensajes el
  servidor emite FLOOD_WAIT de ~50 min → el watchdog lo mataba → "se cortaba".
- **Aprendizaje CLAVE (diagnóstico definitivo, Rondas 22f)**: NO hay tope de
  2000 en el cliente. Un reenvío de 2334 mensajes avanzó lote a lote sin un
  solo fallo hasta el índice 20 (mensaje 2001-2100), donde el servidor devolvió
  `FLOOD_WAIT secs=3283` (≈55 min). El muro lo pone el SERVICIO al agotar la
  cuota horaria de reenvíos de la cuenta (~2000 en pocos minutos), y escala
  abruptamente. "Con 10000 funcionaba" solo porque en esas pruebas la cuota
  estaba descansada.
- **Corrección**: watchdog a 2 horas → la cadena **respeta y espera** el flood
  y se reanuda sola (Ronda 22f). Funciona, pero la app estaba abierta a mitad
  de una espera larga ≈ el usuario percibe "se cortó".

### Pacing anti-saturación (Ronda 22g) — 30 s por lote
- **Qué**: espaciar los lotes 30 s para que el servidor no vea nunca un ritmo
  agresivo y no tenga que frenar.
- **Funcionó (parcial)**: con pacing de 30 s un reenvío de 2333 mensajes fluyó
  SIN floods por lote durante los ~19 primeros lotes (índice 0-19). La cadena
  no se rompe.
- **No bastó**: el muro del servidor sigue apareciendo en torno al mensaje
  ~2000 *a pesar* de la cadencia; el lote siguiente recibió
  `FLOOD_WAIT secs=3000` (≥ ~50 min). El pacing suaviza el ritmo, pero la cuota
  horaria es finita: llegar a ~2000 mensajes tarde o temprano reclama la
  espera. El usuario cerró la app ~1 min después → "se cortó".

## Solución final (Ronda 23) — pacing adaptativo + oleadas con pausa honesta
**Estrategia**: no se puede evitar que el servidor cobre la cuota; sí se puede
*hacerle caso* con inteligencia y *avisar de forma honesta*:

1. **Pacing adaptativo** (`apiwrap.cpp`): el gap entre lotes ya no es fijo; se
   ajusta (clamp 30-300 s) a lo que duró el lote anterior. Si el servidor
   frenó (flood pequeño), el siguiente lote se separa más y da aire a la
   cuenta; si todo fue rápido, se mantiene el ritmo base de 30 s.
2. **Cierre de oleada al tocar el muro** (`apiwrap.cpp`): si un lote tardó
   ≥ 10 min (`kForwardFloodWallMs`), el servidor agotó la cuota y exige la
   espera larga. Se cierra la oleada en limpio, se muestra un toast honesto
   ("Pausa anti-flood: continúa solo en ~55 min. Restan X de N") y el resto se
   reanuda SOLO después de `kForwardFloodPauseMs = 55 min`, cuando la cuota se
   recupera ("reintentar hasta que el servidor ceda", sin quemar reintentos a
   ciegas: cada lote consecutivo recién saldría a chocar con el mismo muro).
3. **Watchdog de 2 h** (Ronda 22f) intacto: cualquier FLOOD_WAIT del servidor
   se respeta; el watchdog solo interviene en el atasco real (backoff eterno de
   fallos de red) y garantiza que la cadena siempre termine.
4. **Corte limpio al cerrar la app** (verificado, no requiere código): todo el
   estado del reenvío vive en `shared_ptr` locales de `forwardMessages`; al
   cerrar la app mueren los timers (watchdog, pacer) y el request en vuelo, sin
   dejar mitad de nada persistida → al reabrir se puede **reenviar en limpio**.

**Comportamiento esperado**: un reenvío de ~2000-3000 mensajes avanza fluido
(más de la mitad en ~30-40 min con el pacing base), el muro del servidor ya no
rompe nada (se espera y se reanuda solo) y el usuario sabe exactamente cuándo
sigue. El avance NUNCA se pierde ni se duplica (mismos randomIds por intento).

**Reversión prevista y firmemente descartada**: el límite de selección
(`MaxSelectedItems = 50000`) NO se reduce de nuevo: los logs demostraron que
un reenvío de 2334 mensajes avanza lote a lote con ese valor; el número de
selección nunca fue el mecanismo del corte de ~2000.

## Puntos de código (Ronda 23)
- `Telegram/SourceFiles/apiwrap.cpp` — `forwardMessages`:
  - constantes `kForwardChunkMinGapMs = 30 s`, `kForwardChunkMaxGapMs = 300 s`,
    `kForwardFloodWallMs = 10 min`, `kForwardFloodPauseMs = 55 min`;
  - estado `lastChunkDuration` (duración efectiva del último lote);
  - cabecera de `sendNextChunk`: `fwdGap = clamp(lastChunkDuration, 30 s, 300 s)`
    (guarda anti-underflow por `crl::time` uint64);
  - `settle()`: mide la duración y, si `>= kForwardFloodWallMs`, registra
    `[FWD WAVE …]`, muestra el toast de pausa honesta y arma
    `pacer->callOnce(kForwardFloodPauseMs)` para reanudar el resto; si no,
    continúa la cadena como siempre (Rondas 22c–22h intactas).
- `Telegram/SourceFiles/info/profile/info_profile_shared_media_classic.cpp`:
  iconos de los módulos del perfil ahora DISTINTOS (ver selección/iconos).
- `Telegram/SourceFiles/info/info.style`: `infoIconMediaAll: icon {{ "menu/all_media", infoIconFg }};`
  (icono existente reutilizado).

## Auditoría de selección desde el perfil (Ronda 23, parte B)
- El flujo es UNIFORME por módulo (Media, Fotos, Videos, Archivos, Todos,
  Fotos+Archivos): botón **Seleccionar todo** en la top bar
  (`info_top_bar.cpp` → `SelectionAction::SelectAll`), **Borrar selección**
  (`Clear`), **Reenviar** (`Forward` → `forwardSelected` →
  `ShowForwardMessagesBox`, la MISMA cadena masiva de `forwardMessages`),
  **Eliminar** (`Delete`) y contador "X de N" en vivo
  (`generateSelectedText` → `_selectedItems.title`).
- `ListWidget::selectAll()` (`info/media/info_media_list_widget.cpp:1944`)
  selecciona todo lo CARGADO en ese momento (sin tope: `_selectedLimit` = max
  int salvo el caso de álbum de historias), respetando `isMyItem`. Es el
  diseño deliberado de tdesktop: no fuerza a cargar el historial completo en
  memoria; se hace scroll (o se relega al reenvío lote a lote) para abarcar
  más.

## Iconos por módulo (Ronda 23, parte C)
Antes PhotoVideo, Fotos+Archivos y "Todos" mostraban el MISMO icono (cámara).
Ahora, reutilizando iconos ya existentes:
- Fotos y videos → `infoIconMediaPhoto` (cámara)
- Archivos → `infoIconMediaFile` (documento)
- **Fotos y archivos → `infoIconMediaVideo`** (carrete/película)
- **Todos → `infoIconMediaAll`** (cuadrícula de medios, `menu/all_media`)

## Verificación pendiente
- Compilar (usuario) y ejecutar un reenvío > 2000 esperando el muro: debe
  tocar `[FWD WAVE …]`, mostrar la pausa honesta y reanudar solo cuando toque.
- Confirmar en `depurador.txt` que tras el muro no hay `[FWD WATCHDOG]`
  (significaría que la pausa no bastó) y que el reenvío completa el 100%.