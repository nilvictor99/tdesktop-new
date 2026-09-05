/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "info/media/info_media_inner_widget.h"

#include <rpl/flatten_latest.h>
#include <rpl/combine.h>
#include <rpl/distinct_until_changed.h>
#include "boxes/abstract_box.h"
#include "info/media/info_media_list_widget.h"
#include "info/media/info_media_buttons.h"
#include "info/media/info_media_empty_widget.h"
#include "info/profile/info_profile_icon.h"
#include "info/info_controller.h"
#include "data/data_forum_topic.h"
#include "data/data_peer.h"
#include "data/data_saved_sublist.h"
#include "ui/widgets/discrete_sliders.h"
#include "ui/widgets/shadow.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/box_content_divider.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/search_field_controller.h"
#include "data/data_shared_media.h"
#include "info/profile/info_profile_values.h"
#include "styles/style_info.h"
#include "lang/lang_keys.h"

namespace Info {
namespace Media {

namespace {

[[nodiscard]] QString FilterLabel(MediaFilter filter) {
	switch (filter) {
	case MediaFilter::All: return tr::lng_filters_all_short(tr::now);
	case MediaFilter::Photos: return tr::lng_media_type_photos(tr::now);
	case MediaFilter::Videos: return tr::lng_media_type_videos(tr::now);
	case MediaFilter::Files: return tr::lng_media_type_files(tr::now);
	}
	Unexpected("Bad MediaFilter in Info::Media::FilterLabel()");
}

[[nodiscard]] std::vector<MediaFilter> FilterSections(
		int photos,
		int videos) {
	auto result = std::vector<MediaFilter>();
	result.push_back(MediaFilter::All);
	if (photos > 0) {
		result.push_back(MediaFilter::Photos);
	}
	if (videos > 0) {
		result.push_back(MediaFilter::Videos);
	}
	return result;
}

} // namespace

InnerWidget::InnerWidget(
	QWidget *parent,
	not_null<Controller*> controller)
: RpWidget(parent)
, _controller(controller)
, _empty(this) {
	_empty->heightValue(
	) | rpl::on_next(
		[this] { refreshHeight(); },
		_empty->lifetime());
	_list = setupList();
	setupMediaFilter();
	setupFileViewToggle();
}

// Allows showing additional shared media links and tabs.
// Used for shared media in Saved Messages.
void InnerWidget::setupOtherTypes() {
	if (_controller->key().peer()->sharedMediaInfo() && _isStackBottom) {
		createOtherTypes();
	} else {
		_otherTypes.destroy();
		refreshHeight();
	}
}

void InnerWidget::createOtherTypes() {
	_otherTypes.create(this);
	_otherTypes->show();

	createTypeButtons();
	_otherTypes->add(object_ptr<Ui::BoxContentDivider>(_otherTypes));

	_otherTypes->resizeToWidth(width());
	_otherTypes->heightValue(
	) | rpl::on_next(
		[this] { refreshHeight(); },
		_otherTypes->lifetime());
}

void InnerWidget::createTypeButtons() {
	auto wrap = _otherTypes->add(object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
		_otherTypes,
		object_ptr<Ui::VerticalLayout>(_otherTypes)));
	auto content = wrap->entity();
	content->add(object_ptr<Ui::FixedHeightWidget>(
		content,
		st::infoProfileSkip));

	auto tracker = Ui::MultiSlideTracker();
	const auto peer = _controller->key().peer();
	const auto topic = _controller->key().topic();
	const auto sublist = _controller->key().sublist();
	const auto topicRootId = topic ? topic->rootId() : MsgId();
	const auto monoforumPeerId = sublist
		? sublist->sublistPeer()->id
		: PeerId();
	const auto migrated = _controller->migrated();
	const auto addMediaButton = [&](
			Type buttonType,
			const style::icon &icon) {
		if (buttonType != Type::PhotoVideo && buttonType == type()) {
			return;
		}
		auto result = AddButton(
			content,
			_controller,
			peer,
			topicRootId,
			monoforumPeerId,
			migrated,
			buttonType,
			tracker);
		object_ptr<Profile::FloatingIcon>(
			result,
			icon,
			st::infoSharedMediaButtonIconPosition)->show();
	};

