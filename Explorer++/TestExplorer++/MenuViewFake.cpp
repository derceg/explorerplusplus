// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MenuViewFake.h"

MenuViewFake::MenuViewFake() : m_menu(CreatePopupMenu())
{
	CHECK(m_menu);
}

int MenuViewFake::GetItemCount() const
{
	int itemCount = GetMenuItemCount(m_menu.get());
	CHECK_NE(itemCount, -1);
	return itemCount;
}

UINT MenuViewFake::GetItemId(int index) const
{
	UINT id = GetMenuItemID(m_menu.get(), index);
	CHECK_NE(id, static_cast<UINT>(-1));
	return id;
}

std::wstring MenuViewFake::GetItemText(UINT id) const
{
	wchar_t text[256];

	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_STRING;
	menuItemInfo.dwTypeData = text;
	menuItemInfo.cch = std::size(text);
	auto res = GetMenuItemInfo(m_menu.get(), id, false, &menuItemInfo);
	CHECK(res);

	return text;
}

HBITMAP MenuViewFake::GetItemBitmap(UINT id) const
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_BITMAP;
	auto res = GetMenuItemInfo(m_menu.get(), id, false, &menuItemInfo);
	CHECK(res);

	return menuItemInfo.hbmpItem;
}

HMENU MenuViewFake::GetMenu() const
{
	return m_menu.get();
}
