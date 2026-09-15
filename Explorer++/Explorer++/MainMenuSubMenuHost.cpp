// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainMenuSubMenuHost.h"
#include "BrowserWindow.h"
#include "../Helper/MenuHelper.h"

MainMenuSubMenuHost::MainMenuSubMenuHost(BrowserWindow *browser, HMENU mainMenu,
	UINT subMenuItemId) :
	m_menu(MenuHelper::AttachNewSubMenu(mainMenu, subMenuItemId, false)),
	m_view(m_menu),
	m_controller(&m_view, browser),
	m_hwnd(browser->GetHWND())
{
}

MenuView *MainMenuSubMenuHost::GetView()
{
	return &m_view;
}

HMENU MainMenuSubMenuHost::GetMenu() const
{
	return m_menu;
}

void MainMenuSubMenuHost::OnSubMenuWillShow()
{
	m_controller.NotifyMenuWillShow(m_hwnd);
}

void MainMenuSubMenuHost::SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	m_controller.SelectItem(id, isCtrlKeyDown, isShiftKeyDown);
}
