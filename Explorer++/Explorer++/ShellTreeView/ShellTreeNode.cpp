// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellTreeNode.h"
#include "DirectoryWatcher.h"

ShellTreeNode::ShellTreeNode(ShellTreeNodeType type, PCIDLIST_ABSOLUTE pidl,
	IShellItem2 *shellItem) :
	m_type(type),
	m_rootPidl(type == ShellTreeNodeType::Root ? ILCloneFull(pidl) : nullptr),
	m_childPidl(type == ShellTreeNodeType::Child ? ILCloneChild(ILFindLastID(pidl)) : nullptr),
	m_shellItem(shellItem)
{
}

ShellTreeNode::~ShellTreeNode() = default;

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

void ShellTreeNode::UpdateItemDetails(PCIDLIST_ABSOLUTE simpleUpdatedPidl)
{
	UpdateShellItem(simpleUpdatedPidl);

	PCIDLIST_ABSOLUTE finalPidl;

	// The simple pidl won't contain proper item information, so it's important to use the full pidl
	// instead.
	unique_pidl_absolute updatedPidl;
	HRESULT hr = SHGetIDListFromObject(m_shellItem.get(), wil::out_param(updatedPidl));

	if (SUCCEEDED(hr))
	{
		finalPidl = updatedPidl.get();
	}
	else
	{
		assert(false);

		finalPidl = simpleUpdatedPidl;
	}

	if (m_type == ShellTreeNodeType::Root)
	{
		m_rootPidl.reset(ILCloneFull(finalPidl));
	}
	else
	{
		m_childPidl.reset(ILCloneChild(ILFindLastID(finalPidl)));
	}
}

void ShellTreeNode::UpdateShellItem(PCIDLIST_ABSOLUTE simpleUpdatedPidl)
{
	if (ShouldRecreateShellItem(simpleUpdatedPidl))
	{
		wil::com_ptr_nothrow<IShellItem2> updatedShellItem;
		HRESULT hr = SHCreateItemFromIDList(simpleUpdatedPidl, IID_PPV_ARGS(&updatedShellItem));

		if (FAILED(hr))
		{
			// The call above might fail if the item no longer exists. In that case, the item should
			// be removed soon and keeping the stale shell item isn't much of an issue.
			return;
		}

		m_shellItem = updatedShellItem;
	}

	// This always needs to be done, since the shell item will be out of date if this is an update
	// and won't contain actual item information if this was a rename and the shell item was
	// recreated (since the simple pidl won't contain any item information).
	m_shellItem->Update(nullptr);
}

// The pidl of the shell item might not match the updated pidl if the item was renamed, or one of
// its parents was renamed. In that case, the shell item should be recreated.
bool ShellTreeNode::ShouldRecreateShellItem(PCIDLIST_ABSOLUTE simpleUpdatedPidl)
{
	unique_pidl_absolute shellItemPidl;
	HRESULT hr = SHGetIDListFromObject(m_shellItem.get(), wil::out_param(shellItemPidl));

	if (FAILED(hr))
	{
		assert(false);
		return true;
	}

	if (!ArePidlsEquivalent(shellItemPidl.get(), simpleUpdatedPidl))
	{
		return true;
	}

	return false;
}

const DirectoryWatcher *ShellTreeNode::GetDirectoryWatcher() const
{
	return m_directoryWatcher.get();
}

void ShellTreeNode::SetDirectoryWatcher(std::unique_ptr<DirectoryWatcher> directoryWatcher)
{
	m_directoryWatcher = std::move(directoryWatcher);
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
