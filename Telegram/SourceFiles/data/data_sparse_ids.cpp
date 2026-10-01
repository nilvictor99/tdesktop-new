/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "data/data_sparse_ids.h"

#include "debug_depurador.h"
#include <rpl/combine.h>
#include "storage/storage_sparse_ids_list.h"

SparseIdsMergedSlice::SparseIdsMergedSlice(Key key)
: SparseIdsMergedSlice(
	key,
	SparseIdsSlice(),
	MigratedSlice(key)) {
}

SparseIdsMergedSlice::SparseIdsMergedSlice(
	Key key,
	SparseIdsSlice part,
	std::optional<SparseIdsSlice> migrated)
: _key(key)
, _part(std::move(part))
, _migrated(std::move(migrated)) {
}

SparseIdsMergedSlice::SparseIdsMergedSlice(
	Key key,
	SparseUnsortedIdsSlice unsorted)
: _key(key)
, _unsorted(std::move(unsorted)) {
}

std::optional<int> SparseIdsMergedSlice::fullCount() const {
	return _unsorted
		? _unsorted->fullCount()
		: Add(
			_part.fullCount(),
			_migrated ? _migrated->fullCount() : 0);
}

std::optional<int> SparseIdsMergedSlice::skippedBefore() const {
	return _unsorted
		? _unsorted->skippedBefore()
		: Add(
			isolatedInMigrated() ? 0 : _part.skippedBefore(),
			_migrated
				? (isolatedInPart()
					? _migrated->fullCount()
					: _migrated->skippedBefore())
				: 0
		);
}

std::optional<int> SparseIdsMergedSlice::skippedAfter() const {
	return _unsorted
		? _unsorted->skippedAfter()
		: Add(
			isolatedInMigrated() ? _part.fullCount() : _part.skippedAfter(),
			isolatedInPart() ? 0 : _migrated->skippedAfter()
		);
}

std::optional<int> SparseIdsMergedSlice::indexOf(
		FullMsgId fullId) const {
	return _unsorted
		? _unsorted->indexOf(fullId.msg)
		: isFromPart(fullId)
		? (_part.indexOf(fullId.msg) | func::add(migratedSize()))
		: isolatedInPart()
			? std::nullopt
			: isFromMigrated(fullId)
				? _migrated->indexOf(fullId.msg)
				: std::nullopt;
}

int SparseIdsMergedSlice::size() const {
	return _unsorted
		? _unsorted->size()
		: (isolatedInPart() ? 0 : migratedSize())
			+ (isolatedInMigrated() ? 0 : _part.size());
}

FullMsgId SparseIdsMergedSlice::operator[](int index) const {
	Expects(index >= 0 && index < size());

	if (_unsorted) {
		return ComputeId(_key.peerId, (*_unsorted)[index]);
	}

	if (const auto size = migratedSize()) {
		if (index < size) {
			return ComputeId(_key.migratedPeerId, (*_migrated)[index]);
		}
		index -= size;
	}
	return ComputeId(_key.peerId, _part[index]);
}

std::optional<int> SparseIdsMergedSlice::distance(
		const Key &a,
		const Key &b) const {
	if (const auto i = indexOf(ComputeId(a))) {
		if (const auto j = indexOf(ComputeId(b))) {
			return *j - *i;
		}
	}
	return std::nullopt;
}

auto SparseIdsMergedSlice::nearest(
		UniversalMsgId id) const -> std::optional<FullMsgId> {
	if (_unsorted) {
		if (_unsorted->indexOf(id).has_value()) {
			return ComputeId(_key.peerId, id);
		} else if (const auto count = _unsorted->size()) {
			return ComputeId(_key.peerId, (*_unsorted)[count / 2]);
		}
		return std::nullopt;
	}
	const auto convertFromPartNearest = [&](MsgId result) {
		return ComputeId(_key.peerId, result);
	};
	const auto convertFromMigratedNearest = [&](MsgId result) {
		return ComputeId(_key.migratedPeerId, result);
	};
	if (IsServerMsgId(id)) {
		if (auto partNearestId = _part.nearest(id)) {
			return partNearestId
				| convertFromPartNearest;
		} else if (isolatedInPart()) {
			return std::nullopt;
		}
		return _migrated->nearest(ServerMaxMsgId - 1)
			| convertFromMigratedNearest;
	}
	if (auto migratedNearestId = _migrated
		? _migrated->nearest(id + ServerMaxMsgId)
		: std::nullopt) {
		return migratedNearestId
			| convertFromMigratedNearest;
	} else if (isolatedInMigrated()) {
		return std::nullopt;
	}
	return _part.nearest(0)
		| convertFromPartNearest;
}

