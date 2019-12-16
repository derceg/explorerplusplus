// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkMenu.h"
#include "MainResource.h"
#include "../Helper/Macros.h"

BookmarkMenu::BookmarkMenu(HINSTANCE instance) :
	m_instance(instance)
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

	m_idCounter = 1;
	m_menuItemMap.clear();
	BOOL res = BuildBookmarksMenu(menu, bookmarkItem, 0);

	if (!res)
	{
		return FALSE;
	}

	int cmd = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, parentWindow, nullptr);

	if (cmd != 0)
	{
		OnMenuItemSelected(cmd, callback);
	}

	DestroyMenu(menu);

	return TRUE;
}

BOOL BookmarkMenu::BuildBookmarksMenu(HMENU menu, const BookmarkItem *bookmarkItem, int startPosition)
{
	if (bookmarkItem->GetChildren().empty())
	{
		return AddEmptyBookmarkFolderToMenu(menu, startPosition);
	}

	int position = startPosition;

	for (auto &childItem : bookmarkItem->GetChildren())
	{
		BOOL res;

		if (childItem->GetType() == BookmarkItem::Type::Folder)
		{
			res = AddBookmarkFolderToMenu(menu, bookmarkItem, position);
		}
		else
		{
			res = AddBookmarkToMenu(menu, bookmarkItem, position);
		}

		if (!res)
		{
			return FALSE;
		}

		position++;
	}

	return TRUE;
}

BOOL BookmarkMenu::AddEmptyBookmarkFolderToMenu(HMENU menu, int position)
{
	TCHAR bookmarkFolderEmpty[32];
	LoadString(m_instance, IDS_BOOKMARK_FOLDER_EMPTY,
		bookmarkFolderEmpty, SIZEOF_ARRAY(bookmarkFolderEmpty));

	TCHAR menuText[64];
	StringCchPrintf(menuText, SIZEOF_ARRAY(menuText), _T("(%s)"), bookmarkFolderEmpty);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING | MIIM_STATE;
	mii.fState = MFS_DISABLED;
	mii.dwTypeData = menuText;

	return InsertMenuItem(menu, position, TRUE, &mii);
}

BOOL BookmarkMenu::AddBookmarkFolderToMenu(HMENU menu, const BookmarkItem *bookmarkItem, int position)
{
	TCHAR bookmarkFolderName[256];
	StringCchCopy(bookmarkFolderName, SIZEOF_ARRAY(bookmarkFolderName), bookmarkItem->GetName().c_str());

	HMENU subMenu = CreateMenu();

	if (subMenu == nullptr)
	{
		return FALSE;
	}

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING | MIIM_SUBMENU;
	mii.dwTypeData = bookmarkFolderName;
	mii.hSubMenu = subMenu;
	BOOL res = InsertMenuItem(menu, position, TRUE, &mii);

	if (!res)
	{
		return FALSE;
	}

	return BuildBookmarksMenu(subMenu, bookmarkItem, 0);
}

BOOL BookmarkMenu::AddBookmarkToMenu(HMENU menu, const BookmarkItem *bookmarkItem, int position)
{
	TCHAR bookmarkName[256];
	StringCchCopy(bookmarkName, SIZEOF_ARRAY(bookmarkName), bookmarkItem->GetName().c_str());

	int id = m_idCounter++;

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING | MIIM_ID;
	mii.wID = id;
	mii.dwTypeData = bookmarkName;
	BOOL res = InsertMenuItem(menu, position, TRUE, &mii);

	if (!res)
	{
		return FALSE;
	}

	m_menuItemMap.insert({ id, bookmarkItem });

	return res;
}

void BookmarkMenu::OnMenuItemSelected(int menuItemId, MenuCallback callback)
{
	auto itr = m_menuItemMap.find(menuItemId);

	if (itr == m_menuItemMap.end())
	{
		return;
	}

	if (!callback)
	{
		return;
	}

	callback(itr->second);
}