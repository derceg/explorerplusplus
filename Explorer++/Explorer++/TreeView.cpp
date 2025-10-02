// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TreeView.h"
#include "TestHelper.h"
#include "TreeViewAdapter.h"
#include "../Helper/Helper.h"
#include "../Helper/WindowSubclass.h"
#include <wil/common.h>

TreeView::TreeView(HWND hwnd) : m_hwnd(hwnd)
{
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(GetParent(m_hwnd),
		std::bind_front(&TreeView::ParentWndProc, this)));
}

TreeView::~TreeView() = default;

HWND TreeView::GetHWND() const
{
	return m_hwnd;
}

void TreeView::SetAdapter(TreeViewAdapter *adapter)
{
	m_adapter = adapter;

	m_connections.clear();
	m_connections.push_back(
		m_adapter->nodeAddedSignal.AddObserver(std::bind_front(&TreeView::AddNode, this)));
	m_connections.push_back(
		m_adapter->nodeUpdatedSignal.AddObserver(std::bind_front(&TreeView::UpdateNode, this)));
	m_connections.push_back(
		m_adapter->nodeMovedSignal.AddObserver(std::bind_front(&TreeView::MoveNode, this)));
	m_connections.push_back(
		m_adapter->nodeRemovedSignal.AddObserver(std::bind_front(&TreeView::RemoveNode, this)));

	RemoveAllNodes();
	AddNodeRecursive(m_adapter->GetRoot());
}

void TreeView::SetDelegate(TreeViewDelegate *delegate)
{
	m_delegate = delegate ? delegate : &m_noOpDelegate;
}

void TreeView::AddNodeRecursive(TreeViewNode *node)
{
	node->VisitRecursively([this](TreeViewNode *currentNode) { AddNode(currentNode); });
}

void TreeView::AddNode(TreeViewNode *node)
{
	if (node == m_adapter->GetRoot())
	{
		return;
	}

	std::wstring text = node->GetText();

	TVITEMEX tvItem = {};
	tvItem.mask = TVIF_TEXT | TVIF_CHILDREN;
	tvItem.pszText = text.data();
	tvItem.cChildren = node->GetChildren().empty() ? 0 : 1;

	auto iconIndex = node->GetIconIndex();

	if (iconIndex)
	{
		WI_SetAllFlags(tvItem.mask, TVIF_IMAGE | TVIF_SELECTEDIMAGE);
		tvItem.iImage = *iconIndex;
		tvItem.iSelectedImage = *iconIndex;
	}

	auto *parentNode = node->GetParent();

	HTREEITEM parentHandle =
		(parentNode == m_adapter->GetRoot()) ? nullptr : GetHandleForNode(parentNode);
	size_t position = parentNode->GetChildIndex(node);
	HTREEITEM insertAfterHandle;

	if (position == 0)
	{
		insertAfterHandle = TVI_FIRST;
	}
	else
	{
		auto *previousNode = parentNode->GetChildAtIndex(position - 1);
		insertAfterHandle = GetHandleForNode(previousNode);
	}

	TVINSERTSTRUCT tvInsertData = {};
	tvInsertData.hParent = parentHandle;
	tvInsertData.hInsertAfter = insertAfterHandle;
	tvInsertData.itemex = tvItem;
	auto handle = TreeView_InsertItem(m_hwnd, &tvInsertData);
	CHECK(handle);

	auto [itr, didInsert] = m_handleToNodeMap.insert({ handle, node });
	CHECK(didInsert);

	if (parentHandle)
	{
		TVITEM tvParentItem = {};
		tvParentItem.mask = TVIF_CHILDREN;
		tvParentItem.hItem = parentHandle;
		tvParentItem.cChildren = 1;
		auto res = TreeView_SetItem(m_hwnd, &tvParentItem);
		CHECK(res);
	}
}

