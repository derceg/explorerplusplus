// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuView.h"
#include <wil/resource.h>

class PopupMenuView : public MenuView
{
public:
	PopupMenuView();

	void Show(HWND hwnd, const POINT &point);

private:
	HMENU GetMenu() const override;

	LRESULT ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnMenuMiddleButtonUp(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown);

	wil::unique_hmenu m_menu;
};