SparseIdsSliceBuilder::SparseIdsSliceBuilder(
	Key key,
	int limitBefore,
	int limitAfter)
: _key(key)
, _limitBefore(limitBefore)
, _limitAfter(limitAfter) {
	_walkTimer.setCallback([this] { walkStep(); });
}

bool SparseIdsSliceBuilder::applyInitial(
		const Storage::SparseIdsListResult &result) {
	mergeSliceData(
		result.count,
		result.messageIds,
		result.skippedBefore,
		result.skippedAfter);
	return true;
}

bool SparseIdsSliceBuilder::applyUpdate(
		const Storage::SparseIdsSliceUpdate &update) {
	auto intersects = [](MsgRange range1, MsgRange range2) {
		return (range1.from <= range2.till)
			&& (range2.from <= range1.till);
	};
	auto needMergeMessages = (update.messages != nullptr)
		&& (_ids.empty()
			|| intersects(update.range, {
				_ids.front(),
				_ids.back()
			}));
	// Para las slices compuestas (Fotos+Archivos / Todo) cada actualización
	// trae la UNION de varias secuencias de servidor (p.ej. Photo+File). La
	// primera página que llega ya cubre un rango ancho y las siguientes NO
	// intersectan el estado visible actual, por lo que sin este forzado el
	// builder descartaría sus ids (`needMergeMessages=false`), `_ids` no
	// crecería, el msgId de la petición "Before" nunca avanzaría y el loader
	// entraría en un bucle infinito re-pidiendo la misma página sin cargar
	// el resto del contenido. Forzamos el merge de los ids recibidos (el
	// `base::flat_set::merge` es idempotente, no duplica), de modo que la
	// slice acumula todas las páginas y explora el historial completo. No
	// afecta a Media/Archivos (cuya secuencia es contigua y ya intersecta).
	needMergeMessages = needMergeMessages || (update.messages != nullptr);
	if (!needMergeMessages && !update.count) {
		return false;
	}
	auto skippedBefore = (update.range.from == 0)
		? 0
		: std::optional<int> {};
	auto skippedAfter = (update.range.till == ServerMaxMsgId)
		? 0
		: std::optional<int> {};
	mergeSliceData(
		update.count,
		needMergeMessages
			? *update.messages
			: base::flat_set<MsgId> {},
		skippedBefore,
		skippedAfter);
	// Traza del borde del builder DESPUÉS del merge: muestra cuántos ids tiene
	// _ids, sus extremos y cómo quedan los skipped. Sirve para comparar Media
	// (que cierra en 0/0) con FilesPhotos/All (para ver por qué no cierran).
	Depur::append(
		QStringLiteral("[DEPUR] SPARSE applied range=%1..%2 items=%3 "
			"ids=%4 f=%5..%6 skipB=%7 skipA=%8 count=%9")
		.arg(Depur::num(update.range.from.bare))
		.arg(Depur::num(update.range.till.bare))
		.arg(update.messages ? Depur::num(update.messages->size()) : QStringLiteral("n/a"))
		.arg(Depur::num(_ids.size()))
		.arg(_ids.empty() ? QStringLiteral("-") : Depur::num(_ids.front().bare))
		.arg(_ids.empty() ? QStringLiteral("-") : Depur::num(_ids.back().bare))
		.arg(_skippedBefore.has_value() ? Depur::num(*_skippedBefore) : QStringLiteral("?"))
		.arg(_skippedAfter.has_value() ? Depur::num(*_skippedAfter) : QStringLiteral("?"))
		.arg(update.count.has_value() ? Depur::num(*update.count) : QStringLiteral("n/a")));
	return true;
}

