// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuHelper.h"

void MenuHelper::AddStringItem(HMENU menu, UINT id, std::wstring &text)
{
	AddStringItem(menu, id, text, GetMenuItemCount(menu), TRUE);
}

void MenuHelper::AddStringItem(HMENU menu, UINT id, std::wstring &text, UINT item, BOOL byPosition)
{
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_STRING;
	mii.wID = id;
	mii.dwTypeData = text.data();
	InsertMenuItem(menu, item, byPosition, &mii);
}

void MenuHelper::AddSeparator(HMENU menu)
{
	AddSeparator(menu, GetMenuItemCount(menu), TRUE);
}

void MenuHelper::AddSeparator(HMENU menu, UINT item, BOOL byPosition)
{
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_FTYPE;
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(menu, item, byPosition, &mii);
}

void MenuHelper::AttachSubMenu(
	HMENU parentMenu, wil::unique_hmenu subMenu, UINT item, BOOL byPosition)
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

void MenuHelper::CheckItem(HMENU hMenu, UINT itemID, BOOL bCheck)
{
	UINT state = bCheck ? MF_CHECKED : MF_UNCHECKED;
	CheckMenuItem(hMenu, itemID, state);
}

void MenuHelper::EnableItem(HMENU hMenu, UINT itemID, BOOL bEnable)
{
	UINT state = bEnable ? MF_ENABLED : MF_DISABLED;
	EnableMenuItem(hMenu, itemID, state);
}