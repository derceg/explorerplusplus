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

TabHistoryMenu::TabHistoryMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	BrowserWindow *browserWindow, ShellIconLoader *shellIconLoader, MenuType type) :
	MenuBase(menuView, acceleratorManager),
	m_browserWindow(browserWindow),
	m_shellIconLoader(shellIconLoader),
	m_type(type)
{
	m_menuView->SetDelegate(this);

	BuildMenu();
}

void TabHistoryMenu::BuildMenu()
{
	auto *shellBrowser = GetShellBrowser();
	std::vector<HistoryEntry *> history;

	if (m_type == MenuType::Back)
	{
		history = shellBrowser->GetNavigationController()->GetBackHistory();
	}
	else
	{
		history = shellBrowser->GetNavigationController()->GetForwardHistory();
	}

	// This class shouldn't be invoked in a situation where there is no history for a tab.
	DCHECK(!history.empty());

	for (auto *entry : history)
	{
		AddMenuItemForHistoryEntry(entry);
	}
}

void TabHistoryMenu::AddMenuItemForHistoryEntry(const HistoryEntry *entry)
{
	auto id = m_idCounter++;

	m_menuView->AppendItem(id, GetDisplayNameWithFallback(entry->GetPidl().Raw(), SHGDN_INFOLDER),
		std::make_unique<ShellIconModel>(m_shellIconLoader, entry->GetPidl().Raw()));
}

void TabHistoryMenu::OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	NavigateToHistoryEntry(id, false, isCtrlKeyDown, isShiftKeyDown);
}

void TabHistoryMenu::OnItemMiddleClicked(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	NavigateToHistoryEntry(id, true, isCtrlKeyDown, isShiftKeyDown);
}

void TabHistoryMenu::NavigateToHistoryEntry(UINT id, bool isMiddleButtonDown, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	int offset = id;

	if (m_type == MenuType::Back)
	{
		offset = -offset;
	}

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
