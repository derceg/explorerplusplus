// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkMenuBuilder.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include <boost/format.hpp>

BookmarkMenuBuilder::BookmarkMenuBuilder(HMODULE resourceModule) : m_resourceModule(resourceModule)
{
}

BOOL BookmarkMenuBuilder::BuildMenu(HMENU menu, BookmarkItem *bookmarkItem,
	const MenuIdRange &menuIdRange, int startPosition, ItemIdMap &itemIdMap,
	ItemPositionMap *itemPositionMap)
{
	assert(bookmarkItem->IsFolder());

	m_menuIdRange = menuIdRange;
	m_idCounter = menuIdRange.startId;

	return BuildMenu(menu, bookmarkItem, startPosition, itemIdMap, itemPositionMap);
}

BOOL BookmarkMenuBuilder::BuildMenu(HMENU menu, BookmarkItem *bookmarkItem, int startPosition,
	ItemIdMap &itemIdMap, ItemPositionMap *itemPositionMap)
{
	if (bookmarkItem->GetChildren().empty())
	{
		return AddEmptyBookmarkFolderToMenu(menu, bookmarkItem, startPosition, itemPositionMap);
	}

	int position = startPosition;

	for (auto &childItem : bookmarkItem->GetChildren())
	{
		BOOL res;

		if (childItem->IsFolder())
		{
			res = AddBookmarkFolderToMenu(
				menu, childItem.get(), position, itemIdMap, itemPositionMap);
		}
		else
		{
			res = AddBookmarkToMenu(menu, childItem.get(), position, itemIdMap, itemPositionMap);
		}

		if (!res)
		{
			return FALSE;
		}

		position++;
	}

	return TRUE;
}

BOOL BookmarkMenuBuilder::AddEmptyBookmarkFolderToMenu(
	HMENU menu, BookmarkItem *bookmarkItem, int position, ItemPositionMap *itemPositionMap)
{
	std::wstring bookmarkFolderEmpty =
		ResourceHelper::LoadString(m_resourceModule, IDS_BOOKMARK_FOLDER_EMPTY);
	std::wstring menuText = (boost::wformat(L"(%s)") % bookmarkFolderEmpty).str();

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING | MIIM_STATE;
	mii.fState = MFS_DISABLED;
	mii.dwTypeData = menuText.data();
	BOOL res = InsertMenuItem(menu, position, TRUE, &mii);

	if (!res)
	{
		return FALSE;
	}

	if (itemPositionMap)
	{
		// If you right-click the empty item shown in a bookmark drop-down in
		// Chrome/Firefox, the parent item will be used as the target of any
		// context menu operations (e.g. selecting "Copy" will copy the parent
		// folder).
		// To enable similar behavior here, the empty item is mapped to the
		// parent.
		itemPositionMap->insert({ { menu, position }, bookmarkItem });
	}

	return res;
}

BOOL BookmarkMenuBuilder::AddBookmarkFolderToMenu(HMENU menu, BookmarkItem *bookmarkItem,
	int position, ItemIdMap &itemIdMap, ItemPositionMap *itemPositionMap)
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

	if (itemPositionMap)
	{
		itemPositionMap->insert({ { menu, position }, bookmarkItem });
	}

	return BuildMenu(subMenu, bookmarkItem, 0, itemIdMap, itemPositionMap);
}

BOOL BookmarkMenuBuilder::AddBookmarkToMenu(HMENU menu, BookmarkItem *bookmarkItem, int position,
	ItemIdMap &itemIdMap, ItemPositionMap *itemPositionMap)
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

	itemIdMap.insert({ id, bookmarkItem });

	if (itemPositionMap)
	{
		itemPositionMap->insert({ { menu, position }, bookmarkItem });
	}

	return res;
}