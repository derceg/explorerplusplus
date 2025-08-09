// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkContextMenu.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkTree.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceLoader.h"
#include "../Helper/ClipboardStore.h"
#include <algorithm>
#include <ranges>

BookmarkContextMenu::BookmarkContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, BookmarkTree *bookmarkTree,
	const RawBookmarkItems &bookmarkItems, const ResourceLoader *resourceLoader,
	BrowserWindow *browser, HWND parentWindow, ClipboardStore *clipboardStore) :
	MenuBase(menuView, acceleratorManager),
	m_bookmarkTree(bookmarkTree),
	m_bookmarkItems(bookmarkItems),
	m_resourceLoader(resourceLoader),
	m_browser(browser),
	m_parentWindow(parentWindow),
	m_clipboardStore(clipboardStore)
{
	BuildMenu();

	m_connections.push_back(m_menuView->AddItemSelectedObserver(
		std::bind(&BookmarkContextMenu::OnMenuItemSelected, this, std::placeholders::_1)));
}

void BookmarkContextMenu::BuildMenu()
{
	CHECK(AreBookmarkItemsValid());

	bool singleBookmarkSelected = (m_bookmarkItems.size() == 1) && m_bookmarkItems[0]->IsBookmark();
	bool permanentNodeSelected =
		(m_bookmarkItems.size() == 1) && m_bookmarkTree->IsPermanentNode(m_bookmarkItems[0]);

	if (singleBookmarkSelected)
	{
		m_menuView->AppendItem(IDM_BOOKMARK_CONTEXT_MENU_OPEN,
			m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_OPEN), {},
			m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_OPEN_HELP_TEXT));
		m_menuView->AppendItem(IDM_BOOKMARK_CONTEXT_MENU_OPEN_IN_NEW_TAB,
			m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_OPEN_IN_NEW_TAB), {},
			m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_OPEN_IN_NEW_TAB_HELP_TEXT));

		m_menuView->AppendSeparator();
	}
	else
	{
		size_t totalBookmarks = GetTotalBookmarks();

		m_menuView->AppendItem(IDM_BOOKMARK_CONTEXT_MENU_OPEN_ALL,
			std::format(L"{}\t{}", m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_OPEN_ALL),
				totalBookmarks),
			{}, m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_OPEN_ALL_HELP_TEXT));

		m_menuView->EnableItem(IDM_BOOKMARK_CONTEXT_MENU_OPEN_ALL, totalBookmarks > 0);

		m_menuView->AppendSeparator();
	}

	if (m_bookmarkItems.size() == 1)
	{
		m_menuView->AppendItem(IDM_BOOKMARK_CONTEXT_MENU_NEW_BOOKMARK,
			m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_NEW_BOOKMARK), {},
			m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_NEW_BOOKMARK_HELP_TEXT));
		m_menuView->AppendItem(IDM_BOOKMARK_CONTEXT_MENU_NEW_FOLDER,
			m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_NEW_FOLDER), {},
			m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_NEW_FOLDER_HELP_TEXT));

		m_menuView->AppendSeparator();
	}

	m_menuView->AppendItem(IDM_BOOKMARK_CONTEXT_MENU_CUT,
		m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_CUT), {},
		m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_CUT_HELP_TEXT));
	m_menuView->AppendItem(IDM_BOOKMARK_CONTEXT_MENU_COPY,
		m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_COPY), {},
		m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_COPY_HELP_TEXT));
	m_menuView->AppendItem(IDM_BOOKMARK_CONTEXT_MENU_PASTE,
		m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_PASTE), {},
		m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_PASTE_HELP_TEXT));

	m_menuView->AppendSeparator();

	m_menuView->AppendItem(IDM_BOOKMARK_CONTEXT_MENU_DELETE,
		m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_DELETE), {},
		m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_DELETE_HELP_TEXT));

	m_menuView->EnableItem(IDM_BOOKMARK_CONTEXT_MENU_CUT, !permanentNodeSelected);
	m_menuView->EnableItem(IDM_BOOKMARK_CONTEXT_MENU_PASTE,
		m_clipboardStore->IsDataAvailable(BookmarkClipboard::GetClipboardFormat()));
	m_menuView->EnableItem(IDM_BOOKMARK_CONTEXT_MENU_DELETE, !permanentNodeSelected);

	if (m_bookmarkItems.size() == 1)
	{
		m_menuView->AppendSeparator();

		m_menuView->AppendItem(IDM_BOOKMARK_CONTEXT_MENU_PROPERTIES,
			m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_PROPERTIES), {},
			m_resourceLoader->LoadString(IDS_BOOKMARK_CONTEXT_MENU_PROPERTIES_HELP_TEXT));

		m_menuView->EnableItem(IDM_BOOKMARK_CONTEXT_MENU_PROPERTIES, !permanentNodeSelected);
	}
}

bool BookmarkContextMenu::AreBookmarkItemsValid()
{
	if (m_bookmarkItems.empty())
	{
		return false;
	}

	// It's expected that all items will be in the same bookmark folder (i.e. they'll all have the
	// same parent).
	const BookmarkItem *parentFolder = m_bookmarkItems[0]->GetParent();
	return std::ranges::all_of(m_bookmarkItems, [parentFolder](const auto *bookmarkItem)
		{ return bookmarkItem->GetParent() == parentFolder; });
}

