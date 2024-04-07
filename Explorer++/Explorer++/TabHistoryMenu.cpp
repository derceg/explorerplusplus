// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabHistoryMenu.h"
#include "BrowserPane.h"
#include "BrowserWindow.h"
#include "NavigationHelper.h"
#include "ShellBrowser/HistoryEntry.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "Tab.h"
#include "TabContainer.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/ShellHelper.h"

TabHistoryMenu::TabHistoryMenu(BrowserWindow *browserWindow, MenuType type) :
	m_browserWindow(browserWindow),
	m_type(type)
{
	Initialize();
}

TabHistoryMenu::TabHistoryMenu(ShellBrowser *shellBrowser, MenuType type) :
	m_testShellBrowser(shellBrowser),
	m_type(type)
{
	Initialize();
}

void TabHistoryMenu::Initialize()
{
	FAIL_FAST_IF_FAILED(SHGetImageList(SHIL_SYSSMALL, IID_PPV_ARGS(&m_systemImageList)));
	FAIL_FAST_IF_FAILED(GetDefaultFolderIconIndex(m_defaultFolderIconIndex));

	m_menuView = BuildMenu();
}

const PopupMenuView *TabHistoryMenu::GetMenuViewForTesting() const
{
	return m_menuView.get();
}

void TabHistoryMenu::Show(HWND hwnd, const POINT &point)
{
	m_menuView->Show(hwnd, point);
}

std::unique_ptr<PopupMenuView> TabHistoryMenu::BuildMenu()
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
	assert(!history.empty());

	auto menuView = std::make_unique<PopupMenuView>(this);

	for (auto *entry : history)
	{
		AddMenuItemForHistoryEntry(menuView.get(), entry);
	}

	return menuView;
}

void TabHistoryMenu::AddMenuItemForHistoryEntry(PopupMenuView *menuView, HistoryEntry *entry)
{
	auto id = m_idCounter++;

	wil::unique_hbitmap bitmap;
	auto iconIndex = entry->GetSystemIconIndex();

	if (iconIndex)
	{
		bitmap = ImageHelper::ImageListIconToBitmap(m_systemImageList.get(), *iconIndex);
	}
	else
	{
		bitmap =
			ImageHelper::ImageListIconToBitmap(m_systemImageList.get(), m_defaultFolderIconIndex);
	}

	menuView->AppendItem(id, entry->GetDisplayName(), std::move(bitmap));
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

	// m_browserWindow will be null in tests.
	if (m_browserWindow)
	{
		auto disposition =
			DetermineOpenDisposition(isMiddleButtonDown, isCtrlKeyDown, isShiftKeyDown);

		if (disposition != OpenFolderDisposition::CurrentTab)
		{
			m_browserWindow->OpenItem(entry->GetPidl().Raw(), disposition);
			return;
		}
	}

	shellBrowser->GetNavigationController()->GoToOffset(offset);
}

ShellBrowser *TabHistoryMenu::GetShellBrowser() const
{
	if (m_testShellBrowser)
	{
		return m_testShellBrowser;
	}

	return m_browserWindow->GetActivePane()->GetTabContainer()->GetSelectedTab().GetShellBrowser();
}
