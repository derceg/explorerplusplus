// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkMenuBuilder.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include <boost/format.hpp>

BookmarkMenuBuilder::BookmarkMenuBuilder(HMODULE resourceModule) :
	m_resourceModule(resourceModule)
{

}

BOOL BookmarkMenuBuilder::BuildMenu(HMENU menu, BookmarkItem *bookmarkItem,
	const MenuIdRange &menuIdRange, int startPosition, ItemMap &itemMap)
{
	assert(bookmarkItem->IsFolder());

	m_menuIdRange = menuIdRange;
	m_idCounter = menuIdRange.startId;
	itemMap.clear();

	return BuildMenu(menu, bookmarkItem, startPosition, itemMap);
}

BOOL BookmarkMenuBuilder::BuildMenu(HMENU menu, BookmarkItem *bookmarkItem,
	int startPosition, ItemMap &itemMap)
{
	if (bookmarkItem->GetChildren().empty())
	{
		return AddEmptyBookmarkFolderToMenu(menu, startPosition);
	}

	int position = startPosition;

	for (auto &childItem : bookmarkItem->GetChildren())
	{
		BOOL res;

		if (childItem->IsFolder())
		{
			res = AddBookmarkFolderToMenu(menu, childItem.get(), position, itemMap);
		}
		else
		{
			res = AddBookmarkToMenu(menu, childItem.get(), position, itemMap);
		}

		if (!res)
		{
			return FALSE;
		}

		position++;
	}

	return TRUE;
}

BOOL BookmarkMenuBuilder::AddEmptyBookmarkFolderToMenu(HMENU menu, int position)
{
	std::wstring bookmarkFolderEmpty = ResourceHelper::LoadString(m_resourceModule, IDS_BOOKMARK_FOLDER_EMPTY);
	std::wstring menuText = (boost::wformat(L"(%s)") % bookmarkFolderEmpty).str();

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING | MIIM_STATE;
	mii.fState = MFS_DISABLED;
	mii.dwTypeData = menuText.data();
	return InsertMenuItem(menu, position, TRUE, &mii);
}

BOOL BookmarkMenuBuilder::AddBookmarkFolderToMenu(HMENU menu, BookmarkItem *bookmarkItem,
	int position, ItemMap &itemMap)
{
	HMENU subMenu = CreatePopupMenu();

	if (subMenu == nullptr)
	{
		return FALSE;
	}

	std::wstring bookmarkFolderName = bookmarkItem->GetName();

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING | MIIM_SUBMENU;
	mii.hSubMenu = subMenu;
	mii.dwTypeData = bookmarkFolderName.data();
	BOOL res = InsertMenuItem(menu, position, TRUE, &mii);

	if (!res)
	{
		return FALSE;
	}

	return BuildMenu(subMenu, bookmarkItem, 0, itemMap);
}

BOOL BookmarkMenuBuilder::AddBookmarkToMenu(HMENU menu, BookmarkItem *bookmarkItem,
	int position, ItemMap &itemMap)
{
	int id = m_idCounter++;

	if (id >= m_menuIdRange.endId)
	{
		return FALSE;
	}

	std::wstring bookmarkName = bookmarkItem->GetName();

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING | MIIM_ID;
	mii.wID = id;
	mii.dwTypeData = bookmarkName.data();
	BOOL res = InsertMenuItem(menu, position, TRUE, &mii);

	if (!res)
	{
		return FALSE;
	}

	itemMap.insert({ id, bookmarkItem });

	return res;
}