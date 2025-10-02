// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/UI/BookmarkColumn.h"
#include "ListViewItem.h"
#include "../Helper/WeakPtr.h"
#include "../Helper/WeakPtrFactory.h"
#include <boost/signals2.hpp>
#include <optional>

class BookmarkIconManager;
class BookmarkTree;
struct Config;

class BookmarkListViewItem : public ListViewItem
{
public:
	BookmarkListViewItem(BookmarkItem *bookmarkItem, const BookmarkTree *bookmarkTree,
		BookmarkIconManager *bookmarkIconManager, const Config *config);

	std::wstring GetColumnText(ListViewColumnId columnId) const override;
	std::optional<int> GetIconIndex() const override;
	bool CanRename() const override;
	bool CanRemove() const override;

	BookmarkItem *GetBookmarkItem();
	const BookmarkItem *GetBookmarkItem() const;

private:
	void OnBookmarkItemUpdated(BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType);

	std::wstring GetTextForColumn(BookmarkColumn column) const;
	std::wstring FormatDate(const FILETIME &date) const;

	static void OnIconAvailable(WeakPtr<BookmarkListViewItem> weakSelf, int iconIndex);

	BookmarkItem *const m_bookmarkItem;
	const BookmarkTree *const m_bookmarkTree;
	BookmarkIconManager *const m_bookmarkIconManager;
	const Config *const m_config;
	mutable std::optional<int> m_iconIndex;
	boost::signals2::scoped_connection m_updateConnection;

	WeakPtrFactory<BookmarkListViewItem> m_weakPtrFactory{ this };
};
