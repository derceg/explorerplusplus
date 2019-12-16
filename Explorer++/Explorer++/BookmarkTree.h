// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BookmarkItem.h"

class BookmarkTree
{
public:

	BookmarkTree();

	BookmarkItem *GetRoot();
	BookmarkItem *GetBookmarksToolbarFolder();
	BookmarkItem *GetBookmarksMenuFolder();

	void LoadRegistrySettings(HKEY parentKey);
	void SaveRegistrySettings(HKEY parentKey);

private:

	void LoadPermanentFolderFromRegistry(HKEY parentKey, BookmarkItem *bookmarkItem, const std::wstring &name);
	void LoadBookmarkChildrenFromRegistry(HKEY parentKey, BookmarkItem *parentBookmarkItem);
	std::unique_ptr<BookmarkItem> LoadBookmarkItemFromRegistry(HKEY key);

	void SavePermanentFolderToRegistry(HKEY parentKey, const BookmarkItem *bookmarkItem, const std::wstring &name);
	void SaveBookmarkChildrenToRegistry(HKEY parentKey, const BookmarkItem *parentBookmarkItem);
	void SaveBookmarkItemToRegistry(HKEY key, const BookmarkItem *bookmarkItem);

	BookmarkItem m_root;
	BookmarkItem *m_bookmarksToolbar;
	BookmarkItem *m_bookmarksMenu;
};