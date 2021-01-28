/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "api/api_as_copy.h"

#include "apiwrap.h"
#include "api/api_sending.h"
#include "api/api_text_entities.h"
#include "base/openssl_help.h"
#include "boxes/confirm_box.h"
#include "data/data_document.h"
#include "data/data_drafts.h"
#include "data/data_photo.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "chat_helpers/message_field.h"

namespace Api::AsCopy {
namespace {

MTPInputSingleMedia PrepareAlbumItemMedia(
		not_null<HistoryItem*> item,
		const MTPInputMedia &media,
		uint64 randomId,
		bool emptyText,
		TextWithTags comment) {
	auto commentEntities = TextWithEntities {
		comment.text,
		TextUtilities::ConvertTextTagsToEntities(comment.tags)
	};

	auto caption = item->originalText();
	TextUtilities::Trim(caption);
	auto sentEntities = Api::EntitiesToMTP(
		&item->history()->session(),
		emptyText ? commentEntities.entities : caption.entities,
		Api::ConvertOption::SkipLocal);
	const auto flags = !sentEntities.v.isEmpty()
		? MTPDinputSingleMedia::Flag::f_entities
		: MTPDinputSingleMedia::Flag(0);

	return MTP_inputSingleMedia(
		MTP_flags(flags),
		media,
		MTP_long(randomId),
		MTP_string(emptyText ? commentEntities.text : caption.text),
		sentEntities);
}

MTPinputMedia InputMediaFromItem(not_null<HistoryItem*> i) {
	if (const auto document = i->media()->document()) {
		return MTP_inputMediaDocument(
			MTP_flags(MTPDinputMediaDocument::Flag(0)),
			document->mtpInput(),
			MTP_int(0),
			MTPstring());
	} else if (const auto photo = i->media()->photo()) {
		return MTP_inputMediaPhoto(
			MTP_flags(MTPDinputMediaPhoto::Flag(0)),
			photo->mtpInput(),
			MTP_int(0));
	} else {
		return MTP_inputMediaEmpty();
	}
}

MsgId ReplyToIdFromDraft(not_null<PeerData*> peer) {
	const auto history = peer->owner().history(peer);
	const auto replyTo = [&] {
		if (const auto localDraft = history->localDraft()) {
			return localDraft->msgId;
		} else if (const auto cloudDraft = history->cloudDraft()) {
			return cloudDraft->msgId;
		} else {
			return 0;
		}
	}();
	if (replyTo) {
		history->clearCloudDraft();
		history->clearLocalDraft();
	}
	return replyTo;
}

} // namespace

void SendAlbumFromItems(HistoryItemsList items, ToSend &&toSend) {
	if (items.empty()) {
		return;
	}
	const auto history = items.front()->history();
	auto medias = QVector<MTPInputSingleMedia>();
	for (const auto i : items) {
		medias.push_back(PrepareAlbumItemMedia(
			i,
			InputMediaFromItem(i),
			openssl::RandomValue<uint64>(),
			toSend.emptyText,
			medias.empty() ? toSend.comment : TextWithTags()));
	}
	auto &api = history->owner().session().api();

	for (const auto &peer : toSend.peers) {
		const auto replyTo = ReplyToIdFromDraft(peer);

		const auto flags = MTPmessages_SendMultiMedia::Flags(0)
			| (replyTo
				? MTPmessages_SendMultiMedia::Flag::f_reply_to_msg_id
				: MTPmessages_SendMultiMedia::Flag(0))
			| (toSend.silent
				? MTPmessages_SendMultiMedia::Flag::f_silent
				: MTPmessages_SendMultiMedia::Flag(0));

		const auto requestId = api.request(MTPmessages_SendMultiMedia(
			MTP_flags(flags),
			peer->input,
			MTP_int(replyTo),
			MTP_vector<MTPInputSingleMedia>(medias),
			MTP_int(0)
		)).done([=](const MTPUpdates &result) {
			history->owner().session().api().applyUpdates(result);
		}).fail([=](const RPCError &error) {
		}).send();
	}
}

void SendExistingAlbumFromItem(
		not_null<HistoryItem*> item,
		Api::AsCopy::ToSend &&toSend) {
	if (!item->groupId()) {
		return;
	}
	SendAlbumFromItems(
		item->history()->owner().groups().find(item)->items,
		std::move(toSend));
}

void SendExistingMediaFromItem(
		not_null<HistoryItem*> item,
		Api::AsCopy::ToSend &&toSend) {
	for (const auto peer : toSend.peers) {
		const auto history = peer->owner().history(peer);
		auto message = ApiWrap::MessageToSend(history);
		if (!item->media()) {
			message.textWithTags = PrepareEditText(item);
			history->session().api().sendMessage(std::move(message));
			continue;
		}
		message.textWithTags = toSend.emptyText
			? toSend.comment
			: PrepareEditText(item);
		message.action.options.silent = toSend.silent;
		message.action.replyTo = ReplyToIdFromDraft(peer);
		if (const auto document = item->media()->document()) {
			Api::SendExistingDocument(std::move(message), document);
		} else if (const auto photo = item->media()->photo()) {
			Api::SendExistingPhoto(std::move(message), photo);
		}
	}
}

} // namespace Api::AsCopy
