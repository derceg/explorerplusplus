// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "DefaultToolbarButtons.h"
#include "Explorer++_internal.h"
#include "MainImages.h"
#include "MainResource.h"
#include "../Helper/Controls.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/MenuWrapper.h"
#include "../Helper/ShellHelper.h"


void Explorerplusplus::OnToolbarRClick(HWND sourceWindow)
{
	auto parentMenu = MenuPtr(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_TOOLBAR_MENU)));

	if (!parentMenu)
	{
		return;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	lCheckMenuItem(menu, IDM_TOOLBARS_ADDRESSBAR, m_config->showAddressBar);
	lCheckMenuItem(menu, IDM_TOOLBARS_MAINTOOLBAR, m_config->showMainToolbar);
	lCheckMenuItem(menu, IDM_TOOLBARS_BOOKMARKSTOOLBAR, m_config->showBookmarksToolbar);
	lCheckMenuItem(menu, IDM_TOOLBARS_DRIVES, m_config->showDrivesToolbar);
	lCheckMenuItem(menu, IDM_TOOLBARS_APPLICATIONTOOLBAR, m_config->showApplicationToolbar);
	lCheckMenuItem(menu, IDM_TOOLBARS_LOCKTOOLBARS, m_bLockToolbars);

	DWORD dwPos = GetMessagePos();

	POINT ptCursor;
	ptCursor.x = GET_X_LPARAM(dwPos);
	ptCursor.y = GET_Y_LPARAM(dwPos);

	// Give any observers a chance to modify the menu.
	m_toolbarContextMenuSignal(menu, sourceWindow);

	TrackPopupMenu(menu, TPM_LEFTALIGN, ptCursor.x, ptCursor.y, 0, m_hMainRebar, NULL);
}