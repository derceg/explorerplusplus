// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "TreeViewNode.h"
#include <boost/signals2.hpp>

class BookmarkTree;

class BookmarkTreeViewNode : public TreeViewNode
{
public:
	BookmarkTreeViewNode(BookmarkItem *bookmarkFolder, const BookmarkTree *bookmarkTree,
		int bookmarkFolderIconIndex);

	std::wstring GetText() const override;
	std::optional<std::wstring> MaybeGetEditingText() const override;
	std::optional<int> GetIconIndex() const override;
	bool CanRename() const override;
	bool CanRemove() const override;
	bool IsGhosted() const override;
	bool IsFile() const override;

	BookmarkItem *GetBookmarkFolder();
	const BookmarkItem *GetBookmarkFolder() const;

private:
	void OnBookmarkFolderUpdated(BookmarkItem &bookmarkFolder,
		BookmarkItem::PropertyType propertyType);

	BookmarkItem *const m_bookmarkFolder;
	const BookmarkTree *const m_bookmarkTree;
	const int m_bookmarkFolderIconIndex;
	boost::signals2::scoped_connection m_updateConnection;
};
