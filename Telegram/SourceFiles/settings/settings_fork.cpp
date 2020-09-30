/*
Author: 23rd.
*/
#include "settings/settings_fork.h"

#include "app.h"
#include "base/qthelp_url.h"
#include "base/weak_ptr.h"
#include "boxes/abstract_box.h"
#include "boxes/confirm_box.h"
#include "core/application.h"
#include "core/file_utilities.h"
#include "facades.h"
#include "lang/lang_keys.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "settings/settings_common.h"
#include "storage/localstorage.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"
#include "styles/style_settings.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/input_fields.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

namespace Settings {
namespace {

using langString = tr::phrase<>;
using SessionController = not_null<Window::SessionController*>;

class SettingBox : public Ui::BoxContent, public base::has_weak_ptr  {
public:
	explicit SettingBox(
		QWidget*,
		Fn<void(bool)> callback,
		langString title,
		langString info);

	void setInnerFocus() override;

protected:
	void prepare() override;

	virtual QString getOrSetGlobal(QString value) = 0;
	virtual bool isInvalidUrl(QString linkUrl) = 0;

	Fn<void(bool)> _callback;
	Fn<void()> _setInnerFocus;
	langString _info;
	langString _title;
};

SettingBox::SettingBox(
	QWidget*,
	Fn<void(bool)> callback,
	langString title,
	langString info)
: _callback(std::move(callback))
, _info(info)
, _title(title) {
	Expects(_callback != nullptr);
}

void SettingBox::setInnerFocus() {
	Expects(_setInnerFocus != nullptr);

	_setInnerFocus();
}

void SettingBox::prepare() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	const auto url = content->add(
		object_ptr<Ui::InputField>(
			content,
			st::defaultInputField,
			_info(),
			getOrSetGlobal(QString())),
		st::markdownLinkFieldPadding);

	const auto submit = [=] {
		const auto linkUrl = url->getLastText();
		const auto isInvalid = isInvalidUrl(linkUrl);
		if (isInvalid) {
			url->showError();
			return;
		}
		const auto weak = base::make_weak(this);
		getOrSetGlobal(linkUrl);
		Core::App().saveSettingsDelayed();
		_callback(!isInvalid);
		if (weak) {
			closeBox();
		}
	};

	connect(url, &Ui::InputField::submitted, [=] {
		submit();
	});

	setTitle(_title());

	addButton(tr::lng_box_ok(), submit);
	addButton(tr::lng_cancel(), [=] {
		_callback(!getOrSetGlobal(QString()).isEmpty());
		closeBox();
	});

	content->resizeToWidth(st::boxWidth);
	content->moveToLeft(0, 0);
	setDimensions(st::boxWidth, content->height());

	_setInnerFocus = [=] {
		url->setFocusFast();
	};
}

//////

class SearchEngineBox : public SettingBox {

	using SettingBox::SettingBox;

protected:
	QString getOrSetGlobal(QString value) override;
	bool isInvalidUrl(QString linkUrl) override;
};

QString SearchEngineBox::getOrSetGlobal(QString value) {
	if (value.isEmpty()) {
		return Core::App().settings().searchEngineUrl();
	}
	Core::App().settings().setSearchEngineUrl(value);
	return QString();
}

bool SearchEngineBox::isInvalidUrl(QString linkUrl) {
	linkUrl = qthelp::validate_url(linkUrl);
	return linkUrl.isEmpty() || linkUrl.indexOf("%q") == -1;
}


//////

class URISchemeBox : public SettingBox {