bool SparseIdsSliceBuilder::removeOne(MsgId messageId) {
	auto changed = false;
	if (_fullCount && *_fullCount > 0) {
		--*_fullCount;
		changed = true;
	}
	if (_ids.contains(messageId)) {
		_ids.remove(messageId);
		changed = true;
	} else if (!_ids.empty()) {
		if (_ids.front() > messageId
			&& _skippedBefore
			&& *_skippedBefore > 0) {
			--*_skippedBefore;
			changed = true;
		} else if (_ids.back() < messageId
			&& _skippedAfter
			&& *_skippedAfter > 0) {
			--*_skippedAfter;
			changed = true;
		}
	}
	if (changed) {
		checkInsufficient();
	}
	return changed;
}

bool SparseIdsSliceBuilder::removeAll() {
	_ids = {};
	_fullCount = 0;
	_skippedBefore = 0;
	_skippedAfter = 0;
	_walkNoProgress = 0;
	_walkLastFront = MsgId(0);
	_walkEndLogged = false;
	return true;
}

bool SparseIdsSliceBuilder::invalidateBottom() {
	_fullCount = _skippedAfter = std::nullopt;
	checkInsufficient();
	return true;
}

void SparseIdsSliceBuilder::checkInsufficient() {
	sliceToLimits();
}

void SparseIdsSliceBuilder::mergeSliceData(
		std::optional<int> count,
		const base::flat_set<MsgId> &messageIds,
		std::optional<int> skippedBefore,
		std::optional<int> skippedAfter) {
	if (messageIds.empty()) {
		if (count && _fullCount != count) {
			_fullCount = count;
			if (*_fullCount <= _ids.size()) {
				_fullCount = _ids.size();
				_skippedBefore = _skippedAfter = 0;
			}
		}
		fillSkippedAndSliceToLimits();
		return;
	}
	if (count) {
		_fullCount = count;
	}
	auto wasMinId = _ids.empty() ? -1 : _ids.front();
	auto wasMaxId = _ids.empty() ? -1 : _ids.back();
	_ids.merge(messageIds.begin(), messageIds.end());

	auto adjustSkippedBefore = [&](MsgId oldId, int oldSkippedBefore) {
		auto it = _ids.find(oldId);
		Assert(it != _ids.end());
		_skippedBefore = oldSkippedBefore - (it - _ids.begin());
		accumulate_max(*_skippedBefore, 0);
	};
	if (skippedBefore) {
		adjustSkippedBefore(messageIds.front(), *skippedBefore);
	} else if (wasMinId >= 0 && _skippedBefore) {
		adjustSkippedBefore(wasMinId, *_skippedBefore);
	} else {
		_skippedBefore = std::nullopt;
	}

	auto adjustSkippedAfter = [&](MsgId oldId, int oldSkippedAfter) {
		auto it = _ids.find(oldId);
		Assert(it != _ids.end());
		_skippedAfter = oldSkippedAfter - (_ids.end() - it - 1);
		accumulate_max(*_skippedAfter, 0);
	};
	if (skippedAfter) {
		adjustSkippedAfter(messageIds.back(), *skippedAfter);
	} else if (wasMaxId >= 0 && _skippedAfter) {
		adjustSkippedAfter(wasMaxId, *_skippedAfter);
	} else {
		_skippedAfter = std::nullopt;
	}
	fillSkippedAndSliceToLimits();
}

void SparseIdsSliceBuilder::fillSkippedAndSliceToLimits() {
	if (_alwaysPageBefore && _skippedBefore && *_skippedBefore == 0) {
		// Fijar el conteo exacto ANTES de derivar las colas: así el skippedAfter
		// que se calcula aquí usa el total real (ids recuperados) y no la
		// estimación del servidor (que podía producir un valor negativo).
		maybeFinalizeWalk();
	}
	if (_fullCount) {
		if (_skippedBefore && !_skippedAfter) {
			_skippedAfter = *_fullCount
				- *_skippedBefore
				- int(_ids.size());
		} else if (_skippedAfter && !_skippedBefore) {
			_skippedBefore = *_fullCount
				- *_skippedAfter
				- int(_ids.size());
		}
	}
	sliceToLimits();
}

