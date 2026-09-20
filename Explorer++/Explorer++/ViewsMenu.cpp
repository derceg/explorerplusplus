// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ViewsMenu.h"
#include "BrowserWindow.h"
#include "MenuView.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ViewModeHelper.h"

ViewsMenu::ViewsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	BrowserWindow *browser, const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_browser(browser)
{
	BuildMenu(resourceLoader);
}

void ViewsMenu::BuildMenu(const ResourceLoader *resourceLoader)
{
	ViewMode currentViewMode = GetActiveShellBrowser()->GetViewMode();

	for (auto viewMode : VIEW_MODES)
	{
		auto id = m_idCounter++;
		m_menuView->AppendItem(this, id, GetViewModeMenuText(resourceLoader, viewMode));

		if (viewMode == currentViewMode)
		{
			m_menuView->CheckItem(id, true);
		}

		m_idToViewModeMap.insert({ id, viewMode });
	}
}

void ViewsMenu::OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	UNREFERENCED_PARAMETER(isCtrlKeyDown);
	UNREFERENCED_PARAMETER(isShiftKeyDown);

	auto itr = m_idToViewModeMap.find(id);
	CHECK(itr != m_idToViewModeMap.end());
	GetActiveShellBrowser()->SetViewMode(itr->second);
}

ShellBrowser *ViewsMenu::GetActiveShellBrowser()
{
	return m_browser->GetActiveShellBrowser();
}
