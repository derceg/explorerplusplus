// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "WindowMenuView.h"
#include "../Helper/MenuHelper.h"

WindowMenuView::WindowMenuView(HWND ownerWindow, MenuHelpTextHost *menuHelpTextHost) :
	RootMenuView(MenuHelper::CheckedCreateMenu(), ownerWindow, MenuType::WindowMenu,
		menuHelpTextHost)
{
	MenuHelper::AddStyleFlags(m_menu, MNS_NOTIFYBYPOS);

	auto res = SetMenu(ownerWindow, m_menu);
	CHECK(res);

	// The menu is now owned by the window and will be destroyed when the window is destroyed.
	ReleaseMenu();
}
