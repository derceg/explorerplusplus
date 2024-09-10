// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuView.h"
#include "../Helper/MenuHelper.h"

MenuView::~MenuView()
{
	m_viewDestroyedSignal();
}

void MenuView::AppendItem(UINT id, const std::wstring &text, wil::unique_hbitmap bitmap,
	const std::wstring &helpText)
{
	// The value 0 shouldn't be used as an item ID. That's because a call like TrackPopupMenu() will
	// use a return value of 0 to indicate the menu was canceled, or an error occurred.
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

	auto [itr, didInsert] = m_itemHelpTextMapping.insert({ id, helpText });
	DCHECK(didInsert);
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

void MenuView::EnableItem(UINT id, bool enable)
{
	MenuHelper::EnableItem(GetMenu(), id, enable);
}

void MenuView::ClearMenu()
{
	for (int i = GetMenuItemCount(GetMenu()) - 1; i >= 0; i--)
	{
		auto res = DeleteMenu(GetMenu(), i, MF_BYPOSITION);
		DCHECK(res);
	}

	m_menuImages.clear();
	m_itemHelpTextMapping.clear();
}

std::wstring MenuView::GetHelpTextForItem(UINT id) const
{
	auto itr = m_itemHelpTextMapping.find(id);
	CHECK(itr != m_itemHelpTextMapping.end());
	return itr->second;
}

void MenuView::SelectItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	if (!MenuHelper::IsMenuItemEnabled(GetMenu(), id, false))
	{
		return;
	}

	m_itemSelectedSignal(id, isCtrlKeyDown, isShiftKeyDown);
}

void MenuView::MiddleClickItem(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	if (!MenuHelper::IsMenuItemEnabled(GetMenu(), id, false))
	{
		return;
	}

	m_itemMiddleClickedSignal(id, isCtrlKeyDown, isShiftKeyDown);
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
