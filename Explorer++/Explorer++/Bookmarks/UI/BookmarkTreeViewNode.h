// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TreeViewNode.h"

class BookmarkItem;
class BookmarkTree;

class BookmarkTreeViewNode : public TreeViewNode
{
public:
	BookmarkTreeViewNode(BookmarkItem *bookmarkFolder, const BookmarkTree *bookmarkTree,
		int bookmarkFolderIconIndex);

	std::wstring GetText() const override;
	std::optional<int> GetIconIndex() const override;
	bool CanRename() const override;
	bool CanRemove() const override;

	BookmarkItem *GetBookmarkFolder();
	const BookmarkItem *GetBookmarkFolder() const;

private:
	BookmarkItem *const m_bookmarkFolder;
	const BookmarkTree *const m_bookmarkTree;
	const int m_bookmarkFolderIconIndex;
};
