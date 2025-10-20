// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TreeView.h"
#include "MouseEvent.h"
#include "TestHelper.h"
#include "TreeViewAdapter.h"
#include "../Helper/AutoReset.h"
#include "../Helper/KeyboardState.h"
#include "../Helper/WindowSubclass.h"
#include <wil/common.h>

TreeView::TreeView(HWND hwnd, const KeyboardState *keyboardState,
	LabelEditHandlerFactory labelEditHandlerFactory) :
	m_hwnd(hwnd),
	m_keyboardState(keyboardState),
	m_labelEditHandlerFactory(labelEditHandlerFactory)
{
	m_windowSubclasses.push_back(
		std::make_unique<WindowSubclass>(m_hwnd, std::bind_front(&TreeView::WndProc, this)));
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

void TreeView::AddExtendedStyles(DWORD styles)
{
	TreeView_SetExtendedStyle(m_hwnd, styles, styles);
}

void TreeView::AddNodeRecursive(TreeViewNode *node)
{
	for (auto *currentNode : node->GetNodesDepthFirst())
	{
		AddNode(currentNode);
	}
}

void TreeView::AddNode(TreeViewNode *node)
{
	if (m_adapter->IsRoot(node))
	{
		return;
	}

	std::wstring text = node->GetText();

	TVITEMEX tvItem = {};
	tvItem.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE;
	tvItem.pszText = text.data();
	tvItem.cChildren = I_CHILDRENCALLBACK;
	tvItem.stateMask = TVIS_CUT;
	tvItem.state = node->IsGhosted() ? TVIS_CUT : 0;

	if (m_imageList)
	{
		WI_SetAllFlags(tvItem.mask, TVIF_IMAGE | TVIF_SELECTEDIMAGE);
		tvItem.iImage = I_IMAGECALLBACK;
		tvItem.iSelectedImage = I_IMAGECALLBACK;
	}

	auto *parentNode = node->GetParent();

	HTREEITEM parentHandle = m_adapter->IsRoot(parentNode) ? nullptr : GetHandleForNode(parentNode);
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

	if (!TreeView_GetSelection(m_hwnd))
	{
		// There should always be a selected node. If there isn't, then this is the first node being
		// added and it should be selected.
		SelectNode(node);
	}
}

void TreeView::UpdateNode(TreeViewNode *node, TreeViewNode::Property property)
{
	std::wstring nodeText;

	TVITEMEX tvItem = {};
	tvItem.hItem = GetHandleForNode(node);

	switch (property)
	{
	case TreeViewNode::Property::Text:
		WI_SetFlag(tvItem.mask, TVIF_TEXT);
		nodeText = node->GetText();
		tvItem.pszText = nodeText.data();
		break;

	case TreeViewNode::Property::Icon:
	{
		// The icon for a node should only be updated if images are being displayed.
		CHECK(m_imageList);

		// Additionally, an icon index should be returned here.
		auto iconIndex = node->GetIconIndex();
		CHECK(iconIndex);

		WI_SetAllFlags(tvItem.mask, TVIF_IMAGE | TVIF_SELECTEDIMAGE);
		tvItem.iImage = *iconIndex;
		tvItem.iSelectedImage = *iconIndex;
	}
	break;

	case TreeViewNode::Property::Ghosted:
		WI_SetFlag(tvItem.mask, TVIF_STATE);
		tvItem.stateMask = TVIS_CUT;
		tvItem.state = node->IsGhosted() ? TVIS_CUT : 0;
		break;

	case TreeViewNode::Property::MayLazyLoadChildren:
		if (!node->GetChildren().empty())
		{
			// This hint is only valid when the node has no children. If the node currently has
			// children, the hint can be ignored.
			return;
		}

		WI_SetFlag(tvItem.mask, TVIF_CHILDREN);
		tvItem.cChildren = IsNodeExpandable(node) ? 1 : 0;
		break;
	}

	CHECK(tvItem.mask != 0);

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

	auto *selectedNode = GetSelectedNode();

	// The node being moved (or one of its descendants) may be selected. When the node is removed,
	// that will then result in the selection changing. Setting this here ensures that no selection
	// change event will be raised.
	//
	// Therefore, although the selection can temporarily change below, from the point of view of
	// external clients, the selection remains unchanged.
	AutoReset autoReset(&m_blockSelectionChangeEvent, true);

	RemoveNode(node);
	AddNodeRecursive(node);

	// The selection is restored here, since the calls above may have changed it. If the selection
	// wasn't changed, this call is unnecessary, but won't have any impact.
	SelectNode(selectedNode);
}

void TreeView::RemoveNode(TreeViewNode *node)
{
	// Note that node->GetParent() shouldn't be used here, since if the node was moved, the parent
	// node may not match the parent in the view, and if the node was removed, the node won't have a
	// parent node.
	auto handle = GetHandleForNode(node);
	auto parentHandle = TreeView_GetParent(m_hwnd, handle);

	auto res = TreeView_DeleteItem(m_hwnd, handle);
	CHECK(res);

	for (auto *currentNode : node->GetNodesDepthFirst())
	{
		auto numErased = m_handleToNodeMap.right.erase(currentNode);
		CHECK_EQ(numErased, 1u);
	}

	if (parentHandle && !TreeView_GetChild(m_hwnd, parentHandle))
	{
		// Note that passing I_CHILDRENCALLBACK here will reset the expansion state of the item
		// (i.e. TVIS_EXPANDED and TVIS_EXPANDEDONCE will be reset).
		TVITEM tvItem = {};
		tvItem.mask = TVIF_CHILDREN;
		tvItem.hItem = parentHandle;
		tvItem.cChildren = I_CHILDRENCALLBACK;
		res = TreeView_SetItem(m_hwnd, &tvItem);
		CHECK(res);
	}
}

void TreeView::RemoveAllNodes()
{
	auto res = TreeView_DeleteAllItems(m_hwnd);
	CHECK(res);
}

LRESULT TreeView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CAPTURECHANGED:
		OnCaptureChanged(reinterpret_cast<HWND>(lParam));
		break;

	case WM_MBUTTONDOWN:
		OnMiddleButtonDown({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		break;

	case WM_MBUTTONUP:
		OnMiddleButtonUp({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void TreeView::OnCaptureChanged(HWND target)
{
	if (target != m_hwnd)
	{
		m_middleClickNodeId.reset();
	}
}

void TreeView::OnMiddleButtonDown(const POINT &pt)
{
	const auto *node = MaybeGetNodeAtPoint(pt, HitTestScope::IconOrText);

	if (!node)
	{
		return;
	}

	SetCapture(m_hwnd);

	m_middleClickNodeId = node->GetId();
}

void TreeView::OnMiddleButtonUp(const POINT &pt)
{
	if (!m_middleClickNodeId)
	{
		return;
	}

	auto releaseCapture = wil::scope_exit([] { ReleaseCapture(); });

	auto *node = MaybeGetNodeAtPoint(pt, HitTestScope::IconOrText);

	if (!node || node->GetId() != *m_middleClickNodeId)
	{
		return;
	}

	m_delegate->OnNodeMiddleClicked(node,
		{ pt, m_keyboardState->IsShiftDown(), m_keyboardState->IsCtrlDown() });
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
			case TVN_GETDISPINFO:
				OnGetDispInfo(reinterpret_cast<NMTVDISPINFO *>(lParam));
				break;

			case TVN_ITEMEXPANDING:
				OnNodeExpanding(reinterpret_cast<NMTREEVIEW *>(lParam));
				break;

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

			case TVN_BEGINRDRAG:
				OnBeginRightButtonDrag(reinterpret_cast<NMTREEVIEW *>(lParam));
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
	bool highlightTargetNode = false;

	if (ptScreen.x == -1 && ptScreen.y == -1)
	{
		auto *selectedNode = GetSelectedNode();
		auto textRect = GetNodeRect(selectedNode, NodeRectType::Text);

		ptScreenFinal = { textRect.left, textRect.top + (textRect.bottom - textRect.top) / 2 };
		ClientToScreen(m_hwnd, &ptScreenFinal);

		targetNode = selectedNode;
	}
	else
	{
		ptScreenFinal = ptScreen;

		POINT ptClient = ptScreen;
		ScreenToClient(m_hwnd, &ptClient);

		targetNode = MaybeGetNodeAtPoint(ptClient, HitTestScope::IconOrText);

		if (!targetNode)
		{
			return;
		}

		highlightTargetNode = true;
	}

	int targetNodeId = targetNode->GetId();

	if (highlightTargetNode)
	{
		SetNodeHighlighted(targetNode, true);
	}

	m_delegate->OnShowContextMenu(targetNode, ptScreenFinal);

	// The delegate can delete the node in the callback above.
	if (highlightTargetNode && m_adapter->MaybeGetNodeById(targetNodeId))
	{
		SetNodeHighlighted(targetNode, false);
	}
}

void TreeView::OnGetDispInfo(NMTVDISPINFO *dispInfo)
{
	const auto *node = GetNodeForHandle(dispInfo->item.hItem);

	if (WI_IsAnyFlagSet(dispInfo->item.mask, TVIF_IMAGE | TVIF_SELECTEDIMAGE))
	{
		// The image index and selected image index will only be set to I_IMAGECALLBACK if an image
		// list is set, so that's the only time this message should be received and GetIconIndex()
		// should return an icon index.
		auto iconIndex = node->GetIconIndex();
		CHECK(iconIndex);

		dispInfo->item.iImage = *iconIndex;
		dispInfo->item.iSelectedImage = *iconIndex;
	}

	if (WI_IsFlagSet(dispInfo->item.mask, TVIF_CHILDREN))
	{
		dispInfo->item.cChildren = IsNodeExpandable(node) ? 1 : 0;
	}

	WI_SetFlag(dispInfo->item.mask, TVIF_DI_SETITEM);
}

void TreeView::OnNodeExpanding(const NMTREEVIEW *notifyInfo)
{
	auto *node = GetNodeForHandle(notifyInfo->itemNew.hItem);

	if (notifyInfo->action == TVE_EXPAND)
	{
		m_adapter->OnNodeExpanding(node);
	}
	else
	{
		m_adapter->OnNodeCollapsing(node);
	}
}

LRESULT TreeView::OnKeyDown(const NMTVKEYDOWN *keyDown)
{
	switch (keyDown->wVKey)
	{
	case VK_F2:
		StartRenamingNode(GetSelectedNode());
		break;

	case 'C':
		if (m_keyboardState->IsCtrlDown() && !m_keyboardState->IsShiftDown()
			&& !m_keyboardState->IsAltDown())
		{
			m_delegate->OnNodeCopied(GetSelectedNode());
		}
		break;

	case 'X':
		if (m_keyboardState->IsCtrlDown() && !m_keyboardState->IsShiftDown()
			&& !m_keyboardState->IsAltDown())
		{
			m_delegate->OnNodeCut(GetSelectedNode());
		}
		break;

	case 'V':
		if (m_keyboardState->IsCtrlDown() && !m_keyboardState->IsShiftDown()
			&& !m_keyboardState->IsAltDown())
		{
			m_delegate->OnPaste(GetSelectedNode());
		}
		break;

	case VK_INSERT:
		if (m_keyboardState->IsCtrlDown() && !m_keyboardState->IsShiftDown()
			&& !m_keyboardState->IsAltDown())
		{
			m_delegate->OnNodeCopied(GetSelectedNode());
		}
		if (!m_keyboardState->IsCtrlDown() && m_keyboardState->IsShiftDown()
			&& !m_keyboardState->IsAltDown())
		{
			m_delegate->OnPaste(GetSelectedNode());
		}
		break;

	case VK_DELETE:
		OnDeletePressed(
			m_keyboardState->IsShiftDown() ? RemoveMode::Permanent : RemoveMode::Standard);
		break;
	}

	// If the ctrl key is down, this key sequence is likely a modifier. Stop any other pressed key
	// from been used in an incremental search.
	if (m_keyboardState->IsCtrlDown())
	{
		return 1;
	}

	return 0;
}

void TreeView::OnDeletePressed(RemoveMode removeMode)
{
	auto *node = GetSelectedNode();

	if (!node->CanRemove())
	{
		return;
	}

	m_delegate->OnNodeRemoved(node, removeMode);
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
	m_labelEditHandlerFactory(editControl, node->IsFile());

	return false;
}

bool TreeView::OnEndLabelEdit(const NMTVDISPINFO *dispInfo)
{
	if (!dispInfo->item.pszText)
	{
		// Editing was cancelled.
		return false;
	}

	return m_delegate->OnNodeRenamed(GetNodeForHandle(dispInfo->item.hItem),
		dispInfo->item.pszText);
}

void TreeView::OnSelectionChanged(const NMTREEVIEW *notifyInfo)
{
	if (m_blockSelectionChangeEvent)
	{
		return;
	}

	m_delegate->OnSelectionChanged(GetNodeForHandle(notifyInfo->itemNew.hItem));
}

void TreeView::OnBeginDrag(const NMTREEVIEW *notifyInfo)
{
	m_delegate->OnBeginDrag(GetNodeForHandle(notifyInfo->itemNew.hItem));
}

void TreeView::OnBeginRightButtonDrag(const NMTREEVIEW *notifyInfo)
{
	m_delegate->OnBeginRightButtonDrag(GetNodeForHandle(notifyInfo->itemNew.hItem));
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

bool TreeView::IsNodeExpandable(const TreeViewNode *node) const
{
	return !node->GetChildren().empty() || node->GetMayLazyLoadChildren();
}

bool TreeView::IsNodeExpanded(const TreeViewNode *node) const
{
	UINT state = TreeView_GetItemState(m_hwnd, GetHandleForNode(node), TVIS_EXPANDED);
	return WI_IsFlagSet(state, TVIS_EXPANDED);
}

// ExpandNode() and CollapseNode() both take a non-const TreeViewNode *, since the adapter may
// choose to modify the node in response to the notification that a node has expanded or collapsed.
// For example, the adapter can lazily load children when a node is expanded and remove all children
// when a node is collapsed.
void TreeView::ExpandNode(TreeViewNode *node)
{
	if (!IsNodeExpandable(node))
	{
		return;
	}

	auto res = TreeView_Expand(m_hwnd, GetHandleForNode(node), TVE_EXPAND);

	// Expanding the node can fail if the node doesn't add any children when expanded.
	CHECK(node->GetChildren().empty() || res);
}

void TreeView::CollapseNode(TreeViewNode *node)
{
	if (!IsNodeExpanded(node))
	{
		return;
	}

	// The collapse below is programmatically initiated, so no TVN_ITEMEXPANDING notification will
	// be sent. Therefore, the adapter will be manually notified.
	m_adapter->OnNodeCollapsing(node);

	auto res = TreeView_Expand(m_hwnd, GetHandleForNode(node), TVE_COLLAPSE);

	// Collapsing the node can fail if the node removes all of its children when collapsed.
	CHECK(node->GetChildren().empty() || res);
}

RECT TreeView::GetNodeRect(const TreeViewNode *node, NodeRectType rectType) const
{
	RECT nodeRect;
	auto res = TreeView_GetItemRect(m_hwnd, GetHandleForNode(node), &nodeRect,
		rectType == NodeRectType::Text ? true : false);
	CHECK(res);
	return nodeRect;
}

TreeViewNode *TreeView::MaybeGetNodeAtPoint(const POINT &pt, HitTestScope scope)
{
	TVHITTESTINFO hitTestInfo = {};
	hitTestInfo.pt = pt;
	auto handle = TreeView_HitTest(m_hwnd, &hitTestInfo);

	if (!handle)
	{
		return nullptr;
	}

	if (scope == HitTestScope::IconOrText && WI_AreAllFlagsClear(hitTestInfo.flags, TVHT_ONITEM))
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
		auto rect = GetNodeRect(GetNodeForHandle(handle), NodeRectType::EntireLine);

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

void TreeView::SetNodeHighlighted(const TreeViewNode *node, bool highlighted)
{
	UpdateNodeState(node, TVIS_DROPHILITED, highlighted ? ItemStateOp::Set : ItemStateOp::Clear);
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

	for (auto *currentNode : m_adapter->GetRoot()->GetNodesDepthFirst())
	{
		if (m_adapter->IsRoot(currentNode) || !IsNodeExpanded(currentNode))
		{
			continue;
		}

		expandedNodes.push_back(currentNode);
	}

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

std::wstring TreeView::GetNodeTextForTesting(const TreeViewNode *node) const
{
	CHECK(IsInTest());

	wchar_t text[260];

	TVITEM tvItem = {};
	tvItem.mask = TVIF_HANDLE | TVIF_TEXT;
	tvItem.hItem = GetHandleForNode(node);
	tvItem.pszText = text;
	tvItem.cchTextMax = std::size(text);
	auto res = TreeView_GetItem(m_hwnd, &tvItem);
	CHECK(res);

	return text;
}

bool TreeView::IsNodeGhostedForTesting(const TreeViewNode *node) const
{
	CHECK(IsInTest());

	UINT state = TreeView_GetItemState(m_hwnd, GetHandleForNode(node), TVIS_CUT);
	return WI_IsFlagSet(state, TVIS_CUT);
}

bool TreeView::IsExpanderShownForTesting(const TreeViewNode *node) const
{
	CHECK(IsInTest());

	TVITEM tvItem = {};
	tvItem.mask = TVIF_HANDLE | TVIF_CHILDREN;
	tvItem.hItem = GetHandleForNode(node);
	auto res = TreeView_GetItem(m_hwnd, &tvItem);
	CHECK(res);
	return tvItem.cChildren != 0;
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

void TreeView::NoOpDelegate::OnNodeMiddleClicked(TreeViewNode *targetNode, const MouseEvent &event)
{
	UNREFERENCED_PARAMETER(targetNode);
	UNREFERENCED_PARAMETER(event);
}

bool TreeView::NoOpDelegate::OnNodeRenamed(TreeViewNode *targetNode, const std::wstring &name)
{
	UNREFERENCED_PARAMETER(targetNode);
	UNREFERENCED_PARAMETER(name);
	return false;
}

void TreeView::NoOpDelegate::OnNodeRemoved(TreeViewNode *targetNode, RemoveMode removeMode)
{
	UNREFERENCED_PARAMETER(targetNode);
	UNREFERENCED_PARAMETER(removeMode);
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

void TreeView::NoOpDelegate::OnBeginRightButtonDrag(TreeViewNode *targetNode)
{
	UNREFERENCED_PARAMETER(targetNode);
}
