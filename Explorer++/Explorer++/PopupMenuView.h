// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "RootMenuView.h"

class PopupMenuView : public RootMenuView
{
public:
	PopupMenuView(HWND ownerWindow, MenuHelpTextHost *menuHelpTextHost);

	void Show(const POINT &ptScreen);

private:
	const HWND m_ownerWindow;
};
