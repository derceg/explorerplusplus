// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkContextMenu.h"
#include "BookmarkClipboard.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/MenuHelper.h"
#include <wil/resource.h>

BookmarkContextMenu::BookmarkContextMenu(BookmarkTree *bookmarkTree, HMODULE resourceModule,
	IExplorerplusplus *expp) :
	m_resourceModule(resourceModule),
	m_controller(bookmarkTree, resourceModule, expp),
	m_showingMenu(false)
{

}

BOOL BookmarkContextMenu::ShowMenu(HWND parentWindow, BookmarkItem *parentFolder,
	const RawBookmarkItems &bookmarkItems, const POINT &ptScreen, bool recursive)
{
	assert(!bookmarkItems.empty());

	wil::unique_hmenu parentMenu;

	if (bookmarkItems.size() == 1)
	{
		parentMenu.reset(LoadMenu(m_resourceModule, MAKEINTRESOURCE(IDR_SINGLE_BOOKMARK_CONTEXT_MENU)));
	}
	else
	{
		parentMenu.reset(LoadMenu(m_resourceModule, MAKEINTRESOURCE(IDR_MULTIPLE_BOOKMARK_CONTEXT_MENU)));
	}

	if (!parentMenu)
	{
		return FALSE;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	SetUpMenu(menu, bookmarkItems);

	UINT flags = TPM_LEFTALIGN | TPM_RETURNCMD;

	if (recursive)
	{
		// This flag is needed to show the popup menu when another menu is
		// already being shown.
		WI_SetFlag(flags, TPM_RECURSE);
	}

	m_showingMenu = true;

	int menuItemId = TrackPopupMenu(menu, flags, ptScreen.x, ptScreen.y, 0, parentWindow, nullptr);

	m_showingMenu = false;

	if (menuItemId != 0)
	{
		// If this menu is being shown on top of another menu, the original menu
		// should be closed when an item from this menu has been selected.
		EndMenu();

		BookmarkItem *targetParentFolder = parentFolder;

		if (bookmarkItems.size() == 1 && bookmarkItems[0]->IsFolder())
		{
			targetParentFolder = bookmarkItems[0];
		}

		m_controller.OnMenuItemSelected(menuItemId, targetParentFolder, bookmarkItems, parentWindow);
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

		std::wstring openAll = ResourceHelper::LoadString(m_resourceModule, IDS_BOOKMARK_OPEN_ALL);

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

		for (const auto bookmarkItem : bookmarkItems)
		{
			if (bookmarkItem->IsBookmark())
			{
				totalBookmarks++;
			}
			else
			{
				auto &children = bookmarkItem->GetChildren();

				auto numChildBookmarks = std::count_if(children.begin(), children.end(), [] (auto &child) {
					return child->IsBookmark();
				});

				totalBookmarks += static_cast<int>(numChildBookmarks);
			}
		}

		if (totalBookmarks == 0)
		{
			lEnableMenuItem(menu, IDM_BOOKMARKS_OPEN_ALL, FALSE);
		}
		else
		{
			std::wstring openAll = ResourceHelper::LoadString(m_resourceModule, IDS_BOOKMARK_OPEN_ALL);
			openAll += L"\t" + std::to_wstring(totalBookmarks);

			MENUITEMINFO menuItemInfo;
			menuItemInfo.cbSize = sizeof(menuItemInfo);
			menuItemInfo.fMask = MIIM_STRING;
			menuItemInfo.dwTypeData = openAll.data();
			SetMenuItemInfo(menu, IDM_BOOKMARKS_OPEN_ALL, FALSE, &menuItemInfo);
		}
	}

	SetMenuItemStates(menu);
}

void BookmarkContextMenu::SetMenuItemStates(HMENU menu)
{
	lEnableMenuItem(menu, IDM_BOOKMARKS_PASTE, IsClipboardFormatAvailable(BookmarkClipboard::GetClipboardFormat()));
}

bool BookmarkContextMenu::IsShowingMenu() const
{
	return m_showingMenu;
}