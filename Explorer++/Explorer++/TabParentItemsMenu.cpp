// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabParentItemsMenu.h"
#include "BrowserWindow.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "../Helper/ShellHelper.h"

TabParentItemsMenu::TabParentItemsMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, BrowserWindow *browserWindow,
	IconFetcher *iconFetcher) :
	ShellItemsMenu(menuView, acceleratorManager,
		GetParentPidlCollection(browserWindow->GetActiveShellBrowser()), browserWindow, iconFetcher)
{
}

TabParentItemsMenu::TabParentItemsMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, BrowserWindow *browserWindow,
	IconFetcher *iconFetcher, UINT menuStartId, UINT menuEndId) :
	ShellItemsMenu(menuView, acceleratorManager,
		GetParentPidlCollection(browserWindow->GetActiveShellBrowser()), browserWindow, iconFetcher,
		menuStartId, menuEndId)
{
}

// Returns a vector containing the pidl of each parent item, proceeding from the root to the first
// parent.
std::vector<PidlAbsolute> TabParentItemsMenu::GetParentPidlCollection(
	const ShellBrowser *shellBrowser)
{
	auto *currentEntry = shellBrowser->GetNavigationController()->GetCurrentEntry();
	CHECK(currentEntry);

	std::vector<PidlAbsolute> pidls;
	auto currentPidl = currentEntry->GetPidl();

	// Shouldn't be attempting to show the list of parent items for the root folder.
	DCHECK(!IsNamespaceRoot(currentPidl.Raw()));

	while (ILRemoveLastID(currentPidl.Raw()))
	{
		pidls.emplace_back(currentPidl.Raw());
	}

	// Items in the menu will be displayed in the same order they appear in the vector. Since the
	// root item should appear first, the vector generated above needs to be reversed.
	std::reverse(pidls.begin(), pidls.end());

	return pidls;
}
