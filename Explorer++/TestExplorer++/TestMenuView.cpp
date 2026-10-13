// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TestMenuView.h"
#include "../Helper/MenuHelper.h"

TestMenuView::TestMenuView(HWND ownerWindow, MenuHelpTextHost *menuHelpTextHost) :
	RootMenuView(MenuHelper::CheckedCreatePopupMenu(), ownerWindow, MenuType::ContextMenu,
		menuHelpTextHost)
{
}
