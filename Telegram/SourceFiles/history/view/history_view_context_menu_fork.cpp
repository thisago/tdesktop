/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "history/view/history_view_context_menu_fork.h"

#include "api/api_common.h"
#include "api/api_editing.h"
#include "data/data_document.h"
#include "data/data_peer.h"
#include "data/data_photo.h"
#include "data/data_session.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "window/window_session_controller.h"
#include "ui/widgets/menu/menu_multiline_action.h"
#include "styles/style_chat.h"

namespace Fork {

namespace MediaReplacement {

struct Entry final {
	crl::time last = crl::time(0);
	MTPInputMedia media;
};

base::flat_map<Main::Session*, Entry> Medias;

bool HasInputMedia(not_null<HistoryItem*> item) {
	const auto it = Medias.find(&item->from()->session());
	return (it != end(Medias)) ? (it->second.last > 0) : false;
}

void UseInputMedia(
		not_null<HistoryItem*> item,
		Api::SendOptions options) {
	const auto it = Medias.find(&item->from()->session());
	if (it != end(Medias)) {
		if ((crl::now() - it->second.last) < 60000) {
			const auto &media = it->second.media;
			Api::Fork::EditMessageMedia(item, options, media);
		} else {
			Medias.remove(&item->from()->session());
		}
	}
}

void RememberDocumentAsInputMedia(not_null<DocumentData*> document) {
	Medias[&document->session()] = Entry{
		.last = crl::now(),
		.media = MTP_inputMediaDocument(
			MTP_flags(0),
			document->mtpInput(),
			MTPint(),
			MTPstring()),
	};
}

void RememberPhotoAsInputMedia(not_null<PhotoData*> photo) {
	Medias[&photo->session()] = Entry{
		.last = crl::now(),
		.media = MTP_inputMediaPhoto(
			MTP_flags(0),
			photo->mtpInput(),
			MTPint()),
	};
}

} // namespace MediaReplacement

void AddReplaceMedia(
		not_null<Ui::PopupMenu*> menu,
		not_null<HistoryItem*> item,
		not_null<Window::SessionController*> controller) {
	const auto media = item->media();
	const auto photo = media ? media->photo() : nullptr;
	const auto document = media ? media->document() : nullptr;

	const auto addAction = [&](QString &&s, Fn<void()> callback) {
		auto item = base::make_unique_q<Ui::Menu::MultilineAction>(
			menu,
			st::defaultMenu,
			st::historyHasCustomEmoji,
			st::historyHasCustomEmojiPosition,
			TextWithEntities{ std::move(s) });
		item->clicks() | rpl::start_with_next(callback, menu->lifetime());
		menu->addAction(std::move(item));
	};
	if (photo) {
		addAction(u"Remember photo"_q, [=] {
			MediaReplacement::RememberPhotoAsInputMedia(photo);
		});
	}
	if (document) {
		addAction(u"Remember document"_q, [=] {
			MediaReplacement::RememberDocumentAsInputMedia(document);
		});
	}
	if ((photo || document) && MediaReplacement::HasInputMedia(item)) {
		addAction(u"Replace media with remembered one"_q, [=] {
			MediaReplacement::UseInputMedia(item, {});
		});
	}
}

TimeId DurationFromItem(HistoryItem *item) {
	const auto media = item ? item->media() : nullptr;
	const auto document = media ? media->document() : nullptr;
	return document ? document->getDuration() : TimeId(0);
}

TimeId DurationFromItem(
		FullMsgId itemId,
		not_null<Window::SessionController*> controller) {
	return DurationFromItem(controller->session().data().message(itemId));
}

} // namespace Fork
