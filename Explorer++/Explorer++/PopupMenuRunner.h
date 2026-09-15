// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuController.h"
#include "MenuView.h"
#include <wil/resource.h>

class MenuHelpTextHost;

class PopupMenuRunner
{
public:
	PopupMenuRunner(MenuHelpTextHost *menuHelpTextHost);

	MenuView *GetView();
	void Show(HWND hwnd, const POINT &ptScreen);

private:
	wil::unique_hmenu m_ownedMenu;
	MenuView m_view;
	MenuController m_controller;
	MenuHelpTextHost *const m_menuHelpTextHost;
};