	addMediaButton(Type::PhotoVideo, st::infoIconMediaPhoto);
	addMediaButton(Type::File, st::infoIconMediaFile);
	addMediaButton(Type::MusicFile, st::infoIconMediaAudio);
	addMediaButton(Type::Link, st::infoIconMediaLink);
	addMediaButton(Type::RoundVoiceFile, st::infoIconMediaVoice);
	addMediaButton(Type::GIF, st::infoIconMediaGif);

	content->add(object_ptr<Ui::FixedHeightWidget>(
		content,
		st::infoProfileSkip));
	wrap->toggleOn(tracker.atLeastOneShownValue());
	wrap->finishAnimating();
}

Type InnerWidget::type() const {
	return _controller->section().mediaType();
}

void InnerWidget::visibleTopBottomUpdated(
		int visibleTop,
		int visibleBottom) {
	setChildVisibleTopBottom(_list, visibleTop, visibleBottom);
}

bool InnerWidget::showInternal(not_null<Memento*> memento) {
	if (!_controller->validateMementoPeer(memento)) {
		return false;
	}
	auto mementoType = memento->section().mediaType();
	if (mementoType == type()) {
		restoreState(memento);
		return true;
	}
	return false;
}

object_ptr<ListWidget> InnerWidget::setupList() {
	auto result = object_ptr<ListWidget>(this, _controller);
	result->heightValue(
	) | rpl::on_next(
		[this] { refreshHeight(); },
		result->lifetime());
	using namespace rpl::mappers;
	result->scrollToRequests(
	) | rpl::map([widget = result.data()](int to) {
		return Ui::ScrollToRequest {
			widget->y() + to,
			-1
		};
	}) | rpl::start_to_stream(
		_scrollToRequests,
		result->lifetime());
	_selectedLists.fire(result->selectedListValue());
	_listTops.fire(result->topValue());
	_empty->setType(_controller->section().mediaType());
	_controller->mediaSourceQueryValue(
	) | rpl::on_next([this](const QString &query) {
		_empty->setSearchQuery(query);
	}, result->lifetime());
	return result;
}

bool InnerWidget::supportsMediaFilter() const {
	return _list && _list->supportsMediaFilter();
}

void InnerWidget::setupMediaFilter() {
	if (!supportsMediaFilter()) {
		return;
	}
	const auto peer = _controller->key().peer();
	const auto topic = _controller->key().topic();
	const auto sublist = _controller->key().sublist();
	const auto topicRootId = topic ? topic->rootId() : MsgId();
	const auto monoforumPeerId = sublist
		? sublist->sublistPeer()->id
		: PeerId();
	const auto migrated = _controller->migrated();
	using SharedMediaType = Storage::SharedMediaType;
	rpl::combine(
		Profile::SharedMediaCountValue(
			peer,
			topicRootId,
			monoforumPeerId,
			migrated,
			SharedMediaType::Photo),
		Profile::SharedMediaCountValue(
			peer,
			topicRootId,
			monoforumPeerId,
			migrated,
			SharedMediaType::Video)
	) | rpl::map([](int photos, int videos) {
		return FilterSections(photos, videos);
	}) | rpl::distinct_until_changed() | rpl::on_next([=](
			const std::vector<MediaFilter> &sections) {
		rebuildMediaFilter(sections);
	}, _filterCountsLifetime);
}

void InnerWidget::rebuildMediaFilter(
		const std::vector<MediaFilter> &sections) {
	if (sections == _filterSections) {
		return;
	}
	const auto current = _list->mediaFilter();
	const auto found = [&] {
		for (const auto &filter : sections) {
			if (filter == current) {
				return true;
			}
		}
		return false;
	}();
	if (!found) {
		_list->setMediaFilter(MediaFilter::All);
	}
	_filterSections = sections;
	_filter.destroy();
	_filter.create(this);
	_filter->show();
	for (const auto &filter : _filterSections) {
		_filter->addSection(FilterLabel(filter));
	}
	const auto active = _list->mediaFilter();
	auto activeIndex = 0;
	for (auto i = 0; i < int(_filterSections.size()); ++i) {
		if (_filterSections[i] == active) {
			activeIndex = i;
			break;
		}
	}
	_filter->setActiveSectionFast(activeIndex);
	_filter->sectionActivated(
	) | rpl::on_next([=](int section) {
		if (section >= 0 && section < int(_filterSections.size())) {
			_list->setMediaFilter(_filterSections[section]);
		}
	}, _filter->lifetime());
	_filter->resizeToWidth(width());
	refreshHeight();
}

void InnerWidget::setupFileViewToggle() {
	if (type() != Type::File) {
		return;
	}
	_rows.create(this);
	_rows->show();
	_rows->addSection(u"Lista"_q);
	_rows->addSection(u"4"_q);
	_rows->setActiveSectionFast(0);
	_rows->sectionActivated(
	) | rpl::on_next([=](int index) {
		const auto columns = [&] {
			switch (index) {
			case 0: return 0;
			case 1: return 4;
			}
			Unexpected("Index in InnerWidget::setupFileViewToggle()");
		}();
		_list->setFileGridColumns(columns);
	}, _rows->lifetime());
}

void InnerWidget::saveState(not_null<Memento*> memento) {
	_list->saveState(memento);
}

void InnerWidget::restoreState(not_null<Memento*> memento) {
	_list->restoreState(memento);
}

rpl::producer<SelectedItems> InnerWidget::selectedListValue() const {
	return _selectedLists.events_starting_with(
		_list->selectedListValue()
	) | rpl::flatten_latest();
}

void InnerWidget::selectionAction(SelectionAction action) {
	_list->selectionAction(action);
}

InnerWidget::~InnerWidget() = default;

int InnerWidget::resizeGetHeight(int newWidth) {
	_inResize = true;
	auto guard = gsl::finally([this] { _inResize = false; });

	if (_otherTypes) {
		_otherTypes->resizeToWidth(newWidth);
	}
	if (_filter) {
		_filter->resizeToWidth(newWidth);
	}
	if (_rows) {
		_rows->resizeToWidth(newWidth);
	}
	_list->resizeToWidth(newWidth);
	_empty->resizeToWidth(newWidth);
	return recountHeight();
}

void InnerWidget::refreshHeight() {
	if (_inResize) {
		return;
	}
	resize(width(), recountHeight());
}

int InnerWidget::recountHeight() {
	auto top = 0;
	if (_otherTypes) {
		_otherTypes->moveToLeft(0, top);
		top += _otherTypes->heightNoMargins() - st::lineWidth;
	}
	if (_filter) {
		_filter->moveToLeft(0, top);
		top += _filter->heightNoMargins();
	}
	if (_rows) {
		_rows->moveToLeft(0, top);
		top += _rows->heightNoMargins();
	}
	auto listHeight = 0;
	if (_list) {
		_list->moveToLeft(0, top);
		listHeight = _list->heightNoMargins();
		top += listHeight;
	}
	if (listHeight > _emptyHeightThreshold && !_empty->loading()) {
		_empty->hide();
	} else {
		_empty->show();
		_empty->moveToLeft(0, top);
		top += _empty->heightNoMargins();
	}
	return top;
}

void InnerWidget::setScrollHeightValue(rpl::producer<int> value) {
	using namespace rpl::mappers;
	_empty->setFullHeight(rpl::combine(
		std::move(value),
		_listTops.events_starting_with(
			_list->topValue()
		) | rpl::flatten_latest(),
		_1 - _2));
}

rpl::producer<Ui::ScrollToRequest> InnerWidget::scrollToRequests() const {
	return _scrollToRequests.events();
}

bool InnerWidget::processZoomWheel(not_null<QWheelEvent*> e) {
	return _list->processZoomWheel(e);
}

void InnerWidget::zoomIn() {
	_list->zoomIn();
}

void InnerWidget::zoomOut() {
	_list->zoomOut();
}

bool InnerWidget::canZoomIn() const {
	return _list->canZoomIn();
}

bool InnerWidget::canZoomOut() const {
	return _list->canZoomOut();
}

void InnerWidget::jumpToMessage(MsgId msgId) {
	_empty->setLoading(true);
	_emptyHeightThreshold = st::semiboldFont->height;
	_list->jumpToMessage(msgId);
	_emptyLoadingLifetime = _list->heightValue(
	) | rpl::skip(1) | rpl::filter(
		rpl::mappers::_1 > _emptyHeightThreshold
	) | rpl::take(1) | rpl::on_next([=](int height) {
		_empty->setLoading(false);
		_emptyHeightThreshold = 0;
		recountHeight();
	});
}

} // namespace Media
} // namespace Info
