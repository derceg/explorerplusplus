// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuHelper.h"

namespace MenuHelper
{

void AddStringItem(HMENU menu, UINT id, const std::wstring &text)
{
	AddStringItem(menu, id, text, GetMenuItemCount(menu), TRUE);
}

void AddStringItem(HMENU menu, UINT id, const std::wstring &text, UINT item, BOOL byPosition)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_ID | MIIM_STRING;
	menuItemInfo.wID = id;
	menuItemInfo.dwTypeData = const_cast<LPWSTR>(text.c_str());
	auto res = InsertMenuItem(menu, item, byPosition, &menuItemInfo);
	CHECK(res);
}

void AddSeparator(HMENU menu)
{
	AddSeparator(menu, GetMenuItemCount(menu), TRUE);
}

void AddSeparator(HMENU menu, UINT item, BOOL byPosition)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_FTYPE;
	menuItemInfo.fType = MFT_SEPARATOR;
	auto res = InsertMenuItem(menu, item, byPosition, &menuItemInfo);
	CHECK(res);
}

void AddSubMenuItem(HMENU menu, UINT id, const std::wstring &text, wil::unique_hmenu subMenu)
{
	AddSubMenuItem(menu, id, text, std::move(subMenu), GetMenuItemCount(menu), TRUE);
}

// Note that an ID can be passed in here. While that is perhaps not the typical usage (e.g.
// GetMenuItemID() will return -1 for a submenu item), it does work correctly. That is, the ID set
// here can be retrieved by GetMenuItemInfo().
void AddSubMenuItem(HMENU menu, UINT id, const std::wstring &text, wil::unique_hmenu subMenu,
	UINT item, BOOL byPosition)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_ID | MIIM_STRING | MIIM_SUBMENU;
	menuItemInfo.wID = id;
	menuItemInfo.dwTypeData = const_cast<LPWSTR>(text.c_str());
	menuItemInfo.hSubMenu = subMenu.release();
	auto res = InsertMenuItem(menu, item, byPosition, &menuItemInfo);
	CHECK(res);
}

void AttachSubMenu(HMENU parentMenu, wil::unique_hmenu subMenu, UINT item, BOOL byPosition)
{
	// As the sub menu is now part of the parent menu, it will be destroyed when the parent menu is
	// destroyed.
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_SUBMENU;
	menuItemInfo.hSubMenu = subMenu.release();
	auto res = SetMenuItemInfo(parentMenu, item, byPosition, &menuItemInfo);
	CHECK(res);
}

void CheckItem(HMENU hMenu, UINT itemID, BOOL bCheck)
{
	UINT state = bCheck ? MF_CHECKED : MF_UNCHECKED;
	auto res = CheckMenuItem(hMenu, itemID, state);
	CHECK_NE(res, static_cast<DWORD>(-1));
}

void EnableItem(HMENU hMenu, UINT itemID, BOOL bEnable)
{
	UINT state = bEnable ? MF_ENABLED : MF_DISABLED;
	auto res = EnableMenuItem(hMenu, itemID, state);
	CHECK_NE(res, -1);
}

void SetMenuStyle(HMENU menu, DWORD style)
{
	MENUINFO menuInfo = {};
	menuInfo.cbSize = sizeof(menuInfo);
	menuInfo.fMask = MIM_STYLE;
	menuInfo.dwStyle = style;
	auto res = SetMenuInfo(menu, &menuInfo);
	CHECK(res);
}

void SetBitmapForItem(HMENU menu, UINT id, HBITMAP bitmap)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_BITMAP;
	menuItemInfo.hbmpItem = bitmap;
	auto res = SetMenuItemInfo(menu, id, false, &menuItemInfo);
	CHECK(res);
}

