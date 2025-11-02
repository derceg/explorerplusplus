// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TreeViewAdapter.h"
#include <algorithm>

TreeViewAdapter::TreeViewAdapter()
{
	m_idToNodeMap.insert({ m_rootNode.GetId(), &m_rootNode });
}

TreeViewNode *TreeViewAdapter::GetRoot()
{
	return &m_rootNode;
}

const TreeViewNode *TreeViewAdapter::GetRoot() const
{
	return &m_rootNode;
}

bool TreeViewAdapter::IsRoot(const TreeViewNode *node) const
{
	return node == &m_rootNode;
}

TreeViewNode *TreeViewAdapter::MaybeGetNodeById(int id)
{
	auto itr = m_idToNodeMap.find(id);

	if (itr == m_idToNodeMap.end())
	{
		return nullptr;
	}

	return itr->second;
}

TreeViewNode *TreeViewAdapter::AddNode(TreeViewNode *parentNode, std::unique_ptr<TreeViewNode> node)
{
	CHECK(IsInTree(parentNode));

	for (auto *currentNode : node->GetNodesDepthFirst())
	{
		auto [itr, didInsert] = m_idToNodeMap.insert({ currentNode->GetId(), currentNode });
		CHECK(didInsert);

		// The observer here doesn't need to be removed, since this class owns the node.
		std::ignore = currentNode->AddUpdatedObserver(
			std::bind_front(&TreeViewAdapter::OnNodeUpdated, this, currentNode));
	}

	size_t index = GetNodeSortedIndex(node.get(), parentNode);
	auto *newNode = parentNode->AddChild(std::move(node), index);
	nodeAddedSignal.m_signal(newNode);
	return newNode;
}

void TreeViewAdapter::MoveNode(TreeViewNode *node, TreeViewNode *newParent)
{
	CHECK(IsInTree(node));
	CHECK(IsInTree(newParent));

	auto *oldParent = node->GetParent();
	size_t oldIndex = oldParent->GetChildIndex(node);

	auto ownedNode = oldParent->RemoveChild(node);
	size_t index = GetNodeSortedIndex(node, newParent);
	newParent->AddChild(std::move(ownedNode), index);

	if (oldParent == newParent && oldIndex == index)
	{
		return;
	}

	nodeMovedSignal.m_signal(node, oldParent, oldIndex, newParent, index);
}

void TreeViewAdapter::RemoveNode(TreeViewNode *node)
{
	CHECK(IsInTree(node));

	for (auto *currentNode : node->GetNodesDepthFirst())
	{
		auto numErased = m_idToNodeMap.erase(currentNode->GetId());
		CHECK_EQ(numErased, 1u);
	}

	auto *parentNode = node->GetParent();
	CHECK(parentNode);

	auto ownedNode = parentNode->RemoveChild(node);
	nodeRemovedSignal.m_signal(node);
}

void TreeViewAdapter::OnNodeUpdated(TreeViewNode *node)
{
	nodeUpdatedSignal.m_signal(node);

	MaybeRepositionNode(node);
}

size_t TreeViewAdapter::GetNodeSortedIndex(const TreeViewNode *node,
	const TreeViewNode *parentNode) const
{
	DCHECK(!node->GetParent());

	auto itr = std::ranges::upper_bound(parentNode->GetChildren(), node,
		std::bind_front(&TreeViewAdapter::CompareItemsWrapper, this),
		[](const auto &currentNode) { return currentNode.get(); });
	return std::distance(parentNode->GetChildren().begin(), itr);
}

bool TreeViewAdapter::CompareItemsWrapper(const TreeViewNode *first,
	const TreeViewNode *second) const
{
	auto cmp = CompareItems(first, second);
	return std::is_lt(cmp);
}

void TreeViewAdapter::MaybeRepositionNode(TreeViewNode *node)
{
	// MoveNode() determines the sorted position of the node when adding it to its new parent. So,
	// this effectively updates the sorted position of the node, if necessary.
	MoveNode(node, node->GetParent());
}

bool TreeViewAdapter::IsInTree(const TreeViewNode *node) const
{
	return m_idToNodeMap.contains(node->GetId());
}

void TreeViewAdapter::OnNodeExpanding(TreeViewNode *node)
{
	UNREFERENCED_PARAMETER(node);
}

void TreeViewAdapter::OnNodeCollapsing(TreeViewNode *node)
{
	UNREFERENCED_PARAMETER(node);
}

std::wstring TreeViewAdapter::RootTreeViewNode::GetText() const
{
	// The root node should never be displayed, so it's not expected that any of the methods here
	// will be called.
	DCHECK(false);
	return L"";
}

std::optional<std::wstring> TreeViewAdapter::RootTreeViewNode::MaybeGetEditingText() const
{
	DCHECK(false);
	return std::nullopt;
}

std::optional<int> TreeViewAdapter::RootTreeViewNode::GetIconIndex() const
{
	DCHECK(false);
	return std::nullopt;
}

bool TreeViewAdapter::RootTreeViewNode::CanRename() const
{
	DCHECK(false);
	return false;
}

bool TreeViewAdapter::RootTreeViewNode::CanRemove() const
{
	DCHECK(false);
	return false;
}

bool TreeViewAdapter::RootTreeViewNode::IsGhosted() const
{
	DCHECK(false);
	return false;
}

bool TreeViewAdapter::RootTreeViewNode::IsFile() const
{
	DCHECK(false);
	return false;
}
