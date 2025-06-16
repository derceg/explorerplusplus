// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuView.h"
#include "NoOpMenuHelpTextHost.h"
#include <wil/resource.h>
#include <string>

class MenuViewFake : public MenuView
{
public:
	MenuViewFake(MenuHelpTextHost *menuHelpTextHost = NoOpMenuHelpTextHost::GetInstance());

	using MenuView::OnMenuClosed;
	using MenuView::OnMenuWillShowForDpi;

	HMENU GetMenu() const override;

	int GetItemCount() const;
	UINT GetItemId(int index) const;
	std::wstring GetItemText(UINT id) const;
	HBITMAP GetItemBitmap(UINT id) const;

private:
	wil::unique_hmenu m_menu;
};
