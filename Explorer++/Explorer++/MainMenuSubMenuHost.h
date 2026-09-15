// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuController.h"
#include "MenuView.h"

class BrowserWindow;

class MainMenuSubMenuHost
{
public:
	MainMenuSubMenuHost(BrowserWindow *browser, HMENU mainMenu, UINT subMenuItemId);

	MenuView *GetView();
	HMENU GetMenu() const;
	void OnSubMenuWillShow();
	void SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown);

private:
	const HMENU m_menu;
	MenuView m_view;
	MenuController m_controller;
	const HWND m_hwnd;
};
