// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellItemsMenu.h"
#include "BrowserWindow.h"
#include "MenuView.h"
#include "NavigationHelper.h"
#include "ShellIconModel.h"
#include "../Helper/DragDropHelper.h"
#include "../Helper/Pidl.h"
#include "../Helper/ShellHelper.h"

ShellItemsMenu::ShellItemsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	const std::vector<PidlAbsolute> &pidls, BrowserWindow *browser,
	ShellIconLoader *shellIconLoader) :
	MenuBase(menuView, acceleratorManager),
	m_browser(browser),
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
			.SetSelectedCallback(
				std::bind_front(&ShellItemsMenu::OpenSelectedItem, this, pidl, false))
			.SetMiddleClickedCallback(
				std::bind_front(&ShellItemsMenu::OpenSelectedItem, this, pidl, true))
			.SetDraggedCallback([this, pidl] { return OnItemDragged(pidl); }));
}

void ShellItemsMenu::OpenSelectedItem(const PidlAbsolute &pidl, bool isMiddleButtonDown,
	bool isCtrlKeyDown, bool isShiftKeyDown)
{
	m_browser->OpenItem(pidl.Raw(),
		DetermineOpenDisposition(isMiddleButtonDown, isCtrlKeyDown, isShiftKeyDown));
}

MenuDragAction ShellItemsMenu::OnItemDragged(const PidlAbsolute &pidl)
{
	HRESULT hr = StartDragForShellItems({ pidl.Raw() });
	return hr == DRAGDROP_S_DROP ? MenuDragAction::CloseMenu : MenuDragAction::ContinueMenu;
}