void TreeView::UpdateNode(TreeViewNode *node)
{
	std::wstring text = node->GetText();

	TVITEMEX tvItem = {};
	tvItem.mask = TVIF_TEXT | TVIF_CHILDREN;
	tvItem.hItem = GetHandleForNode(node);
	tvItem.pszText = text.data();
	tvItem.cChildren = node->GetChildren().empty() ? 0 : 1;

	auto iconIndex = node->GetIconIndex();

	if (iconIndex)
	{
		WI_SetAllFlags(tvItem.mask, TVIF_IMAGE | TVIF_SELECTEDIMAGE);
		tvItem.iImage = *iconIndex;
		tvItem.iSelectedImage = *iconIndex;
	}

	auto res = TreeView_SetItem(m_hwnd, &tvItem);
	CHECK(res);
}

void TreeView::MoveNode(TreeViewNode *node, const TreeViewNode *oldParent, size_t oldIndex,
	const TreeViewNode *newParent, size_t newIndex)
{
	UNREFERENCED_PARAMETER(oldParent);
	UNREFERENCED_PARAMETER(oldIndex);
	UNREFERENCED_PARAMETER(newParent);
	UNREFERENCED_PARAMETER(newIndex);

	RemoveNode(node);
	AddNodeRecursive(node);
}

void TreeView::RemoveNode(TreeViewNode *node)
{
	// Note that node->GetParent() shouldn't be used here, since if the node was moved, the parent
	// node may not match the parent in the view, and if the node was removed, the node won't have a
	// parent node.
	auto handle = GetHandleForNode(node);
	auto parentHandle = TreeView_GetParent(m_hwnd, handle);
	bool shouldCollapseParent = parentHandle && TreeView_GetPrevSibling(m_hwnd, handle) == nullptr
		&& TreeView_GetNextSibling(m_hwnd, handle) == nullptr;

	if (shouldCollapseParent)
	{
		// The node to be removed is the only node under the parent node. The parent node needs to
		// be collapsed before the child node is removed. Attempting to collapse the parent node
		// after its only child has been removed will fail. Not collapsing the node will result in
		// the unusual situation where the node will immediately re-expand if a child is added
		// again.
		CollapseNode(GetNodeForHandle(parentHandle));
	}

	auto res = TreeView_DeleteItem(m_hwnd, handle);
	CHECK(res);

	node->VisitRecursively(
		[this](TreeViewNode *currentNode)
		{
			auto numErased = m_handleToNodeMap.right.erase(currentNode);
			CHECK_EQ(numErased, 1u);
		});

	if (shouldCollapseParent)
	{
		TVITEM tvItem = {};
		tvItem.mask = TVIF_CHILDREN;
		tvItem.hItem = parentHandle;
		tvItem.cChildren = 0;
		res = TreeView_SetItem(m_hwnd, &tvItem);
		CHECK(res);
	}
}

void TreeView::RemoveAllNodes()
{
	auto res = TreeView_DeleteAllItems(m_hwnd);
	CHECK(res);
}

