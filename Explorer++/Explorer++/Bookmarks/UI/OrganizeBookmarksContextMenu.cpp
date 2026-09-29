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
	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_NEW_BOOKMARK))
			.SetKey(Item::NewBookmark)
			.SetSelectedCallback([this] { OnNewBookmarkItem(BookmarkItem::Type::Bookmark); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_NEW_FOLDER))
			.SetKey(Item::NewFolder)
			.SetSelectedCallback([this] { OnNewBookmarkItem(BookmarkItem::Type::Folder); }));

	m_rootMenuView->AppendSeparator();

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_CUT))
			.SetKey(Item::Cut)
			.SetEnabledCallback([this] { return CanRemoveSelectedItems(); })
			.SetSelectedCallback([this] { OnCopy(ClipboardAction::Cut); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_COPY))
			.SetKey(Item::Copy)
			.SetEnabledCallback([this] { return !m_delegate->GetSelectedItems().empty(); })
			.SetSelectedCallback([this] { OnCopy(ClipboardAction::Copy); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_PASTE))
			.SetKey(Item::Paste)
			.SetEnabledCallback([this] { return CanPaste(); })
			.SetSelectedCallback([this] { OnPaste(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_DELETE))
			.SetKey(Item::Delete)
			.SetEnabledCallback([this] { return CanRemoveSelectedItems(); })
			.SetSelectedCallback([this] { OnDelete(); }));

	m_rootMenuView->AppendSeparator();

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_ORGANIZE_BOOKMARKS_CXMENU_SELECT_ALL))
			.SetKey(Item::SelectAll)
			.SetEnabledCallback([this] { return CanSelectAll(); })
			.SetSelectedCallback([this] { m_delegate->SelectAllItems(); }));
}

bool OrganizeBookmarksContextMenu::CanPaste() const
{
	return m_clipboardStore->IsDataAvailable(BookmarkClipboard::GetClipboardFormat());
}

bool OrganizeBookmarksContextMenu::CanRemoveSelectedItems() const
{
	auto selectedBookmarkItems = m_delegate->GetSelectedItems();
	return !selectedBookmarkItems.empty()
		&& std::ranges::none_of(selectedBookmarkItems, [this](const auto *bookmarkItem)
			{ return m_bookmarkTree->IsPermanentNode(bookmarkItem); });
}

bool OrganizeBookmarksContextMenu::CanSelectAll() const
{
	return !m_targetFolder->GetChildren().empty() && m_delegate->CanSelectAllItems();
}

void OrganizeBookmarksContextMenu::OnNewBookmarkItem(BookmarkItem::Type type)
{
	if (!m_targetFolder)
	{
		return;
	}

	if (type == BookmarkItem::Type::Bookmark)
	{
		m_delegate->CreateBookmark(m_targetFolder.Get(), GetTargetIndex());
	}
	else
	{
		m_delegate->CreateFolder(m_targetFolder.Get(), GetTargetIndex());
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
	if (!m_targetFolder)
	{
		return;
	}

	BookmarkHelper::PasteBookmarkItems(m_clipboardStore, m_bookmarkTree, m_targetFolder.Get(),
		GetTargetIndex());
}

void OrganizeBookmarksContextMenu::OnDelete()
{
	BookmarkHelper::RemoveBookmarks(m_bookmarkTree, m_delegate->GetSelectedItems());
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
