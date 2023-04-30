// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellHelper.h"
#include <memory>
#include <vector>

class ShellTreeNode;

using ShellTreeNodes = std::vector<std::unique_ptr<ShellTreeNode>>;

// Represents an node in the treeview. There are two types of nodes:
//
// 1. Root nodes. These nodes appear at the top level of the treeview and have an absolute path.
// 2. Child nodes. These appear at every other level. Each child only stores its relative path.
//
// Full item paths are then built dynamically.
class ShellTreeNode
{
public:
	ShellTreeNode(unique_pidl_child childPidl);
	ShellTreeNode(unique_pidl_absolute pidl);

	int GetId() const;
	unique_pidl_absolute GetFullPidl() const;

	void UpdateChildPidl(unique_pidl_child childPidl);

	ULONG GetChangeNotifyId() const;
	void SetChangeNotifyId(ULONG changeNotifyId);
	void ResetChangeNotifyId();

	ShellTreeNode *GetParent();

	ShellTreeNode *AddChild(std::unique_ptr<ShellTreeNode> node);
	std::unique_ptr<ShellTreeNode> RemoveChild(ShellTreeNode *child);
	void RemoveAllChildren();

	const ShellTreeNodes &GetChildren() const;

private:
	const int m_id = idCounter++;

	static inline int idCounter = 0;

	// This is only used if this item is a root item.
	unique_pidl_absolute m_rootPidl;

	// This is only used if this item is a child item.
	unique_pidl_child m_childPidl;

	// This will be non-zero if the directory associated with this item is being monitored for
	// changes.
	ULONG m_changeNotifyId = 0;

	ShellTreeNode *m_parent = nullptr;
	ShellTreeNodes m_children;
};
