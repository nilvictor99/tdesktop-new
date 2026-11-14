# Plan de implementación — Galería y reenvío/borrado masivo (COMPLETADO)

> **ESTADO: ✅ IMPLEMENTADO Y VERIFICADO (agosto 2026).** Este documento se
> conserva como registro técnico de cómo se hizo cada cambio. El resumen
> actual para usuarios está en [12-CAMBIOS-RONDAS-1-22B.md](12-CAMBIOS-RONDAS-1-22B.md).

**Repositorio:** `C:\Users\VICTOR\code\tdesktop`
(versión 1 eliminada; todo fue portado y verificado aquí)

---

## 1. Objetivos (todos cumplidos)

### A. Perfil: un único acceso "Media" — ✅
- Botón único `PhotoVideo` en el perfil, incluido el layout classic del canal.
- Toggle opcional de división (`MediaTabsExpanded`) queda desactivado.

### B. Submenú Todo / Fotos / Videos — ✅
- Slider superior en la galería unificada, sin GIF, default "Todo".
- Filtro aplicado en vivo desde el provider (`matchesFilter`).

### C. Selección ilimitada — ✅
- `_selectedLimit = std::numeric_limits<int>::max()`.

### D. Selección por rango (Shift+clic) — ✅
- Ancla + rango continuo con `_selectionAnchor` / `selectRange`.

### E. Seleccionar todo — ✅
- Ctrl+A (`info_wrap_widget.cpp`) + menú contextual + botón top bar con
  icono check circular (`toast/check-36x36`).
- Key propia añadida: `lng_context_select_all`.

### F. Reenvío por lotes de 100 cronológico — ✅
- `apiwrap.cpp`: `kMaxForwardMessages = 100` + corte por tamaño o cambio de peer.
- Orden ascendente garantizado en `collectSelectedIds`.

### G. Recordar opciones de reenvío — ✅
- Prefs `forward_drop_names` / `forward_drop_captions`.

### H. Borrado masivo fiable (añadido posterior) — ✅
- `data_histories.cpp`: `deleteMessages` dividido en lotes de 100.
- Corrige la reaparición de mensajes tras borrar selecciones grandes
  (el servidor solo procesa ~100 IDs por petición).

---

## 2. Principios aplicados

- Cambios mínimos y localizados; sin tocar build/CMake/scripts.
- El agente nunca compila; el usuario compila y reporta errores.
- Investigar causa raíz ANTES de corregir; no parchear síntomas.
- No romper funcionalidad existente (selección, filtros, providers ajenos).

## 3. Lecciones aprendidas (para futuros cambios)

1. **El servidor MTProto limita a 100 IDs por request** tanto en forward como
   en delete. Cualquier acción masiva nueva (fijar, copiar links…) debe lotearse.
2. El perfil del canal usa DOS layouts: tabs modernos y botones classic
   (`info_profile_shared_media_classic.cpp`). Un cambio de botones debe cubrir ambos.
3. Los iconos nuevos requieren que el SVG exista en `Telegram/Resources/icons/`;
   verificar antes de referenciarlos en `.style`.
4. Las keys de idioma inexistentes hay que añadirlas a
   `Telegram/Resources/langs/lang.strings`.
