// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuController.h"
#include "MenuView.h"
#include "NoOpMenuHelpTextHost.h"
#include <wil/resource.h>

class MenuTestHost
{
public:
	MenuTestHost(MenuHelpTextHost *menuHelpTextHost = NoOpMenuHelpTextHost::GetInstance());

	MenuView *GetView();

	void SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown);
	void MiddleClickItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown);
	void SelectItemAtIndex(int index, bool isCtrlKeyDown, bool isShiftKeyDown);
	void MiddleClickItemAtIndex(int index, bool isCtrlKeyDown, bool isShiftKeyDown);

private:
	wil::unique_hmenu m_ownedMenu;
	MenuView m_view;
	MenuController m_controller;
};
