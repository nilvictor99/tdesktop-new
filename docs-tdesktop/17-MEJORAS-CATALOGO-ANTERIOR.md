# Mejoras futuras — Catálogo y precedente

Qué se puede mejorar AÚN, cómo hacerlo sin romper lo existente, y qué riesgo tiene.
Este documento es el punto de partida para cualquier nueva mejora: leer antes de
tocar código. Los cambios ya aplicados están en [12-CAMBIOS-RONDAS-1-22B.md](12-CAMBIOS-RONDAS-1-22B.md).

---

## Cómo proponer/implementar una mejora nueva (método probado)

1. **Investigar primero**: localizar el flujo real en el código (grep del entry
   point UI → capa de datos → llamada MTP) y entender la causa raíz.
2. **Verificar límites del servidor MTProto**: cualquier acción masiva debe
   lotearse a ≤100 IDs por request (patrón ya usado en forward y delete).
3. **Cambio mínimo y localizado**: un archivo o pocos; no cambiar firmas
   virtuales que rompan providers/otros módulos; cubrir los DOS layouts del
   perfil (tabs modernos + classic).
4. **No compilar**: entregar al usuario para compilación y prueba.
5. **Registrar** el resultado en `11-DIARIO-RONDAS.md` y `12-CAMBIOS-RONDAS-1-22B.md`.

---

## Pendientes (ordenadas por relación beneficio/riesgo)

### 1. Barra de progreso en reenvíos masivos — riesgo medio
- **Qué**: mostrar avance "lote 3 de 9" mientras se envían los grupos de 100.
- **Cómo**: `ApiWrap::forwardMessages` ya itera lotes (`kMaxForwardMessages`);
  añadir callback de progreso opcional en `SendAction` y un toast/barra en
  `history_view_forward_panel.cpp`. No tocar la cola MTP.

### 2. Descarga masiva de la selección — riesgo medio
- **Qué**: botón "Descargar todo" para los items seleccionados a una carpeta.
- **Cómo**: reutilizar el flujo existente de descargas (`DownloadManager`) por
  item; la selección ilimitada ya existe en `info_media_list_widget.*`.
- **Ojo**: no bloquear la UI; las descargas ya van en colas propias.

### 3. Invertir selección (atajo/menú) — riesgo medio-bajo
- **Qué**: Ctrl+Shift+I o menú → desmarcar lo marcado y viceversa.
- **Cómo**: nueva acción en `SelectionAction` + recorrido de `_selected`
  vs items visibles en `info_media_list_widget.cpp` (mismo patrón que
  `selectAll()`, ya probado).

### 4. Contador "X seleccionados" refinado — riesgo bajo
- **Qué**: el top bar ya muestra `_selectionText`; asegurar formato
  "X de Y" con el total cargado.
- **Cómo**: solo texto en `generateSelectedText()` (`info_top_bar.cpp`).

### 5. Búsqueda/filtro por fecha en la galería — riesgo medio-alto
- **Qué**: saltar a un mes concreto desde la galería.
- **Cómo**: requiere consultas slice por rango de fechas (capa
  `Storage::SharedMedia`); tocar solo el provider de media, nunca los slices base.

### 6. Seleccionar todo cargando TODO el historial — riesgo alto
- **Qué**: hoy "Seleccionar todo" marca lo cargado en RAM; esta mejora paginaría
  todo el canal antes de marcar.
- **Cómo**: encadenar requests de shared-media hasta agotar, luego selectAll.
- **Riesgo**: memoria con canales enormes; mantener como último recurso.

### 7. Reenviar a varios chats a la vez — riesgo alto (descartado por ahora)
- **Qué**: múltiples destinatarios en una operación.
- **Por qué espera**: multiplica los lotes y los fallos del servidor; requeriría
  cola con reintento. Revisitar solo si hay necesidad real.

---

## Ya aplicadas (referencia rápida)

| Mejora | Dónde quedó |
|---|---|
| Copiar/guardar/descargar desbloqueados | restricciones locales eliminadas |
| Selección ilimitada + Shift+clic + Ctrl+A + botón check | `info_media_list_widget.*`, `info_wrap_widget.cpp`, `info_top_bar.*`, `info.style` |
| Galería unificada Media + filtros Todo/Fotos/Videos | `info_profile_shared_media_classic.cpp`, `info_media_inner_widget.*`, provider |
| Reenvío por lotes de 100 cronológico | `apiwrap.cpp`, `collectSelectedIds` |
| Borrado por lotes de 100 (sin reapariciones) | `data_histories.cpp` |
| Recordar opciones de reenvío | `history_view_forward_panel.*`, `window_peer_menu.cpp` |
| Descargar siempre visible / Ctrl+Shift+G | `media_view_overlay_widget.cpp` |

## Límites que NO se pueden saltar (honestidad técnica)

- **noforwards**: el servidor rechaza el forward final aunque la app permita
  seleccionar. No es corregible del lado cliente.
- **Límite 100/request**: aplica a forward y delete (ya loteado). Cualquier
  método masivo nuevo asumir lo mismo.
- Copiar texto/guardar archivos sí funcionan porque esos flujos no se validan
  en servidor.
