/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "ui/boxes/confirm_box.h"
#include "ui/text/format_values.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_session_controller.h"

class HistoryItem;

namespace Fork {

[[nodiscard]] TimeId DurationFromItem(HistoryItem *item);
[[nodiscard]] TimeId DurationFromItem(
	FullMsgId itemId,
	not_null<Window::SessionController*> controller);

template <typename Items>
void AddShowSumDurations(
		not_null<Ui::PopupMenu*> menu,
		Items items,
		not_null<Window::SessionController*> controller) {
	auto result = TimeId(0);

	constexpr bool isContainer = requires(const Items &t) {
		t.begin();
	};
	constexpr bool isInfoSelectedItem = requires(const Items &t) {
		t.list;
	};
	if constexpr (isContainer) {
		using T = decltype(begin(items));
		constexpr bool isPair = requires(const T &t) {
			t->first;
		};
		constexpr bool isListWidgetSelectedItem = requires(const T &t) {
			t->msgId;
		};
		if constexpr (isPair) {
			for (const auto &[item, _] : items) {
				result += DurationFromItem(item);
			}
		} else if constexpr (isListWidgetSelectedItem) {
			for (const auto &selected : items) {
				result += DurationFromItem(selected.msgId, controller);
			}
		}
	} else if constexpr (isInfoSelectedItem) {
		for (const auto &item : items.list) {
			result += DurationFromItem(item.globalId.itemId, controller);
		}
	} else {
		throw std::invalid_argument("Can't find good type.");
	}

	if (result > 0) {
		menu->addAction("Calculate duration", [=] {
			const auto text = QString("Summary duration: %1.").arg(
				Ui::FormatDurationText(result));
			Window::Show(controller).showBox(Ui::MakeInformBox(text));
		});
	}
}

void AddReplaceMedia(
	not_null<Ui::PopupMenu*> menu,
	not_null<HistoryItem*> item,
	not_null<Window::SessionController*> controller);

} // namespace Fork