void SparseIdsSliceBuilder::sliceToLimits() {
	if (!_key) {
		if (!_fullCount) {
			requestMessagesCount();
		}
		return;
	}
	auto requestedSomething = false;
	auto aroundIt = ranges::lower_bound(_ids, _key);
	auto removeFromBegin = (aroundIt - _ids.begin() - _limitBefore);
	auto removeFromEnd = (_ids.end() - aroundIt - _limitAfter - 1);
	if (removeFromBegin > 0) {
		// En modo caminata NO recortamos el inicio: conservamos todos los ids
		// recuperados en el builder. Así el conteo exacto es _ids.size() (sin
		// que los reintegros de la unión del storage re-cuenten ids). El
		// recorte normal (ventana acotada) se aplica sólo fuera de la caminata.
		if (!_alwaysPageBefore) {
			_ids.erase(_ids.begin(), _ids.begin() + removeFromBegin);
			if (_skippedBefore) {
				*_skippedBefore += removeFromBegin;
			}
		}
	} else if (removeFromBegin < 0
		&& (!_skippedBefore || *_skippedBefore > 0)) {
		requestedSomething = true;
		requestMessages(RequestDirection::Before);
	}
	if (removeFromEnd > 0) {
		_ids.erase(_ids.end() - removeFromEnd, _ids.end());
		if (_skippedAfter) {
			*_skippedAfter += removeFromEnd;
		}
	} else if (removeFromEnd < 0
		&& (!_skippedAfter || *_skippedAfter > 0)) {
		requestedSomething = true;
		requestMessages(RequestDirection::After);
	}
	if (_alwaysPageBefore && !requestedSomething) {
		// Caminata completa: aunque la ventana ya esté llena (o el estado
		// cacheado cubra el ancla), seguimos solicitando páginas anteriores
		// hasta demostrar el inicio real del historial. El timer interno
		// (una petición en vuelo, con backoff) alimenta el ciclo en segundo
		// plano sin bloquear la interfaz.
		scheduleWalk();
	}
	if (!_fullCount && !requestedSomething) {
		requestMessagesCount();
	}
	maybeFinalizeWalk();
}

void SparseIdsSliceBuilder::requestMessages(
		RequestDirection direction) {
	auto requestAroundData = [&]() -> AroundData {
		if (_ids.empty()) {
			return { _key, Data::LoadDirection::Around };
		} else if (direction == RequestDirection::Before) {
			return { _ids.front(), Data::LoadDirection::Before };
		}
		return { _ids.back(), Data::LoadDirection::After };
	};
	_insufficientAround.fire(requestAroundData());
}

void SparseIdsSliceBuilder::requestMessagesCount() {
	_insufficientAround.fire({ 0, Data::LoadDirection::Around });
}

void SparseIdsSliceBuilder::setAlwaysPageBefore(bool value) {
	_alwaysPageBefore = value;
	if (!value) {
		_walkTimer.cancel();
	}
}

void SparseIdsSliceBuilder::scheduleWalk() {
	if (!_alwaysPageBefore || _walkTimer.isActive()) {
		return;
	}
	if (_skippedBefore && *_skippedBefore == 0) {
		// Inicio demostrado: la caminata no tiene nada más que pedir.
		maybeFinalizeWalk();
		return;
	}
	if (!_limitBefore) {
		// Viewer sin ventana (messageId == 0): no hay caminata que alimentar.
		return;
	}
	// La caminata avanza pausada: una página y una reconstrucción de rejilla
	// cada ~1.5 s deja a la UI respirar (en canales de >1000 media, el pase
	// completo de materialización + relayout por página congelaba la app).
	constexpr auto kWalkGapMs = 1500;
	constexpr auto kWalkMaxBackoffShift = 6;
	const auto delay = kWalkGapMs << qMin(_walkNoProgress, kWalkMaxBackoffShift);
	_walkTimer.callOnce(delay);
}

