// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "InsertMarkPosition.h"
#include "ItemStateOp.h"
#include "TreeViewDelegate.h"
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <functional>
#include <memory>
#include <vector>

class KeyboardState;
class TreeViewAdapter;
class TreeViewNode;
class WindowSubclass;

using RawTreeViewNodes = std::vector<TreeViewNode *>;
using ConstRawTreeViewNodes = std::vector<const TreeViewNode *>;

class TreeView
{
public:
	TreeView(HWND hwnd, const KeyboardState *keyboardState);
	~TreeView();

	HWND GetHWND() const;
	void SetAdapter(TreeViewAdapter *adapter);
	void SetDelegate(TreeViewDelegate *delegate);

	void SetImageList(wil::unique_himagelist imageList);
	TreeViewNode *GetSelectedNode();
	const TreeViewNode *GetSelectedNode() const;
	void SelectNode(const TreeViewNode *node);
	void StartRenamingNode(const TreeViewNode *node);
	bool IsNodeExpanded(const TreeViewNode *node) const;
	void ExpandNode(const TreeViewNode *node);
	void CollapseNode(const TreeViewNode *node);
	RECT GetNodeRect(const TreeViewNode *node) const;
	TreeViewNode *MaybeGetNodeAtPoint(const POINT &pt);
	TreeViewNode *MaybeGetNextVisibleNode(const POINT &pt);
	bool IsNodeHighlighted(const TreeViewNode *node) const;
	void HighlightNode(const TreeViewNode *node);
	void UnhighlightNode(const TreeViewNode *node);
	void ShowInsertMark(const TreeViewNode *targetNode, InsertMarkPosition position);
	void RemoveInsertMark();

	RawTreeViewNodes GetExpandedNodes();

	ConstRawTreeViewNodes GetAllNodesDepthFirstForTesting() const;

private:
	struct PtrHash
	{
		using is_transparent = void;

		size_t operator()(const void *p) const
		{
			return std::hash<const void *>{}(p);
		}
	};

	class NoOpDelegate : public TreeViewDelegate
	{
	public:
		bool OnNodeRenamed(TreeViewNode *targetNode, const std::wstring &name) override;
		void OnNodeRemoved(TreeViewNode *targetNode) override;
		void OnNodeCopied(TreeViewNode *targetNode) override;
		void OnNodeCut(TreeViewNode *targetNode) override;
		void OnPaste(TreeViewNode *targetNode) override;
		void OnSelectionChanged(TreeViewNode *selectedNode) override;
		void OnShowContextMenu(TreeViewNode *targetNode, const POINT &ptScreen) override;
		void OnBeginDrag(TreeViewNode *targetNode) override;
	};

	void AddNodeRecursive(TreeViewNode *node);
	void AddNode(TreeViewNode *node);
	void UpdateNode(TreeViewNode *node);
	void MoveNode(TreeViewNode *node, const TreeViewNode *oldParent, size_t oldIndex,
		const TreeViewNode *newParent, size_t newIndex);
	void RemoveNode(TreeViewNode *node);
	void RemoveAllNodes();

	LRESULT ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnShowContextMenu(const POINT &ptScreen);
	LRESULT OnKeyDown(const NMTVKEYDOWN *keyDown);
	void OnDeletePressed();
	bool OnBeginLabelEdit(const NMTVDISPINFO *dispInfo);
	bool OnEndLabelEdit(const NMTVDISPINFO *dispInfo);
	LRESULT EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnSelectionChanged(const NMTREEVIEW *notifyInfo);
	void OnBeginDrag(const NMTREEVIEW *notifyInfo);

	void UpdateNodeState(const TreeViewNode *node, UINT state, ItemStateOp stateOp);

	void GetAllNodesDepthFirstForTesting(HTREEITEM firstSiblingHandle,
		ConstRawTreeViewNodes &nodes) const;

	HTREEITEM GetHandleForNode(const TreeViewNode *node) const;
	TreeViewNode *GetNodeForHandle(HTREEITEM handle);
	const TreeViewNode *GetNodeForHandle(HTREEITEM handle) const;

	const HWND m_hwnd;
	TreeViewAdapter *m_adapter = nullptr;
	NoOpDelegate m_noOpDelegate;
	TreeViewDelegate *m_delegate = &m_noOpDelegate;
	const KeyboardState *const m_keyboardState;
	wil::unique_himagelist m_imageList;
	boost::bimap<boost::bimaps::unordered_set_of<HTREEITEM>,
		boost::bimaps::unordered_set_of<TreeViewNode *, PtrHash, std::equal_to<void>>>
		m_handleToNodeMap;
	bool m_blockSelectionChangeEvent = false;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	std::unique_ptr<WindowSubclass> m_editSubclass;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
