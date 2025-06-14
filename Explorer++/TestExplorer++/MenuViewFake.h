// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuView.h"
#include <wil/resource.h>
#include <string>

class MenuViewFake : public MenuView
{
public:
	MenuViewFake();

	using MenuView::OnMenuClosed;
	using MenuView::OnMenuWillShowForDpi;

	int GetItemCount() const;
	UINT GetItemId(int index) const;
	std::wstring GetItemText(UINT id) const;
	HBITMAP GetItemBitmap(UINT id) const;

private:
	HMENU GetMenu() const override;

	wil::unique_hmenu m_menu;
};
