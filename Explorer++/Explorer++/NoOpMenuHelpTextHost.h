// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/MenuHelpTextHost.h"

// There are times when a context menu is shown, but no help text can be displayed. For example, if
// a context menu is shown in a dialog, there's not going to be any place to display help text. An
// instance of this class can then be provided in those situations, rather than simply providing a
// nullptr.
//
// As a concrete example:
//
// PopupMenuView popupMenu(NoOpMenuHelpTextHost::GetInstance());
//
// gives at least some indication of intent, as opposed to:
//
// PopupMenuView popupMenu(nullptr);
//
// which is hard to understand at the call site.
//
// As the name of the class indicates, this class can essentially be thought of as representing a
// host that can't display menu text and therefore doesn't take any action when
// AddMenuHelpTextRequestObserver() is invoked.
class NoOpMenuHelpTextHost final : public MenuHelpTextHost
{
public:
	void MenuItemSelected(HMENU menu, UINT itemId, UINT flags) override;
	boost::signals2::connection AddMenuHelpTextRequestObserver(
		const MenuHelpTextRequestSignal::slot_type &observer) override;

	static NoOpMenuHelpTextHost *GetInstance();

private:
	NoOpMenuHelpTextHost() = default;
};
