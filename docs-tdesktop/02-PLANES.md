# Planes — diseños previos de módulos ya implementados

Documentos de diseño **completados** (✅). Sirven de contexto técnico; el
registro del cambio aplicado está en `03-CAMBIOS.md` (rondas referenciadas).

---

## Plan original — Galería y reenvío/borrado masivo (✅ Rondas 1–6)

### Objetivos (todos cumplidos)
- **Galería Media única**: botón `PhotoVideo` en el perfil (incl. layout classic),
  con submenú **Todo / Fotos / Videos** (sin GIF), default "Todo".
- **Selección ilimitada**: `_selectedLimit = max`, con Shift+clic (ancla + rango)
  y "Seleccionar todo" (Ctrl+A, menú, botón top bar con check circular).
- **Orden cronológico** en reenvío: `ranges::sort` ascendente en `collectSelectedIds`.
- **Reenvío por lotes de 100** (`kMaxForwardMessages = 100`) con corte por peer.
- **Recordar opciones de reenvío**: prefs `forward_drop_names` / `forward_drop_captions`.
- **Borrado masivo fiable**: `deleteMessages` en lotes de 100 (evita reapariciones).

### Principios
Cambios mínimos y localizados; sin tocar build/CMake/scripts; el agente no
compila; investigar causa raíz antes de parchear; no romper lo que funciona.

### Lecciones aprendidas
1. MTProto limita a **100 IDs por request** (forward y delete); cualquier acción
   masiva nueva debe lotearse.
2. El perfil del canal usa DOS layouts (tabs + classic); un cambio de botones
   debe cubrir ambos.
3. Los iconos nuevos requieren SVG en `Telegram/Resources/icons/` antes de
   referenciarlos en `.style`.
4. Las keys de idioma nuevas van a `Telegram/Resources/langs/lang.strings`.

---

## Plan — Módulo "Todo" (All) (✅ Ronda 9)

Cuadrícula única con **fotos + videos + archivos** (excluye stickers/GIF/voz/
música/round/links), carga completa y orden cronológico. Claves del diseño:

- `All` = nuevo `Storage::SharedMediaType` (índice 14, antes de `kCount`) →
  su propia lista `SparseIdsList` independiente. `base::enum_mask` aguanta 15
  valores sin overflow.
- No existe filtro MTP único foto+video+documento → `requestSharedMediaAll`
  lanza **3 búsquedas paralelas** (Photos + Video + Documents) y fusiona
  `fullCount` + `messageIds`, con el mismo truco `noSkipRange` del lazy loading
  de FilesPhotos. `ParseSearchResult` valida cada sub-resultado con SU sub-tipo.
- Stickers nunca llegan: máscara vacía de `MediaFile::sharedMediaTypes()`; además
  `IsItemGoodForType(All)` descarta `document->sticker()`.
- Archivos tocados: `storage_shared_media.h` (enum), `data_media_types.cpp`
  (máscaras), `apiwrap.*` (3 búsquedas + merge), provider (switches + layout
  híbrido foto/video/docs hideName), títulos (`lng_media_type_all`,
  `lng_profile_all`, `lng_media_all_empty`), botón y contador del perfil
  (Photo+Video+File).
- Decisiones: sin zoom de cuadrícula ni búsqueda; NO en tabs experimentales
  (como FilesPhotos); global media no afectado.

---

## Plan — Carga completa de FilesPhotos/All (✅ Rondas 11–19.7)

### Problema
En canales grandes, scroll de FilesPhotos/All dejaba **huecos** (zonas solo
fotos o solo archivos). La cuadrícula omite la celda si el `HistoryItem` no está
materializado en memoria (`createLayout` → `session.data().message(id)` == null).

### Evolución de la solución (ver detalle en 03-CAMBIOS)
- **Ronda 11** — auto-reparación: ante un id sin materializar se pide una
  búsqueda `Around` de ese id (repara storage). No bastaba con varios huecos.
- **Ronda 12** — materialización masiva: pedir **exactamente los ids faltantes**
  vía `requestMessageData` (agrupa en `messages.getMessages`). Search de ids a la vez.
- **Ronda 13** — batching encadenado de ≤100 ids (límite de la API).
- **Ronda 14** — blindaje `PEER_ID_INVALID`/red: si todas las sub-búsquedas
  fallan se omite la actualización (no envenenar el loader con fin de lista falso).
- **Ronda 16** — rango honesto `{min,max}` por lote (evita `skippedAfter=0`
  fantasma) + MERGE-SKIP en lote vacío sin inicio alcanzado.
- **Ronda 17** — merge forzado en `applyUpdate` (`needMergeMessages = ... || update.messages`) → el `base::flat_set::merge` idempotente acumula todas las páginas (fin del bucle de carga).
- **Ronda 18** — getMessages propios por lotes de ≤100 (`kMaxMaterializeBatch`)
  en vez de `requestMessageData` (resuelve el límite de ids por petición en
  canales grandes) + tope de reintentos (3).
- **Ronda 18b/c** — ventana de entrada grande (`kFullIdsLimit = 400`) y reset
  de `_missingRetries` al scrollear.
- **Ronda 19.2** — `kFullIdsLimit` 400→1000 (≈2000 ids): el front del sparse
  builder llega al inicio real.
- **Ronda 19.5** — `checkPreload` conserva `kFullIdsLimit` durante el scroll
  (antes encogía la ventana a ~128 → congelamiento).
- **Ronda 19.6** — materialización **proactiva**: en `refreshViewer`, para
  FilesPhotos/All, `requestMissingAround()` antes de refrescar la slice.
- **Ronda 19.7** — paralelismo acotado (`kMaxMaterializeParallel = 3` lotes en
  vuelo) → ~3x más rápido, sin congelarse.

### Seguridad general (se mantiene)
Guards `_missingRequested` (una pasada a la vez) y `_missingRetries`
(reset por slice); call-backs protegidos contra use-after-free
(`shared_ptr<bool> alive` + `_lifetime`); lote vacío sin inicio → MERGE-SKIP;
Media y Archivos NO se tocan.