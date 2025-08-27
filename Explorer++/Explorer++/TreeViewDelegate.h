// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

class TreeViewNode;

// // Allows the TreeView controller to be notified of events that occur within the view.
class TreeViewDelegate
{
public:
	virtual ~TreeViewDelegate() = default;

	virtual bool OnNodeRenamed(TreeViewNode *targetNode, const std::wstring &name) = 0;
	virtual void OnNodeRemoved(TreeViewNode *targetNode) = 0;
	virtual void OnNodeCopied(TreeViewNode *targetNode) = 0;
	virtual void OnNodeCut(TreeViewNode *targetNode) = 0;
	virtual void OnPaste(TreeViewNode *targetNode) = 0;
	virtual void OnSelectionChanged(TreeViewNode *selectedNode) = 0;
	virtual void OnShowContextMenu(TreeViewNode *targetNode, const POINT &ptScreen) = 0;
	virtual void OnBeginDrag(TreeViewNode *targetNode) = 0;
};
