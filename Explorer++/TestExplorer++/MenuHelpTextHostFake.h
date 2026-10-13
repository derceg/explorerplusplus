// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/MenuHelpTextHost.h"

class MenuHelpTextHostFake : public MenuHelpTextHost
{
public:
	// MenuHelpTextHost
	void NotifyTopLevelMenuShown() override;
	void NotifyTopLevelMenuClosed() override;
	void MenuItemSelected(HMENU menu, WORD indexOrId, UINT flags) override;
	boost::signals2::connection AddMenuHelpTextRequestObserver(
		const MenuHelpTextRequestSignal::slot_type &observer) override;

	std::optional<std::wstring> TriggerHelpTextRequest(HMENU menu, UINT id);

private:
	MenuHelpTextRequestSignal m_menuHelpTextRequestSignal;
};
