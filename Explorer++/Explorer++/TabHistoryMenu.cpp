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

TabHistoryMenu::TabHistoryMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	BrowserWindow *browserWindow, ShellIconLoader *shellIconLoader, MenuType type) :
	MenuBase(menuView, acceleratorManager),
	m_browserWindow(browserWindow),
	m_shellIconLoader(shellIconLoader),
	m_type(type)
{
	Initialize();
}

void TabHistoryMenu::Initialize()
{
	BuildMenu();

	m_connections.push_back(m_menuView->AddItemSelectedObserver(
		std::bind_front(&TabHistoryMenu::OnMenuItemSelected, this)));
	m_connections.push_back(m_menuView->AddItemMiddleClickedObserver(
		std::bind_front(&TabHistoryMenu::OnMenuItemMiddleClicked, this)));
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

	m_menuView->AppendItem(id, entry->GetDisplayName(),
		ShellIconModel(m_shellIconLoader, entry->GetPidl().Raw()));
}

void TabHistoryMenu::OnMenuItemSelected(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	NavigateToHistoryEntry(menuItemId, false, isCtrlKeyDown, isShiftKeyDown);
}

void TabHistoryMenu::OnMenuItemMiddleClicked(UINT menuItemId, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	NavigateToHistoryEntry(menuItemId, true, isCtrlKeyDown, isShiftKeyDown);
}

void TabHistoryMenu::NavigateToHistoryEntry(UINT menuItemId, bool isMiddleButtonDown,
	bool isCtrlKeyDown, bool isShiftKeyDown)
{
	int offset = menuItemId;

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