void RemoveDuplicateSeperators(HMENU menu)
{
	int count = GetMenuItemCount(menu);
	CHECK_NE(count, -1);

	bool previousItemSeperator = false;

	for (int i = count - 1; i >= 0; i--)
	{
		MENUITEMINFO menuItemInfo = {};
		menuItemInfo.cbSize = sizeof(menuItemInfo);
		menuItemInfo.fMask = MIIM_FTYPE;
		auto res = GetMenuItemInfo(menu, i, TRUE, &menuItemInfo);
		CHECK(res);

		bool currentItemSeparator = WI_IsFlagSet(menuItemInfo.fType, MFT_SEPARATOR);

		if (previousItemSeperator && currentItemSeparator)
		{
			res = DeleteMenu(menu, i, MF_BYPOSITION);
			CHECK(res);
		}
		else
		{
			previousItemSeperator = currentItemSeparator;
		}
	}
}

// Removes any separators from the end of a menu.
void RemoveTrailingSeparators(HMENU menu)
{
	int count = GetMenuItemCount(menu);

	if (count == -1)
	{
		return;
	}

	for (int i = count - 1; i >= 0; i--)
	{
		MENUITEMINFO menuItemInfo = {};
		menuItemInfo.cbSize = sizeof(menuItemInfo);
		menuItemInfo.fMask = MIIM_FTYPE;
		auto res = GetMenuItemInfo(menu, i, TRUE, &menuItemInfo);
		CHECK(res);

		if (menuItemInfo.fType != MFT_SEPARATOR)
		{
			break;
		}

		res = DeleteMenu(menu, i, MF_BYPOSITION);
		CHECK(res);
	}
}

// Finds the parent of the specified menu item, within the provided menu. The return value will be
// the menu itself, or one of its submenus.
HMENU FindParentMenu(HMENU menu, UINT id)
{
	int numItems = GetMenuItemCount(menu);

	for (int i = 0; i < numItems; i++)
	{
		UINT currentId = GetMenuItemID(menu, i);

		if (currentId != -1 && currentId == id)
		{
			return menu;
		}

		HMENU subMenu = GetSubMenu(menu, i);

		if (subMenu)
		{
			HMENU parentMenu = FindParentMenu(subMenu, id);

			if (parentMenu)
			{
				return parentMenu;
			}
		}
	}

	return nullptr;
}

std::wstring GetMenuItemString(HMENU menu, UINT item, bool byPosition)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_STRING;
	menuItemInfo.dwTypeData = nullptr;
	auto res = GetMenuItemInfo(menu, item, byPosition, &menuItemInfo);
	CHECK(res);

	// The returned size doesn't include space for the terminating null character.
	menuItemInfo.cch++;

	std::wstring text;
	text.resize(menuItemInfo.cch);

	menuItemInfo.dwTypeData = text.data();
	res = GetMenuItemInfo(menu, item, byPosition, &menuItemInfo);
	CHECK(res);

	text.resize(menuItemInfo.cch);

	return text;
}

// While it is possible to set the ID of a submenu item, GetMenuItemID() will explicitly return -1
// in that case. This function, however, will call GetMenuItemInfo(), which will work even if the
// item is a submenu item.
UINT GetMenuItemIDIncludingSubmenu(HMENU menu, int index)
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

	for (int i = 0; i < GetMenuItemCount(menu); i++)
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

bool IsMenuItemEnabled(HMENU menu, UINT item, bool byPosition)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_STATE;
	auto res = GetMenuItemInfo(menu, item, byPosition, &menuItemInfo);
	CHECK(res);

	return !(menuItemInfo.fState & MFS_DISABLED);
}

// Returns the ID of the menu item at the specified point in screen coordinates, if there is such an
// item. Only items that are part of the provided menu will be considered.
std::optional<UINT> MaybeGetMenuItemAtPoint(HMENU menu, const POINT &ptScreen)
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
		return GetMenuItemIDIncludingSubmenu(menu, item);
	}

	for (int i = 0; i < GetMenuItemCount(menu); i++)
	{
		auto subMenu = GetSubMenu(menu, i);

		if (!subMenu)
		{
			continue;
		}

		auto menuItemId = MaybeGetMenuItemAtPoint(subMenu, ptScreen);

		if (!menuItemId)
		{
			continue;
		}

		return *menuItemId;
	}

	return std::nullopt;
}

}
