// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ViewsMenuBuilder.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ViewModeHelper.h"
#include "../Helper/MenuHelper.h"

ViewsMenuBuilder::ViewsMenuBuilder(const ResourceLoader *resourceLoader) :
	m_resourceLoader(resourceLoader)
{
}

wil::unique_hmenu ViewsMenuBuilder::BuildMenu(const BrowserWindow *browser)
{
	wil::unique_hmenu menu(CreatePopupMenu());
	AddViewModesToMenu(menu.get(), 0, true);

	ViewMode currentViewMode = browser->GetActiveShellBrowser()->GetViewMode();
	CheckMenuRadioItem(menu.get(), IDM_VIEW_EXTRALARGEICONS, IDM_VIEW_TILES,
		GetViewModeMenuId(currentViewMode), MF_BYCOMMAND);

	return menu;
}

void ViewsMenuBuilder::AddViewModesToMenu(HMENU menu, UINT startPosition, bool byPosition)
{
	UINT position = startPosition;

	for (auto viewMode : VIEW_MODES)
	{
		MenuHelper::AddStringItem(menu, GetViewModeMenuId(viewMode),
			GetViewModeMenuText(m_resourceLoader, viewMode), position, byPosition);

		if (byPosition)
		{
			position++;
		}
	}
}
