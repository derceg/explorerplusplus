// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/UI/BookmarkColumnModel.h"
#include "ListViewModel.h"
#include <boost/signals2.hpp>
#include <unordered_map>
#include <vector>

class BookmarkIconManager;
class BookmarkItem;
class BookmarkTree;
struct Config;

class BookmarkListViewModel : public ListViewModel
{
public:
	BookmarkListViewModel(BookmarkTree *bookmarkTree, BookmarkIconManager *bookmarkIconManager,
		const BookmarkColumnModel &columnModel, const Config *config);

	void SetCurrentFolder(BookmarkItem *folder);

	BookmarkColumnModel *GetColumnModel() override;
	const BookmarkColumnModel *GetColumnModel() const override;

	BookmarkItem *GetBookmarkForItem(ListViewItem *item);
	const BookmarkItem *GetBookmarkForItem(const ListViewItem *item) const;
	ListViewItem *GetItemForBookmark(const BookmarkItem *bookmarkItem);
	const ListViewItem *GetItemForBookmark(const BookmarkItem *bookmarkItem) const;

protected:
	std::weak_ordering CompareItems(const ListViewItem *first,
		const ListViewItem *second) const override;

private:
	void OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index);
	void OnBookmarkItemMoved(BookmarkItem *bookmarkItem, const BookmarkItem *oldParent,
		size_t oldIndex, const BookmarkItem *newParent, size_t newIndex);
	void OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem);

	void AddBookmarkItem(BookmarkItem *bookmarkItem);
	void RemoveBookmarkItem(BookmarkItem *bookmarkItem);

	void RemoveAllBookmarkItems();

	BookmarkTree *const m_bookmarkTree;
	BookmarkIconManager *const m_bookmarkIconManager;
	BookmarkColumnModel m_columnModel;
	const Config *const m_config;
	BookmarkItem *m_currentFolder = nullptr;
	std::unordered_map<const BookmarkItem *, ListViewItem *> m_bookmarkToItemMap;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
