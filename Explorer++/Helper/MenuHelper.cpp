// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuHelper.h"

namespace MenuHelper
{

void AddStyleFlags(HMENU menu, DWORD styleFlags)
{
	MENUINFO menuInfo = {};
	menuInfo.cbSize = sizeof(menuInfo);
	menuInfo.fMask = MIM_STYLE;
	auto res = GetMenuInfo(menu, &menuInfo);
	CHECK(res);

	WI_SetAllFlags(menuInfo.dwStyle, styleFlags);

	res = SetMenuInfo(menu, &menuInfo);
	CHECK(res);
}

DWORD GetStyleFlags(HMENU menu)
{
	MENUINFO menuInfo = {};
	menuInfo.cbSize = sizeof(menuInfo);
	menuInfo.fMask = MIM_STYLE;
	auto res = GetMenuInfo(menu, &menuInfo);
	CHECK(res);

	return menuInfo.dwStyle;
}

void AddStringItem(HMENU menu, UINT id, const std::wstring &text)
{
	AddStringItem(menu, id, text, GetMenuItemCount(menu), ItemLookup::ByPosition);
}

void AddStringItem(HMENU menu, UINT id, const std::wstring &text, UINT item, ItemLookup lookupType)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_ID | MIIM_STRING;
	menuItemInfo.wID = id;
	menuItemInfo.dwTypeData = const_cast<LPWSTR>(text.c_str());
	auto res = InsertMenuItem(menu, item, lookupType == ItemLookup::ByPosition, &menuItemInfo);
	CHECK(res);
}

void AddSeparator(HMENU menu)
{
	AddSeparator(menu, GetMenuItemCount(menu), ItemLookup::ByPosition);
}

void AddSeparator(HMENU menu, UINT item, ItemLookup lookupType)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_FTYPE;
	menuItemInfo.fType = MFT_SEPARATOR;
	auto res = InsertMenuItem(menu, item, lookupType == ItemLookup::ByPosition, &menuItemInfo);
	CHECK(res);
}

HMENU AddNewSubMenuItem(HMENU menu, UINT id, const std::wstring &text)
{
	return AddSubMenuItem(menu, id, text, CheckedCreatePopupMenu(), GetMenuItemCount(menu),
		ItemLookup::ByPosition);
}

HMENU AddSubMenuItem(HMENU menu, UINT id, const std::wstring &text, wil::unique_hmenu subMenu)
{
	return AddSubMenuItem(menu, id, text, std::move(subMenu), GetMenuItemCount(menu),
		ItemLookup::ByPosition);
}

// Note that an ID can be passed in here. While that is perhaps not the typical usage (e.g.
// GetMenuItemID() will return -1 for a submenu item), it does work correctly. That is, the ID set
// here can be retrieved by GetMenuItemInfo().
HMENU AddSubMenuItem(HMENU menu, UINT id, const std::wstring &text, wil::unique_hmenu subMenu,
	UINT item, ItemLookup lookupType)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_ID | MIIM_STRING | MIIM_SUBMENU;
	menuItemInfo.wID = id;
	menuItemInfo.dwTypeData = const_cast<LPWSTR>(text.c_str());
	menuItemInfo.hSubMenu = subMenu.get();
	auto res = InsertMenuItem(menu, item, lookupType == ItemLookup::ByPosition, &menuItemInfo);
	CHECK(res);

	// As the sub menu is now part of the parent menu, it will be destroyed when the parent menu is
	// destroyed.
	return subMenu.release();
}

void AddItemTypeFlags(HMENU menu, UINT item, ItemLookup lookupType, UINT typeFlags)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_FTYPE;
	auto res = GetMenuItemInfo(menu, item, lookupType == ItemLookup::ByPosition, &menuItemInfo);
	CHECK(res);

	WI_SetAllFlags(menuItemInfo.fType, typeFlags);
	res = SetMenuItemInfo(menu, item, lookupType == ItemLookup::ByPosition, &menuItemInfo);
	CHECK(res);
}

