// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "../Helper/SignalWrapper.h"
#include <tchar.h>

class BookmarkTree
{
public:
	BookmarkTree();

	BookmarkItem *GetRoot();
	BookmarkItem *GetBookmarksToolbarFolder();
	const BookmarkItem *GetBookmarksToolbarFolder() const;
	BookmarkItem *GetBookmarksMenuFolder();
	const BookmarkItem *GetBookmarksMenuFolder() const;
	BookmarkItem *GetOtherBookmarksFolder();
	const BookmarkItem *GetOtherBookmarksFolder() const;

	bool CanAddChildren(const BookmarkItem *bookmarkItem) const;
	bool IsPermanentNode(const BookmarkItem *bookmarkItem) const;

	BookmarkItem *AddBookmarkItem(BookmarkItem *parent, std::unique_ptr<BookmarkItem> bookmarkItem);
	BookmarkItem *AddBookmarkItem(BookmarkItem *parent, std::unique_ptr<BookmarkItem> bookmarkItem,
		size_t index);
	void MoveBookmarkItem(BookmarkItem *bookmarkItem, BookmarkItem *newParent, size_t index);
	void RemoveBookmarkItem(BookmarkItem *bookmarkItem);

	// Signals
	SignalWrapper<BookmarkTree, void(BookmarkItem &bookmarkItem, size_t index)>
		bookmarkItemAddedSignal;
	SignalWrapper<BookmarkTree,
		void(BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType)>
		bookmarkItemUpdatedSignal;
	SignalWrapper<BookmarkTree,
		void(BookmarkItem *bookmarkItem, const BookmarkItem *oldParent, size_t oldIndex,
			const BookmarkItem *newParent, size_t newIndex)>
		bookmarkItemMovedSignal;
	SignalWrapper<BookmarkTree, void(BookmarkItem &bookmarkItem)> bookmarkItemPreRemovalSignal;
	SignalWrapper<BookmarkTree, void(const std::wstring &guid)> bookmarkItemRemovedSignal;

private:
	// These GUID's are statically defined, so that they will always be the same
	// across separate instances of the process.
	static inline const TCHAR *ROOT_FOLDER_GUID = _T("00000000-0000-0000-0000-000000000001");
	static inline const TCHAR *TOOLBAR_FOLDER_GUID = _T("00000000-0000-0000-0000-000000000002");
	static inline const TCHAR *MENU_FOLDER_GUID = _T("00000000-0000-0000-0000-000000000003");
	static inline const TCHAR *OTHER_FOLDER_GUID = _T("00000000-0000-0000-0000-000000000004");

	void OnBookmarkItemUpdated(BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType);

	BookmarkItem m_root;
	BookmarkItem *m_bookmarksToolbar;
	BookmarkItem *m_bookmarksMenu;
	BookmarkItem *m_otherBookmarks;
};