LRESULT TreeView::ParentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CONTEXTMENU:
		if (reinterpret_cast<HWND>(wParam) == m_hwnd)
		{
			OnShowContextMenu({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
			return 0;
		}
		break;

	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hwnd)
		{
			switch (reinterpret_cast<NMHDR *>(lParam)->code)
			{
			case TVN_KEYDOWN:
				return OnKeyDown(reinterpret_cast<NMTVKEYDOWN *>(lParam));

			case TVN_BEGINLABELEDIT:
				return OnBeginLabelEdit(reinterpret_cast<NMTVDISPINFO *>(lParam));

			case TVN_ENDLABELEDIT:
				return OnEndLabelEdit(reinterpret_cast<NMTVDISPINFO *>(lParam));

			case TVN_SELCHANGED:
				OnSelectionChanged(reinterpret_cast<NMTREEVIEW *>(lParam));
				break;

			case TVN_BEGINDRAG:
				OnBeginDrag(reinterpret_cast<NMTREEVIEW *>(lParam));
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void TreeView::OnShowContextMenu(const POINT &ptScreen)
{
	TreeViewNode *targetNode = nullptr;
	POINT ptScreenFinal;

	if (ptScreen.x == -1 && ptScreen.y == -1)
	{
		auto *selectedNode = GetSelectedNode();
		auto nodeRect = GetNodeRect(selectedNode);

		ptScreenFinal = { nodeRect.left, nodeRect.top + (nodeRect.bottom - nodeRect.top) / 2 };
		ClientToScreen(m_hwnd, &ptScreenFinal);

		targetNode = selectedNode;
	}
	else
	{
		ptScreenFinal = ptScreen;

		POINT ptClient = ptScreen;
		ScreenToClient(m_hwnd, &ptClient);

		targetNode = MaybeGetNodeAtPoint(ptClient);

		if (!targetNode)
		{
			return;
		}
	}

	m_delegate->OnShowContextMenu(targetNode, ptScreenFinal);
}

LRESULT TreeView::OnKeyDown(const NMTVKEYDOWN *keyDown)
{
	switch (keyDown->wVKey)
	{
	case VK_F2:
		StartRenamingNode(GetSelectedNode());
		break;

	case 'C':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			m_delegate->OnNodeCopied(GetSelectedNode());
		}
		break;

	case 'X':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			m_delegate->OnNodeCut(GetSelectedNode());
		}
		break;

	case 'V':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			m_delegate->OnPaste(GetSelectedNode());
		}
		break;

	case VK_INSERT:
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			m_delegate->OnNodeCopied(GetSelectedNode());
		}
		if (!IsKeyDown(VK_CONTROL) && IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			m_delegate->OnPaste(GetSelectedNode());
		}
		break;

	case VK_DELETE:
		OnDeletePressed();
		break;
	}

	// If the ctrl key is down, this key sequence is likely a modifier. Stop any other pressed key
	// from been used in an incremental search.
	if (IsKeyDown(VK_CONTROL))
	{
		return 1;
	}

	return 0;
}

void TreeView::OnDeletePressed()
{
	auto *node = GetSelectedNode();

	if (!node->CanRemove())
	{
		return;
	}

	m_delegate->OnNodeRemoved(node);
}

bool TreeView::OnBeginLabelEdit(const NMTVDISPINFO *dispInfo)
{
	auto *node = GetNodeForHandle(dispInfo->item.hItem);

	if (!node->CanRename())
	{
		return true;
	}

	HWND editControl = TreeView_GetEditControl(m_hwnd);
	CHECK(editControl);
	m_editSubclass = std::make_unique<WindowSubclass>(editControl,
		std::bind_front(&TreeView::EditWndProc, this));

	return false;
}

bool TreeView::OnEndLabelEdit(const NMTVDISPINFO *dispInfo)
{
	m_editSubclass.reset();

	if (!dispInfo->item.pszText)
	{
		// Editing was cancelled.
		return false;
	}

	return m_delegate->OnNodeRenamed(GetNodeForHandle(dispInfo->item.hItem),
		dispInfo->item.pszText);
}

