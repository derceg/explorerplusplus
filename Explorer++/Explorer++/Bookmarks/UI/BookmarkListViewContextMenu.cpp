// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkListViewContextMenu.h"
#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/UI/BookmarkItemCreationDelegate.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceLoader.h"

BookmarkListViewContextMenu::BookmarkListViewContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, BookmarkItemCreationDelegate *delegate,
	WeakPtr<BookmarkItem> targetFolder, const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_delegate(delegate),
	m_targetFolder(targetFolder),
	m_resourceLoader(resourceLoader)
{
	BuildMenu();
}

void BookmarkListViewContextMenu::BuildMenu()
{
	m_menuView->AppendItem(this, IDM_BOOKMARK_LISTVIEW_CONTEXT_MENU_NEW_BOOKMARK,
		m_resourceLoader->LoadString(IDS_BOOKMARK_LISTVIEW_CONTEXT_MENU_NEW_BOOKMARK));
	m_menuView->AppendItem(this, IDM_BOOKMARK_LISTVIEW_CONTEXT_MENU_NEW_FOLDER,
		m_resourceLoader->LoadString(IDS_BOOKMARK_LISTVIEW_CONTEXT_MENU_NEW_FOLDER));
}

void BookmarkListViewContextMenu::OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	UNREFERENCED_PARAMETER(isCtrlKeyDown);
	UNREFERENCED_PARAMETER(isShiftKeyDown);

	if (!m_targetFolder)
	{
		return;
	}

	auto index = m_targetFolder->GetChildren().size();

	switch (id)
	{
	case IDM_BOOKMARK_LISTVIEW_CONTEXT_MENU_NEW_BOOKMARK:
		m_delegate->CreateBookmark(m_targetFolder.Get(), index);
		break;

	case IDM_BOOKMARK_LISTVIEW_CONTEXT_MENU_NEW_FOLDER:
		m_delegate->CreateFolder(m_targetFolder.Get(), index);
		break;

	default:
		DCHECK(false);
		break;
	}
}
