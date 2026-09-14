// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellItemsMenu.h"
#include "BrowserWindow.h"
#include "MenuView.h"
#include "NavigationHelper.h"
#include "ShellIconModel.h"
#include "../Helper/ShellHelper.h"
#include <glog/logging.h>

ShellItemsMenu::ShellItemsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	const std::vector<PidlAbsolute> &pidls, BrowserWindow *browserWindow,
	ShellIconLoader *shellIconLoader, UINT startId, UINT endId) :
	MenuBase(menuView, acceleratorManager, startId, endId),
	m_browserWindow(browserWindow),
	m_shellIconLoader(shellIconLoader)
{
	m_menuView->SetDelegate(this);

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
	auto id = m_idCounter++;

	if (id >= GetIdRange().endId)
	{
		return;
	}

	m_menuView->AppendItem(id, GetDisplayNameWithFallback(pidl, SHGDN_NORMAL),
		std::make_unique<ShellIconModel>(m_shellIconLoader, pidl),
		GetFolderPathForDisplayWithFallback(pidl));

	auto [itr, didInsert] = m_idPidlMap.insert({ id, pidl });
	DCHECK(didInsert);
}

void ShellItemsMenu::OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	OpenSelectedItem(id, false, isCtrlKeyDown, isShiftKeyDown);
}

void ShellItemsMenu::OnItemMiddleClicked(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	OpenSelectedItem(id, true, isCtrlKeyDown, isShiftKeyDown);
}

void ShellItemsMenu::OpenSelectedItem(UINT id, bool isMiddleButtonDown, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	auto itr = m_idPidlMap.find(id);
	CHECK(itr != m_idPidlMap.end());
	m_browserWindow->OpenItem(itr->second.Raw(),
		DetermineOpenDisposition(isMiddleButtonDown, isCtrlKeyDown, isShiftKeyDown));
}
