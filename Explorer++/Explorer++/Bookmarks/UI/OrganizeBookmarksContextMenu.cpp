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
#include <ranges>

OrganizeBookmarksContextMenu::OrganizeBookmarksContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, BookmarkTree *bookmarkTree,
	WeakPtr<BookmarkItem> targetFolder, OrganizeBookmarksContextMenuDelegate *delegate,
	ClipboardStore *clipboardStore, const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_bookmarkTree(bookmarkTree),
	m_targetFolder(targetFolder),
	m_delegate(delegate),
	m_clipboardStore(clipboardStore),
	m_resourceLoader(resourceLoader)
{
	BuildMenu();
}

void OrganizeBookmarksContextMenu::BuildMenu()
{
	m_rootMenuView->AppendItem(this, IDM_ORGANIZE_BOOKMARKS_CXMENU_NEW_BOOKMARK,
		m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_NEW_BOOKMARK));
	m_rootMenuView->AppendItem(this, IDM_ORGANIZE_BOOKMARKS_CXMENU_NEW_FOLDER,
		m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_NEW_FOLDER));

	m_rootMenuView->AppendSeparator();

	m_rootMenuView->AppendItem(this, IDM_ORGANIZE_BOOKMARKS_CXMENU_CUT,
		m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_CUT));
	m_rootMenuView->AppendItem(this, IDM_ORGANIZE_BOOKMARKS_CXMENU_COPY,
		m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_COPY));
	m_rootMenuView->AppendItem(this, IDM_ORGANIZE_BOOKMARKS_CXMENU_PASTE,
		m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_PASTE));
	m_rootMenuView->AppendItem(this, IDM_ORGANIZE_BOOKMARKS_CXMENU_DELETE,
		m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_DELETE));

	m_rootMenuView->AppendSeparator();

	m_rootMenuView->AppendItem(this, IDM_ORGANIZE_BOOKMARKS_CXMENU_SELECT_ALL,
		m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_SELECT_ALL));

	auto selectedBookmarkItems = m_delegate->GetSelectedItems();
	bool canDelete = !selectedBookmarkItems.empty()
		&& std::ranges::none_of(selectedBookmarkItems, [this](const auto *bookmarkItem)
			{ return m_bookmarkTree->IsPermanentNode(bookmarkItem); });
	m_rootMenuView->EnableItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_CUT, canDelete);
	m_rootMenuView->EnableItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_COPY, !selectedBookmarkItems.empty());

	m_rootMenuView->EnableItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_PASTE,
		m_clipboardStore->IsDataAvailable(BookmarkClipboard::GetClipboardFormat()));

	m_rootMenuView->EnableItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_DELETE, canDelete);

	m_rootMenuView->EnableItem(IDM_ORGANIZE_BOOKMARKS_CXMENU_SELECT_ALL,
		!m_targetFolder->GetChildren().empty() && m_delegate->CanSelectAllItems());
}

void OrganizeBookmarksContextMenu::OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	UNREFERENCED_PARAMETER(isCtrlKeyDown);
	UNREFERENCED_PARAMETER(isShiftKeyDown);

	if (!m_targetFolder)
	{
		return;
	}

	switch (id)
	{
	case IDM_ORGANIZE_BOOKMARKS_CXMENU_NEW_BOOKMARK:
		m_delegate->CreateBookmark(m_targetFolder.Get(), GetTargetIndex());
		break;

	case IDM_ORGANIZE_BOOKMARKS_CXMENU_NEW_FOLDER:
		m_delegate->CreateFolder(m_targetFolder.Get(), GetTargetIndex());
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
	BookmarkHelper::PasteBookmarkItems(m_clipboardStore, m_bookmarkTree, m_targetFolder.Get(),
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
	auto selectedChildren = m_delegate->GetSelectedChildItems(m_targetFolder.Get());

	if (selectedChildren.empty())
	{
		return m_targetFolder->GetChildren().size();
	}

	return std::ranges::max(selectedChildren
			   | std::views::transform([this](const auto *bookmarkItem)
				   { return m_targetFolder->GetChildIndex(bookmarkItem); }))
		+ 1;
}