	using SettingBox::SettingBox;

protected:
	QString getOrSetGlobal(QString value) override;
	bool isInvalidUrl(QString linkUrl) override;
};

QString URISchemeBox::getOrSetGlobal(QString value) {
	if (value.isEmpty()) {
		return Core::App().settings().uriScheme();
	}
	Core::App().settings().setUriScheme(value);
	return QString();
}

bool URISchemeBox::isInvalidUrl(QString linkUrl) {
	return linkUrl.indexOf("://") < 2;
}

//////

class StickerSizeBox : public SettingBox {
	using SettingBox::SettingBox;

protected:
	QString getOrSetGlobal(QString value) override;
	bool isInvalidUrl(QString linkUrl) override;

private:
	int _startSize = 0;
};

QString StickerSizeBox::getOrSetGlobal(QString value) {
	if (value.isEmpty()) {
		if (!_startSize) {
			_startSize = Core::App().settings().customStickerSize();
		} else if (_startSize == Core::App().settings().customStickerSize()) {
			return QString();
		}
		return QString::number(Core::App().settings().customStickerSize());
	}
	if (const auto number = value.toInt()) {
		Core::App().settings().setCustomStickerSize(number);
	}
	return QString();
}

bool StickerSizeBox::isInvalidUrl(QString linkUrl) {
	const auto number = linkUrl.toInt();
	return !number || number < 50 || number > 256;
}

//////

void SetupForkContent(
	not_null<Ui::VerticalLayout*> container,
	SessionController controller) {

	const auto session = &controller->session();

	auto wrap = object_ptr<Ui::VerticalLayout>(container);
	const auto inner = wrap.data();
	container->add(object_ptr<Ui::OverrideMargins>(
		container,
		std::move(wrap),
		QMargins(0, 0, 0, st::settingsCheckbox.margin.bottom())));

	const auto checkbox = [&](const QString &label, bool checked) {
		return object_ptr<Ui::Checkbox>(
			container,
			label,
			checked,
			st::settingsCheckbox);
	};
	const auto add = [&](const QString &label, bool checked, auto &&handle) {
		inner->add(
			checkbox(label, checked),
			st::settingsCheckboxPadding
		)->checkedChanges(
		) | rpl::start_with_next(
			std::move(handle),
			inner->lifetime());
	};

	const auto restartBox = [=](Fn<void()> ok, Fn<void()> cancel) {
		Ui::show(Box<ConfirmBox>(
			tr::lng_settings_need_restart(tr::now),
			tr::lng_settings_restart_now(tr::now),
			[=] {
				ok();
				Core::App().saveSettingsDelayed(0);
				App::restart();
			},
			[=] {
				cancel();
			}),
		Ui::LayerOption::KeepOther);
	};
	const auto addRestart = [&](
			const QString &label,
			auto checkedCallback,
			auto ok) {
		const auto checkRow = inner->add(
			checkbox(label, checkedCallback()),
			st::settingsCheckboxPadding
		);
		checkRow->checkedChanges(
		) | rpl::filter([=](bool checked) {
			return (checked != checkedCallback());
		}) | rpl::start_with_next([=](bool checked) {
			restartBox(
				[=] { ok(checked); },
				[=] { checkRow->setChecked(!checked); });
		}, inner->lifetime());
	};

	//
	addRestart(
		tr::lng_settings_square_avatats(tr::now),
		[] { return Core::App().settings().squareUserpics(); },
		[=](bool checked) {
			Core::App().settings().setSquareUserpics(checked);
		});

	//
	add(
		tr::lng_settings_audio_fade(tr::now),
		Core::App().settings().audioFade(),
		[=](bool checked) {
			Core::App().settings().setAudioFade(checked);
			Core::App().saveSettingsDelayed();
		});

	//
	const auto uriScheme = inner->add(
		checkbox(
			tr::lng_settings_uri_scheme(tr::now),
			Core::App().settings().askUriScheme()),
		st::settingsCheckboxPadding
	);
	uriScheme->checkedChanges(
	) | rpl::start_with_next([=](bool checked) {
		if (checked) {
			auto callback = [=](bool isSuccess) {
				uriScheme->setChecked(isSuccess);
				Core::App().settings().setAskUriScheme(checked);
				Core::App().saveSettingsDelayed();
			};
			Ui::show(Box<URISchemeBox>(
				std::move(callback),
				tr::lng_settings_uri_scheme_box_title,
				tr::lng_settings_uri_scheme_field_label),
			Ui::LayerOption::KeepOther);
		} else {
			Core::App().settings().setAskUriScheme(checked);
			Core::App().saveSettingsDelayed();
		}
	}, uriScheme->lifetime());

	//
	add(
		tr::lng_settings_last_seen_in_dialogs(tr::now),
		Core::App().settings().lastSeenInDialogs(),
		[=](bool checked) {
			Core::App().settings().setLastSeenInDialogs(checked);
			Core::App().saveSettingsDelayed();
		});

	//
	const auto searchEngine = inner->add(
		checkbox(
			tr::lng_settings_search_engine(tr::now),
			Core::App().settings().searchEngine()),
		st::settingsCheckboxPadding
	);

	//
	searchEngine->checkedChanges(
	) | rpl::start_with_next([=](bool checked) {
		if (checked) {
			auto callback = [=](bool isSuccess) {
				searchEngine->setChecked(isSuccess);
				Core::App().settings().setSearchEngine(checked);
				Core::App().saveSettingsDelayed();
			};
			Ui::show(Box<SearchEngineBox>(
				std::move(callback),
				tr::lng_settings_search_engine_box_title,
				tr::lng_settings_search_engine_field_label),
			Ui::LayerOption::KeepOther);
		} else {
			Core::App().settings().setSearchEngine(checked);
			Core::App().saveSettingsDelayed();
		}
	}, searchEngine->lifetime());

#ifndef Q_OS_LINUX
#ifdef Q_OS_WIN
	add(
		tr::lng_settings_use_black_tray_icon(tr::now),
		Core::App().settings().useBlackTrayIcon(),
		[](bool checked) {
			Core::App().settings().setUseBlackTrayIcon(checked);
			Core::App().saveSettingsDelayed();
			Core::App().domain().notifyUnreadBadgeChanged();
		});
#else // !Q_OS_WIN
	addRestart(
		tr::lng_settings_use_black_tray_icon(tr::now),
		[] { return Core::App().settings().useBlackTrayIcon(); },
		[](bool checked) {
			Core::App().settings().setUseBlackTrayIcon(checked);
		});
#endif // Q_OS_WIN

	addRestart(
		tr::lng_settings_use_original_tray_icon(tr::now),
		[] { return Core::App().settings().useOriginalTrayIcon(); },
		[](bool checked) {
			Core::App().settings().setUseOriginalTrayIcon(checked);
		});
#endif // !Q_OS_LINUX

	AddButton(
		container,
		tr::lng_settings_custom_sticker_size(),
		st::settingsChatButton,
		&st::settingsIconStickers,
		st::settingsChatIconLeft
	)->addClickHandler([=] {
		Ui::show(Box<StickerSizeBox>([=](bool isSuccess) {
			if (isSuccess) {
				restartBox([] {}, [] {});
			}
		},
		tr::lng_settings_custom_sticker_size,
		tr::lng_settings_sticker_size_label));
	});

}

void SetupFork(
	not_null<Ui::VerticalLayout*> container,
	SessionController controller) {
	AddSkip(container, st::settingsCheckboxesSkip);

	auto wrap = object_ptr<Ui::VerticalLayout>(container);
	SetupForkContent(wrap.data(), controller);

	container->add(object_ptr<Ui::OverrideMargins>(
		container,
		std::move(wrap)));

	AddSkip(container, st::settingsCheckboxesSkip);
}

} // namespace

Fork::Fork(QWidget *parent, SessionController controller)
: Section(parent) {
	setupContent(controller);
}

void Fork::setupContent(SessionController controller) {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	SetupFork(content, controller);

	Ui::ResizeFitChild(this, content);
}

} // namespace Settings
