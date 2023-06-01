// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuHelper.h"

namespace MenuHelper
{

void AddStringItem(HMENU menu, UINT id, std::wstring &text)
{
	AddStringItem(menu, id, text, GetMenuItemCount(menu), TRUE);
}

void AddStringItem(HMENU menu, UINT id, std::wstring &text, UINT item, BOOL byPosition)
{
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.wID = id;
	mii.dwTypeData = text.data();
	InsertMenuItem(menu, item, byPosition, &mii);
}

void AddSeparator(HMENU menu)
{
	AddSeparator(menu, GetMenuItemCount(menu), TRUE);
}

void AddSeparator(HMENU menu, UINT item, BOOL byPosition)
{
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_FTYPE;
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(menu, item, byPosition, &mii);
}

void AddSubMenuItem(HMENU menu, std::wstring &text, wil::unique_hmenu subMenu)
{
	AddSubMenuItem(menu, text, std::move(subMenu), GetMenuItemCount(menu), TRUE);
}

void AddSubMenuItem(HMENU menu, std::wstring &text, wil::unique_hmenu subMenu, UINT item,
	BOOL byPosition)
{
	MENUITEMINFO mii = {};
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING | MIIM_SUBMENU;
	mii.dwTypeData = text.data();
	mii.hSubMenu = subMenu.release();
	InsertMenuItem(menu, item, byPosition, &mii);
}

void AttachSubMenu(HMENU parentMenu, wil::unique_hmenu subMenu, UINT item, BOOL byPosition)
{
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_SUBMENU;
	mii.hSubMenu = subMenu.get();
	SetMenuItemInfo(parentMenu, item, byPosition, &mii);

	// As the sub menu is now part of the parent menu, it will be destroyed when the parent menu is
	// destroyed.
	subMenu.release();
}

void CheckItem(HMENU hMenu, UINT itemID, BOOL bCheck)
{
	UINT state = bCheck ? MF_CHECKED : MF_UNCHECKED;
	CheckMenuItem(hMenu, itemID, state);
}

void EnableItem(HMENU hMenu, UINT itemID, BOOL bEnable)
{
	UINT state = bEnable ? MF_ENABLED : MF_DISABLED;
	EnableMenuItem(hMenu, itemID, state);
}

void SetMenuStyle(HMENU menu, DWORD style)
{
	MENUINFO menuInfo = {};
	menuInfo.cbSize = sizeof(menuInfo);
	menuInfo.fMask = MIM_STYLE;
	menuInfo.dwStyle = style;
	SetMenuInfo(menu, &menuInfo);
}

void RemoveDuplicateSeperators(HMENU menu)
{
	int count = GetMenuItemCount(menu);

	if (count == -1)
	{
		return;
	}

	bool previousItemSeperator = false;

	for (int i = count - 1; i >= 0; i--)
	{
		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_FTYPE;
		GetMenuItemInfo(menu, i, TRUE, &mii);

		bool currentItemSeparator = WI_IsFlagSet(mii.fType, MFT_SEPARATOR);

		if (previousItemSeperator && currentItemSeparator)
		{
			DeleteMenu(menu, i, MF_BYPOSITION);
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
		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_FTYPE;
		BOOL res = GetMenuItemInfo(menu, i, TRUE, &mii);

		if (!res || mii.fType != MFT_SEPARATOR)
		{
			break;
		}

		DeleteMenu(menu, i, MF_BYPOSITION);
	}
}

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

std::optional<std::wstring> GetMenuItemString(HMENU menu, UINT item, bool byPosition)
{
	MENUITEMINFO menuItemInfo = {};
	menuItemInfo.cbSize = sizeof(menuItemInfo);
	menuItemInfo.fMask = MIIM_STRING;
	menuItemInfo.dwTypeData = nullptr;
	auto res = GetMenuItemInfo(menu, item, byPosition, &menuItemInfo);

	if (!res)
	{
		return std::nullopt;
	}

	// The returned size doesn't include space for the terminating null character.
	menuItemInfo.cch++;

	std::wstring text;
	text.resize(menuItemInfo.cch);

	menuItemInfo.dwTypeData = text.data();
	res = GetMenuItemInfo(menu, item, byPosition, &menuItemInfo);

	if (!res)
	{
		return std::nullopt;
	}

	text.resize(menuItemInfo.cch);

	return text;
}

}
