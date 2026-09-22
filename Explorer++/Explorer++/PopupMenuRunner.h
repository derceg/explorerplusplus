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
	PopupMenuRunner(HWND ownerWindow, MenuHelpTextHost *menuHelpTextHost);

	MenuView *GetView();
	void Show(const POINT &ptScreen);

private:
	const HWND m_ownerWindow;
	const wil::unique_hmenu m_ownedMenu;
	MenuView m_view;
	MenuController m_controller;
};