size_t BookmarkContextMenu::GetTotalBookmarks()
{
	size_t totalBookmarks = 0;

	for (const auto *bookmarkItem : m_bookmarkItems)
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

			totalBookmarks += numChildBookmarks;
		}
	}

	return totalBookmarks;
}

void BookmarkContextMenu::OnMenuItemSelected(UINT menuItemId)
{
	BookmarkItem *targetParentFolder;
	size_t targetIndex;

	if (m_bookmarkItems.size() == 1 && m_bookmarkItems[0]->IsFolder())
	{
		targetParentFolder = m_bookmarkItems[0];
		targetIndex = targetParentFolder->GetChildren().size();
	}
	else
	{
		targetParentFolder = m_bookmarkItems[0]->GetParent();
		targetIndex = std::ranges::max(m_bookmarkItems
						  | std::views::transform([targetParentFolder](const auto *bookmarkItem)
							  { return targetParentFolder->GetChildIndex(bookmarkItem); }))
			+ 1;
	}

	switch (menuItemId)
	{
	case IDM_BOOKMARK_CONTEXT_MENU_OPEN:
		OnOpen();
		break;

	case IDM_BOOKMARK_CONTEXT_MENU_OPEN_IN_NEW_TAB:
		OnOpenInNewTab();
		break;

	case IDM_BOOKMARK_CONTEXT_MENU_OPEN_ALL:
		OnOpenAll();
		break;

	case IDM_BOOKMARK_CONTEXT_MENU_NEW_BOOKMARK:
		OnNewBookmarkItem(BookmarkItem::Type::Bookmark, targetParentFolder, targetIndex);
		break;

	case IDM_BOOKMARK_CONTEXT_MENU_NEW_FOLDER:
		OnNewBookmarkItem(BookmarkItem::Type::Folder, targetParentFolder, targetIndex);
		break;

	case IDM_BOOKMARK_CONTEXT_MENU_CUT:
		OnCopy(ClipboardAction::Cut);
		break;

	case IDM_BOOKMARK_CONTEXT_MENU_COPY:
		OnCopy(ClipboardAction::Copy);
		break;

	case IDM_BOOKMARK_CONTEXT_MENU_PASTE:
		OnPaste(targetParentFolder, targetIndex);
		break;

	case IDM_BOOKMARK_CONTEXT_MENU_DELETE:
		OnDelete();
		break;

	case IDM_BOOKMARK_CONTEXT_MENU_PROPERTIES:
		OnShowProperties();
		break;

	default:
		DCHECK(false);
		break;
	}
}

void BookmarkContextMenu::OnOpen()
{
	DCHECK(m_bookmarkItems.size() == 1 && m_bookmarkItems[0]->IsBookmark());
	BookmarkHelper::OpenBookmarkItemWithDisposition(m_bookmarkItems[0],
		OpenFolderDisposition::CurrentTab, m_browser);
}

void BookmarkContextMenu::OnOpenInNewTab()
{
	DCHECK(m_bookmarkItems.size() == 1 && m_bookmarkItems[0]->IsBookmark());
	BookmarkHelper::OpenBookmarkItemWithDisposition(m_bookmarkItems[0],
		OpenFolderDisposition::NewTabDefault, m_browser);
}

void BookmarkContextMenu::OnOpenAll()
{
	OpenFolderDisposition disposition = OpenFolderDisposition::NewTabDefault;

	for (auto *bookmarkItem : m_bookmarkItems)
	{
		BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem, disposition, m_browser);

		disposition = OpenFolderDisposition::BackgroundTab;
	}
}

void BookmarkContextMenu::OnNewBookmarkItem(BookmarkItem::Type type,
	BookmarkItem *targetParentFolder, size_t targetIndex)
{
	BookmarkHelper::AddBookmarkItem(m_bookmarkTree, type, targetParentFolder, targetIndex,
		m_parentWindow, m_browser, m_acceleratorManager, m_resourceLoader);
}

void BookmarkContextMenu::OnCopy(ClipboardAction action)
{
	BookmarkHelper::CopyBookmarkItems(m_clipboardStore, m_bookmarkTree, m_bookmarkItems, action);
}

void BookmarkContextMenu::OnPaste(BookmarkItem *targetParentFolder, size_t targetIndex)
{
	BookmarkHelper::PasteBookmarkItems(m_clipboardStore, m_bookmarkTree, targetParentFolder,
		targetIndex);
}

void BookmarkContextMenu::OnDelete()
{
	BookmarkHelper::RemoveBookmarks(m_bookmarkTree, m_bookmarkItems);
}

void BookmarkContextMenu::OnShowProperties()
{
	DCHECK_EQ(m_bookmarkItems.size(), 1U);
	BookmarkHelper::EditBookmarkItem(m_bookmarkItems[0], m_bookmarkTree, m_acceleratorManager,
		m_resourceLoader, m_parentWindow);
}
