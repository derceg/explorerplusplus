// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkMenuBuilder.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/ImageHelper.h"
#include <boost/format.hpp>
#include <wil/common.h>

BookmarkMenuBuilder::BookmarkMenuBuilder(
	IExplorerplusplus *expp, IconFetcher *iconFetcher, HMODULE resourceModule) :
	m_expp(expp),
	m_iconFetcher(iconFetcher),
	m_resourceModule(resourceModule)
{
}

BOOL BookmarkMenuBuilder::BuildMenu(HWND parentWindow, HMENU menu, BookmarkItem *bookmarkItem,
	const MenuIdRange &menuIdRange, int startPosition, ItemIdMap &itemIdMap,
	std::vector<wil::unique_hbitmap> &menuImages, ItemPositionMap *itemPositionMap,
	int *nextMenuItemId, IncludePredicate includePredicate)
{
	assert(bookmarkItem->IsFolder());

	m_menuIdRange = menuIdRange;
	m_idCounter = menuIdRange.startId;

	UINT dpi = m_dpiCompat.GetDpiForWindow(parentWindow);
	int iconWidth = m_dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = m_dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);

	BookmarkIconManager bookmarkIconManager(m_expp, m_iconFetcher, nullptr, iconWidth, iconHeight);

	BOOL res = BuildMenu(menu, bookmarkItem, startPosition, itemIdMap, bookmarkIconManager,
		menuImages, itemPositionMap, true, includePredicate);
	wil::assign_to_opt_param(nextMenuItemId, m_idCounter);

	return res;
}

BOOL BookmarkMenuBuilder::BuildMenu(HMENU menu, BookmarkItem *bookmarkItem, int startPosition,
	ItemIdMap &itemIdMap, BookmarkIconManager &bookmarkIconManager,
	std::vector<wil::unique_hbitmap> &menuImages, ItemPositionMap *itemPositionMap,
	bool applyIncludePredicate, IncludePredicate includePredicate)
{
	if (bookmarkItem->GetChildren().empty())
	{
		return AddEmptyBookmarkFolderToMenu(menu, bookmarkItem, startPosition, itemPositionMap);
	}

	int position = startPosition;

	for (auto &childItem : bookmarkItem->GetChildren())
	{
		if (applyIncludePredicate && includePredicate && !includePredicate(childItem.get()))
		{
			continue;
		}

		BOOL res;

		if (childItem->IsFolder())
		{
			res = AddBookmarkFolderToMenu(menu, childItem.get(), position, itemIdMap,
				bookmarkIconManager, menuImages, itemPositionMap);
		}
		else
		{
			res = AddBookmarkToMenu(menu, childItem.get(), position, itemIdMap, bookmarkIconManager,
				menuImages, itemPositionMap);
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
	int position, ItemIdMap &itemIdMap, BookmarkIconManager &bookmarkIconManager,
	std::vector<wil::unique_hbitmap> &menuImages, ItemPositionMap *itemPositionMap)
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

	AddIconToMenuItem(menu, position, bookmarkItem, bookmarkIconManager, menuImages);

	if (itemPositionMap)
	{
		itemPositionMap->insert({ { menu, position }, bookmarkItem });
	}

	return BuildMenu(subMenu, bookmarkItem, 0, itemIdMap, bookmarkIconManager, menuImages,
		itemPositionMap, false, nullptr);
}

BOOL BookmarkMenuBuilder::AddBookmarkToMenu(HMENU menu, BookmarkItem *bookmarkItem, int position,
	ItemIdMap &itemIdMap, BookmarkIconManager &bookmarkIconManager,
	std::vector<wil::unique_hbitmap> &menuImages, ItemPositionMap *itemPositionMap)
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

	AddIconToMenuItem(menu, position, bookmarkItem, bookmarkIconManager, menuImages);

	itemIdMap.insert({ id, bookmarkItem });

	if (itemPositionMap)
	{
		itemPositionMap->insert({ { menu, position }, bookmarkItem });
	}

	return res;
}

void BookmarkMenuBuilder::AddIconToMenuItem(HMENU menu, int position,
	const BookmarkItem *bookmarkItem, BookmarkIconManager &bookmarkIconManager,
	std::vector<wil::unique_hbitmap> &menuImages)
{
	int iconIndex = bookmarkIconManager.GetBookmarkItemIconIndex(bookmarkItem);

	wil::com_ptr<IImageList> imageList;
	HRESULT hr =
		HIMAGELIST_QueryInterface(bookmarkIconManager.GetImageList(), IID_PPV_ARGS(&imageList));

	if (FAILED(hr))
	{
		return;
	}

	auto bitmap = ImageHelper::ImageListIconToBitmap(imageList.get(), iconIndex);

	if (!bitmap)
	{
		return;
	}

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_BITMAP;
	mii.hbmpItem = bitmap.get();
	const BOOL res = SetMenuItemInfo(menu, position, true, &mii);

	if (res)
	{
		menuImages.push_back(std::move(bitmap));
	}
}