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
	for (auto viewMode : VIEW_MODES)
	{
		auto id = m_idCounter++;
		m_rootMenuView->AppendItem(this, id, GetViewModeMenuText(resourceLoader, viewMode));

		m_idToViewModeMap.insert({ id, viewMode });
	}
}

bool ViewsMenu::IsItemChecked(UINT id) const
{
	return GetViewModeForItem(id) == GetActiveShellBrowser()->GetViewMode();
}

void ViewsMenu::OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	UNREFERENCED_PARAMETER(isCtrlKeyDown);
	UNREFERENCED_PARAMETER(isShiftKeyDown);

	GetActiveShellBrowser()->SetViewMode(GetViewModeForItem(id));
}

ViewMode ViewsMenu::GetViewModeForItem(UINT id) const
{
	auto itr = m_idToViewModeMap.find(id);
	CHECK(itr != m_idToViewModeMap.end());
	return itr->second;
}

ShellBrowser *ViewsMenu::GetActiveShellBrowser() const
{
	return m_browser->GetActiveShellBrowser();
}
