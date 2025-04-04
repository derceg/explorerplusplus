// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellHelper.h"
#include <memory>
#include <vector>

class FileSystemChangeWatcher;
class ShellTreeNode;

using ShellTreeNodes = std::vector<std::unique_ptr<ShellTreeNode>>;

enum class ShellTreeNodeType
{
	// The node is a root node in the tree. Note that this isn't related to whether or not the
	// associated shell item is at the root of the namespace. That is, it's valid to have an
	// arbitrary shell item (or multiple shell items) displayed at the root level.
	Root,

	// Any node under a root node is classed as a child node.
	Child
};

class ShellTreeNode
{
public:
	ShellTreeNode(ShellTreeNodeType type, PCIDLIST_ABSOLUTE pidl, IShellItem2 *shellItem);

	int GetId() const;
	IShellItem2 *GetShellItem() const;
	ShellTreeNodeType GetType() const;
	unique_pidl_absolute GetFullPidl() const;

	void UpdateItemDetails(PCIDLIST_ABSOLUTE simpleUpdatedPidl);

	ULONG GetChangeNotifyId() const;
	void SetChangeNotifyId(ULONG changeNotifyId);
	void ResetChangeNotifyId();

	const FileSystemChangeWatcher *GetFileSystemChangeWatcher() const;
	void SetFileSystemChangeWatcher(
		std::unique_ptr<FileSystemChangeWatcher> fileSystemChangeWatcher);

	ShellTreeNode *GetParent();

	ShellTreeNode *AddChild(std::unique_ptr<ShellTreeNode> node);
	std::unique_ptr<ShellTreeNode> RemoveChild(ShellTreeNode *child);
	void RemoveAllChildren();

	const ShellTreeNodes &GetChildren() const;

private:
	void UpdateShellItem(PCIDLIST_ABSOLUTE simpleUpdatedPidl);
	bool ShouldRecreateShellItem(PCIDLIST_ABSOLUTE simpleUpdatedPidl);

	const int m_id = idCounter++;

	static inline int idCounter = 0;

	ShellTreeNodeType m_type;

	// This is only used if this item is a root item.
	unique_pidl_absolute m_rootPidl;

	// This is only used if this item is a child item.
	unique_pidl_child m_childPidl;

	// The shell item corresponding to this node. Note that the pidl of the shell item may become
	// out of date. When a parent item is renamed, the pidl of a child item, as retrieved by
	// GetFullPidl(), will be correct, since pidls are generated dynamically. The shell item for the
	// parent will be updated, but the shell items for the children will be left as-is, with the
	// shell item for a particular child only being updated if the child is updated.
	// That should be ok, since the shell item caches data. It does, however, mean that calling
	// SHGetIDListFromObject() on the shell item is invalid in general. The returned pidl may refer
	// to an item at its previous path. Only GetFullPidl() should be used to retrieve the pidl of a
	// node.
	wil::com_ptr_nothrow<IShellItem2> m_shellItem;

	// This will be non-zero if the directory associated with this item is being monitored for
	// changes.
	ULONG m_changeNotifyId = 0;

	std::unique_ptr<FileSystemChangeWatcher> m_fileSystemChangeWatcher;

	ShellTreeNode *m_parent = nullptr;
	ShellTreeNodes m_children;
};
