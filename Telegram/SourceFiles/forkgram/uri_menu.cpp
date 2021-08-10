/*
Author: 23rd.
*/

#include "forkgram/uri_menu.h"

#include "forkgram/uri_open.h"
#include "core/application.h"
#include "lang/lang_keys.h"
#include "ui/widgets/popup_menu.h"

namespace Forkgram {

void FillUrlWithCustomUri(
		not_null<Ui::PopupMenu*> menu,
		const QString &url) {
	if (!Core::App().settings().fork().askUriScheme()) {
		return;
	}
	auto callback = [=] {
		OpenUrlWithCustomUri(Core::App().settings().fork().uriScheme() + url);
	};
	menu->addAction(
		tr::lng_context_open_uri_link(tr::now),
		std::move(callback));
}

} // namespace Forkgram
