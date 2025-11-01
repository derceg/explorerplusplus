// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TreeViewNode.h"
#include "../Helper/SignalWrapper.h"
#include <compare>
#include <memory>
#include <unordered_map>

// Represents a set of nodes displayed in a TreeView. The nodes are stored hierarchically, in the
// same way they're displayed in the view. This is designed to act as a bride between an existing
// model and a TreeView.
class TreeViewAdapter
{
public:
	TreeViewAdapter();
	virtual ~TreeViewAdapter() = default;

	TreeViewNode *GetRoot();
	const TreeViewNode *GetRoot() const;
	bool IsRoot(const TreeViewNode *node) const;
	TreeViewNode *MaybeGetNodeById(int id);

	// The tree can use this notification to lazily load the children of the specified node, if
	// appropriate.
	virtual void OnNodeExpanding(TreeViewNode *node);

	// The tree can use this notification to remove the children of the specified node, if
	// appropriate.
	virtual void OnNodeCollapsing(TreeViewNode *node);

	// Signals
	SignalWrapper<TreeViewAdapter, void(TreeViewNode *node)> nodeAddedSignal;
	SignalWrapper<TreeViewAdapter, void(TreeViewNode *node)> nodeUpdatedSignal;
	SignalWrapper<TreeViewAdapter,
		void(TreeViewNode *node, const TreeViewNode *oldParent, size_t oldIndex,
			const TreeViewNode *newParent, size_t newIndex)>
		nodeMovedSignal;
	SignalWrapper<TreeViewAdapter, void(TreeViewNode *node)> nodeRemovedSignal;

protected:
	TreeViewNode *AddNode(TreeViewNode *parentNode, std::unique_ptr<TreeViewNode> node);
	void NotifyNodeUpdated(TreeViewNode *node);
	void MoveNode(TreeViewNode *node, TreeViewNode *newParent);
	void RemoveNode(TreeViewNode *node);

	virtual std::weak_ordering CompareItems(const TreeViewNode *first,
		const TreeViewNode *second) const = 0;

private:
	class RootTreeViewNode : public TreeViewNode
	{
	public:
		std::wstring GetText() const override;
		std::optional<std::wstring> MaybeGetEditingText() const override;
		std::optional<int> GetIconIndex() const override;
		bool CanRename() const override;
		bool CanRemove() const override;
		bool IsGhosted() const override;
		bool IsFile() const override;
	};

	size_t GetNodeSortedIndex(const TreeViewNode *node, const TreeViewNode *parentNode) const;
	bool CompareItemsWrapper(const TreeViewNode *first, const TreeViewNode *second) const;
	void MaybeRepositionNode(TreeViewNode *node);

	bool IsInTree(const TreeViewNode *node) const;

	RootTreeViewNode m_rootNode;
	std::unordered_map<int, TreeViewNode *> m_idToNodeMap;
};
