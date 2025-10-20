// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "InsertMarkPosition.h"
#include "ItemStateOp.h"
#include "TreeViewDelegate.h"
#include "TreeViewNode.h"
#include <boost/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class KeyboardState;
class LabelEditHandler;
class TreeViewAdapter;
class WindowSubclass;

using RawTreeViewNodes = std::vector<TreeViewNode *>;
using ConstRawTreeViewNodes = std::vector<const TreeViewNode *>;

class TreeView
{
public:
	using LabelEditHandlerFactory = std::function<LabelEditHandler *(HWND hwnd, bool itemIsFile)>;

	enum class NodeRectType
	{
		Text,
		EntireLine
	};

	enum class HitTestScope
	{
		// Indicates that a node will be returned if the specified point is over the node's icon or
		// text.
		IconOrText,

		// Indicates that a node will be returned if the specified point is anywhere within the
		// node's row.
		Row
	};

	TreeView(HWND hwnd, const KeyboardState *keyboardState,
		LabelEditHandlerFactory labelEditHandlerFactory);
	~TreeView();

	HWND GetHWND() const;
	void SetAdapter(TreeViewAdapter *adapter);
	void SetDelegate(TreeViewDelegate *delegate);

	void AddExtendedStyles(DWORD styles);
	void SetImageList(wil::unique_himagelist imageList);
	TreeViewNode *GetSelectedNode();
	const TreeViewNode *GetSelectedNode() const;
	void SelectNode(const TreeViewNode *node);
	void StartRenamingNode(const TreeViewNode *node);
	void CancelRenaming();
	bool IsNodeExpanded(const TreeViewNode *node) const;
	void ExpandNode(TreeViewNode *node);
	void CollapseNode(TreeViewNode *node);
	RECT GetNodeRect(const TreeViewNode *node, NodeRectType rectType) const;
	TreeViewNode *MaybeGetNodeAtPoint(const POINT &pt, HitTestScope scope);
	TreeViewNode *MaybeGetNextVisibleNode(const POINT &pt);
	bool IsNodeHighlighted(const TreeViewNode *node) const;
	void SetNodeHighlighted(const TreeViewNode *node, bool highlighted);
	void ShowInsertMark(const TreeViewNode *targetNode, InsertMarkPosition position);
	void RemoveInsertMark();

	RawTreeViewNodes GetExpandedNodes();

	ConstRawTreeViewNodes GetAllNodesDepthFirstForTesting() const;
	std::wstring GetNodeTextForTesting(const TreeViewNode *node) const;
	HWND GetEditControlForTesting() const;
	bool IsNodeGhostedForTesting(const TreeViewNode *node) const;
	bool IsExpanderShownForTesting(const TreeViewNode *node) const;

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
		void OnNodeMiddleClicked(TreeViewNode *targetNode, const MouseEvent &event) override;
		bool OnNodeRenamed(TreeViewNode *targetNode, const std::wstring &name) override;
		void OnNodeRemoved(TreeViewNode *targetNode, RemoveMode removeMode) override;
		void OnNodeCopied(TreeViewNode *targetNode) override;
		void OnNodeCut(TreeViewNode *targetNode) override;
		void OnPaste(TreeViewNode *targetNode) override;
		void OnSelectionChanged(TreeViewNode *selectedNode) override;
		void OnShowContextMenu(TreeViewNode *targetNode, const POINT &ptScreen) override;
		void OnBeginDrag(TreeViewNode *targetNode) override;
		void OnBeginRightButtonDrag(TreeViewNode *targetNode) override;
	};

	void AddNodeRecursive(TreeViewNode *node);
	void AddNode(TreeViewNode *node);
	void UpdateNode(TreeViewNode *node, TreeViewNode::Property property);
	void MoveNode(TreeViewNode *node, const TreeViewNode *oldParent, size_t oldIndex,
		const TreeViewNode *newParent, size_t newIndex);
	void RemoveNode(TreeViewNode *node);
	void RemoveAllNodes();

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnCaptureChanged(HWND target);
	void OnMiddleButtonDown(const POINT &pt);
	void OnMiddleButtonUp(const POINT &pt);

	LRESULT ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnShowContextMenu(const POINT &ptScreen);
	void OnGetDispInfo(NMTVDISPINFO *dispInfo);
	void OnNodeExpanding(const NMTREEVIEW *notifyInfo);
	LRESULT OnKeyDown(const NMTVKEYDOWN *keyDown);
	void OnDeletePressed(RemoveMode removeMode);
	bool OnBeginLabelEdit(const NMTVDISPINFO *dispInfo);
	bool OnEndLabelEdit(const NMTVDISPINFO *dispInfo);
	void OnSelectionChanged(const NMTREEVIEW *notifyInfo);
	void OnBeginDrag(const NMTREEVIEW *notifyInfo);
	void OnBeginRightButtonDrag(const NMTREEVIEW *notifyInfo);

	bool IsNodeExpandable(const TreeViewNode *node) const;
	void UpdateNodeState(const TreeViewNode *node, UINT state, ItemStateOp stateOp);

	void GetAllNodesDepthFirstForTesting(HTREEITEM firstSiblingHandle,
		ConstRawTreeViewNodes &nodes) const;

	HWND GetEditControl() const;

	HTREEITEM GetHandleForNode(const TreeViewNode *node) const;
	TreeViewNode *GetNodeForHandle(HTREEITEM handle);
	const TreeViewNode *GetNodeForHandle(HTREEITEM handle) const;

	const HWND m_hwnd;
	TreeViewAdapter *m_adapter = nullptr;
	NoOpDelegate m_noOpDelegate;
	TreeViewDelegate *m_delegate = &m_noOpDelegate;
	const KeyboardState *const m_keyboardState;
	LabelEditHandlerFactory m_labelEditHandlerFactory;
	wil::unique_himagelist m_imageList;
	boost::bimap<boost::bimaps::unordered_set_of<HTREEITEM>,
		boost::bimaps::unordered_set_of<TreeViewNode *, PtrHash, std::equal_to<void>>>
		m_handleToNodeMap;
	std::optional<int> m_middleClickNodeId;
	bool m_blockSelectionChangeEvent = false;
	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
