/*
Author: 23rd.
*/
#pragma once

#include "settings/settings_common.h"

namespace Settings {

class Fork : public Section<Fork> {
public:
	Fork(QWidget *parent, not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent(not_null<Window::SessionController*> controller);

};

} // namespace Settings
