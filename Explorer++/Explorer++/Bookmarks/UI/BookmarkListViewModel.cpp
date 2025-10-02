// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkListViewModel.h"
#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/BookmarkColumnHelper.h"
#include "Bookmarks/UI/BookmarkListViewItem.h"

BookmarkListViewModel::BookmarkListViewModel(BookmarkTree *bookmarkTree,
	BookmarkIconManager *bookmarkIconManager, const BookmarkColumnModel &columnModel,
	const Config *config) :
	ListViewModel(SortPolicy::HasDefault),
	m_bookmarkTree(bookmarkTree),
	m_bookmarkIconManager(bookmarkIconManager),
	m_columnModel(columnModel),
	m_config(config)
{
	m_connections.push_back(m_bookmarkTree->bookmarkItemAddedSignal.AddObserver(
		std::bind_front(&BookmarkListViewModel::OnBookmarkItemAdded, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemMovedSignal.AddObserver(
		std::bind_front(&BookmarkListViewModel::OnBookmarkItemMoved, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemPreRemovalSignal.AddObserver(
		std::bind_front(&BookmarkListViewModel::OnBookmarkItemPreRemoval, this)));
}

void BookmarkListViewModel::SetCurrentFolder(BookmarkItem *folder)
{
	DCHECK(folder->IsFolder());
	m_currentFolder = folder;

	RemoveAllBookmarkItems();

	for (const auto &child : folder->GetChildren())
	{
		AddBookmarkItem(child.get());
	}
}

void BookmarkListViewModel::OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index)
{
	UNREFERENCED_PARAMETER(index);

	if (bookmarkItem.GetParent() != m_currentFolder)
	{
		return;
	}

	AddBookmarkItem(&bookmarkItem);
}

void BookmarkListViewModel::OnBookmarkItemMoved(BookmarkItem *bookmarkItem,
	const BookmarkItem *oldParent, size_t oldIndex, const BookmarkItem *newParent, size_t newIndex)
{
	UNREFERENCED_PARAMETER(oldIndex);
	UNREFERENCED_PARAMETER(newIndex);

	if (oldParent == m_currentFolder && newParent == m_currentFolder)
	{
		MaybeRepositionItem(GetItemForBookmark(bookmarkItem));
	}
	else if (oldParent == m_currentFolder)
	{
		RemoveBookmarkItem(bookmarkItem);
	}
	else if (newParent == m_currentFolder)
	{
		AddBookmarkItem(bookmarkItem);
	}
}

void BookmarkListViewModel::OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem)
{
	if (&bookmarkItem == m_currentFolder)
	{
		m_currentFolder = nullptr;
		RemoveAllBookmarkItems();
	}
	else if (bookmarkItem.GetParent() == m_currentFolder)
	{
		RemoveBookmarkItem(&bookmarkItem);
	}
}

void BookmarkListViewModel::AddBookmarkItem(BookmarkItem *bookmarkItem)
{
	auto item = std::make_unique<BookmarkListViewItem>(bookmarkItem, m_bookmarkTree,
		m_bookmarkIconManager, m_config);

	auto [mapItr, didInsert] = m_bookmarkToItemMap.insert({ bookmarkItem, item.get() });
	CHECK(didInsert);

	AddItem(std::move(item));
}

void BookmarkListViewModel::RemoveBookmarkItem(BookmarkItem *bookmarkItem)
{
	auto *item = GetItemForBookmark(bookmarkItem);

	auto numErased = m_bookmarkToItemMap.erase(bookmarkItem);
	CHECK_EQ(numErased, 1u);

	RemoveItem(item);
}

void BookmarkListViewModel::RemoveAllBookmarkItems()
{
	m_bookmarkToItemMap.clear();
	RemoveAllItems();
}

std::weak_ordering BookmarkListViewModel::CompareItems(const ListViewItem *first,
	const ListViewItem *second) const
{
	const auto *firstBookmark = GetBookmarkForItem(first);
	const auto *secondBookmark = GetBookmarkForItem(second);

	auto sortColumnId = GetSortColumnId();

	if (sortColumnId)
	{
		return CompareBookmarksByColumn(
			BookmarkColumnModel::ColumnIdToBookmarkColumn(*sortColumnId), firstBookmark,
			secondBookmark);
	}
	else
	{
		auto firstIndex = m_currentFolder->GetChildIndex(firstBookmark);
		auto secondIndex = m_currentFolder->GetChildIndex(secondBookmark);

		return (firstIndex < secondIndex) ? std::weak_ordering::less
			: (firstIndex > secondIndex)  ? std::weak_ordering::greater
										  : std::weak_ordering::equivalent;
	}
}

BookmarkColumnModel *BookmarkListViewModel::GetColumnModel()
{
	return &m_columnModel;
}

const BookmarkColumnModel *BookmarkListViewModel::GetColumnModel() const
{
	return &m_columnModel;
}

BookmarkItem *BookmarkListViewModel::GetBookmarkForItem(ListViewItem *item)
{
	return static_cast<BookmarkListViewItem *>(item)->GetBookmarkItem();
}

const BookmarkItem *BookmarkListViewModel::GetBookmarkForItem(const ListViewItem *item) const
{
	return static_cast<const BookmarkListViewItem *>(item)->GetBookmarkItem();
}

ListViewItem *BookmarkListViewModel::GetItemForBookmark(const BookmarkItem *bookmarkItem)
{
	return const_cast<ListViewItem *>(std::as_const(*this).GetItemForBookmark(bookmarkItem));
}

const ListViewItem *BookmarkListViewModel::GetItemForBookmark(
	const BookmarkItem *bookmarkItem) const
{
	auto itr = m_bookmarkToItemMap.find(bookmarkItem);
	CHECK(itr != m_bookmarkToItemMap.end());
	return itr->second;
}
