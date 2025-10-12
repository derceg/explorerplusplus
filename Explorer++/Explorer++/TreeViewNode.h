// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class TreeViewNode;

using TreeViewNodes = std::vector<std::unique_ptr<TreeViewNode>>;

// Acts as a base class for a single node in a TreeView.
class TreeViewNode : private boost::noncopyable
{
public:
	TreeViewNode();
	virtual ~TreeViewNode() = default;

	int GetId() const;

	virtual std::wstring GetText() const = 0;
	virtual std::optional<int> GetIconIndex() const = 0;
	virtual bool CanRename() const = 0;
	virtual bool CanRemove() const = 0;

	TreeViewNode *GetParent();
	const TreeViewNode *GetParent() const;

	TreeViewNode *AddChild(std::unique_ptr<TreeViewNode> node, size_t index);
	std::unique_ptr<TreeViewNode> RemoveChild(TreeViewNode *node);

	TreeViewNode *GetChildAtIndex(size_t index);
	const TreeViewNode *GetChildAtIndex(size_t index) const;
	size_t GetChildIndex(const TreeViewNode *node) const;
	const TreeViewNodes &GetChildren() const;

	void VisitRecursively(std::function<void(TreeViewNode *currentNode)> callback);

private:
	static inline int m_idCounter = 1;
	const int m_id;

	TreeViewNode *m_parent = nullptr;
	TreeViewNodes m_children;
};
