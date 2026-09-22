// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuController.h"
#include "MenuView.h"
#include <wil/resource.h>

class BrowserWindow;

class MainMenuHost
{
public:
	MainMenuHost(BrowserWindow *browser);

	HMENU ReleaseMenu();

	MenuView *GetView();

	bool CanHandleSelection(UINT id) const;
	void SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown);

private:
	wil::unique_hmenu m_menu;
	MenuView m_view;
	MenuController m_controller;
};
