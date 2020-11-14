/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

class HistoryItem;
class PeerData;
struct TextWithTags;

namespace Api::AsCopy {

struct ToSend {
	std::vector<not_null<PeerData*>> peers;
	TextWithTags comment;
	bool emptyText = false;
	bool silent = false;
};

void SendExistingAlbumFromItem(not_null<HistoryItem*> item, ToSend &&toSend);

void SendExistingMediaFromItem(not_null<HistoryItem*> item, ToSend &&toSend);

void SendAlbumFromItems(HistoryItemsList items, ToSend &&toSend);

} // namespace Api::AsCopy
