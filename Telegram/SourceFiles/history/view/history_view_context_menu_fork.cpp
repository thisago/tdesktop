/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "history/view/history_view_context_menu_fork.h"

#include "data/data_document.h"
#include "data/data_session.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "window/window_session_controller.h"

namespace Fork {

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
