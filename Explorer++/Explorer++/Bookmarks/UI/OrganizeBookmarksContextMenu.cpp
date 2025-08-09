// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "OrganizeBookmarksContextMenu.h"
#include "Bookmarks/BookmarkClipboard.h"
#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkTree.h"
#include "MainResource.h"
#include "MenuView.h"
#include "OrganizeBookmarksContextMenuDelegate.h"
#include "ResourceLoader.h"
#include "../Helper/ClipboardStore.h"
#include <algorithm>

OrganizeBookmarksContextMenu::OrganizeBookmarksContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, HWND parentWindow, BookmarkTree *bookmarkTree,
	BookmarkItem *targetFolder, OrganizeBookmarksContextMenuDelegate *delegate,
	ClipboardStore *clipboardStore, const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_parentWindow(parentWindow),
	m_bookmarkTree(bookmarkTree),
	m_targetFolder(targetFolder),
	m_delegate(delegate),
	m_clipboardStore(clipboardStore),
	m_resourceLoader(resourceLoader)
{
	BuildMenu();

	m_connections.push_back(m_menuView->AddItemSelectedObserver(
		std::bind(&OrganizeBookmarksContextMenu::OnMenuItemSelected, this, std::placeholders::_1)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemPreRemovalSignal.AddObserver(
		std::bind_front(&OrganizeBookmarksContextMenu::OnBookmarkItemPreRemoval, this)));
}

void OrganizeBookmarksContextMenu::BuildMenu()
{
	m_menuView->AppendItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_NEW_BOOKMARK,
		m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_NEW_BOOKMARK));
	m_menuView->AppendItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_NEW_FOLDER,
		m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_NEW_FOLDER));

	m_menuView->AppendSeparator();

	m_menuView->AppendItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_CUT,
		m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_CUT));
	m_menuView->AppendItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_COPY,
		m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_COPY));
	m_menuView->AppendItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_PASTE,
		m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_PASTE));
	m_menuView->AppendItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_DELETE,
		m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_DELETE));

	m_menuView->AppendSeparator();

	m_menuView->AppendItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_SELECT_ALL,
		m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_SELECT_ALL));

	auto selectedBookmarkItems = m_delegate->GetSelectedItems();
	bool canDelete = !selectedBookmarkItems.empty()
		&& std::ranges::none_of(selectedBookmarkItems, [this](const auto *bookmarkItem)
			{ return m_bookmarkTree->IsPermanentNode(bookmarkItem); });
	m_menuView->EnableItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_CUT, canDelete);
	m_menuView->EnableItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_COPY, !selectedBookmarkItems.empty());

	m_menuView->EnableItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_PASTE,
		m_clipboardStore->IsDataAvailable(BookmarkClipboard::GetClipboardFormat()));

	m_menuView->EnableItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_DELETE, canDelete);

	m_menuView->EnableItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_SELECT_ALL,
		m_delegate->CanSelectAllItems());
}

void OrganizeBookmarksContextMenu::OnMenuItemSelected(UINT menuItemId)
{
	if (!m_targetFolder)
	{
		return;
	}

	switch (menuItemId)
	{
	case IDM_ORGANIZE_BOOKMARKS_CXMENU_NEW_BOOKMARK:
		OnNewBookmark();
		break;

	case IDM_ORGANIZE_BOOKMARKS_CXMENU_NEW_FOLDER:
		OnNewFolder();
		break;

	case IDM_ORGANIZE_BOOKMARKS_CXMENU_CUT:
		OnCopy(ClipboardAction::Cut);
		break;

	case IDM_ORGANIZE_BOOKMARKS_CXMENU_COPY:
		OnCopy(ClipboardAction::Copy);
		break;

	case IDM_ORGANIZE_BOOKMARKS_CXMENU_PASTE:
		OnPaste();
		break;

	case IDM_ORGANIZE_BOOKMARKS_CXMENU_DELETE:
		OnDelete();
		break;

	case IDM_ORGANIZE_BOOKMARKS_CXMENU_SELECT_ALL:
		OnSelectAll();
		break;

	default:
		DCHECK(false);
		break;
	}
}

void OrganizeBookmarksContextMenu::OnNewBookmark()
{
	auto *bookmark = BookmarkHelper::AddBookmarkItem(m_bookmarkTree, BookmarkItem::Type::Bookmark,
		m_targetFolder, GetTargetIndex(), m_parentWindow, nullptr, m_acceleratorManager,
		m_resourceLoader);

	if (!bookmark || bookmark->GetParent() != m_targetFolder)
	{
		return;
	}

	m_delegate->SelectItem(bookmark);
}

void OrganizeBookmarksContextMenu::OnNewFolder()
{
	m_delegate->CreateFolder(GetTargetIndex());
}

void OrganizeBookmarksContextMenu::OnCopy(ClipboardAction action)
{
	auto selectedItems = m_delegate->GetSelectedItems();

	if (selectedItems.empty())
	{
		return;
	}

	BookmarkHelper::CopyBookmarkItems(m_clipboardStore, m_bookmarkTree, selectedItems, action);
}

void OrganizeBookmarksContextMenu::OnPaste()
{
	BookmarkHelper::PasteBookmarkItems(m_clipboardStore, m_bookmarkTree, m_targetFolder,
		GetTargetIndex());
}

void OrganizeBookmarksContextMenu::OnDelete()
{
	BookmarkHelper::RemoveBookmarks(m_bookmarkTree, m_delegate->GetSelectedItems());
}

void OrganizeBookmarksContextMenu::OnSelectAll()
{
	m_delegate->SelectAllItems();
}

size_t OrganizeBookmarksContextMenu::GetTargetIndex() const
{
	auto selectedChildren = m_delegate->GetSelectedChildItems(m_targetFolder);

	if (selectedChildren.empty())
	{
		return m_targetFolder->GetChildren().size();
	}

	return std::ranges::max(selectedChildren
			   | std::views::transform([this](const auto *bookmarkItem)
				   { return m_targetFolder->GetChildIndex(bookmarkItem); }))
		+ 1;
}

void OrganizeBookmarksContextMenu::OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem)
{
	if (BookmarkHelper::IsAncestor(m_targetFolder, &bookmarkItem))
	{
		m_targetFolder = nullptr;
	}
}
