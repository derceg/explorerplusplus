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
	m_rootMenuView->AppendItem(this, IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_RENAME,
		m_resourceLoader->LoadString(IDS_BOOKMARK_TREEVIEW_CONTEXT_MENU_RENAME));
	m_rootMenuView->AppendItem(this, IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_DELETE,
		m_resourceLoader->LoadString(IDS_BOOKMARK_TREEVIEW_CONTEXT_MENU_DELETE));
	m_rootMenuView->AppendItem(this, IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_NEW_FOLDER,
		m_resourceLoader->LoadString(IDS_BOOKMARK_TREEVIEW_CONTEXT_MENU_NEW_FOLDER));

	m_rootMenuView->EnableItem(IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_RENAME,
		!m_bookmarkTree->IsPermanentNode(m_targetFolder.Get()));
	m_rootMenuView->EnableItem(IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_DELETE,
		!m_bookmarkTree->IsPermanentNode(m_targetFolder.Get()));
}

void BookmarkTreeViewContextMenu::OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	UNREFERENCED_PARAMETER(isCtrlKeyDown);
	UNREFERENCED_PARAMETER(isShiftKeyDown);

	if (!m_targetFolder)
	{
		return;
	}

	switch (id)
	{
	case IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_RENAME:
		m_delegate->StartRenamingFolder(m_targetFolder.Get());
		break;

	case IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_DELETE:
		DeleteItem();
		break;

	case IDM_BOOKMARK_TREEVIEW_CONTEXT_MENU_NEW_FOLDER:
		m_delegate->CreateFolder(m_targetFolder.Get(), m_targetFolder->GetChildren().size());
		break;

	default:
		DCHECK(false);
		break;
	}
}

void BookmarkTreeViewContextMenu::DeleteItem()
{
	m_bookmarkTree->RemoveBookmarkItem(m_targetFolder.Get());
}
