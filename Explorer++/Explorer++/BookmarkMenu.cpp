// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkMenu.h"
#include "MainResource.h"
#include "../Helper/Macros.h"

BookmarkMenu::BookmarkMenu(HINSTANCE instance) :
	m_instance(instance),
	m_menuBuilder(instance)
{

}

BOOL BookmarkMenu::ShowMenu(HWND parentWindow, const BookmarkItem *bookmarkItem,
	const POINT &pt, MenuCallback callback)
{
	HMENU menu = CreatePopupMenu();

	if (menu == nullptr)
	{
		return FALSE;
	}

	BookmarkMenuBuilder::ItemMap menuItemMappings;
	BOOL res = m_menuBuilder.BuildMenu(menu, bookmarkItem, { MIN_ID, MAX_ID }, 0, menuItemMappings);

	if (!res)
	{
		return FALSE;
	}

	int cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, parentWindow, nullptr);

	if (cmd != 0)
	{
		OnMenuItemSelected(cmd, menuItemMappings, callback);
	}

	DestroyMenu(menu);

	return TRUE;
}

void BookmarkMenu::OnMenuItemSelected(int menuItemId, BookmarkMenuBuilder::ItemMap &menuItemMappings,
	MenuCallback callback)
{
	auto itr = menuItemMappings.find(menuItemId);

	if (itr == menuItemMappings.end())
	{
		return;
	}

	if (!callback)
	{
		return;
	}

	callback(itr->second);
}