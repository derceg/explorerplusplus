// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TreeViewNode.h"
#include <algorithm>

TreeViewNode::TreeViewNode() : m_id(m_idCounter++)
{
}

int TreeViewNode::GetId() const
{
	return m_id;
}

bool TreeViewNode::GetMayLazyLoadChildren() const
{
	// By default, it's assumed that nodes are loaded eagerly.
	return false;
}

const TreeViewNode *TreeViewNode::GetParent() const
{
	return m_parent;
}

TreeViewNode *TreeViewNode::GetParent()
{
	return m_parent;
}

TreeViewNode *TreeViewNode::AddChild(std::unique_ptr<TreeViewNode> node, size_t index)
{
	CHECK(index <= m_children.size());

	node->m_parent = this;

	auto *rawNode = node.get();
	m_children.insert(m_children.begin() + index, std::move(node));

	return rawNode;
}

std::unique_ptr<TreeViewNode> TreeViewNode::RemoveChild(TreeViewNode *node)
{
	auto itr = std::ranges::find_if(m_children,
		[node](const auto &currentNode) { return currentNode.get() == node; });
	CHECK(itr != m_children.end());

	auto erasedNode = std::move(*itr);
	erasedNode->m_parent = nullptr;

	m_children.erase(itr);

	return erasedNode;
}

TreeViewNode *TreeViewNode::GetChildAtIndex(size_t index)
{
	return const_cast<TreeViewNode *>(std::as_const(*this).GetChildAtIndex(index));
}

const TreeViewNode *TreeViewNode::GetChildAtIndex(size_t index) const
{
	CHECK(index < m_children.size());
	return m_children[index].get();
}

size_t TreeViewNode::GetChildIndex(const TreeViewNode *node) const
{
	auto itr = std::ranges::find_if(m_children,
		[node](const auto &currentNode) { return currentNode.get() == node; });
	CHECK(itr != m_children.end());
	return itr - m_children.begin();
}

const TreeViewNodes &TreeViewNode::GetChildren() const
{
	return m_children;
}

concurrencpp::generator<TreeViewNode *> TreeViewNode::GetNodesDepthFirst()
{
	co_yield this;

	for (auto &child : m_children)
	{
		// TODO: This can use std::ranges::elements_of() once C++23 support is available.
		for (auto *node : child->GetNodesDepthFirst())
		{
			co_yield node;
		}
	}
}

boost::signals2::connection TreeViewNode::AddUpdatedObserver(
	const typename UpdatedSignal::slot_type &observer)
{
	return m_updatedSignal.connect(observer);
}
