// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkContextMenuController.h"
#include "BookmarkClipboard.h"
#include "BookmarkTree.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ShellBrowser/NavigationController.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabContainer.h"

BookmarkContextMenuController::BookmarkContextMenuController(BookmarkTree *bookmarkTree,
	HMODULE resourceModule, IExplorerplusplus *expp) :
	m_bookmarkTree(bookmarkTree),
	m_resourceModule(resourceModule),
	m_expp(expp)
{

}

void BookmarkContextMenuController::OnMenuItemSelected(int menuItemId, BookmarkItem *parentFolder,
	const RawBookmarkItems &bookmarkItems, HWND parentWindow)
{
	switch (menuItemId)
	{
	case IDM_BOOKMARKS_OPEN:
	{
		assert(bookmarkItems.size() == 1 && bookmarkItems[0]->IsBookmark());

		Tab &selectedTab = m_expp->GetTabContainer()->GetSelectedTab();
		selectedTab.GetShellBrowser()->GetNavigationController()->BrowseFolder(bookmarkItems[0]->GetLocation());
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

void BookmarkContextMenuController::OnOpenAll(const RawBookmarkItems &bookmarkItems)
{
	for (auto bookmarkItem : bookmarkItems)
	{
		BookmarkHelper::OpenBookmarkItemInNewTab(bookmarkItem, m_expp);
	}
}

void BookmarkContextMenuController::OnNewBookmarkItem(BookmarkItem::Type type, BookmarkItem *parentFolder,
	HWND parentWindow)
{
	BookmarkHelper::AddBookmarkItem(m_bookmarkTree, type, parentFolder, m_resourceModule, parentWindow,
		m_expp->GetTabContainer(), m_expp);
}

void BookmarkContextMenuController::OnCopy(const RawBookmarkItems &bookmarkItems, bool cut)
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

void BookmarkContextMenuController::OnPaste(BookmarkItem *parentFolder)
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

void BookmarkContextMenuController::OnDelete(const RawBookmarkItems &bookmarkItems)
{
	for (auto bookmarkItem : bookmarkItems)
	{
		m_bookmarkTree->RemoveBookmarkItem(bookmarkItem);
	}
}

void BookmarkContextMenuController::OnEditBookmarkItem(BookmarkItem *bookmarkItem, HWND parentWindow)
{
	BookmarkHelper::EditBookmarkItem(bookmarkItem, m_bookmarkTree, m_resourceModule,
		parentWindow, m_expp);
}