// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabHistoryMenu.h"
#include "BrowserWindow.h"
#include "MenuView.h"
#include "NavigationHelper.h"
#include "ShellBrowser/HistoryEntry.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellIconModel.h"
#include "../Helper/ShellHelper.h"

TabHistoryMenu::TabHistoryMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	BrowserWindow *browserWindow, ShellIconLoader *shellIconLoader, MenuType type) :
	MenuBase(menuView, acceleratorManager),
	m_browserWindow(browserWindow)
{
	BuildMenu(shellIconLoader, type);
}

void TabHistoryMenu::BuildMenu(ShellIconLoader *shellIconLoader, MenuType type)
{
	auto *shellBrowser = GetShellBrowser();
	std::vector<HistoryEntry *> history;

	if (type == MenuType::Back)
	{
		history = shellBrowser->GetNavigationController()->GetBackHistory();
	}
	else
	{
		history = shellBrowser->GetNavigationController()->GetForwardHistory();
	}

	// This class shouldn't be invoked in a situation where there is no history for a tab.
	DCHECK(!history.empty());
	int index = 0;

	for (auto *entry : history)
	{
		const auto &pidl = entry->GetPidl();
		int offset = (index + 1) * (type == MenuType::Back ? -1 : 1);

		m_rootMenuView->AppendItem(
			MenuItemParams(GetDisplayNameWithFallback(pidl.Raw(), SHGDN_INFOLDER))
				.SetIconModel(std::make_unique<ShellIconModel>(shellIconLoader, pidl.Raw()))
				.SetHelpText(GetFolderPathForDisplayWithFallback(pidl.Raw()))
				.SetSelectedCallback([this, offset](bool isCtrlKeyDown, bool isShiftKeyDown)
					{ NavigateToHistoryEntry(offset, false, isCtrlKeyDown, isShiftKeyDown); })
				.SetMiddleClickedCallback([this, offset](bool isCtrlKeyDown, bool isShiftKeyDown)
					{ NavigateToHistoryEntry(offset, true, isCtrlKeyDown, isShiftKeyDown); }));

		index++;
	}
}

void TabHistoryMenu::NavigateToHistoryEntry(int offset, bool isMiddleButtonDown, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	auto *shellBrowser = GetShellBrowser();
	auto *entry = shellBrowser->GetNavigationController()->GetEntry(offset);

	if (!entry)
	{
		return;
	}

	auto disposition = DetermineOpenDisposition(isMiddleButtonDown, isCtrlKeyDown, isShiftKeyDown);

	if (disposition != OpenFolderDisposition::CurrentTab)
	{
		m_browserWindow->OpenItem(entry->GetPidl().Raw(), disposition);
		return;
	}

	shellBrowser->GetNavigationController()->GoToOffset(offset);
}

ShellBrowser *TabHistoryMenu::GetShellBrowser() const
{
	return m_browserWindow->GetActiveShellBrowser();
}
