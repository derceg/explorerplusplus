// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkItem.h"
#include "SignalWrapper.h"

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

	bool IsPermanentNode(const BookmarkItem *bookmarkItem) const;

	void AddBookmarkItem(BookmarkItem *parent, std::unique_ptr<BookmarkItem> bookmarkItem, size_t index);
	void MoveBookmarkItem(BookmarkItem *bookmarkItem, BookmarkItem *newParent, size_t index);
	void RemoveBookmarkItem(BookmarkItem *bookmarkItem);

	void LoadRegistrySettings(HKEY parentKey);
	void SaveRegistrySettings(HKEY parentKey);

	// Signals
	SignalWrapper<BookmarkTree, void(BookmarkItem &bookmarkItem, size_t index)> bookmarkItemAddedSignal;
	SignalWrapper<BookmarkTree, void(BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType)> bookmarkItemUpdatedSignal;
	SignalWrapper<BookmarkTree, void(BookmarkItem *bookmarkItem, const BookmarkItem *oldParent, size_t oldIndex,
		const BookmarkItem *newParent, size_t newIndex)> bookmarkItemMovedSignal;
	SignalWrapper<BookmarkTree, void(BookmarkItem &bookmarkItem)> bookmarkItemPreRemovalSignal;
	SignalWrapper<BookmarkTree, void(const std::wstring &guid)> bookmarkItemRemovedSignal;

private:

	void AddBookmarkItemUpdatedObservers(BookmarkItem *bookmarkItem);

	void OnBookmarkItemUpdated(BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType);

	void LoadPermanentFolderFromRegistry(HKEY parentKey, BookmarkItem *bookmarkItem, const std::wstring &name);
	void LoadBookmarkChildrenFromRegistry(HKEY parentKey, BookmarkItem *parentBookmarkItem);
	std::unique_ptr<BookmarkItem> LoadBookmarkItemFromRegistry(HKEY key);

	void SavePermanentFolderToRegistry(HKEY parentKey, const BookmarkItem *bookmarkItem, const std::wstring &name);
	void SaveBookmarkChildrenToRegistry(HKEY parentKey, const BookmarkItem *parentBookmarkItem);
	void SaveBookmarkItemToRegistry(HKEY key, const BookmarkItem *bookmarkItem);

	BookmarkItem m_root;
	BookmarkItem *m_bookmarksToolbar;
	BookmarkItem *m_bookmarksMenu;
	BookmarkItem *m_otherBookmarks;
};