void SetItemChecked(HMENU menu, UINT item, ItemLookup lookupType, bool check)
{
	UINT state = check ? MF_CHECKED : MF_UNCHECKED;

	if (lookupType == ItemLookup::ByPosition)
	{
		WI_SetFlag(state, MF_BYPOSITION);
	}

	auto res = CheckMenuItem(menu, item, state);
	CHECK_NE(res, static_cast<DWORD>(-1));
}

void SetItemEnabled(HMENU menu, UINT item, ItemLookup lookupType, bool enable)
{
	UINT state = enable ? MF_ENABLED : MF_DISABLED;

	if (lookupType == ItemLookup::ByPosition)
	{
		WI_SetFlag(state, MF_BYPOSITION);
	}

	auto res = EnableMenuItem(menu, item, state);
	CHECK_NE(res, -1);
}

void RemoveDuplicateSeparators(HMENU menu)
{
	int count = GetMenuItemCount(menu);
	CHECK_NE(count, -1);

	bool previousItemSeparator = false;

	for (int i = count - 1; i >= 0; i--)
	{
		MENUITEMINFO menuItemInfo = {};
		menuItemInfo.cbSize = sizeof(menuItemInfo);
		menuItemInfo.fMask = MIIM_FTYPE;
		auto res = GetMenuItemInfo(menu, i, true, &menuItemInfo);
		CHECK(res);

		bool currentItemSeparator = WI_IsFlagSet(menuItemInfo.fType, MFT_SEPARATOR);

		if (previousItemSeparator && currentItemSeparator)
		{
			res = DeleteMenu(menu, i, MF_BYPOSITION);
			CHECK(res);
		}
		else
		{
			previousItemSeparator = currentItemSeparator;
		}
	}
}

// Removes any separators from the end of a menu.
void RemoveTrailingSeparators(HMENU menu)
{
	int count = GetMenuItemCount(menu);
	CHECK_NE(count, -1);

	for (int i = count - 1; i >= 0; i--)
	{
		MENUITEMINFO menuItemInfo = {};
		menuItemInfo.cbSize = sizeof(menuItemInfo);
		menuItemInfo.fMask = MIIM_FTYPE;
		auto res = GetMenuItemInfo(menu, i, true, &menuItemInfo);
		CHECK(res);

		if (menuItemInfo.fType != MFT_SEPARATOR)
		{
			break;
		}

		res = DeleteMenu(menu, i, MF_BYPOSITION);
		CHECK(res);
	}
}

std::wstring GetItemText(HMENU menu, UINT item, ItemLookup lookupType)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_STRING;
	menuItemInfo.dwTypeData = nullptr;
	auto res = GetMenuItemInfo(menu, item, lookupType == ItemLookup::ByPosition, &menuItemInfo);
	CHECK(res);

	// The returned size doesn't include space for the terminating null character.
	menuItemInfo.cch++;

	std::wstring text;
	text.resize(menuItemInfo.cch);

	menuItemInfo.dwTypeData = text.data();
	res = GetMenuItemInfo(menu, item, lookupType == ItemLookup::ByPosition, &menuItemInfo);
	CHECK(res);

	text.resize(menuItemInfo.cch);

	return text;
}

// While it is possible to set the ID of a submenu item, GetMenuItemID() will explicitly return -1
// in that case. This function, however, will call GetMenuItemInfo(), which will work even if the
// item is a submenu item.
UINT GetItemIdIncludingSubmenu(HMENU menu, int index)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_ID;
	auto res = GetMenuItemInfo(menu, index, true, &menuItemInfo);
	CHECK(res);

	return menuItemInfo.wID;
}

// Tests whether one menu is part of another menu. That is, whether the menus are equivalent, or
// whether the second menu is contained as a submenu of the first.
bool IsPartOfMenu(HMENU menu, HMENU potentiallyRelatedMenu)
{
	if (potentiallyRelatedMenu == menu)
	{
		return true;
	}

	int count = GetMenuItemCount(menu);
	CHECK_NE(count, -1);

	for (int i = 0; i < count; i++)
	{
		auto subMenu = GetSubMenu(menu, i);

		if (!subMenu)
		{
			continue;
		}

		bool partOfMenu = IsPartOfMenu(subMenu, potentiallyRelatedMenu);

		if (!partOfMenu)
		{
			continue;
		}

		return partOfMenu;
	}

	return false;
}

