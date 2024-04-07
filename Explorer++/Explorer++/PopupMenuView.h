// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <vector>

class MenuController
{
public:
	virtual ~MenuController() = default;

	virtual void OnMenuItemSelected(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown) = 0;
	virtual void OnMenuItemMiddleClicked(UINT menuItemId, bool isCtrlKeyDown,
		bool isShiftKeyDown) = 0;
};

class PopupMenuView
{
public:
	PopupMenuView(MenuController *controller);

	void AppendItem(UINT id, const std::wstring &text, wil::unique_hbitmap bitmap = nullptr);
	void SetBitmapForItem(UINT id, wil::unique_hbitmap bitmap);
	void Show(HWND hwnd, const POINT &point);

	int GetItemCountForTesting() const;
	UINT GetItemIdForTesting(int index) const;
	std::wstring GetItemTextForTesting(UINT id) const;
	HBITMAP GetItemBitmapForTesting(UINT id) const;

private:
	LRESULT ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnMenuMiddleButtonUp(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown);

	wil::unique_hmenu m_menu;
	std::vector<wil::unique_hbitmap> m_menuImages;
	MenuController *m_controller = nullptr;
};
