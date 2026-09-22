// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainMenuHost.h"
#include "BrowserWindow.h"
#include "../Helper/MenuHelper.h"

MainMenuHost::MainMenuHost(BrowserWindow *browser) :
	m_menu(MenuHelper::CheckedCreateMenu()),
	m_view(m_menu.get()),
	m_controller(&m_view, browser->GetHWND(), browser)
{
}

HMENU MainMenuHost::ReleaseMenu()
{
	CHECK(m_menu);
	return m_menu.release();
}

MenuView *MainMenuHost::GetView()
{
	return &m_view;
}

bool MainMenuHost::CanHandleSelection(UINT id) const
{
	return m_controller.CanHandleSelection(id);
}

void MainMenuHost::SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	m_controller.SelectItem(id, isCtrlKeyDown, isShiftKeyDown);
}
