/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "info/media/info_media_common.h"
#include "data/data_shared_media.h"

namespace Info {
class AbstractController;
} // namespace Info

namespace Info::Media {

class Provider final : public ListProvider, private ListSectionDelegate {
public:
	explicit Provider(not_null<AbstractController*> controller);

	Type type() override;
	bool hasSelectRestriction() override;
	rpl::producer<bool> hasSelectRestrictionChanges() override;
	bool isPossiblyMyItem(not_null<const HistoryItem*> item) override;

	std::optional<int> fullCount() override;

	void restart() override;
	void checkPreload(
		QSize viewport,
		not_null<BaseLayout*> topLayout,
		not_null<BaseLayout*> bottomLayout,
		bool preloadTop,
		bool preloadBottom) override;
	void refreshViewer() override;
	rpl::producer<> refreshed() override;

	std::vector<ListSection> fillSections(
		not_null<Overview::Layout::Delegate*> delegate) override;
	rpl::producer<not_null<BaseLayout*>> layoutRemoved() override;
	BaseLayout *lookupLayout(const HistoryItem *item) override;
	bool isMyItem(not_null<const HistoryItem*> item) override;
	bool isAfter(
		not_null<const HistoryItem*> a,
		not_null<const HistoryItem*> b) override;

	void setSearchQuery(QString query) override;

	void jumpToMessage(MsgId, Fn<void(FullMsgId)> done) override;

	void setMediaFilter(MediaFilter filter) override;
	[[nodiscard]] MediaFilter mediaFilter() const override { return _filter; }
	[[nodiscard]] bool supportsMediaFilter() const override;
	void setFileGridColumns(int columns) override;

	[[nodiscard]] bool anchorWhileAtTop() override;

	ListItemSelectionData computeSelectionData(
		not_null<const HistoryItem*> item,
		TextSelection selection) override;
	void applyDragSelection(
		ListSelectedMap &selected,
		not_null<const HistoryItem*> fromItem,
		bool skipFrom,
		not_null<const HistoryItem*> tillItem,
		bool skipTill) override;

	bool allowSaveFileAs(
		not_null<const HistoryItem*> item,
		not_null<DocumentData*> document) override;
	QString showInFolderPath(
		not_null<const HistoryItem*> item,
		not_null<DocumentData*> document) override;

	int64 scrollTopStatePosition(not_null<HistoryItem*> item) override;
	HistoryItem *scrollTopStateItem(ListScrollTopState state) override;
	void saveState(
		not_null<Memento*> memento,
		ListScrollTopState scrollState) override;
	void restoreState(
		not_null<Memento*> memento,
		Fn<void(ListScrollTopState)> restoreScrollState) override;

private:
	static constexpr auto kMinimalIdsLimit = 16;
	static constexpr auto kDefaultAroundId = (ServerMaxMsgId - 1);
	// Para FilesPhotos/All pedimos y materializamos una ventana grande de
	// entrada (en vez de arrancar en 16), de modo que el módulo compuesto
	// muestre todo su contenido sin huecos ni límites visibles, igual que el
	// módulo Media. La materialización por lotes de Ronda 18 ya hace seguro
	// procesar esta ventana sin superar el límite de ids por petición del API.
	// Subido a 1000 (window ≈ 2000 ids): en la unión, SparseIdsSliceBuilder poda
	// la slice a limitBefore+limitAfter alrededor del around; una ventana
	// pequeña recortaba el front y la petición "Before" no avanzaba hacia el
	// inicio real (el log mostraba skipBefore≈13000 sin bajar), dejando archivos
	// antiguos sin mostrar. Con esta ventana el front avanza mucho más por página.
	static constexpr auto kFullIdsLimit = 1000;

	bool sectionHasFloatingHeader() override;
	QString sectionTitle(not_null<const BaseLayout*> item) override;
	bool sectionItemBelongsHere(
		not_null<const BaseLayout*> item,
		not_null<const BaseLayout*> previous) override;

	[[nodiscard]] bool isPossiblyMyPeerId(PeerId peerId) const;
	[[nodiscard]] FullMsgId computeFullId(UniversalMsgId universalId) const;
	[[nodiscard]] bool matchesFilter(
		not_null<const HistoryItem*> item) const;
	[[nodiscard]] BaseLayout *getLayout(
		UniversalMsgId universalId,
		not_null<Overview::Layout::Delegate*> delegate);
	[[nodiscard]] std::unique_ptr<BaseLayout> createLayout(
		UniversalMsgId universalId,
		not_null<Overview::Layout::Delegate*> delegate,
		Type type);

	[[nodiscard]] SparseIdsMergedSlice::Key sliceKey(
		UniversalMsgId universalId) const;

	void requestMissingAround(UniversalMsgId universalId);
	void itemRemoved(not_null<const HistoryItem*> item);
	void markLayoutsStale();
	void clearStaleLayouts();

	const not_null<AbstractController*> _controller;

	const not_null<PeerData*> _peer;
	const MsgId _topicRootId = 0;
	const PeerId _monoforumPeerId = 0;
	PeerData * const _migrated = nullptr;
	const Type _type = Type::Photo;
	MediaFilter _filter = MediaFilter::All;

	UniversalMsgId _universalAroundId = kDefaultAroundId;
	int _idsLimit = kMinimalIdsLimit;
	// Columnas forzadas del módulo "Archivos": 0 = lista, >1 = galería. Solo en
	// galería se reconstruyen los layouts con el nombre del archivo oculto.
	int _fileGridColumns = 0;
	SparseIdsMergedSlice _slice;

	// Advanced self-healing for composite types (FilesPhotos/All): a slice may
	// contain universal ids whose HistoryItem is not materialized in memory
	// (large channels, or the History was unloaded elsewhere). Instead of
	// re-requesting a single composite search around one id (slow, one gap at a
	// time), we collect every un-materialized id of the current slice and
	// materialize them all at once through api().requestMessageData() (which
	// batches the ids into messages.getMessages/channels.getMessages). When all
	// the responses arrive we rebuild, so the whole visible region fills in a
	// single pass. Guarded: one materialization pass in flight at a time
	// (cleared when the materialized responses arrive).
	bool _missingRequested = false;
	// Cuenta cuántas pasadas de materialización seguidas hemos hecho para
	// llenar ids que siguen sin HistoryItem. Se limita para no reintentar en
	// bucle infinito cuando un id no existe de verdad (p.ej. borrado). Con la
	// ventana pequeña de scroll cada pase es de 1-3 lotes y se refresca por
	// zona, tolerando fallos transitorios sin congelar la cuadrícula.
	int _missingRetries = 0;
	static constexpr int kMaxMissingRetries = 8;

	std::unordered_map<UniversalMsgId, CachedItem> _layouts;
	rpl::event_stream<not_null<BaseLayout*>> _layoutRemoved;
	rpl::event_stream<> _refreshed;

	rpl::lifetime _lifetime;
	rpl::lifetime _viewerLifetime;

};

} // namespace Info::Media
