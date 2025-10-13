// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkTreeViewNode.h"
#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/BookmarkTree.h"

BookmarkTreeViewNode::BookmarkTreeViewNode(BookmarkItem *bookmarkFolder,
	const BookmarkTree *bookmarkTree, int bookmarkFolderIconIndex) :
	m_bookmarkFolder(bookmarkFolder),
	m_bookmarkTree(bookmarkTree),
	m_bookmarkFolderIconIndex(bookmarkFolderIconIndex)
{
	DCHECK(bookmarkFolder->IsFolder());
}

std::wstring BookmarkTreeViewNode::GetText() const
{
	return m_bookmarkFolder->GetName();
}

std::optional<int> BookmarkTreeViewNode::GetIconIndex() const
{
	return m_bookmarkFolderIconIndex;
}

bool BookmarkTreeViewNode::CanRename() const
{
	return !m_bookmarkTree->IsPermanentNode(m_bookmarkFolder);
}

bool BookmarkTreeViewNode::CanRemove() const
{
	return !m_bookmarkTree->IsPermanentNode(m_bookmarkFolder);
}

bool BookmarkTreeViewNode::IsFile() const
{
	return false;
}

BookmarkItem *BookmarkTreeViewNode::GetBookmarkFolder()
{
	return m_bookmarkFolder;
}

const BookmarkItem *BookmarkTreeViewNode::GetBookmarkFolder() const
{
	return m_bookmarkFolder;
}
