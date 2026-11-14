/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "menu/menu_checked_action.h"

#include "base/unique_qptr.h"
#include "ui/effects/premium_graphics.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/menu/menu_action.h"
#include "ui/widgets/menu/menu_common.h"
#include "ui/widgets/popup_menu.h"
#include "ui/painter.h"

#include "styles/style_media_player.h"
#include "styles/style_widgets.h"
#include "styles/palette.h"

#include <QtGui/QMouseEvent>
#include <QtSvg/QSvgRenderer>

#include <memory>

namespace {

// Extra room to the right of the premium star in menu actions.
constexpr auto kPremiumStarRightSkip = 10;

// Extra room between the item text and its toggle switch.
constexpr auto kToggleRightGap = 8;

[[nodiscard]] QImage PremiumStarImage(int size) {
	const auto factor = style::DevicePixelRatio();
	const auto side = QSize(size, size);
	auto image = QImage(
		side * factor,
		QImage::Format_ARGB32_Premultiplied);
	image.setDevicePixelRatio(factor);
	image.fill(Qt::transparent);
	{
		auto p = QPainter(&image);
		auto svg = QSvgRenderer(
			Ui::Premium::ColorizedSvg(Ui::Premium::ButtonGradientStops()));
		svg.render(&p, QRectF(QPointF(), QSizeF(side)));
	}
	return image;
}

[[nodiscard]] style::Menu PatchedActiveStyle(
		const style::Menu &base,
		bool active,
		int premiumStarSize,
		bool withShortcut) {
	auto result = base;
	if (active) {
		result.itemFg = st::windowActiveTextFg;
		result.itemFgOver = st::windowActiveTextFg;
		result.itemFgShortcut = st::windowActiveTextFg;
		result.itemFgShortcutOver = st::windowActiveTextFg;
	}
	if (premiumStarSize > 0 && withShortcut) {
		result.itemPadding.setRight(result.itemRightSkip
			+ style::ConvertScale(kPremiumStarRightSkip)
			+ premiumStarSize
			+ result.itemRightSkip);
	}
	return result;
}

// Holds the patched style so it outlives the Ui::Menu::Action base, which keeps
// only a reference to its style::Menu. Declared first => constructed first.
struct OwnedMenuStyle {
	explicit OwnedMenuStyle(style::Menu st) : value(std::move(st)) {
	}