HBITMAP GetItemBitmap(HMENU menu, UINT item, ItemLookup lookupType)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_BITMAP;
	auto res = GetMenuItemInfo(menu, item, lookupType == ItemLookup::ByPosition, &menuItemInfo);
	CHECK(res);

	return menuItemInfo.hbmpItem;
}

bool IsItemEnabled(HMENU menu, UINT item, ItemLookup lookupType)
{
	UINT state = GetItemState(menu, item, lookupType);
	return !(state & MFS_DISABLED);
}

bool IsItemChecked(HMENU menu, UINT item, ItemLookup lookupType)
{
	UINT state = GetItemState(menu, item, lookupType);
	return WI_IsFlagSet(state, MFS_CHECKED);
}

UINT GetItemState(HMENU menu, UINT item, ItemLookup lookupType)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_STATE;
	auto res = GetMenuItemInfo(menu, item, lookupType == ItemLookup::ByPosition, &menuItemInfo);
	CHECK(res);

	return menuItemInfo.fState;
}

UINT GetItemTypeFlags(HMENU menu, UINT item, ItemLookup lookupType)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_FTYPE;
	auto res = GetMenuItemInfo(menu, item, lookupType == ItemLookup::ByPosition, &menuItemInfo);
	CHECK(res);

	return menuItemInfo.fType;
}

std::optional<UINT> MaybeGetHighlightedItem(HMENU menu)
{
	int count = GetMenuItemCount(menu);
	CHECK_NE(count, -1);

	for (int i = 0; i < count; i++)
	{
		MENUITEMINFO menuItemInfo = {};
		menuItemInfo.cbSize = sizeof(menuItemInfo);
		menuItemInfo.fMask = MIIM_ID | MIIM_STATE;
		auto res = GetMenuItemInfo(menu, i, true, &menuItemInfo);
		CHECK(res);

		if (WI_IsFlagSet(menuItemInfo.fState, MFS_HILITE))
		{
			return menuItemInfo.wID;
		}
	}

	return std::nullopt;
}

std::optional<UINT> MaybeGetItemAtPoint(HMENU menu, const POINT &ptScreen)
{
	// Note that the POINT passed to MenuItemFromPoint() needs to be in screen coordinates (even
	// though the documentation for the method indicates the POINT needs to be in client
	// coordinates).
	int item = MenuItemFromPoint(nullptr, menu, ptScreen);

	// Although the documentation for MenuItemFromPoint() states that it returns -1 if there's no
	// menu item at the specified position, it appears the method will also return other negative
	// values on failure.
	// That happens, for example, if the HWND parameter passed to MenuItemFromPoint() is non-null,
	// the menu refers to a submenu of the window menu and the mouse is above the top of the menu
	// bar.
	// While the HWND parameter here is always null, it's still probably better to check whether or
	// not the return value is positive, rather than whether it's not equal to -1. That's because
	// the function isn't adhering to its documented properties in at least some cases.
	if (item >= 0)
	{
		return GetItemIdIncludingSubmenu(menu, item);
	}

	int count = GetMenuItemCount(menu);
	CHECK_NE(count, -1);

	for (int i = 0; i < count; i++)
	{
		auto subMenu = GetSubMenu(menu, i);

		if (!subMenu)
		{
			continue;
		}

		auto menuItemId = MaybeGetItemAtPoint(subMenu, ptScreen);

		if (!menuItemId)
		{
			continue;
		}

		return *menuItemId;
	}

	return std::nullopt;
}

wil::unique_hmenu CheckedCreateMenu()
{
	wil::unique_hmenu menu(CreateMenu());
	CHECK(menu);
	return menu;
}

wil::unique_hmenu CheckedCreatePopupMenu()
{
	wil::unique_hmenu menu(CreatePopupMenu());
	CHECK(menu);
	return menu;
}

}
