/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "data/data_file_origin.h"

namespace Main {
class Session;
} // namespace Main

class History;
class PhotoData;
class DocumentData;
struct FileLoadResult;

namespace Api {

struct MessageToSend;
struct SendAction;

void SendExistingDocument(
	MessageToSend &&message,
	not_null<DocumentData*> document);

void SendExistingPhoto(
	MessageToSend &&message,
	not_null<PhotoData*> photo);

void SendExistingDocument(
	MessageToSend &&message,
	not_null<DocumentData*> document,
	Data::FileOrigin origin);

void SendExistingPhoto(
	MessageToSend &&message,
	not_null<PhotoData*> photo,
	Data::FileOrigin origin);

bool SendDice(MessageToSend &message);

void FillMessagePostFlags(
	const SendAction &action,
	not_null<PeerData*> peer,
	MessageFlags &flags);

void SendConfirmedFile(
	not_null<Main::Session*> session,
	const std::shared_ptr<FileLoadResult> &file);

} // namespace Api
