// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkMenuBuilder.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/ImageHelper.h"
#include <glog/logging.h>
#include <wil/common.h>
#include <format>

BookmarkMenuBuilder::BookmarkMenuBuilder(CoreInterface *coreInterface, IconFetcher *iconFetcher,
	HINSTANCE resourceInstance) :
	m_coreInterface(coreInterface),
	m_iconFetcher(iconFetcher),
	m_resourceInstance(resourceInstance)
{
}

BOOL BookmarkMenuBuilder::BuildMenu(HWND parentWindow, HMENU menu, BookmarkItem *bookmarkItem,
	const MenuIdRange &menuIdRange, int startPosition, std::vector<wil::unique_hbitmap> &menuImages,
	MenuInfo &menuInfo, IncludePredicate includePredicate)
{
	DCHECK(bookmarkItem->IsFolder());

	m_menuIdRange = menuIdRange;
	m_idCounter = menuIdRange.startId;

	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(parentWindow);
	int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);

	BookmarkIconManager bookmarkIconManager(m_coreInterface, m_iconFetcher, iconWidth, iconHeight);

	BOOL res = BuildMenu(menu, bookmarkItem, startPosition, bookmarkIconManager, menuImages,
		menuInfo, true, includePredicate);
	menuInfo.menus.insert(menu);
	menuInfo.nextMenuId = m_idCounter;

	return res;
}

BOOL BookmarkMenuBuilder::BuildMenu(HMENU menu, BookmarkItem *bookmarkItem, int startPosition,
	BookmarkIconManager &bookmarkIconManager, std::vector<wil::unique_hbitmap> &menuImages,
	MenuInfo &menuInfo, bool applyIncludePredicate, IncludePredicate includePredicate)
{
	if (bookmarkItem->GetChildren().empty())
	{
		return AddEmptyBookmarkFolderToMenu(menu, bookmarkItem, startPosition, menuInfo);
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
			res = AddBookmarkFolderToMenu(menu, childItem.get(), position, bookmarkIconManager,
				menuImages, menuInfo);
		}
		else
		{
			res = AddBookmarkToMenu(menu, childItem.get(), position, bookmarkIconManager,
				menuImages, menuInfo);
		}

		if (!res)
		{
			return FALSE;
		}

		position++;
	}

	return TRUE;
}

BOOL BookmarkMenuBuilder::AddEmptyBookmarkFolderToMenu(HMENU menu, BookmarkItem *bookmarkItem,
	int position, MenuInfo &menuInfo)
{
	std::wstring bookmarkFolderEmpty =
		ResourceHelper::LoadString(m_resourceInstance, IDS_BOOKMARK_FOLDER_EMPTY);
	std::wstring menuText = std::format(L"({})", bookmarkFolderEmpty);

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

	// If you right-click the empty item shown in a bookmark drop-down in Chrome/Firefox, the parent
	// item will be used as the target of any context menu operations (e.g. selecting "Copy" will
	// copy the parent folder).
	// To enable similar behavior here, the empty item is mapped to the parent.
	menuInfo.itemPositionMap.insert(
		{ { menu, position }, { bookmarkItem, MenuItemType::EmptyItem } });

	return res;
}

BOOL BookmarkMenuBuilder::AddBookmarkFolderToMenu(HMENU menu, BookmarkItem *bookmarkItem,
	int position, BookmarkIconManager &bookmarkIconManager,
	std::vector<wil::unique_hbitmap> &menuImages, MenuInfo &menuInfo)
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

	menuInfo.itemPositionMap.insert(
		{ { menu, position }, { bookmarkItem, MenuItemType::BookmarkItem } });
	menuInfo.menus.insert(subMenu);

	return BuildMenu(subMenu, bookmarkItem, 0, bookmarkIconManager, menuImages, menuInfo, false,
		nullptr);
}

BOOL BookmarkMenuBuilder::AddBookmarkToMenu(HMENU menu, BookmarkItem *bookmarkItem, int position,
	BookmarkIconManager &bookmarkIconManager, std::vector<wil::unique_hbitmap> &menuImages,
	MenuInfo &menuInfo)
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

	menuInfo.itemIdMap.insert({ id, bookmarkItem });
	menuInfo.itemPositionMap.insert(
		{ { menu, position }, { bookmarkItem, MenuItemType::BookmarkItem } });

	return res;
}

void BookmarkMenuBuilder::AddIconToMenuItem(HMENU menu, int position,
	const BookmarkItem *bookmarkItem, BookmarkIconManager &bookmarkIconManager,
	std::vector<wil::unique_hbitmap> &menuImages)
{
	int iconIndex = bookmarkIconManager.GetBookmarkItemIconIndex(bookmarkItem);

	wil::com_ptr_nothrow<IImageList> imageList;
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
