// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuView.h"

class MainMenuSubMenuView : public MenuView
{
public:
	MainMenuSubMenuView(HMENU mainMenu, UINT subMenuItemId);

private:
	HMENU GetMenu() const override;

	HMENU m_menu = nullptr;
};
