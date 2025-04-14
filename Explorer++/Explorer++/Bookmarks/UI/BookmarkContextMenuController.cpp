// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkContextMenuController.h"
#include "Bookmarks/BookmarkTree.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainerImpl.h"

BookmarkContextMenuController::BookmarkContextMenuController(BookmarkTree *bookmarkTree,
	const ResourceLoader *resourceLoader, HINSTANCE resourceInstance, BrowserWindow *browserWindow,
	CoreInterface *coreInterface, const AcceleratorManager *acceleratorManager,
	ThemeManager *themeManager) :
	m_bookmarkTree(bookmarkTree),
	m_resourceLoader(resourceLoader),
	m_resourceInstance(resourceInstance),
	m_browserWindow(browserWindow),
	m_coreInterface(coreInterface),
	m_acceleratorManager(acceleratorManager),
	m_themeManager(themeManager)
{
}

void BookmarkContextMenuController::OnMenuItemSelected(UINT menuItemId,
	BookmarkItem *targetParentFolder, size_t targetIndex, const RawBookmarkItems &bookmarkItems,
	HWND parentWindow)
{
	switch (menuItemId)
	{
	case IDM_BOOKMARKS_OPEN:
	{
		DCHECK(bookmarkItems.size() == 1 && bookmarkItems[0]->IsBookmark());

		BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItems[0],
			OpenFolderDisposition::CurrentTab, m_browserWindow);
	}
	break;

	case IDM_BOOKMARKS_OPEN_IN_NEW_TAB:
		DCHECK(bookmarkItems.size() == 1 && bookmarkItems[0]->IsBookmark());
		BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItems[0],
			OpenFolderDisposition::NewTabDefault, m_browserWindow);
		break;

	case IDM_BOOKMARKS_OPEN_ALL:
		OnOpenAll(bookmarkItems);
		break;

	case IDM_BOOKMARKS_NEW_BOOKMARK:
		OnNewBookmarkItem(BookmarkItem::Type::Bookmark, targetParentFolder, targetIndex,
			parentWindow);
		break;

	case IDM_BOOKMARKS_NEW_FOLDER:
		OnNewBookmarkItem(BookmarkItem::Type::Folder, targetParentFolder, targetIndex,
			parentWindow);
		break;

	case IDM_BOOKMARKS_CUT:
		OnCopy(bookmarkItems, true);
		break;

	case IDM_BOOKMARKS_COPY:
		OnCopy(bookmarkItems, false);
		break;

	case IDM_BOOKMARKS_PASTE:
		OnPaste(targetParentFolder, targetIndex);
		break;

	case IDM_BOOKMARKS_DELETE:
		OnDelete(bookmarkItems);
		break;

	case IDM_BOOKMARKS_PROPERTIES:
		DCHECK_EQ(bookmarkItems.size(), 1U);
		OnEditBookmarkItem(bookmarkItems[0], parentWindow);
		break;

	default:
		DCHECK(false);
		break;
	}
}

void BookmarkContextMenuController::OnOpenAll(const RawBookmarkItems &bookmarkItems)
{
	OpenFolderDisposition disposition = OpenFolderDisposition::NewTabDefault;

	for (auto *bookmarkItem : bookmarkItems)
	{
		BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem, disposition, m_browserWindow);

		disposition = OpenFolderDisposition::BackgroundTab;
	}
}

void BookmarkContextMenuController::OnNewBookmarkItem(BookmarkItem::Type type,
	BookmarkItem *targetParentFolder, size_t targetIndex, HWND parentWindow)
{
	BookmarkHelper::AddBookmarkItem(m_bookmarkTree, type, targetParentFolder, targetIndex,
		parentWindow, m_themeManager, m_coreInterface, m_acceleratorManager, m_resourceLoader);
}

void BookmarkContextMenuController::OnCopy(const RawBookmarkItems &bookmarkItems, bool cut)
{
	BookmarkHelper::CopyBookmarkItems(m_bookmarkTree, bookmarkItems, cut);
}

void BookmarkContextMenuController::OnPaste(BookmarkItem *targetParentFolder, size_t targetIndex)
{
	BookmarkHelper::PasteBookmarkItems(m_bookmarkTree, targetParentFolder, targetIndex);
}

void BookmarkContextMenuController::OnDelete(const RawBookmarkItems &bookmarkItems)
{
	for (auto *bookmarkItem : bookmarkItems)
	{
		m_bookmarkTree->RemoveBookmarkItem(bookmarkItem);
	}
}

void BookmarkContextMenuController::OnEditBookmarkItem(BookmarkItem *bookmarkItem,
	HWND parentWindow)
{
	BookmarkHelper::EditBookmarkItem(bookmarkItem, m_bookmarkTree, m_acceleratorManager,
		m_resourceLoader, m_resourceInstance, parentWindow, m_themeManager);
}
