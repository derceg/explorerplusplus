// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkTreeViewAdapter.h"
#include "Bookmarks/BookmarkTree.h"
#include "Bookmarks/UI/BookmarkTreeViewNode.h"
#include <algorithm>
#include <memory>

BookmarkTreeViewAdapter::BookmarkTreeViewAdapter(BookmarkTree *bookmarkTree,
	int bookmarkFolderIconIndex) :
	m_bookmarkTree(bookmarkTree),
	m_bookmarkFolderIconIndex(bookmarkFolderIconIndex)
{
	m_connections.push_back(m_bookmarkTree->bookmarkItemAddedSignal.AddObserver(
		std::bind_front(&BookmarkTreeViewAdapter::OnBookmarkItemAdded, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemUpdatedSignal.AddObserver(
		std::bind_front(&BookmarkTreeViewAdapter::OnBookmarkItemUpdated, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemMovedSignal.AddObserver(
		std::bind_front(&BookmarkTreeViewAdapter::OnBookmarkItemMoved, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemPreRemovalSignal.AddObserver(
		std::bind_front(&BookmarkTreeViewAdapter::OnBookmarkItemPreRemoval, this)));

	AddFolderRecursive(m_bookmarkTree->GetRoot());
}

void BookmarkTreeViewAdapter::AddFolderRecursive(BookmarkItem *bookmarkFolder)
{
	bookmarkFolder->VisitRecursively(
		[this](BookmarkItem *currentItem)
		{
			if (!currentItem->IsFolder())
			{
				return;
			}

			AddFolder(currentItem);
		});
}

void BookmarkTreeViewAdapter::AddFolder(BookmarkItem *bookmarkFolder)
{
	DCHECK(bookmarkFolder->IsFolder());

	if (bookmarkFolder == m_bookmarkTree->GetRoot())
	{
		return;
	}

	auto node = std::make_unique<BookmarkTreeViewNode>(bookmarkFolder, m_bookmarkTree,
		m_bookmarkFolderIconIndex);
	auto *rawNode = node.get();

	AddNode(GetParentNode(bookmarkFolder->GetParent()), std::move(node));

	auto [itr, didInsert] = m_bookmarkToNodeMap.insert({ bookmarkFolder, rawNode });
	CHECK(didInsert);
}

void BookmarkTreeViewAdapter::OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index)
{
	UNREFERENCED_PARAMETER(index);

	if (!bookmarkItem.IsFolder())
	{
		return;
	}

	AddFolderRecursive(&bookmarkItem);
}

void BookmarkTreeViewAdapter::OnBookmarkItemUpdated(BookmarkItem &bookmarkItem,
	BookmarkItem::PropertyType propertyType)
{
	if (!bookmarkItem.IsFolder())
	{
		return;
	}

	if (propertyType == BookmarkItem::PropertyType::Name)
	{
		NotifyNodeUpdated(GetNodeForBookmark(&bookmarkItem), TreeViewNode::Property::Text);
	}
}

void BookmarkTreeViewAdapter::OnBookmarkItemMoved(BookmarkItem *bookmarkItem,
	const BookmarkItem *oldParent, size_t oldIndex, const BookmarkItem *newParent, size_t newIndex)
{
	UNREFERENCED_PARAMETER(oldParent);
	UNREFERENCED_PARAMETER(oldIndex);
	UNREFERENCED_PARAMETER(newParent);
	UNREFERENCED_PARAMETER(newIndex);

	if (!bookmarkItem->IsFolder())
	{
		return;
	}

	MoveNode(GetNodeForBookmark(bookmarkItem), GetParentNode(newParent));
}

void BookmarkTreeViewAdapter::OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem)
{
	if (!bookmarkItem.IsFolder())
	{
		return;
	}

	auto *node = GetNodeForBookmark(&bookmarkItem);

	bookmarkItem.VisitRecursively(
		[this](BookmarkItem *currentItem)
		{
			if (!currentItem->IsFolder())
			{
				return;
			}

			auto numErased = m_bookmarkToNodeMap.erase(currentItem);
			CHECK_EQ(numErased, 1u);
		});

	RemoveNode(node);
}

std::weak_ordering BookmarkTreeViewAdapter::CompareItems(const TreeViewNode *first,
	const TreeViewNode *second) const
{
	auto *firstBookmark = GetBookmarkForNode(first);
	auto *secondBookmark = GetBookmarkForNode(second);

	size_t firstIndex = firstBookmark->GetParent()->GetChildIndex(firstBookmark);
	size_t secondIndex = secondBookmark->GetParent()->GetChildIndex(secondBookmark);

	return firstIndex <=> secondIndex;
}

TreeViewNode *BookmarkTreeViewAdapter::GetParentNode(const BookmarkItem *bookmarkItem)
{
	if (bookmarkItem == m_bookmarkTree->GetRoot())
	{
		return GetRoot();
	}

	return GetNodeForBookmark(bookmarkItem);
}

BookmarkItem *BookmarkTreeViewAdapter::GetBookmarkForNode(TreeViewNode *node)
{
	return const_cast<BookmarkItem *>(std::as_const(*this).GetBookmarkForNode(node));
}

const BookmarkItem *BookmarkTreeViewAdapter::GetBookmarkForNode(const TreeViewNode *node) const
{
	CHECK(!IsRoot(node));
	return static_cast<const BookmarkTreeViewNode *>(node)->GetBookmarkFolder();
}

BookmarkTreeViewNode *BookmarkTreeViewAdapter::GetNodeForBookmark(
	const BookmarkItem *bookmarkFolder)
{
	return const_cast<BookmarkTreeViewNode *>(
		std::as_const(*this).GetNodeForBookmark(bookmarkFolder));
}

const BookmarkTreeViewNode *BookmarkTreeViewAdapter::GetNodeForBookmark(
	const BookmarkItem *bookmarkFolder) const
{
	auto itr = m_bookmarkToNodeMap.find(bookmarkFolder);
	CHECK(itr != m_bookmarkToNodeMap.end());
	return itr->second;
}
