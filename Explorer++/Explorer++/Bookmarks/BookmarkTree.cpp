// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/BookmarkHelper.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include <glog/logging.h>

BookmarkTree::BookmarkTree() :
	m_root(ROOT_FOLDER_GUID,
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_BOOKMARKS_ALLBOOKMARKS),
		std::nullopt)
{
	auto bookmarksToolbarFolder = std::make_unique<BookmarkItem>(TOOLBAR_FOLDER_GUID,
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_BOOKMARKS_BOOKMARKSTOOLBAR),
		std::nullopt);
	m_bookmarksToolbar = bookmarksToolbarFolder.get();
	m_root.AddChild(std::move(bookmarksToolbarFolder));

	auto bookmarksMenuFolder = std::make_unique<BookmarkItem>(MENU_FOLDER_GUID,
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_BOOKMARKS_BOOKMARKSMENU),
		std::nullopt);
	m_bookmarksMenu = bookmarksMenuFolder.get();
	m_root.AddChild(std::move(bookmarksMenuFolder));

	auto otherBookmarksFolder = std::make_unique<BookmarkItem>(OTHER_FOLDER_GUID,
		ResourceHelper::LoadString(GetModuleHandle(nullptr), IDS_BOOKMARKS_OTHER_BOOKMARKS),
		std::nullopt);
	m_otherBookmarks = otherBookmarksFolder.get();
	m_root.AddChild(std::move(otherBookmarksFolder));
}

BookmarkItem *BookmarkTree::GetRoot()
{
	return &m_root;
}

BookmarkItem *BookmarkTree::GetBookmarksToolbarFolder()
{
	return m_bookmarksToolbar;
}

const BookmarkItem *BookmarkTree::GetBookmarksToolbarFolder() const
{
	return m_bookmarksToolbar;
}

BookmarkItem *BookmarkTree::GetBookmarksMenuFolder()
{
	return m_bookmarksMenu;
}

const BookmarkItem *BookmarkTree::GetBookmarksMenuFolder() const
{
	return m_bookmarksMenu;
}

BookmarkItem *BookmarkTree::GetOtherBookmarksFolder()
{
	return m_otherBookmarks;
}

const BookmarkItem *BookmarkTree::GetOtherBookmarksFolder() const
{
	return m_otherBookmarks;
}

BookmarkItem *BookmarkTree::AddBookmarkItem(BookmarkItem *parent,
	std::unique_ptr<BookmarkItem> bookmarkItem)
{
	return AddBookmarkItem(parent, std::move(bookmarkItem), parent->GetChildren().size());
}

BookmarkItem *BookmarkTree::AddBookmarkItem(BookmarkItem *parent,
	std::unique_ptr<BookmarkItem> bookmarkItem, size_t index)
{
	if (!CanAddChildren(parent))
	{
		DCHECK(false);
		return nullptr;
	}

	// For an item to be added to the tree, the parent item should already be part of the tree.
	CHECK(IsInTree(parent));

	bookmarkItem->VisitRecursively(
		[this](BookmarkItem *currentItem)
		{
			currentItem->ClearOriginalGUID();

			// Adds an observer to each bookmark item that's being added. This is needed so that
			// this class can broadcast an event whenever an individual bookmark item is updated.
			currentItem->updatedSignal.AddObserver(
				std::bind_front(&BookmarkTree::OnBookmarkItemUpdated, this),
				boost::signals2::at_front);
		});

	if (index > parent->GetChildren().size())
	{
		index = parent->GetChildren().size();
	}

	BookmarkItem *rawBookmarkItem = parent->AddChild(std::move(bookmarkItem), index);
	bookmarkItemAddedSignal.m_signal(*rawBookmarkItem, index);

	return rawBookmarkItem;
}

void BookmarkTree::MoveBookmarkItem(BookmarkItem *bookmarkItem, BookmarkItem *newParent,
	size_t index)
{
	if (!CanAddChildren(newParent) || IsPermanentNode(bookmarkItem))
	{
		DCHECK(false);
		return;
	}

	CHECK(IsInTree(bookmarkItem));
	CHECK(IsInTree(newParent));

	BookmarkItem *oldParent = bookmarkItem->GetParent();
	size_t oldIndex = oldParent->GetChildIndex(bookmarkItem);

	if (index > newParent->GetChildren().size())
	{
		index = newParent->GetChildren().size();
	}

	if (oldParent == newParent && index > oldIndex)
	{
		index--;
	}

	if (oldParent == newParent && index == oldIndex)
	{
		return;
	}

	auto item = oldParent->RemoveChild(oldIndex);
	newParent->AddChild(std::move(item), index);

	bookmarkItemMovedSignal.m_signal(bookmarkItem, oldParent, oldIndex, newParent, index);
}

void BookmarkTree::RemoveBookmarkItem(BookmarkItem *bookmarkItem)
{
	if (IsPermanentNode(bookmarkItem))
	{
		return;
	}

	CHECK(IsInTree(bookmarkItem));

	bookmarkItemPreRemovalSignal.m_signal(*bookmarkItem);

	BookmarkItem *parent = bookmarkItem->GetParent();
	DCHECK_NOTNULL(bookmarkItem->GetParent());

	std::wstring guid = bookmarkItem->GetGUID();

	size_t childIndex = parent->GetChildIndex(bookmarkItem);
	parent->RemoveChild(childIndex);
	bookmarkItemRemovedSignal.m_signal(guid);
}

bool BookmarkTree::IsInTree(const BookmarkItem *bookmarkItem)
{
	return BookmarkHelper::GetBookmarkItemById(this, bookmarkItem->GetGUID());
}

void BookmarkTree::OnBookmarkItemUpdated(BookmarkItem &bookmarkItem,
	BookmarkItem::PropertyType propertyType)
{
	bookmarkItemUpdatedSignal.m_signal(bookmarkItem, propertyType);
}

bool BookmarkTree::CanAddChildren(const BookmarkItem *bookmarkItem) const
{
	return bookmarkItem != &m_root;
}

bool BookmarkTree::IsPermanentNode(const BookmarkItem *bookmarkItem) const
{
	if (bookmarkItem == &m_root || bookmarkItem == m_bookmarksToolbar
		|| bookmarkItem == m_bookmarksMenu || bookmarkItem == m_otherBookmarks)
	{
		return true;
	}

	return false;
}
