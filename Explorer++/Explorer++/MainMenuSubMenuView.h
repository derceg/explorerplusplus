// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuView.h"

class BrowserWindow;

class MainMenuSubMenuView : public MenuView
{
public:
	MainMenuSubMenuView(BrowserWindow *browser, HMENU mainMenu, UINT subMenuItemId);

	HMENU GetMenu() const override;

	void OnSubMenuWillShow();
	void OnSubMenuClosed();

private:
	const HWND m_hwnd;
	HMENU m_menu = nullptr;
};
