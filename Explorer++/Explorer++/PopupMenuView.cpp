// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PopupMenuView.h"
#include "../Helper/Helper.h"
#include "../Helper/WindowSubclassWrapper.h"

PopupMenuView::PopupMenuView(MenuController *controller) :
	m_menu(CreatePopupMenu()),
	m_controller(controller)
{
}

void PopupMenuView::AppendItem(int id, const std::wstring &text, wil::unique_hbitmap bitmap)
{
	// The call to TrackPopupMenu() below will return the ID of the item that was selected, with 0
	// being returned if the menu was canceled, or an error occurred. Therefore, 0 shouldn't be used
	// as an item ID.
	assert(id != 0);

	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_ID | MIIM_STRING;
	menuItemInfo.wID = id;
	menuItemInfo.dwTypeData = const_cast<LPWSTR>(text.c_str());

	if (bitmap)
	{
		menuItemInfo.fMask |= MIIM_BITMAP;
		menuItemInfo.hbmpItem = bitmap.get();

		m_menuImages.push_back(std::move(bitmap));
	}

	[[maybe_unused]] auto res =
		InsertMenuItem(m_menu.get(), GetMenuItemCount(m_menu.get()), true, &menuItemInfo);
	assert(res);
}

void PopupMenuView::SetBitmapForItem(UINT id, wil::unique_hbitmap bitmap)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_BITMAP;
	menuItemInfo.hbmpItem = bitmap.get();
	[[maybe_unused]] auto res = SetMenuItemInfo(m_menu.get(), id, false, &menuItemInfo);
	assert(res);

	if (bitmap)
	{
		m_menuImages.push_back(std::move(bitmap));
	}
}

void PopupMenuView::Show(HWND hwnd, const POINT &point)
{
	assert(GetMenuItemCount(m_menu.get()) > 0);

	// Subclass the parent window to allow middle clicks to be detected.
	auto subclass = std::make_unique<WindowSubclassWrapper>(hwnd,
		std::bind_front(&PopupMenuView::ParentWindowSubclass, this));

	UINT cmd = TrackPopupMenu(m_menu.get(), TPM_LEFTALIGN | TPM_VERTICAL | TPM_RETURNCMD, point.x,
		point.y, 0, hwnd, nullptr);

	if (cmd == 0)
	{
		return;
	}

	m_controller->OnMenuItemSelected(cmd, IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT));
}

LRESULT PopupMenuView::ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_MBUTTONUP:
		OnMenuMiddleButtonUp({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) },
			WI_IsFlagSet(wParam, MK_CONTROL), WI_IsFlagSet(wParam, MK_SHIFT));
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void PopupMenuView::OnMenuMiddleButtonUp(const POINT &pt, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	int itemIndex = MenuItemFromPoint(nullptr, m_menu.get(), pt);

	if (itemIndex < 0)
	{
		return;
	}

	auto menuItemId = GetMenuItemID(m_menu.get(), itemIndex);

	if (menuItemId == -1)
	{
		return;
	}

	m_controller->OnMenuItemMiddleClicked(menuItemId, isCtrlKeyDown, isShiftKeyDown);
}

int PopupMenuView::GetItemCountForTesting() const
{
	return GetMenuItemCount(m_menu.get());
}

UINT PopupMenuView::GetItemIdForTesting(int index) const
{
	return GetMenuItemID(m_menu.get(), index);
}

std::wstring PopupMenuView::GetItemTextForTesting(int index) const
{
	TCHAR text[256];

	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_STRING;
	menuItemInfo.dwTypeData = text;
	menuItemInfo.cch = static_cast<UINT>(std::size(text));
	auto res = GetMenuItemInfo(m_menu.get(), index, true, &menuItemInfo);

	if (!res)
	{
		return L"";
	}

	return text;
}
