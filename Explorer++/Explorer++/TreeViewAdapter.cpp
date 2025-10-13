// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TreeViewAdapter.h"

TreeViewAdapter::TreeViewAdapter()
{
	m_nodes.insert(&m_rootNode);
}

TreeViewNode *TreeViewAdapter::GetRoot()
{
	return &m_rootNode;
}

const TreeViewNode *TreeViewAdapter::GetRoot() const
{
	return &m_rootNode;
}

TreeViewNode *TreeViewAdapter::AddNode(TreeViewNode *parentNode, std::unique_ptr<TreeViewNode> node)
{
	return AddNode(parentNode, std::move(node), parentNode->GetChildren().size());
}

TreeViewNode *TreeViewAdapter::AddNode(TreeViewNode *parentNode, std::unique_ptr<TreeViewNode> node,
	size_t index)
{
	CHECK(IsInTree(parentNode));

	node->VisitRecursively(
		[this](TreeViewNode *currentNode)
		{
			auto [itr, didInsert] = m_nodes.insert(currentNode);
			CHECK(didInsert);
		});

	if (index > parentNode->GetChildren().size())
	{
		index = parentNode->GetChildren().size();
	}

	auto *newNode = parentNode->AddChild(std::move(node), index);
	nodeAddedSignal.m_signal(newNode);
	return newNode;
}

void TreeViewAdapter::NotifyNodeUpdated(TreeViewNode *node)
{
	CHECK(IsInTree(node));

	nodeUpdatedSignal.m_signal(node);
}

void TreeViewAdapter::MoveNode(TreeViewNode *node, TreeViewNode *newParent, size_t index)
{
	CHECK(IsInTree(node));
	CHECK(IsInTree(newParent));

	auto *oldParent = node->GetParent();
	size_t oldIndex = oldParent->GetChildIndex(node);

	if (index > newParent->GetChildren().size())
	{
		index = newParent->GetChildren().size();
	}

	if (oldParent == newParent && index > oldIndex)
	{
		index--;
	}

	if (oldParent == newParent && index == oldIndex)
	{
		return;
	}

	auto ownedNode = oldParent->RemoveChild(node);
	newParent->AddChild(std::move(ownedNode), index);

	nodeMovedSignal.m_signal(node, oldParent, oldIndex, newParent, index);
}

void TreeViewAdapter::RemoveNode(TreeViewNode *node)
{
	CHECK(IsInTree(node));

	node->VisitRecursively(
		[this](TreeViewNode *currentNode)
		{
			auto numErased = m_nodes.erase(currentNode);
			CHECK_EQ(numErased, 1u);
		});

	auto *parentNode = node->GetParent();
	CHECK(parentNode);

	auto ownedNode = parentNode->RemoveChild(node);
	nodeRemovedSignal.m_signal(node);
}

bool TreeViewAdapter::IsInTree(const TreeViewNode *node) const
{
	return m_nodes.contains(node);
}

std::wstring TreeViewAdapter::RootTreeViewNode::GetText() const
{
	// The root node should never be displayed, so it's not expected that any of the methods here
	// will be called.
	DCHECK(false);
	return L"";
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

bool TreeViewAdapter::RootTreeViewNode::IsFile() const
{
	DCHECK(false);
	return false;
}
