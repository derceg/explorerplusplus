// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkContextMenu.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkTree.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/MenuHelper.h"
#include <glog/logging.h>
#include <wil/resource.h>

BookmarkContextMenu::BookmarkContextMenu(BookmarkTree *bookmarkTree,
	const ResourceLoader *resourceLoader, HINSTANCE resourceInstance, BrowserWindow *browserWindow,
	const AcceleratorManager *acceleratorManager) :
	m_bookmarkTree(bookmarkTree),
	m_resourceLoader(resourceLoader),
	m_resourceInstance(resourceInstance),
	m_controller(bookmarkTree, resourceLoader, browserWindow, acceleratorManager),
	m_showingMenu(false)
{
}

BOOL BookmarkContextMenu::ShowMenu(HWND parentWindow, BookmarkItem *parentFolder,
	const RawBookmarkItems &bookmarkItems, const POINT &ptScreen, MenuType menuType)
{
	DCHECK(!bookmarkItems.empty());

	wil::unique_hmenu parentMenu;

	if (bookmarkItems.size() == 1)
	{
		parentMenu.reset(
			LoadMenu(m_resourceInstance, MAKEINTRESOURCE(IDR_SINGLE_BOOKMARK_CONTEXT_MENU)));
	}
	else
	{
		parentMenu.reset(
			LoadMenu(m_resourceInstance, MAKEINTRESOURCE(IDR_MULTIPLE_BOOKMARK_CONTEXT_MENU)));
	}

	if (!parentMenu)
	{
		return FALSE;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	SetUpMenu(menu, bookmarkItems);

	UINT flags = TPM_LEFTALIGN | TPM_RETURNCMD;

	if (menuType == MenuType::Recursive)
	{
		// This flag is needed to show the popup menu when another menu is
		// already being shown.
		WI_SetFlag(flags, TPM_RECURSE);
	}

	m_showingMenu = true;

	UINT menuItemId = TrackPopupMenu(menu, flags, ptScreen.x, ptScreen.y, 0, parentWindow, nullptr);

	m_showingMenu = false;

	if (menuItemId != 0)
	{
		// If this menu is being shown on top of another menu, the original menu
		// should be closed when an item from this menu has been selected.
		EndMenu();

		BookmarkItem *targetParentFolder;
		size_t targetIndex;

		if (bookmarkItems.size() == 1 && bookmarkItems[0]->IsFolder())
		{
			targetParentFolder = bookmarkItems[0];
			targetIndex = targetParentFolder->GetChildren().size();
		}
		else
		{
			targetParentFolder = parentFolder;

			auto lastItem = std::max_element(bookmarkItems.begin(), bookmarkItems.end(),
				[targetParentFolder](BookmarkItem *first, BookmarkItem *second)
				{
					return targetParentFolder->GetChildIndex(first)
						< targetParentFolder->GetChildIndex(second);
				});

			targetIndex = targetParentFolder->GetChildIndex(*lastItem) + 1;
		}

		m_controller.OnMenuItemSelected(menuItemId, targetParentFolder, targetIndex, bookmarkItems,
			parentWindow);
	}

	return TRUE;
}

void BookmarkContextMenu::SetUpMenu(HMENU menu, const RawBookmarkItems &bookmarkItems)
{
	bool folderSelected = bookmarkItems.size() == 1 && bookmarkItems[0]->IsFolder();

	if (folderSelected)
	{
		DeleteMenu(menu, IDM_BOOKMARKS_OPEN, MF_BYCOMMAND);
		DeleteMenu(menu, IDM_BOOKMARKS_OPEN_IN_NEW_TAB, MF_BYCOMMAND);

		std::wstring openAll = m_resourceLoader->LoadString(IDS_BOOKMARK_OPEN_ALL);

		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING | MIIM_ID;
		mii.wID = IDM_BOOKMARKS_OPEN_ALL;
		mii.dwTypeData = openAll.data();
		InsertMenuItem(menu, 0, TRUE, &mii);
	}

	if (folderSelected || bookmarkItems.size() > 1)
	{
		int totalBookmarks = 0;

		for (const auto *bookmarkItem : bookmarkItems)
		{
			if (bookmarkItem->IsBookmark())
			{
				totalBookmarks++;
			}
			else
			{
				const auto &children = bookmarkItem->GetChildren();

				auto numChildBookmarks = std::count_if(children.begin(), children.end(),
					[](auto &child) { return child->IsBookmark(); });

				totalBookmarks += static_cast<int>(numChildBookmarks);
			}
		}

		if (totalBookmarks == 0)
		{
			MenuHelper::EnableItem(menu, IDM_BOOKMARKS_OPEN_ALL, FALSE);
		}
		else
		{
			std::wstring openAll = m_resourceLoader->LoadString(IDS_BOOKMARK_OPEN_ALL);
			openAll += L"\t" + std::to_wstring(totalBookmarks);

			MENUITEMINFO menuItemInfo;
			menuItemInfo.cbSize = sizeof(menuItemInfo);
			menuItemInfo.fMask = MIIM_STRING;
			menuItemInfo.dwTypeData = openAll.data();
			SetMenuItemInfo(menu, IDM_BOOKMARKS_OPEN_ALL, FALSE, &menuItemInfo);
		}
	}

	SetMenuItemStates(menu, bookmarkItems);
}

void BookmarkContextMenu::SetMenuItemStates(HMENU menu, const RawBookmarkItems &bookmarkItems)
{
	if ((bookmarkItems.size() == 1) && m_bookmarkTree->IsPermanentNode(bookmarkItems[0]))
	{
		MenuHelper::EnableItem(menu, IDM_BOOKMARKS_CUT, false);
		MenuHelper::EnableItem(menu, IDM_BOOKMARKS_DELETE, false);
		MenuHelper::EnableItem(menu, IDM_BOOKMARKS_PROPERTIES, false);
	}

	MenuHelper::EnableItem(menu, IDM_BOOKMARKS_PASTE,
		IsClipboardFormatAvailable(BookmarkClipboard::GetClipboardFormat()));
}

bool BookmarkContextMenu::IsShowingMenu() const
{
	return m_showingMenu;
}
