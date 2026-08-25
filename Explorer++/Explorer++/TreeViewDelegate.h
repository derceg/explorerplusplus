// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/RemoveMode.h"
#include <string>

struct MouseEvent;
class TreeViewNode;

// // Allows the TreeView controller to be notified of events that occur within the view.
class TreeViewDelegate
{
public:
	virtual ~TreeViewDelegate() = default;

	virtual void OnNodeMiddleClicked(TreeViewNode *targetNode, const MouseEvent &event)
	{
		UNREFERENCED_PARAMETER(targetNode);
		UNREFERENCED_PARAMETER(event);
	}

	virtual bool OnNodeRenamed(TreeViewNode *targetNode, const std::wstring &name)
	{
		UNREFERENCED_PARAMETER(targetNode);
		UNREFERENCED_PARAMETER(name);

		return false;
	}

	virtual void OnNodeRemoved(TreeViewNode *targetNode, RemoveMode removeMode)
	{
		UNREFERENCED_PARAMETER(targetNode);
		UNREFERENCED_PARAMETER(removeMode);
	}

	virtual void OnNodeCopied(TreeViewNode *targetNode)
	{
		UNREFERENCED_PARAMETER(targetNode);
	}

	virtual void OnNodeCut(TreeViewNode *targetNode)
	{
		UNREFERENCED_PARAMETER(targetNode);
	}

	virtual void OnPaste(TreeViewNode *targetNode)
	{
		UNREFERENCED_PARAMETER(targetNode);
	}

	virtual void OnSelectionChanged(TreeViewNode *selectedNode)
	{
		UNREFERENCED_PARAMETER(selectedNode);
	}

	virtual void OnShowContextMenu(TreeViewNode *targetNode, const POINT &ptScreen)
	{
		UNREFERENCED_PARAMETER(targetNode);
		UNREFERENCED_PARAMETER(ptScreen);
	}

	virtual void OnBeginDrag(TreeViewNode *targetNode)
	{
		UNREFERENCED_PARAMETER(targetNode);
	}

	virtual void OnBeginRightButtonDrag(TreeViewNode *targetNode)
	{
		UNREFERENCED_PARAMETER(targetNode);
	}
};
