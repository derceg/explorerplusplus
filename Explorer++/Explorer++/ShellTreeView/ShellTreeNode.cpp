// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellTreeNode.h"

ShellTreeNode::ShellTreeNode(PCIDLIST_ABSOLUTE pidl, IShellItem2 *shellItem,
	ShellTreeNodeType type) :
	m_shellItem(shellItem),
	m_rootPidl(type == ShellTreeNodeType::Root ? ILCloneFull(pidl) : nullptr),
	m_childPidl(type == ShellTreeNodeType::Child ? ILCloneChild(ILFindLastID(pidl)) : nullptr),
	m_type(type)
{
}

int ShellTreeNode::GetId() const
{
	return m_id;
}

IShellItem2 *ShellTreeNode::GetShellItem() const
{
	return m_shellItem.get();
}

ShellTreeNodeType ShellTreeNode::GetType() const
{
	return m_type;
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

void ShellTreeNode::UpdateItemDetails(PCIDLIST_ABSOLUTE updatedPidl)
{
	if (m_type == ShellTreeNodeType::Root)
	{
		m_rootPidl.reset(ILCloneFull(updatedPidl));
	}
	else
	{
		m_childPidl.reset(ILCloneChild(ILFindLastID(updatedPidl)));
	}

	UpdateShellItem();
}

void ShellTreeNode::UpdateShellItem()
{
	if (ShouldRecreateShellItem())
	{
		wil::com_ptr_nothrow<IShellItem2> updatedShellItem;
		HRESULT hr = SHCreateItemFromIDList(GetFullPidl().get(), IID_PPV_ARGS(&updatedShellItem));

		if (FAILED(hr))
		{
			// The call above might fail if the item no longer exists. In that case, the item should
			// be removed soon and keeping the stale shell item isn't much of an issue.
			return;
		}

		m_shellItem = updatedShellItem;
	}

	m_shellItem->Update(nullptr);
}

// The pidl of the shell item might not match the current pidl if the item was renamed, or one of
// its parents was renamed. In that case, the shell item should be recreated.
bool ShellTreeNode::ShouldRecreateShellItem()
{
	unique_pidl_absolute shellItemPidl;
	HRESULT hr = SHGetIDListFromObject(m_shellItem.get(), wil::out_param(shellItemPidl));

	if (FAILED(hr))
	{
		return true;
	}

	if (!ArePidlsEquivalent(shellItemPidl.get(), GetFullPidl().get()))
	{
		return true;
	}

	return false;
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
