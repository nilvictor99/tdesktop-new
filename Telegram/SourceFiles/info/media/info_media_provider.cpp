/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "info/media/info_media_provider.h"

#include "apiwrap.h"
#include "debug_depurador.h"
#include "info/media/info_media_widget.h"
#include "info/media/info_media_list_section.h"
#include "info/info_controller.h"
#include "layout/layout_selection.h"
#include "main/main_app_config.h"
#include "main/main_session.h"
#include "lang/lang_keys.h"
#include "logs.h"
#include <functional>
#include <limits>
#include <memory>
#include "history/history.h"
#include "history/history_item.h"
#include "history/history_item_helpers.h"
#include "data/data_session.h"
#include "data/data_chat.h"
#include "data/data_channel.h"
#include "data/data_forum_topic.h"
#include "data/data_user.h"
#include "data/data_peer_values.h"
#include "data/data_document.h"
#include "data/data_saved_sublist.h"
#include "storage/storage_facade.h"
#include "storage/storage_shared_media.h"
#include "styles/style_overview.h"

namespace Info::Media {
namespace {

constexpr auto kPreloadedScreensCount = 4;
constexpr auto kPreloadedScreensCountFull
	= kPreloadedScreensCount + 1 + kPreloadedScreensCount;

} // namespace

Provider::Provider(not_null<AbstractController*> controller)
: _controller(controller)
, _peer(_controller->key().peer())
, _topicRootId(_controller->key().topic()
	? _controller->key().topic()->rootId()
	: MsgId())
, _monoforumPeerId(_controller->key().sublist()
	? _controller->key().sublist()->sublistPeer()->id
	: PeerId())
, _migrated(_controller->migrated())
, _type(_controller->section().mediaType())
, _slice(sliceKey(_universalAroundId)) {
	if (_type == Type::FilesPhotos || _type == Type::All) {
		// Ventana grande de entrada para que el módulo compuesto cargue todo
		// su contenido de una vez (ver kFullIdsLimit).
		_idsLimit = kFullIdsLimit;
	}
	_controller->session().data().itemRemoved(
	) | rpl::on_next([this](auto item) {
		itemRemoved(item);
	}, _lifetime);

	style::PaletteChanged(
	) | rpl::on_next([=] {
		for (auto &layout : _layouts) {
			layout.second.item->invalidateCache();
		}
	}, _lifetime);

	_controller->session().appConfig().ignoredRestrictionReasonsChanges(
	) | rpl::on_next([=](std::vector<QString> &&changed) {
		const auto sensitive = Data::UnavailableReason::Sensitive();
		if (ranges::contains(changed, sensitive.reason)) {
			for (auto &[id, layout] : _layouts) {
				layout.item->maybeClearSensitiveSpoiler();
			}
		}
	}, _lifetime);

	if (_type == Type::FilesPhotos || _type == Type::All) {
		Depur::append(
			QStringLiteral("[DEPUR] PROVIDER ENTER peer=%1(%2) type=%3 "
				"topic=%4 sublist=%5 around=%6").arg(
				Depur::num(_peer->id.value))
			.arg(_peer->name())
			.arg(int(_type))
			.arg(Depur::num(_topicRootId.bare))
			.arg(Depur::num(_monoforumPeerId.value))
			.arg(Depur::num(_universalAroundId.bare)));
	}
}

Type Provider::type() {
	return _type;
}

bool Provider::hasSelectRestriction() {
	if (_peer->session().frozen()) {
		return true;
	} else if (_peer->allowsForwarding()) {
		return false;
	} else if (const auto chat = _peer->asChat()) {
		return !chat->canDeleteMessages();
	} else if (const auto channel = _peer->asChannel()) {
		return !channel->canDeleteMessages();
	}
	return true;
}

rpl::producer<bool> Provider::hasSelectRestrictionChanges() {
	if (const auto user = _peer->asUser()) {
		return rpl::combine(
			Data::PeerFlagValue(user, UserDataFlag::NoForwardsMyEnabled),
			Data::PeerFlagValue(user, UserDataFlag::NoForwardsPeerEnabled)
		) | rpl::map([=] {
			return hasSelectRestriction();
		}) | rpl::distinct_until_changed() | rpl::skip(1);
	}
	const auto chat = _peer->asChat();
	const auto channel = _peer->asChannel();
	auto noForwards = chat
		? Data::PeerFlagValue(chat, ChatDataFlag::NoForwards)
		: Data::PeerFlagValue(
			channel,
			ChannelDataFlag::NoForwards
		) | rpl::type_erased;

	auto rights = chat
		? chat->adminRightsValue()
		: channel->adminRightsValue();
	auto canDelete = std::move(
		rights
	) | rpl::map([=] {
		return chat
			? chat->canDeleteMessages()
			: channel->canDeleteMessages();
	});
	return rpl::combine(
		std::move(noForwards),
		std::move(canDelete)
	) | rpl::map([=] {
		return hasSelectRestriction();
	}) | rpl::distinct_until_changed() | rpl::skip(1);
}

bool Provider::sectionHasFloatingHeader() {
	switch (_type) {
	case Type::Photo:
	case Type::FilesPhotos:
	case Type::All:
	case Type::GIF:
	case Type::Video:
	case Type::PhotoVideo:
	case Type::RoundFile:
	case Type::RoundVoiceFile:
	case Type::MusicFile:
		return false;
	case Type::File:
	case Type::Link:
		return true;
	}
	Unexpected("Type in HasFloatingHeader()");
}

QString Provider::sectionTitle(not_null<const BaseLayout*> item) {
	switch (_type) {
	case Type::Photo:
	case Type::FilesPhotos:
	case Type::All:
	case Type::GIF:
	case Type::Video:
	case Type::PhotoVideo:
	case Type::RoundFile:
	case Type::RoundVoiceFile:
	case Type::File:
		return langMonthFull(item->dateTime().date());

	case Type::Link:
		return langDayOfMonthFull(item->dateTime().date());

	case Type::MusicFile:
		return QString();
	}
	Unexpected("Type in ListSection::setHeader()");
}

bool Provider::sectionItemBelongsHere(
		not_null<const BaseLayout*> item,
		not_null<const BaseLayout*> previous) {
	const auto date = item->dateTime().date();
	const auto sectionDate = previous->dateTime().date();

	switch (_type) {
	case Type::Photo:
	case Type::FilesPhotos:
	case Type::All:
	case Type::GIF:
	case Type::Video:
	case Type::PhotoVideo:
	case Type::RoundFile:
	case Type::RoundVoiceFile:
	case Type::File:
		return date.year() == sectionDate.year()
			&& date.month() == sectionDate.month();

	case Type::Link:
		return date == sectionDate;

	case Type::MusicFile:
		return true;
	}
	Unexpected("Type in ListSection::belongsHere()");
}

bool Provider::isPossiblyMyItem(not_null<const HistoryItem*> item) {
	return isPossiblyMyPeerId(item->history()->peer->id);
}

bool Provider::isPossiblyMyPeerId(PeerId peerId) const {
	return (peerId == _peer->id) || (_migrated && peerId == _migrated->id);
}

std::optional<int> Provider::fullCount() {
	return _slice.fullCount();
}

void Provider::restart() {
	_layouts.clear();
	_universalAroundId = kDefaultAroundId;
	_idsLimit = (_type == Type::FilesPhotos || _type == Type::All)
		? kFullIdsLimit
		: kMinimalIdsLimit;
	_slice = SparseIdsMergedSlice(sliceKey(_universalAroundId));
	Depur::append(
		QStringLiteral("[DEPUR] RESTART type=%1 around=%2 limit=%3")
		.arg(int(_type))
		.arg(Depur::num(_universalAroundId.bare))
		.arg(Depur::num(_idsLimit)));
	refreshViewer();
}

void Provider::checkPreload(
		QSize viewport,
		not_null<BaseLayout*> topLayout,
		not_null<BaseLayout*> bottomLayout,
		bool preloadTop,
		bool preloadBottom) {
	const auto visibleWidth = viewport.width();
	const auto visibleHeight = viewport.height();
	const auto preloadedHeight = kPreloadedScreensCountFull * visibleHeight;
	const auto minItemHeight = MinItemHeight(_type, visibleWidth);
	const auto preloadedCount = preloadedHeight / minItemHeight;
	const auto preloadIdsLimitMin = (preloadedCount / 2) + 1;
	const auto preloadIdsLimit = preloadIdsLimitMin
		+ (visibleHeight / minItemHeight);
	const auto after = _slice.skippedAfter();
	const auto topLoaded = after && (*after == 0);
	const auto before = _slice.skippedBefore();
	const auto bottomLoaded = before && (*before == 0);

	const auto minScreenDelta = kPreloadedScreensCount
		- kPreloadIfLessThanScreens;
	const auto minUniversalIdDelta = (minScreenDelta * visibleHeight)
		/ minItemHeight;
	const auto preloadAroundItem = [&](not_null<BaseLayout*> layout) {
		auto preloadRequired = false;
		auto universalId = GetUniversalId(layout);
		if (!preloadRequired) {
			preloadRequired = (_idsLimit < preloadIdsLimitMin);
		}
		if (!preloadRequired) {
			auto delta = _slice.distance(
				sliceKey(_universalAroundId),
				sliceKey(universalId));
			Assert(delta != std::nullopt);
			preloadRequired = (qAbs(*delta) >= minUniversalIdDelta);
		}
		if (preloadRequired) {
			// Los módulos compuestos (FilesPhotos/All) mantienen la ventana
			// GRANDE de entrada también al hacer scroll (en lugar de encogerla a
			// la mínima de preload ~128). Si se encoge, al llegar a una zona del
			// historial cuyos ids aún no están cacheados la cuadrícula se queda
			// sin contenido y el muestreo "se congela", y solo se rellena al
			// saltar a un mensaje (que sí usa kFullIdsLimit). Mantener la
			// ventana grande hace que el sparse-builder pida y cachee muy por
			// delante, igual que al arrancar, evitando el congelamiento.
			_idsLimit = (_type == Type::FilesPhotos || _type == Type::All)
				? kFullIdsLimit
				: preloadIdsLimit;
			_universalAroundId = universalId;
			Depur::append(
				QStringLiteral("[DEPUR] PRELOAD peer=%1 type=%2 around=%3 "
					"limit=%4 top=%5 / bottom=%6").arg(
					Depur::num(_peer->id.value))
				.arg(int(_type))
				.arg(Depur::num(_universalAroundId.bare))
				.arg(Depur::num(_idsLimit))
				.arg(preloadTop ? 1 : 0)
				.arg(preloadBottom ? 1 : 0));
			refreshViewer();
		}
	};

	if (preloadTop && !topLoaded) {
		preloadAroundItem(topLayout);
	} else if (preloadBottom && !bottomLoaded) {
		preloadAroundItem(bottomLayout);
	}
}

void Provider::refreshViewer() {
	_viewerLifetime.destroy();
	const auto idForViewer = sliceKey(_universalAroundId).universalId;
	_controller->mediaSourceOfType(
		_controller->section().mediaType(),
		idForViewer,
		_idsLimit,
		_idsLimit
	) | rpl::on_next([=](SparseIdsMergedSlice &&slice) {
		if (!slice.fullCount()) {
			// Don't display anything while full count is unknown.
			return;
		}
		_slice = std::move(slice);
		if (auto nearest = _slice.nearest(idForViewer)) {
			_universalAroundId = GetUniversalId(*nearest);
		}
		// Nueva región al scrollear: presupuesto de materialización fresco, para
		// que cada zona que el usuario recorre rellene sus huecos aunque zonas
		// anteriores hubieran agotado el tope de reintentos. Sin esto, en
		// canales con mucha media el contador llega a su tope y toda zona nueva
		// quedaba con huecos permanentes hasta abrir el chat.
		_missingRequested = false;
		_missingRetries = 0;
		if (_type == Type::FilesPhotos || _type == Type::All) {
			const auto skippedBefore = _slice.skippedBefore();
			const auto skippedAfter = _slice.skippedAfter();
			Depur::append(
				QStringLiteral("[DEPUR] REFRESH type=%1 slice=%2 full=%3 "
					"around=%4 skipBefore=%5 skipAfter=%6").arg(int(_type))
				.arg(Depur::num(_slice.size()))
				.arg(Depur::num(_slice.fullCount().value_or(0)))
				.arg(Depur::num(_universalAroundId.bare))
				.arg(skippedBefore.has_value()
					? Depur::num(*skippedBefore)
					: QStringLiteral("?"))
				.arg(skippedAfter.has_value()
					? Depur::num(*skippedAfter)
					: QStringLiteral("?")));
		}
		if (_type == Type::FilesPhotos || _type == Type::All) {
			// Materialización PROACTIVA: nada más llegar la slice, pedimos que
			// se materialicen TODOS sus ids sin HistoryItem (lotes ≤100). Así la
			// ventana grande se rellena entera de una vez y no quedan archivos
			// que solo aparecían al usar "mostrar en el chat" (que re-materializa
			// el Historial). El guard _missingRequested evita duplicar peticiones
			// y _missingRetries (reset por slice) evita bucles con ids borrados.
			requestMissingAround(_universalAroundId);
		}
		_refreshed.fire({});
	}, _viewerLifetime);
}

rpl::producer<> Provider::refreshed() {
	return _refreshed.events();
}

void Provider::setMediaFilter(MediaFilter filter) {
	_filter = filter;
	_refreshed.fire({});
}

bool Provider::supportsMediaFilter() const {
	return _type == Type::PhotoVideo;
}

bool Provider::matchesFilter(not_null<const HistoryItem*> item) const {
	switch (_filter) {
	case MediaFilter::All:
		return true;
	case MediaFilter::Photos:
		return item->media() && item->media()->photo() != nullptr;
	case MediaFilter::Videos:
		return item->media()
			&& item->media()->document()
			&& item->media()->document()->isVideoFile();
	}
	Unexpected("Bad MediaFilter in Provider::matchesFilter()");
}

std::vector<ListSection> Provider::fillSections(
		not_null<Overview::Layout::Delegate*> delegate) {
	markLayoutsStale();
	const auto guard = gsl::finally([&] { clearStaleLayouts(); });

	auto result = std::vector<ListSection>();
	auto section = ListSection(_type, sectionDelegate());
	auto count = _slice.size();
	auto missingCount = 0;
	auto addedCount = 0;
	for (auto i = count; i != 0;) {
		auto universalId = GetUniversalId(_slice[--i]);
		auto layout = getLayout(universalId, delegate);
		if (layout && !matchesFilter(layout->getItem())) {
			continue;
		}
		if (!layout && (_type == Type::FilesPhotos || _type == Type::All)) {
			// Composite types can hold ids whose HistoryItem is not yet
			// materialized in memory (large channels, or the History was
			// unloaded elsewhere). We collect every un-materialized id of the
			// slice and materialize them all at once via messages/channels
			// .getMessages, then rebuild so the whole region fills in a single
			// pass. Guarded: one materialization pass in flight at a time.
			++missingCount;
			requestMissingAround(universalId);
			continue;
		}
		if (!layout) {
			continue;
		}
		++addedCount;
		if (!section.addItem(layout)) {
			section.finishSection();
			result.push_back(std::move(section));
			section = ListSection(_type, sectionDelegate());
			section.addItem(layout);
		}
	}
	if (!section.empty()) {
		section.finishSection();
		result.push_back(std::move(section));
	}
	if ((_type == Type::FilesPhotos || _type == Type::All)
		&& (missingCount || addedCount)) {
		Depur::append(
			QStringLiteral("[DEPUR] FILL type=%1 slice=%2 added=%3 "
				"missing=%4").arg(int(_type))
			.arg(Depur::num(count))
			.arg(Depur::num(addedCount))
			.arg(Depur::num(missingCount)));
	}
	return result;
}

void Provider::requestMissingAround(
		[[maybe_unused]] UniversalMsgId universalId) {
	if (_missingRequested) {
		return;
	}
	// Collect every un-materialized id of the current slice and materialize
	// them via messages/channels.getMessages (batched, so we never exceed the
	// API limit for a single getMessages call), so the whole visible region
	// fills reliably. Batches run with bounded parallelism (kMaxMaterializeParallel)
	// so zones fill fast without flooding the API. Some ids may simply no longer
	// exist (deleted) and will never materialize: each batch settles exactly once
	// (done or fail) so the guard is always released and no batch gets stuck.
	constexpr auto kMaxMaterializeBatch = 100;
	auto *session = &_controller->session();
	struct MissingId {
		PeerId peerId;
		MsgId msgId;
	};
	auto missing = std::vector<MissingId>();
	missing.reserve(_slice.size());
	for (auto i = int(_slice.size()); i != 0;) {
		const auto uid = GetUniversalId(_slice[--i]);
		const auto full = computeFullId(uid);
		if (!session->data().message(full)) {
			missing.push_back({ full.peer, full.msg });
		}
	}
	if (missing.empty()) {
		// Already materialized (or otherwise resolved); nothing to request.
		return;
	}
	if (_missingRetries >= kMaxMissingRetries) {
		// Hemos reintentado varias veces y quedan ids sin HistoryItem (p.ej.
		// mensajes borrados que nunca se materializan). No reintentamos más
		// para evitar un bucle infinito de render + getMessages; las celdas
		// pendientes se rellenarán si el id llega a existir por otra vía.
		return;
	}
	_missingRequested = true;
	Depur::append(
		QStringLiteral("[DEPUR] MATER start type=%1 peer=%2 slice=%3 missing=%4")
		.arg(int(_type))
		.arg(Depur::num(_peer->id.value))
		.arg(Depur::num(_slice.size()))
		.arg(Depur::num(missing.size())));
	// Cola de materialización: en lugar de pedir los lotes UNO a uno (en una
	// zona de 1000 ids eran ~10 pasadas de red seguidas y la cuadrícula parecía
	// congelada/parada), se lanzan hasta kMaxMaterializeParallel lotes en vuelo
	// a la vez. Eso rellena cada zona ~3x más rápido manteniendo la carga
	// acotada (2-3 peticiones getMessages concurrentes como máximo) para no
	// sobrecargar la API ni bloquear el procesado.
	constexpr auto kMaxMaterializeParallel = 3;
	struct MaterializeState {
		std::vector<MissingId> missing;
		size_t next = 0;
		int inFlight = 0;
		std::shared_ptr<bool> alive = std::make_shared<bool>(true);
		std::function<void()> startMore;
	};
	auto state = std::make_shared<MaterializeState>();
	state->missing = std::move(missing);
	// If the provider (and its _lifetime) is destroyed before the responses
	// arrive, this marks the guard dead so the call-backs never touch a
	// destroyed object.
	_lifetime.add([weak = std::weak_ptr<bool>(state->alive)] {
		if (auto strong = weak.lock()) {
			*strong = false;
		}
	});
	std::function<void()> batchSettled;
	batchSettled = [this, session, state] {
		if (!*state->alive) {
			// Provider destroyed while the materialization was in flight.
			return;
		}
		--state->inFlight;
		state->startMore();
	};
	state->startMore = [this, session, state, batchSettled] {
		if (!*state->alive) {
			// Provider destroyed while the materialization was in flight.
			return;
		}
		while ((state->inFlight < kMaxMaterializeParallel)
			&& (state->next < state->missing.size())) {
			// Tomamos el siguiente lote de ids sin HistoryItem.
			const auto from = state->next;
			auto batchCount = 0;
			while ((batchCount < kMaxMaterializeBatch)
				&& (state->next < state->missing.size())) {
				++state->next;
				++batchCount;
			}
			++state->inFlight;
			auto ids = QVector<MTPInputMessage>();
			ids.reserve(batchCount);
			for (auto i = from; i != state->next; ++i) {
				ids.push_back(MTP_inputMessageID(MTP_int(
					state->missing[i].msgId.bare)));
			}
			const auto finish = [=](const MTPmessages_Messages &result) {
				if (!*state->alive) {
					return;
				}
				session->data().processExistingMessages(
					_peer->asChannel(),
					result);
				batchSettled();
			};
			const auto fail = [=] {
				if (!*state->alive) {
					return;
				}
				batchSettled();
			};
			if (const auto channel = _peer->asChannel()) {
				session->api().request(
					MTPchannels_GetMessages(
						channel->inputChannel(),
						MTP_vector<MTPInputMessage>(ids))
				).done(finish).fail(fail).send();
			} else {
				session->api().request(
					MTPmessages_GetMessages(
						MTP_vector<MTPInputMessage>(ids))
				).done(finish).fail(fail).send();
			}
			Depur::append(
				QStringLiteral("[DEPUR] MATER batch sent=%1/%2 "
					"(next %3, inFlight %4)").arg(Depur::num(batchCount))
				.arg(Depur::num(state->missing.size()))
				.arg(Depur::num(state->next))
				.arg(Depur::num(state->inFlight)));
		}
		if (state->inFlight == 0) {
			// All batches done.
			_missingRequested = false;
			auto materialized = 0;
			for (const auto &m : state->missing) {
				if (session->data().message(
					FullMsgId(m.peerId, m.msgId))) {
					++materialized;
				}
			}
			const auto stillMissing = int(state->missing.size())
				- materialized;
			// Control del reintento: si quedaron huecos volveremos a intentar
			// (el _refreshed.fire re-dispara requestMissingAround), pero con un
			// tope para no entrar en bucle infinito con ids que nunca existen.
			if (stillMissing > 0) {
				_missingRetries = std::min(
					_missingRetries + 1,
					kMaxMissingRetries);
			} else {
				_missingRetries = 0;
			}
			Depur::append(
				QStringLiteral("[DEPUR] MATER done type=%1 missing=%2 "
					"materialized=%3 stillMissing=%4 slice=%5 retry=%6").arg(int(_type))
				.arg(Depur::num(state->missing.size()))
				.arg(Depur::num(materialized))
				.arg(Depur::num(stillMissing))
				.arg(Depur::num(_slice.size()))
				.arg(Depur::num(_missingRetries)));
			// Romper el ciclo state->startMore->batchSettled->state para que el
			// estado se libere del todo cuando el provider desaparezca (aquí ya
			// no hay peticiones en vuelo: inFlight == 0).
			state->missing.clear();
			state->startMore = nullptr;
			_refreshed.fire({});
			return;
		}
	};
	state->startMore();
}

void Provider::markLayoutsStale() {
	for (auto &layout : _layouts) {
		layout.second.stale = true;
	}
}

void Provider::clearStaleLayouts() {
	for (auto i = _layouts.begin(); i != _layouts.end();) {
		if (i->second.stale) {
			_layoutRemoved.fire(i->second.item.get());
			i = _layouts.erase(i);
		} else {
			++i;
		}
	}
}

rpl::producer<not_null<BaseLayout*>> Provider::layoutRemoved() {
	return _layoutRemoved.events();
}

BaseLayout *Provider::lookupLayout(
		const HistoryItem *item) {
	const auto i = _layouts.find(GetUniversalId(item));
	return (i != _layouts.end()) ? i->second.item.get() : nullptr;
}

bool Provider::isMyItem(not_null<const HistoryItem*> item) {
	const auto peer = item->history()->peer;
	return (_peer == peer) || (_migrated == peer);
}

bool Provider::isAfter(
		not_null<const HistoryItem*> a,
		not_null<const HistoryItem*> b) {
	return (GetUniversalId(a) < GetUniversalId(b));
}

void Provider::setSearchQuery(QString query) {
	Unexpected("Media::Provider::setSearchQuery.");
}

void Provider::jumpToMessage(
		MsgId messageId,
		Fn<void(FullMsgId)> callback) {
	_viewerLifetime.destroy();

	const auto peer = _controller->session().data().peer(_peer->id);
	const auto request = Api::PrepareSearchRequest(
		peer,
		_topicRootId,
		_monoforumPeerId,
		_type,
		QString(),
		messageId,
		Data::LoadDirection::Around);

	if (!request) {
		return;
	}

	const auto finish = [=] {
		const auto fullId = FullMsgId(_peer->id, messageId);
		_universalAroundId = GetUniversalId(fullId);
		if (callback) {
			callback(fullId);
		}
		_idsLimit = (_type == Type::FilesPhotos || _type == Type::All)
			? kFullIdsLimit
			: kMinimalIdsLimit * 2;
		refreshViewer();
	};

	_controller->session().api().request(
		std::move(*request)
	).done([=](const Api::SearchRequestResult &result) {
		auto parsed = Api::ParseSearchResult(
			peer,
			_type,
			messageId,
			Data::LoadDirection::Around,
			result);

		if (!parsed.messageIds.empty()) {
			peer->session().storage().add(Storage::SharedMediaAddSlice(
				peer->id,
				_topicRootId,
				_monoforumPeerId,
				_type,
				std::move(parsed.messageIds),
				parsed.noSkipRange,
				parsed.fullCount));
		}
		finish();
	}).fail([=] {
		finish();
	}).send();
}

bool Provider::anchorWhileAtTop() {
	const auto after = _slice.skippedAfter();
	return !after || (*after > 0);
}

SparseIdsMergedSlice::Key Provider::sliceKey(
		UniversalMsgId universalId) const {
	using Key = SparseIdsMergedSlice::Key;
	if (!_topicRootId && _migrated) {
		return Key(
			_peer->id,
			_topicRootId,
			_monoforumPeerId,
			_migrated->id,
			universalId);
	}
	if (universalId < 0) {
		// Convert back to plain id for non-migrated histories.
		universalId = universalId + ServerMaxMsgId;
	}
	return Key(
		_peer->id,
		_topicRootId,
		_monoforumPeerId,
		PeerId(),
		universalId);
}

void Provider::itemRemoved(not_null<const HistoryItem*> item) {
	const auto id = GetUniversalId(item);
	if (const auto i = _layouts.find(id); i != end(_layouts)) {
		_layoutRemoved.fire(i->second.item.get());
		_layouts.erase(i);
	}
}

FullMsgId Provider::computeFullId(
		UniversalMsgId universalId) const {
	Expects(universalId != 0);

	return (universalId > 0)
		? FullMsgId(_peer->id, universalId)
		: FullMsgId(
			(_migrated ? _migrated : _peer.get())->id,
			ServerMaxMsgId + universalId);
}

BaseLayout *Provider::getLayout(
		UniversalMsgId universalId,
		not_null<Overview::Layout::Delegate*> delegate) {
	auto it = _layouts.find(universalId);
	if (it == _layouts.end()) {
		if (auto layout = createLayout(universalId, delegate, _type)) {
			layout->initDimensions();
			it = _layouts.emplace(
				universalId,
				std::move(layout)).first;
		} else {
			return nullptr;
		}
	}
	it->second.stale = false;
	return it->second.item.get();
}

std::unique_ptr<BaseLayout> Provider::createLayout(
		UniversalMsgId universalId,
		not_null<Overview::Layout::Delegate*> delegate,
		Type type) {
	const auto item = _controller->session().data().message(
		computeFullId(universalId));
	if (!item) {
		return nullptr;
	}
	const auto getPhoto = [&]() -> PhotoData* {
		if (const auto media = item->media()) {
			return media->photo();
		}
		return nullptr;
	};
	const auto getFile = [&]() -> DocumentData* {
		if (const auto media = item->media()) {
			return media->document();
		}
		return nullptr;
	};

	const auto &songSt = st::overviewFileLayout;
	using namespace Overview::Layout;
	const auto options = [&] {
		const auto media = item->media();
		return MediaOptions{ .spoiler = media && media->hasSpoiler() };
	};
	switch (type) {
	case Type::Photo:
		if (const auto photo = getPhoto()) {
			return std::make_unique<Photo>(
				delegate,
				item,
				photo,
				options());
		}
		return nullptr;
	case Type::GIF:
		if (const auto file = getFile()) {
			return std::make_unique<Gif>(delegate, item, file);
		}
		return nullptr;
	case Type::Video:
		if (const auto file = getFile()) {
			return std::make_unique<Video>(delegate, item, file, options());
		}
		return nullptr;
	case Type::PhotoVideo:
		if (const auto photo = getPhoto()) {
			return std::make_unique<Photo>(delegate, item, photo, options());
		} else if (const auto file = getFile()) {
			return std::make_unique<Video>(delegate, item, file, options());
		}
		return nullptr;
	case Type::FilesPhotos:
		if (const auto photo = getPhoto()) {
			return std::make_unique<Photo>(
				delegate,
				item,
				photo,
				options());
		} else if (const auto file = getFile()) {
			return std::make_unique<Document>(
				delegate,
				item,
				DocumentFields{
					.document = file,
					.hideName = true,
				},
				songSt);
		}
		return nullptr;
	case Type::All:
		if (const auto photo = getPhoto()) {
			return std::make_unique<Photo>(
				delegate,
				item,
				photo,
				options());
		} else if (const auto file = getFile()) {
			if (file->isVideoFile()) {
				return std::make_unique<Video>(delegate, item, file, options());
			}
			return std::make_unique<Document>(
				delegate,
				item,
				DocumentFields{
					.document = file,
					.hideName = true,
				},
				songSt);
		}
		return nullptr;
	case Type::File:
		if (const auto file = getFile()) {
			return std::make_unique<Document>(
				delegate,
				item,
				DocumentFields{ .document = file },
				songSt);
		}
		return nullptr;
	case Type::MusicFile:
		if (const auto file = getFile()) {
			return std::make_unique<Document>(
				delegate,
				item,
				DocumentFields{ .document = file },
				songSt);
		}
		return nullptr;
	case Type::RoundVoiceFile:
		if (const auto file = getFile()) {
			return std::make_unique<Voice>(delegate, item, file, songSt);
		}
		return nullptr;
	case Type::Link:
		return std::make_unique<Link>(delegate, item, item->media());
	case Type::RoundFile:
		return nullptr;
	}
	Unexpected("Type in ListWidget::createLayout()");
}

ListItemSelectionData Provider::computeSelectionData(
		not_null<const HistoryItem*> item,
		TextSelection selection) {
	auto result = ListItemSelectionData(selection);
	result.canDelete = item->canDelete();
	result.canForward = item->allowsForward();
	return result;
}

bool Provider::allowSaveFileAs(
		not_null<const HistoryItem*> item,
		not_null<DocumentData*> document) {
	return item->allowsForward();
}

QString Provider::showInFolderPath(
		not_null<const HistoryItem*> item,
		not_null<DocumentData*> document) {
	return document->filepath(true);
}

void Provider::applyDragSelection(
		ListSelectedMap &selected,
		not_null<const HistoryItem*> fromItem,
		bool skipFrom,
		not_null<const HistoryItem*> tillItem,
		bool skipTill) {
	const auto fromId = GetUniversalId(fromItem) - (skipFrom ? 1 : 0);
	const auto tillId = GetUniversalId(tillItem) - (skipTill ? 0 : 1);
	for (auto i = selected.begin(); i != selected.end();) {
		const auto itemId = GetUniversalId(i->first);
		if (itemId > fromId || itemId <= tillId) {
			i = selected.erase(i);
		} else {
			++i;
		}
	}
	for (auto &layoutItem : _layouts) {
		auto &&universalId = layoutItem.first;
		if (universalId <= fromId && universalId > tillId) {
			const auto item = layoutItem.second.item->getItem();
			ChangeItemSelection(
				selected,
				item,
				computeSelectionData(item, FullSelection),
				std::numeric_limits<int>::max());
		}
	}
}

int64 Provider::scrollTopStatePosition(not_null<HistoryItem*> item) {
	return GetUniversalId(item).bare;
}

HistoryItem *Provider::scrollTopStateItem(ListScrollTopState state) {
	if (state.item && _slice.indexOf(state.item->fullId())) {
		return state.item;
	} else if (const auto id = _slice.nearest(state.position)) {
		if (const auto item = _controller->session().data().message(*id)) {
			return item;
		}
	}
	return state.item;
}

void Provider::saveState(
		not_null<Memento*> memento,
		ListScrollTopState scrollState) {
	if (_universalAroundId != kDefaultAroundId && scrollState.item) {
		memento->setAroundId(computeFullId(_universalAroundId));
		memento->setIdsLimit(_idsLimit);
		memento->setScrollTopItem(scrollState.item->globalId());
		memento->setScrollTopItemPosition(scrollState.position);
		memento->setScrollTopShift(scrollState.shift);
	}
}

void Provider::restoreState(
		not_null<Memento*> memento,
		Fn<void(ListScrollTopState)> restoreScrollState) {
	if (const auto limit = memento->idsLimit()) {
		auto wasAroundId = memento->aroundId();
		if (isPossiblyMyPeerId(wasAroundId.peer)) {
			_idsLimit = limit;
			_universalAroundId = GetUniversalId(wasAroundId);
			restoreScrollState({
				.position = memento->scrollTopItemPosition(),
				.item = MessageByGlobalId(memento->scrollTopItem()),
				.shift = memento->scrollTopShift(),
			});
			refreshViewer();
		}
	}
}

} // namespace Info::Media
