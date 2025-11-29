// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TreeViewAdapter.h"
#include <boost/signals2.hpp>
#include <unordered_map>
#include <vector>

class BookmarkItem;
class BookmarkTree;
class BookmarkTreeViewNode;

// This class adapts a BookmarkTree, so that its folders can be displayed in a TreeView.
class BookmarkTreeViewAdapter : public TreeViewAdapter
{
public:
	BookmarkTreeViewAdapter(BookmarkTree *bookmarkTree, int bookmarkFolderIconIndex);

	BookmarkItem *GetBookmarkForNode(TreeViewNode *node);
	const BookmarkItem *GetBookmarkForNode(const TreeViewNode *node) const;

	BookmarkTreeViewNode *GetNodeForBookmark(const BookmarkItem *bookmarkFolder);
	const BookmarkTreeViewNode *GetNodeForBookmark(const BookmarkItem *bookmarkFolder) const;

protected:
	std::weak_ordering CompareNodes(const TreeViewNode *first,
		const TreeViewNode *second) const override;

private:
	void AddFolderRecursive(BookmarkItem *bookmarkFolder);
	void AddFolder(BookmarkItem *bookmarkFolder);

	void OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index);
	void OnBookmarkItemMoved(BookmarkItem *bookmarkItem, const BookmarkItem *oldParent,
		size_t oldIndex, const BookmarkItem *newParent, size_t newIndex);
	void OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem);

	TreeViewNode *GetParentNode(const BookmarkItem *bookmarkItem);

	BookmarkTree *const m_bookmarkTree;
	const int m_bookmarkFolderIconIndex;
	std::unordered_map<const BookmarkItem *, BookmarkTreeViewNode *> m_bookmarkToNodeMap;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
