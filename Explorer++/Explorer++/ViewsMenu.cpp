// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ViewsMenu.h"
#include "BrowserWindow.h"
#include "MenuView.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ViewModes.h"
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
		m_rootMenuView->AppendItem(MenuItemParams(GetViewModeMenuText(resourceLoader, viewMode))
				.SetHelpText(resourceLoader->LoadString(GetViewModeMenuHelpStringId(viewMode)))
				.SetAcceleratorText(GetAcceleratorTextForId(GetViewModeMenuId(viewMode)))
				.SetCheckedCallback(
					[this, viewMode] { return viewMode == GetActiveShellBrowser()->GetViewMode(); })
				.SetSelectedCallback(
					[this, viewMode] { GetActiveShellBrowser()->SetViewMode(viewMode); }));
	}
}

ShellBrowser *ViewsMenu::GetActiveShellBrowser() const
{
	return m_browser->GetActiveShellBrowser();
}
