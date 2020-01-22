// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkContextMenu.h"
#include "BookmarkClipboard.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Helper.h"
#include "../Helper/MenuHelper.h"
#include <wil/resource.h>

BookmarkContextMenu::BookmarkContextMenu(BookmarkTree *bookmarkTree, HMODULE resourceModule,
	IExplorerplusplus *expp) :
	m_bookmarkTree(bookmarkTree),
	m_resourceModule(resourceModule),
	m_expp(expp),
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

		OnMenuItemSelected(menuItemId, targetParentFolder, bookmarkItems, parentWindow);
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

void BookmarkContextMenu::OnMenuItemSelected(int menuItemId, BookmarkItem *parentFolder,
	const RawBookmarkItems &bookmarkItems, HWND parentWindow)
{
	switch (menuItemId)
	{
	case IDM_BOOKMARKS_OPEN:
	{
		assert(bookmarkItems.size() == 1 && bookmarkItems[0]->IsBookmark());

		Tab &selectedTab = m_expp->GetTabContainer()->GetSelectedTab();
		selectedTab.GetShellBrowser()->GetNavigationController()->BrowseFolder(bookmarkItems[0]->GetLocation().c_str());
	}
		break;

	case IDM_BOOKMARKS_OPEN_IN_NEW_TAB:
		assert(bookmarkItems.size() == 1 && bookmarkItems[0]->IsBookmark());
		BookmarkHelper::OpenBookmarkItemInNewTab(bookmarkItems[0], m_expp);
		break;

	case IDM_BOOKMARKS_OPEN_ALL:
		OnOpenAll(bookmarkItems);
		break;

	case IDM_BOOKMARKS_NEW_BOOKMARK:
		OnNewBookmarkItem(BookmarkItem::Type::Bookmark, parentFolder, parentWindow);
		break;

	case IDM_BOOKMARKS_NEW_FOLDER:
		OnNewBookmarkItem(BookmarkItem::Type::Folder, parentFolder, parentWindow);
		break;

	case IDM_BOOKMARKS_CUT:
		OnCopy(bookmarkItems, true);
		break;

	case IDM_BOOKMARKS_COPY:
		OnCopy(bookmarkItems, false);
		break;

	case IDM_BOOKMARKS_PASTE:
		OnPaste(parentFolder);
		break;

	case IDM_BOOKMARKS_DELETE:
		OnDelete(bookmarkItems);
		break;

	case IDM_BOOKMARKS_PROPERTIES:
		assert(bookmarkItems.size() == 1);
		OnEditBookmarkItem(bookmarkItems[0], parentWindow);
		break;

	default:
		assert(false);
		break;
	}
}

void BookmarkContextMenu::OnOpenAll(const RawBookmarkItems &bookmarkItems)
{
	for (auto bookmarkItem : bookmarkItems)
	{
		BookmarkHelper::OpenBookmarkItemInNewTab(bookmarkItem, m_expp);
	}
}

void BookmarkContextMenu::OnNewBookmarkItem(BookmarkItem::Type type, BookmarkItem *parentFolder,
	HWND parentWindow)
{
	BookmarkHelper::AddBookmarkItem(m_bookmarkTree, type, parentFolder, m_resourceModule, parentWindow,
		m_expp->GetTabContainer(), m_expp);
}

void BookmarkContextMenu::OnCopy(const RawBookmarkItems &bookmarkItems, bool cut)
{
	OwnedRefBookmarkItems ownedBookmarkItems;

	for (auto bookmarkItem : bookmarkItems)
	{
		auto &ownedPtr = bookmarkItem->GetParent()->GetChildOwnedPtr(bookmarkItem);
		ownedBookmarkItems.push_back(ownedPtr);
	}

	BookmarkClipboard bookmarkClipboard;
	bool res = bookmarkClipboard.WriteBookmarks(ownedBookmarkItems);

	if (cut && res)
	{
		for (auto bookmarkItem : bookmarkItems)
		{
			m_bookmarkTree->RemoveBookmarkItem(bookmarkItem);
		}
	}
}

void BookmarkContextMenu::OnPaste(BookmarkItem *parentFolder)
{
	BookmarkClipboard bookmarkClipboard;
	auto bookmarkItems = bookmarkClipboard.ReadBookmarks();
	int i = 0;

	for (auto &bookmarkItem : bookmarkItems)
	{
		if (parentFolder->IsFolder())
		{
			m_bookmarkTree->AddBookmarkItem(parentFolder, std::move(bookmarkItem),
				parentFolder->GetChildren().size() + i);
		}
		else
		{
			BookmarkItem *parent = parentFolder->GetParent();
			m_bookmarkTree->AddBookmarkItem(parent, std::move(bookmarkItem),
				parent->GetChildIndex(parentFolder) + i + 1);
		}

		i++;
	}
}

void BookmarkContextMenu::OnDelete(const RawBookmarkItems &bookmarkItems)
{
	for (auto bookmarkItem : bookmarkItems)
	{
		m_bookmarkTree->RemoveBookmarkItem(bookmarkItem);
	}
}

void BookmarkContextMenu::OnEditBookmarkItem(BookmarkItem *bookmarkItem, HWND parentWindow)
{
	BookmarkHelper::EditBookmarkItem(bookmarkItem, m_bookmarkTree, m_resourceModule,
		parentWindow, m_expp);
}

bool BookmarkContextMenu::IsShowingMenu() const
{
	return m_showingMenu;
}