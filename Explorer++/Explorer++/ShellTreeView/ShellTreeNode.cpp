// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellTreeNode.h"

ShellTreeNode::ShellTreeNode(unique_pidl_absolute pidl) : m_rootPidl(std::move(pidl))
{
}

ShellTreeNode::ShellTreeNode(unique_pidl_child childPidl) : m_childPidl(std::move(childPidl))
{
}

int ShellTreeNode::GetId() const
{
	return m_id;
}

unique_pidl_absolute ShellTreeNode::GetFullPidl() const
{
	std::vector<PCITEMID_CHILD> childPidls;
	const ShellTreeNode *currentItem = this;

	while (currentItem->m_parent != nullptr)
	{
		childPidls.push_back(currentItem->m_childPidl.get());

		currentItem = currentItem->m_parent;
	}

	assert(currentItem->m_rootPidl);
	unique_pidl_absolute fullPidl(ILCloneFull(currentItem->m_rootPidl.get()));

	for (PCITEMID_CHILD childPidl : childPidls | std::views::reverse)
	{
		fullPidl.reset(ILCombine(fullPidl.get(), childPidl));
	}

	return fullPidl;
}

void ShellTreeNode::UpdateChildPidl(unique_pidl_child childPidl)
{
	assert(m_parent != nullptr);

	m_childPidl = std::move(childPidl);
}

ULONG ShellTreeNode::GetChangeNotifyId() const
{
	return m_changeNotifyId;
}

void ShellTreeNode::SetChangeNotifyId(ULONG changeNotifyId)
{
	// The directory for an item should only be monitored once.
	assert(m_changeNotifyId == 0);

	m_changeNotifyId = changeNotifyId;
}

void ShellTreeNode::ResetChangeNotifyId()
{
	m_changeNotifyId = 0;
}

ShellTreeNode *ShellTreeNode::GetParent()
{
	return m_parent;
}

ShellTreeNode *ShellTreeNode::AddChild(std::unique_ptr<ShellTreeNode> node)
{
	node->m_parent = this;

	ShellTreeNode *rawNode = node.get();
	m_children.push_back(std::move(node));

	return rawNode;
}

std::unique_ptr<ShellTreeNode> ShellTreeNode::RemoveChild(ShellTreeNode *child)
{
	auto itr = std::find_if(m_children.begin(), m_children.end(),
		[child](auto &currentChild) { return currentChild.get() == child; });

	if (itr == m_children.end())
	{
		assert(false);
		return nullptr;
	}

	child->m_parent = nullptr;

	auto erasedNode = std::move(*itr);

	m_children.erase(itr);

	return erasedNode;
}

void ShellTreeNode::RemoveAllChildren()
{
	m_children.clear();
}

const ShellTreeNodes &ShellTreeNode::GetChildren() const
{
	return m_children;
}
