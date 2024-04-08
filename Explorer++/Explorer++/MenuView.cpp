// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuView.h"

MenuView::~MenuView()
{
	m_viewDestroyedSignal();
}

void MenuView::AppendItem(UINT id, const std::wstring &text, wil::unique_hbitmap bitmap)
{
	// The call to TrackPopupMenu() below will return the ID of the item that was selected, with
	// 0 being returned if the menu was canceled, or an error occurred. Therefore, 0 shouldn't
	// be used as an item ID.
	DCHECK_NE(id, 0U);

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

	auto res = InsertMenuItem(GetMenu(), GetMenuItemCount(GetMenu()), true, &menuItemInfo);
	CHECK(res);
}

void MenuView::SetBitmapForItem(UINT id, wil::unique_hbitmap bitmap)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_BITMAP;
	menuItemInfo.hbmpItem = bitmap.get();
	auto res = SetMenuItemInfo(GetMenu(), id, false, &menuItemInfo);
	CHECK(res);

	if (bitmap)
	{
		m_menuImages.push_back(std::move(bitmap));
	}
}

void MenuView::SelectItem(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	m_itemSelectedSignal(menuItemId, isCtrlKeyDown, isShiftKeyDown);
}

void MenuView::MiddleClickItem(UINT menuItemId, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	m_itemMiddleClickedSignal(menuItemId, isCtrlKeyDown, isShiftKeyDown);
}

boost::signals2::connection MenuView::AddItemSelectedObserver(
	const ItemSelectedSignal::slot_type &observer)
{
	return m_itemSelectedSignal.connect(observer);
}

boost::signals2::connection MenuView::AddItemMiddleClickedObserver(
	const ItemMiddleClickedSignal::slot_type &observer)
{
	return m_itemMiddleClickedSignal.connect(observer);
}

boost::signals2::connection MenuView::AddViewDestroyedObserver(
	const ViewDestroyedSignal::slot_type &observer)
{
	return m_viewDestroyedSignal.connect(observer);
}

int MenuView::GetItemCountForTesting() const
{
	return GetMenuItemCount(GetMenu());
}

UINT MenuView::GetItemIdForTesting(int index) const
{
	return GetMenuItemID(GetMenu(), index);
}

std::wstring MenuView::GetItemTextForTesting(UINT id) const
{
	TCHAR text[256];

	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_STRING;
	menuItemInfo.dwTypeData = text;
	menuItemInfo.cch = static_cast<UINT>(std::size(text));
	auto res = GetMenuItemInfo(GetMenu(), id, false, &menuItemInfo);
	CHECK(res);

	return text;
}

HBITMAP MenuView::GetItemBitmapForTesting(UINT id) const
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_BITMAP;
	auto res = GetMenuItemInfo(GetMenu(), id, false, &menuItemInfo);
	CHECK(res);

	return menuItemInfo.hbmpItem;
}
