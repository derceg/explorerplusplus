// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellItemsMenu.h"
#include "BrowserWindow.h"
#include "MenuView.h"
#include "NavigationHelper.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/ShellHelper.h"
#include <glog/logging.h>

ShellItemsMenu::ShellItemsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	const std::vector<PidlAbsolute> &pidls, BrowserWindow *browserWindow,
	ShellIconLoader *shellIconLoader, UINT startId, UINT endId) :
	MenuBase(menuView, acceleratorManager, startId, endId),
	m_browserWindow(browserWindow),
	m_shellIconLoader(shellIconLoader)
{
	m_connections.push_back(m_menuView->AddItemSelectedObserver(
		std::bind_front(&ShellItemsMenu::OnMenuItemSelected, this)));
	m_connections.push_back(m_menuView->AddItemMiddleClickedObserver(
		std::bind_front(&ShellItemsMenu::OnMenuItemMiddleClicked, this)));

	RebuildMenu(pidls);
}

void ShellItemsMenu::RebuildMenu(const std::vector<PidlAbsolute> &pidls)
{
	m_menuView->ClearMenu();
	m_idCounter = GetIdRange().startId;
	m_idPidlMap.clear();

	for (const auto &pidl : pidls)
	{
		AddMenuItemForPidl(pidl.Raw());
	}
}

void ShellItemsMenu::AddMenuItemForPidl(PCIDLIST_ABSOLUTE pidl)
{
	std::wstring name;
	HRESULT hr = GetDisplayName(pidl, SHGDN_NORMAL, name);

	if (FAILED(hr))
	{
		DCHECK(false);

		name = L"(Unknown)";
	}

	auto id = m_idCounter++;

	if (id >= GetIdRange().endId)
	{
		return;
	}

	std::wstring displayPath;

	if (auto optionalDisplayPath = GetFolderPathForDisplay(pidl))
	{
		displayPath = *optionalDisplayPath;
	}

	m_menuView->AppendItem(id, name, ShellIconModel(m_shellIconLoader, pidl), displayPath);

	auto [itr, didInsert] = m_idPidlMap.insert({ id, pidl });
	DCHECK(didInsert);
}

void ShellItemsMenu::OnMenuItemSelected(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	OpenSelectedItem(menuItemId, false, isCtrlKeyDown, isShiftKeyDown);
}

void ShellItemsMenu::OnMenuItemMiddleClicked(UINT menuItemId, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	OpenSelectedItem(menuItemId, true, isCtrlKeyDown, isShiftKeyDown);
}

void ShellItemsMenu::OpenSelectedItem(UINT menuItemId, bool isMiddleButtonDown, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	auto &pidl = m_idPidlMap.at(menuItemId);
	m_browserWindow->OpenItem(pidl.Raw(),
		DetermineOpenDisposition(isMiddleButtonDown, isCtrlKeyDown, isShiftKeyDown));
}