	style::Menu value;
};

class ActiveColorAction final
	: private OwnedMenuStyle
	, public Ui::Menu::Action {
public:
	ActiveColorAction(
		not_null<Ui::Menu::Menu*> parent,
		const style::Menu &st,
		not_null<QAction*> action,
		const style::icon *icon,
		bool active,
		int premiumStarSize);

private:
	void paintEvent(QPaintEvent *e) override;

	const style::icon *_activeIcon = nullptr;
	const bool _active = false;
	QImage _premiumStar;

};

ActiveColorAction::ActiveColorAction(
	not_null<Ui::Menu::Menu*> parent,
	const style::Menu &st,
	not_null<QAction*> action,
	const style::icon *icon,
	bool active,
	int premiumStarSize)
: OwnedMenuStyle(PatchedActiveStyle(
	st,
	active,
	premiumStarSize,
	action->text().contains(QChar('\t'))))
, Ui::Menu::Action(parent, OwnedMenuStyle::value, action, icon, icon)
, _activeIcon(icon)
, _active(active)
, _premiumStar((premiumStarSize > 0)
	? PremiumStarImage(premiumStarSize)
	: QImage()) {
	if (premiumStarSize > 0
		&& !action->text().contains(QChar('\t'))) {
		setMinWidth(minWidth()
			+ OwnedMenuStyle::value.itemRightSkip
			+ style::ConvertScale(kPremiumStarRightSkip)
			+ premiumStarSize);
	}
}

void ActiveColorAction::paintEvent(QPaintEvent *e) {
	Ui::Menu::Action::paintEvent(e);

	Painter p(this);
	if (_active && _activeIcon) {
		_activeIcon->paint(
			p,
			OwnedMenuStyle::value.itemIconPosition,
			width(),
			st::windowActiveTextFg->c);
	}
	if (!_premiumStar.isNull()) {
		const auto factor = style::DevicePixelRatio();
		const auto starWidth = _premiumStar.width() / factor;
		const auto starHeight = _premiumStar.height() / factor;
		const auto left = width()
			- OwnedMenuStyle::value.itemRightSkip
			- style::ConvertScale(kPremiumStarRightSkip)
			- starWidth;
		const auto top = (height() - starHeight) / 2;
		p.drawImage(left, top, _premiumStar);
	}
}

[[nodiscard]] style::Menu PatchedToggleStyle(
		const style::Menu &base,
		const style::Toggle &toggleSt) {
	auto result = base;
	const auto toggleWidth = 2 * toggleSt.border
		+ toggleSt.diameter
		+ toggleSt.width;
	result.itemPadding.setRight(
		base.itemRightSkip
		+ toggleWidth
		+ style::ConvertScale(kToggleRightGap));
	return result;
}

class ToggleAction final
	: private OwnedMenuStyle
	, public Ui::Menu::Action {
public:
	ToggleAction(
		not_null<Ui::Menu::Menu*> parent,
		const style::Menu &st,
		not_null<QAction*> action,
		const style::icon *icon,
		bool checked,
		Fn<void(bool)> callback);

	void finishAnimating() override;

private:
	void paintEvent(QPaintEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;

	[[nodiscard]] int toggleLeft() const;
	[[nodiscard]] QRect toggleHitRect() const;

	const Fn<void(bool)> _callback;
	const style::Toggle &_toggleSt;
	const style::Toggle &_toggleStOver;
	std::unique_ptr<Ui::ToggleView> _toggle;
	bool _togglePressed = false;

};

ToggleAction::ToggleAction(
	not_null<Ui::Menu::Menu*> parent,
	const style::Menu &st,
	not_null<QAction*> action,
	const style::icon *icon,
	bool checked,
	Fn<void(bool)> callback)
: OwnedMenuStyle(PatchedToggleStyle(st, st.itemToggle))
, Ui::Menu::Action(parent, OwnedMenuStyle::value, action, icon, icon)
, _callback(std::move(callback))
, _toggleSt(st.itemToggle)
, _toggleStOver(st.itemToggleOver)
, _toggle(std::make_unique<Ui::ToggleView>(
	st.itemToggle,
	checked,
	[=] { update(); })) {
}

void ToggleAction::finishAnimating() {
	Ui::Menu::Action::finishAnimating();
	if (_toggle) {
		_toggle->finishAnimating();
	}
}

int ToggleAction::toggleLeft() const {
	const auto toggleWidth = 2 * _toggleSt.border
		+ _toggleSt.diameter
		+ _toggleSt.width;
	return width() - OwnedMenuStyle::value.itemRightSkip - toggleWidth;
}

QRect ToggleAction::toggleHitRect() const {
	const auto left = toggleLeft();
	const auto gap = style::ConvertScale(kToggleRightGap);
	return QRect(
		left - gap,
		0,
		width() - left + gap,
		height());
}

void ToggleAction::paintEvent(QPaintEvent *e) {
	_toggle->setStyle(isSelected() ? _toggleStOver : _toggleSt);
	Ui::Menu::Action::paintEvent(e);

	Painter p(this);
	const auto size = _toggle->getSize();
	const auto top = (contentHeight() - size.height()) / 2;
	_toggle->paint(p, toggleLeft(), top, width());
}

void ToggleAction::mousePressEvent(QMouseEvent *e) {
	if (e->button() == Qt::LeftButton
		&& toggleHitRect().contains(e->pos())) {
		_togglePressed = true;
		e->accept();
		return;
	}
	Ui::Menu::Action::mousePressEvent(e);
}

void ToggleAction::mouseReleaseEvent(QMouseEvent *e) {
	if (_togglePressed) {
		_togglePressed = false;
		if (e->button() == Qt::LeftButton
			&& isEnabled()
			&& toggleHitRect().contains(e->pos())) {
			const auto value = !_toggle->checked();
			_toggle->setChecked(value, anim::type::normal);
			if (_callback) {
				_callback(value);
			}
			e->accept();
		}
		return;
	}
	Ui::Menu::Action::mouseReleaseEvent(e);
}

class CheckedAction final : public Ui::Menu::Action {
public:
	CheckedAction(
		not_null<Ui::Menu::Menu*> parent,
		const style::Menu &st,
		not_null<QAction*> action,
		const style::icon *icon,
		bool checked);

private:
	void paintEvent(QPaintEvent *e) override;

	const bool _checked = false;

};

CheckedAction::CheckedAction(
	not_null<Ui::Menu::Menu*> parent,
	const style::Menu &st,
	not_null<QAction*> action,
	const style::icon *icon,
	bool checked)
: Ui::Menu::Action(parent, st, action, icon, icon)
, _checked(checked) {
	setMinWidth(minWidth() + st.itemRightSkip + st::mediaPlayerMenuCheck.width());
}

void CheckedAction::paintEvent(QPaintEvent *e) {
	Ui::Menu::Action::paintEvent(e);

	if (!_checked) {
		return;
	}

	Painter p(this);
	const auto &icon = st::mediaPlayerMenuCheck;
	const auto left = width() - st().itemRightSkip - icon.width();
	const auto top = (height() - icon.height()) / 2;
	icon.paint(p, left, top, width());
}

} // namespace

namespace Menu {

not_null<QAction*> AddCheckedAction(
		not_null<Ui::PopupMenu*> menu,
		const QString &text,
		Fn<void()> callback,
		const style::icon *icon,
		bool checked) {
	auto item = base::make_unique_q<CheckedAction>(
		menu->menu(),
		menu->st().menu,
		Ui::Menu::CreateAction(
			menu->menu().get(),
			text,
			std::move(callback)),
		icon,
		checked);
	return menu->addAction(std::move(item));
}

not_null<QAction*> AddToggleAction(
		not_null<Ui::PopupMenu*> menu,
		const QString &text,
		Fn<void()> callback,
		Fn<void(bool checked)> toggleCallback,
		const style::icon *icon,
		bool checked) {
	auto item = base::make_unique_q<ToggleAction>(
		menu->menu(),
		menu->st().menu,
		Ui::Menu::CreateAction(
			menu->menu().get(),
			text,
			std::move(callback)),
		icon,
		checked,
		std::move(toggleCallback));
	return menu->addAction(std::move(item));
}

not_null<QAction*> AddActiveColorAction(
		not_null<Ui::PopupMenu*> menu,
		const QString &text,
		Fn<void()> callback,
		const style::icon *icon,
		bool active,
		int premiumStarSize) {
	auto item = base::make_unique_q<ActiveColorAction>(
		menu->menu(),
		menu->st().menu,
		Ui::Menu::CreateAction(
			menu->menu().get(),
			text,
			std::move(callback)),
		icon,
		active,
		premiumStarSize);
	return menu->addAction(std::move(item));
}

} // namespace Menu
