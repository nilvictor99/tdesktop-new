# Mejoras futuras — catálogo y precedente

Qué se puede mejorar AÚN, cómo hacerlo sin romper lo existente y qué riesgo
tiene. Leer antes de tocar código nuevo. Los cambios ya aplicados están en
`03-CAMBIOS.md`.

## Método para proponer/implementar una mejora (probado)

1. **Investigar primero**: localizar el flujo real (entry point UI → capa de
   datos → llamada MTP) y la causa raíz.
2. **Verificar límites MTProto**: las acciones masivas se lotear a ≤100 ids por
   request (patrón ya usado en forward y delete).
3. **Cambio mínimo y localizado**: un archivo o pocos; no cambiar firmas
   virtuales que rompan providers/otros módulos; cubrir los DOS layouts del
   perfil (tabs + classic).
4. **No compilar**: entregar al usuario para compilación y prueba.
5. **Registrar** el resultado en `03-CAMBIOS.md`.

## Pendientes (ordenadas por relación beneficio/riesgo)

1. **Barra de progreso en reenvíos masivos** — riesgo medio. Mostrar avance
   "lote 3 de 9"; `forwardMessages` ya itera lotes; añadir callback de progreso
   en `SendAction` y toast/barra en `history_view_forward_panel.cpp`.
2. **Descarga masiva de la selección** — riesgo medio. Botón "Descargar todo"
   reutilizando `DownloadManager` por item; la selección ilimitada ya existe.
3. **Invertir selección** — riesgo medio-bajo. Ctrl+Shift+I o menú; nueva
   acción en `SelectionAction` + recorrido de `_selected` (patrón de `selectAll()`).
4. **Contador "X seleccionados" refinado** — riesgo bajo. Formato "X de Y" con
   el total cargado en `generateSelectedText()` (`info_top_bar.cpp`).
5. **Búsqueda/filtro por fecha en la galería** — riesgo medio-alto. Requiere
   consultas slice por rango de fechas en `Storage::SharedMedia`; tocar solo el
   provider de media.
6. **Seleccionar todo cargando TODO el historial** — riesgo alto. Hoy marca lo
   cargado en RAM; paginaría todo el canal antes de marcar. Riesgo de memoria
   con canales enormes; último recurso.
7. **Reenviar a varios chats a la vez** — riesgo alto (descartado por ahora).
   Multiplicaría lotes y fallos; requeriría cola con reintento.

## Límites que NO se pueden saltar (honestidad técnica)

- **`noforwards`**: el servidor rechaza el forward final aunque la app permita
  seleccionar; salvado con el fallback "reenviar como copia" (Ronda 22b).
- **Límite 100/request**: aplica a forward y delete (ya loteado). Cualquier
  método masivo nuevo debe asumir lo mismo.
- Copiar texto/guardar archivos sí funcionan porque esos flujos no se validan
  en el servidor.