void SparseIdsSliceBuilder::walkStep() {
	if (!_alwaysPageBefore) {
		return;
	}
	if (_skippedBefore && *_skippedBefore == 0) {
		maybeFinalizeWalk();
		return;
	}
	const auto front = _ids.empty() ? _key : _ids.front();
	if (_walkLastFront == front) {
		// Sin progreso en el frente (petición fallida, página repetida o
		// red lenta): el backoff del próximo disparo se duplica hasta 32 s.
		++_walkNoProgress;
		constexpr auto kWalkMaxWarn = 6;
		if (_walkNoProgress == 1 || _walkNoProgress == kWalkMaxWarn) {
			Depur::append(
				QStringLiteral("[DEPUR] WALK-STALL front=%1 sin-progreso=%2")
				.arg(Depur::num(front.bare))
				.arg(Depur::num(_walkNoProgress)));
		}
	} else {
		_walkLastFront = front;
		_walkNoProgress = 0;
	}
	Depur::append(
		QStringLiteral("[DEPUR] WALK front=%1 ids=%2")
		.arg(Depur::num(front.bare))
		.arg(Depur::num(int(_ids.size()))));
	requestMessages(RequestDirection::Before);
	scheduleWalk();
}

void SparseIdsSliceBuilder::maybeFinalizeWalk() {
	if (!_alwaysPageBefore || !_skippedBefore || *_skippedBefore != 0) {
		return;
	}
	if (!_walkEndLogged) {
		_walkEndLogged = true;
		Depur::append(
			QStringLiteral("[DEPUR] WALK-END ids=%1 fullCount=%2")
			.arg(Depur::num(int(_ids.size())))
			.arg(Depur::num(_fullCount.value_or(0))));
	}
	// Conteo exacto: al llegar al inicio real, el número de archivos es el
	// total de ids distintos recuperados (el builder conserva todos en modo
	// caminata), no la estimación del servidor que suma los fullCount de
	// cada sub-filtro.
	const auto exact = int(_ids.size());
	if (_fullCount != exact) {
		_fullCount = exact;
	}
}

SparseIdsSlice SparseIdsSliceBuilder::snapshot() const {
	return SparseIdsSlice(
		_ids,
		_fullCount,
		_skippedBefore,
		_skippedAfter);
}

rpl::producer<SparseIdsMergedSlice> SparseIdsMergedSlice::CreateViewer(
		SparseIdsMergedSlice::Key key,
		int limitBefore,
		int limitAfter,
		Fn<SimpleViewerFunction> simpleViewer) {
	Expects(!key.topicRootId
		|| (!key.monoforumPeerId && !key.migratedPeerId));
	Expects(!key.monoforumPeerId
		|| (!key.topicRootId && !key.migratedPeerId));
	Expects(IsServerMsgId(key.universalId)
		|| (key.universalId == 0)
		|| (IsServerMsgId(ServerMaxMsgId + key.universalId) && key.migratedPeerId != 0));
	Expects((key.universalId != 0)
		|| (limitBefore == 0 && limitAfter == 0));

	return [=](auto consumer) {
		auto partViewer = simpleViewer(
			key.peerId,
			key.topicRootId,
			key.monoforumPeerId,
			SparseIdsMergedSlice::PartKey(key),
			limitBefore,
			limitAfter
		);
		if (!key.migratedPeerId) {
			return std::move(
				partViewer
			) | rpl::on_next([=](SparseIdsSlice &&part) {
				consumer.put_next(SparseIdsMergedSlice(
					key,
					std::move(part),
					std::nullopt));
			});
		}
		auto migratedViewer = simpleViewer(
			key.migratedPeerId,
			MsgId(0), // topicRootId
			PeerId(0), // monoforumPeerId
			SparseIdsMergedSlice::MigratedKey(key),
			limitBefore,
			limitAfter);
		return rpl::combine(
			std::move(partViewer),
			std::move(migratedViewer)
		) | rpl::on_next([=](
				SparseIdsSlice &&part,
				SparseIdsSlice &&migrated) {
			consumer.put_next(SparseIdsMergedSlice(
				key,
				std::move(part),
				std::move(migrated)));
		});
	};
}
