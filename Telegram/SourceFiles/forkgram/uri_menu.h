/*
Author: 23rd.
*/
#pragma once

namespace Ui {
class PopupMenu;
} // namespace Ui

namespace Forkgram {

void FillUrlWithCustomUri(
	not_null<Ui::PopupMenu*> menu,
	const QString &url);

} // namespace Forkgram
