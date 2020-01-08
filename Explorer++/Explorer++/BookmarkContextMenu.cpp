// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkContextMenu.h"
#include "BookmarkClipboard.h"
#include "BookmarkHelper.h"
#include "MainResource.h"
#include "../Helper/Helper.h"
#include <wil/resource.h>

BookmarkContextMenu::BookmarkContextMenu(BookmarkTree *bookmarkTree, HMODULE resourceModule,
	IExplorerplusplus *expp) :
	m_bookmarkTree(bookmarkTree),
	m_resourceModule(resourceModule),
	m_expp(expp),
	m_showingMenu(false)
{

}

BOOL BookmarkContextMenu::ShowMenu(HWND parentWindow, BookmarkItem *bookmarkItem, const POINT &pt, bool recursive)
{
	auto parentMenu = wil::unique_hmenu(LoadMenu(m_resourceModule, MAKEINTRESOURCE(IDR_BOOKMARKSTOOLBAR_RCLICK_MENU)));

	if (!parentMenu)
	{
		return FALSE;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	UINT flags = TPM_LEFTALIGN | TPM_RETURNCMD;

	if (recursive)
	{
		// This flag is needed to show the popup menu when another menu is
		// already being shown.
		WI_SetFlag(flags, TPM_RECURSE);
	}

	m_showingMenu = true;

	int menuItemId = TrackPopupMenu(menu, flags, pt.x, pt.y, 0, parentWindow, nullptr);

	m_showingMenu = false;

	if (menuItemId != 0)
	{
		// If this menu is being shown on top of another menu, the original menu
		// should be closed when an item from this menu has been selected.
		EndMenu();

		OnMenuItemSelected(menuItemId, bookmarkItem, parentWindow);
	}

	return TRUE;
}

void BookmarkContextMenu::OnMenuItemSelected(int menuItemId, BookmarkItem *bookmarkItem, HWND parentWindow)
{
	switch (menuItemId)
	{
	case IDM_BT_OPEN:
		if (bookmarkItem->IsBookmark())
		{
			Tab &selectedTab = m_expp->GetTabContainer()->GetSelectedTab();
			selectedTab.GetShellBrowser()->GetNavigationController()->BrowseFolder(bookmarkItem->GetLocation().c_str());
		}
		break;

	case IDM_BT_OPENINNEWTAB:
		BookmarkHelper::OpenBookmarkItemInNewTab(bookmarkItem, m_expp);
		break;

	case IDM_BT_NEWBOOKMARK:
		OnNewBookmarkItem(BookmarkItem::Type::Bookmark, parentWindow);
		break;

	case IDM_BT_NEWFOLDER:
		OnNewBookmarkItem(BookmarkItem::Type::Folder, parentWindow);
		break;

	case IDM_BT_CUT:
		OnCopy(bookmarkItem, true);
		break;

	case IDM_BT_COPY:
		OnCopy(bookmarkItem, false);
		break;

	case IDM_BT_PASTE:
		OnPaste(bookmarkItem);
		break;

	case IDM_BT_DELETE:
		m_bookmarkTree->RemoveBookmarkItem(bookmarkItem);
		break;

	case IDM_BT_PROPERTIES:
		OnEditBookmarkItem(bookmarkItem, parentWindow);
		break;
	}
}

void BookmarkContextMenu::OnNewBookmarkItem(BookmarkItem::Type type, HWND parentWindow)
{
	BookmarkHelper::AddBookmarkItem(m_bookmarkTree, type, m_resourceModule, parentWindow,
		m_expp->GetTabContainer(), m_expp);
}

void BookmarkContextMenu::OnCopy(BookmarkItem *bookmarkItem, bool cut)
{
	auto &ownedPtr = bookmarkItem->GetParent()->GetChildOwnedPtr(bookmarkItem);

	BookmarkClipboard bookmarkClipboard;
	bool res = bookmarkClipboard.WriteBookmark(ownedPtr);

	if (cut && res)
	{
		m_bookmarkTree->RemoveBookmarkItem(bookmarkItem);
	}
}

void BookmarkContextMenu::OnPaste(BookmarkItem *selectedBookmarkItem)
{
	BookmarkClipboard bookmarkClipboard;
	auto copiedBookmarkItem = bookmarkClipboard.ReadBookmark();

	if (!copiedBookmarkItem)
	{
		return;
	}

	if (selectedBookmarkItem->IsFolder())
	{
		m_bookmarkTree->AddBookmarkItem(selectedBookmarkItem, std::move(copiedBookmarkItem),
			selectedBookmarkItem->GetChildren().size());
	}
	else
	{
		BookmarkItem *parent = selectedBookmarkItem->GetParent();
		m_bookmarkTree->AddBookmarkItem(parent, std::move(copiedBookmarkItem),
			parent->GetChildIndex(selectedBookmarkItem) + 1);
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