// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkListViewContextMenu.h"
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
	m_rootMenuView->AppendItem(MenuItemParams(
		m_resourceLoader->LoadString(IDS_BOOKMARK_LISTVIEW_CONTEXT_MENU_NEW_BOOKMARK))
			.SetKey(Item::NewBookmark)
			.SetSelectedCallback([this] { CreateBookmarkItem(BookmarkItem::Type::Bookmark); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_BOOKMARK_LISTVIEW_CONTEXT_MENU_NEW_FOLDER))
			.SetKey(Item::NewFolder)
			.SetSelectedCallback([this] { CreateBookmarkItem(BookmarkItem::Type::Folder); }));
}

void BookmarkListViewContextMenu::CreateBookmarkItem(BookmarkItem::Type type)
{
	if (!m_targetFolder)
	{
		return;
	}

	auto index = m_targetFolder->GetChildren().size();

	if (type == BookmarkItem::Type::Bookmark)
	{
		m_delegate->CreateBookmark(m_targetFolder.Get(), index);
	}
	else
	{
		m_delegate->CreateFolder(m_targetFolder.Get(), index);
	}
}
