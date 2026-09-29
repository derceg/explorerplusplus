// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellItemsMenu.h"
#include "BrowserWindow.h"
#include "MenuView.h"
#include "NavigationHelper.h"
#include "ShellIconModel.h"
#include "../Helper/Pidl.h"
#include "../Helper/ShellHelper.h"

ShellItemsMenu::ShellItemsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	const std::vector<PidlAbsolute> &pidls, BrowserWindow *browserWindow,
	ShellIconLoader *shellIconLoader) :
	MenuBase(menuView, acceleratorManager),
	m_browserWindow(browserWindow),
	m_shellIconLoader(shellIconLoader)
{
	RebuildMenu(pidls);
}

void ShellItemsMenu::RebuildMenu(const std::vector<PidlAbsolute> &pidls)
{
	m_rootMenuView->ClearMenu();

	for (const auto &pidl : pidls)
	{
		AddMenuItemForPidl(pidl);
	}
}

void ShellItemsMenu::AddMenuItemForPidl(const PidlAbsolute &pidl)
{
	m_rootMenuView->AppendItem(MenuItemParams(GetDisplayNameWithFallback(pidl.Raw(), SHGDN_NORMAL))
			.SetIconModel(std::make_unique<ShellIconModel>(m_shellIconLoader, pidl.Raw()))
			.SetHelpText(GetFolderPathForDisplayWithFallback(pidl.Raw()))
			.SetSelectedCallback([this, pidl](bool isCtrlKeyDown, bool isShiftKeyDown)
				{ OpenSelectedItem(pidl.Raw(), false, isCtrlKeyDown, isShiftKeyDown); })
			.SetMiddleClickedCallback([this, pidl](bool isCtrlKeyDown, bool isShiftKeyDown)
				{ OpenSelectedItem(pidl.Raw(), true, isCtrlKeyDown, isShiftKeyDown); }));
}

void ShellItemsMenu::OpenSelectedItem(PCIDLIST_ABSOLUTE pidl, bool isMiddleButtonDown,
	bool isCtrlKeyDown, bool isShiftKeyDown)
{
	m_browserWindow->OpenItem(pidl,
		DetermineOpenDisposition(isMiddleButtonDown, isCtrlKeyDown, isShiftKeyDown));
}