LRESULT TreeView::EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_GETDLGCODE:
		switch (wParam)
		{
		// The control may be shown within a dialog. The keys here are used when editing and should
		// be handled by the edit control, not the parent dialog (if any).
		case VK_ESCAPE:
		case VK_RETURN:
			return DLGC_WANTALLKEYS;
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void TreeView::OnSelectionChanged(const NMTREEVIEW *notifyInfo)
{
	m_delegate->OnSelectionChanged(GetNodeForHandle(notifyInfo->itemNew.hItem));
}

void TreeView::OnBeginDrag(const NMTREEVIEW *notifyInfo)
{
	m_delegate->OnBeginDrag(GetNodeForHandle(notifyInfo->itemNew.hItem));
}

void TreeView::SetImageList(wil::unique_himagelist imageList)
{
	// Changing the image list after it's been set isn't supported, so it's expected that the image
	// list will only be set once.
	auto previousImageList = TreeView_SetImageList(m_hwnd, imageList.get(), TVSIL_NORMAL);
	CHECK(!previousImageList);

	m_imageList = std::move(imageList);
}

TreeViewNode *TreeView::GetSelectedNode()
{
	return const_cast<TreeViewNode *>(std::as_const(*this).GetSelectedNode());
}

const TreeViewNode *TreeView::GetSelectedNode() const
{
	auto selectedHandle = TreeView_GetSelection(m_hwnd);
	CHECK(selectedHandle);
	return GetNodeForHandle(selectedHandle);
}

void TreeView::SelectNode(const TreeViewNode *node)
{
	auto res = TreeView_SelectItem(m_hwnd, GetHandleForNode(node));
	CHECK(res);
}

void TreeView::StartRenamingNode(const TreeViewNode *node)
{
	if (!node->CanRename())
	{
		return;
	}

	auto handle = GetHandleForNode(node);
	TreeView_EnsureVisible(m_hwnd, handle);
	auto res = TreeView_EditLabel(m_hwnd, handle);
	CHECK(res);
}

bool TreeView::IsNodeExpanded(const TreeViewNode *node) const
{
	UINT state = TreeView_GetItemState(m_hwnd, GetHandleForNode(node), TVIS_EXPANDED);
	return WI_IsFlagSet(state, TVIS_EXPANDED);
}

void TreeView::ExpandNode(const TreeViewNode *node)
{
	if (node->GetChildren().empty())
	{
		return;
	}

	auto res = TreeView_Expand(m_hwnd, GetHandleForNode(node), TVE_EXPAND);
	CHECK(res);
}

void TreeView::CollapseNode(const TreeViewNode *node)
{
	if (!IsNodeExpanded(node))
	{
		return;
	}

	auto res = TreeView_Expand(m_hwnd, GetHandleForNode(node), TVE_COLLAPSE);
	CHECK(res);
}

RECT TreeView::GetNodeRect(const TreeViewNode *node) const
{
	RECT nodeRect;
	auto res = TreeView_GetItemRect(m_hwnd, GetHandleForNode(node), &nodeRect, false);
	CHECK(res);
	return nodeRect;
}

TreeViewNode *TreeView::MaybeGetNodeAtPoint(const POINT &pt)
{
	TVHITTESTINFO hitTestInfo = {};
	hitTestInfo.pt = pt;
	auto handle = TreeView_HitTest(m_hwnd, &hitTestInfo);

	if (!handle)
	{
		return nullptr;
	}

	return GetNodeForHandle(handle);
}

TreeViewNode *TreeView::MaybeGetNextVisibleNode(const POINT &pt)
{
	HTREEITEM handle = nullptr;

	for (handle = TreeView_GetFirstVisible(m_hwnd); handle != nullptr;
		handle = TreeView_GetNextVisible(m_hwnd, handle))
	{
		auto rect = GetNodeRect(GetNodeForHandle(handle));

		if (pt.y < rect.top)
		{
			break;
		}
	}

	if (!handle)
	{
		return nullptr;
	}

	return GetNodeForHandle(handle);
}

bool TreeView::IsNodeHighlighted(const TreeViewNode *node) const
{
	UINT state = TreeView_GetItemState(m_hwnd, GetHandleForNode(node), TVIS_DROPHILITED);
	return WI_IsFlagSet(state, TVIS_DROPHILITED);
}

void TreeView::HighlightNode(const TreeViewNode *node)
{
	UpdateNodeState(node, TVIS_DROPHILITED, ItemStateOp::Set);
}

void TreeView::UnhighlightNode(const TreeViewNode *node)
{
	UpdateNodeState(node, TVIS_DROPHILITED, ItemStateOp::Clear);
}

void TreeView::UpdateNodeState(const TreeViewNode *node, UINT state, ItemStateOp stateOp)
{
	TVITEM tvItem = {};
	tvItem.mask = TVIF_STATE;
	tvItem.hItem = GetHandleForNode(node);
	tvItem.stateMask = state;
	tvItem.state = (stateOp == ItemStateOp::Set) ? state : 0;
	auto res = TreeView_SetItem(m_hwnd, &tvItem);
	CHECK(res);
}

void TreeView::ShowInsertMark(const TreeViewNode *targetNode, InsertMarkPosition position)
{
	auto res = TreeView_SetInsertMark(m_hwnd, GetHandleForNode(targetNode),
		position == InsertMarkPosition::After);
	CHECK(res);
}

void TreeView::RemoveInsertMark()
{
	auto res = TreeView_SetInsertMark(m_hwnd, nullptr, false);
	CHECK(res);
}

RawTreeViewNodes TreeView::GetExpandedNodes()
{
	if (!m_adapter)
	{
		return {};
	}

	RawTreeViewNodes expandedNodes;
	m_adapter->GetRoot()->VisitRecursively(
		[this, &expandedNodes](TreeViewNode *currentNode)
		{
			if (currentNode == m_adapter->GetRoot() || !IsNodeExpanded(currentNode))
			{
				return;
			}

			expandedNodes.push_back(currentNode);
		});
	return expandedNodes;
}

ConstRawTreeViewNodes TreeView::GetAllNodesDepthFirstForTesting() const
{
	CHECK(IsInTest());

	ConstRawTreeViewNodes nodes;
	GetAllNodesDepthFirstForTesting(TreeView_GetRoot(m_hwnd), nodes);
	return nodes;
}

void TreeView::GetAllNodesDepthFirstForTesting(HTREEITEM firstSiblingHandle,
	ConstRawTreeViewNodes &nodes) const
{
	for (auto siblingHandle = firstSiblingHandle; siblingHandle != nullptr;
		siblingHandle = TreeView_GetNextSibling(m_hwnd, siblingHandle))
	{
		nodes.push_back(GetNodeForHandle(siblingHandle));

		if (auto childHandle = TreeView_GetChild(m_hwnd, siblingHandle))
		{
			GetAllNodesDepthFirstForTesting(childHandle, nodes);
		}
	}
}

HTREEITEM TreeView::GetHandleForNode(const TreeViewNode *node) const
{
	auto itr = m_handleToNodeMap.right.find(node);
	CHECK(itr != m_handleToNodeMap.right.end());
	return itr->second;
}

TreeViewNode *TreeView::GetNodeForHandle(HTREEITEM handle)
{
	return const_cast<TreeViewNode *>(std::as_const(*this).GetNodeForHandle(handle));
}

const TreeViewNode *TreeView::GetNodeForHandle(HTREEITEM handle) const
{
	auto itr = m_handleToNodeMap.left.find(handle);
	CHECK(itr != m_handleToNodeMap.left.end());
	return itr->second;
}

bool TreeView::NoOpDelegate::OnNodeRenamed(TreeViewNode *targetNode, const std::wstring &name)
{
	UNREFERENCED_PARAMETER(targetNode);
	UNREFERENCED_PARAMETER(name);
	return false;
}

void TreeView::NoOpDelegate::OnNodeRemoved(TreeViewNode *targetNode)
{
	UNREFERENCED_PARAMETER(targetNode);
}

void TreeView::NoOpDelegate::OnNodeCopied(TreeViewNode *targetNode)
{
	UNREFERENCED_PARAMETER(targetNode);
}

void TreeView::NoOpDelegate::OnNodeCut(TreeViewNode *targetNode)
{
	UNREFERENCED_PARAMETER(targetNode);
}

void TreeView::NoOpDelegate::OnPaste(TreeViewNode *targetNode)
{
	UNREFERENCED_PARAMETER(targetNode);
}

void TreeView::NoOpDelegate::OnSelectionChanged(TreeViewNode *selectedNode)
{
	UNREFERENCED_PARAMETER(selectedNode);
}

void TreeView::NoOpDelegate::OnShowContextMenu(TreeViewNode *targetNode, const POINT &ptScreen)
{
	UNREFERENCED_PARAMETER(targetNode);
	UNREFERENCED_PARAMETER(ptScreen);
}

void TreeView::NoOpDelegate::OnBeginDrag(TreeViewNode *targetNode)
{
	UNREFERENCED_PARAMETER(targetNode);
}
