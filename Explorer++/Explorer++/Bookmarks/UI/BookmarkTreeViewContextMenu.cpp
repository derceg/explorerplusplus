// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkTreeViewContextMenu.h"
#include "Bookmarks/BookmarkTree.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceLoader.h"

BookmarkTreeViewContextMenu::BookmarkTreeViewContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, BookmarkTreeViewContextMenuDelegate *delegate,
	BookmarkTree *bookmarkTree, WeakPtr<BookmarkItem> targetFolder,
	const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_delegate(delegate),
	m_bookmarkTree(bookmarkTree),
	m_targetFolder(targetFolder),
	m_resourceLoader(resourceLoader)
{
	// This context menu allows a new item to be added as a child of the target item. It's expected
	// that the target item will be a folder.
	DCHECK(targetFolder->IsFolder());

	BuildMenu();
}

void BookmarkTreeViewContextMenu::BuildMenu()
{
	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARK_TREEVIEW_CONTEXT_MENU_RENAME))
			.SetKey(Item::Rename)
			.SetEnabledCallback(
				[this] { return !m_bookmarkTree->IsPermanentNode(m_targetFolder.Get()); })
			.SetSelectedCallback([this] { OnRename(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARK_TREEVIEW_CONTEXT_MENU_DELETE))
			.SetKey(Item::Delete)
			.SetEnabledCallback(
				[this] { return !m_bookmarkTree->IsPermanentNode(m_targetFolder.Get()); })
			.SetSelectedCallback([this] { OnDelete(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARK_TREEVIEW_CONTEXT_MENU_NEW_FOLDER))
			.SetKey(Item::NewFolder)
			.SetSelectedCallback([this] { OnNewFolder(); }));
}

void BookmarkTreeViewContextMenu::OnRename()
{
	if (!m_targetFolder)
	{
		return;
	}

	m_delegate->StartRenamingFolder(m_targetFolder.Get());
}

void BookmarkTreeViewContextMenu::OnDelete()
{
	if (!m_targetFolder)
	{
		return;
	}

	m_bookmarkTree->RemoveBookmarkItem(m_targetFolder.Get());
}

void BookmarkTreeViewContextMenu::OnNewFolder()
{
	if (!m_targetFolder)
	{
		return;
	}

	m_delegate->CreateFolder(m_targetFolder.Get(), m_targetFolder->GetChildren().size());
}
