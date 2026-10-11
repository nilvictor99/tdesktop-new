# Prompt de trabajo — módulo combinado "Fotos + Archivos" (FilesPhotos)

Documento histórico: es el encargo original tal como se escribió, sin editar.
Sirve para reconstruir el contexto y las restricciones del diseño.
Referencia de estado: Ronda 7 (módulo) y Rondas 11-19.7 (carga completa) en
`03-CAMBIOS.md`; plan completo en `15-PLAN-FIX-CARGA-COMPLETA.md`.

---

Prompt:
You are an expert C++ developer working on a fork of Telegram Desktop (tdesktop) from https://github.com/telegramdesktop/tdesktop. The project uses C++ (Qt), with the shared media system located mainly under Telegram/SourceFiles/info/media/.
Context of the custom feature
I added a new combined module in channel/group profiles (Shared Media section) called something like “Photos/Files” or “Media Files”.

It must show only photos + all types of files/documents (PDFs, archives, etc.) without distinction, in a single grid/list view.

It reuses / extends the existing Info::Media::ListWidget + Info::Media::Provider infrastructure (see info_media_list_widget.*, info_media_provider.*, info_media_inner_widget.*, info_media_list_section.*).
Observed bugs (from logs + screenshots)

Incomplete loading in the new module:
After the first batch of thumbnails the list stops. More files/photos exist in the chat but never appear in the module.
Example: a channel shows only PDF icons (red download placeholders). When the user opens the chat or interacts, photos suddenly appear → loading is incomplete or is being cancelled too early.

Dynamic / infinite scroll is broken or cut off:
Scrolling down should keep requesting and appending more media items (photos + files) until everything is loaded.
Currently the load stops / the request chain closes, so the module never shows the full set.

Related errors visible in log.txt:
Multiple RPC Error: ... code 400, error PEER_ID_INVALID
TOP_MSG_ID_NOT_SUPPORTED
MTP “could not find request dc for delayed resend”
Streaming errors (avformat_open_input)


These errors often appear with invalid/departed peers or when the Shared Media request uses incorrect message IDs / peer context. There is a known related crash in upstream (issue #31030) when opening the Files tab with invalid senders.
Requirements for the fix

Make the loading of the new combined Photos+Files module fully dynamic and complete:
Use the same pagination / “load more on scroll” pattern that the official Photos, Videos, Files, Music tabs already use.
Keep requesting the next slice of shared media (via the existing MTP shared media APIs) as the user scrolls, until the server returns no more items.
Never cut the loading chain early; keep it alive while the ListWidget is visible.

The module must continue to filter only photos + documents/files (no pure videos, voice messages, etc. unless they are also files).
Do not break the existing separate tabs (Photos, Videos, Files, Music, Links, Voice, etc.).
Handle gracefully:
Invalid / left / deleted peers (PEER_ID_INVALID) → skip or show a safe placeholder instead of crashing or stopping the whole load.
TOP_MSG_ID_NOT_SUPPORTED and delayed-resend MTP problems.

Prefer reusing the existing Info::Media::Provider / ListWidget / section machinery rather than inventing a completely new loader.
Keep memory usage reasonable (lazy thumbnail loading, no loading every full-resolution file into RAM at once).
The grid must show correct thumbnails for both photos and file types (PDF icons, etc.) and allow opening them from the profile.

What to investigate / change

How the official providers decide the SharedMediaType / filter and how they request the next page (messages.search / shared media slice requests).
The scroll → “need more items” signal path inside Info::Media::ListWidget and Provider.
Any place where a combined type (photos + files) is constructed – make sure the type mask and the “has more” / “continue loading” logic is correct.
Error handling around peer resolution when building the list items (especially document rows that show sender info).
Lifetime of the media request / subscription so it is not cancelled when the first page finishes.

Deliverable
Produce a clear, minimal set of code changes (or a detailed patch description) that:

Fixes the incomplete / cut-off loading.
Makes scrolling load all remaining photos + files dynamically and completely.
Keeps the new module stable and does not regress the rest of the Shared Media UI.
Handles the PEER_ID_INVALID / TOP_MSG_ID_NOT_SUPPORTED cases safely.

Base the solution on the current architecture of info/media/ so the change stays maintainable with future upstream merges.