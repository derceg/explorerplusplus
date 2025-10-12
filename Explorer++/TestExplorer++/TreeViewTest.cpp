// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TreeView.h"
#include "KeyboardStateFake.h"
#include "MouseEvent.h"
#include "TreeViewAdapter.h"
#include "../Helper/Helper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wil/resource.h>
#include <memory>

using namespace testing;

namespace
{

class FakeTreeViewNode : public TreeViewNode
{
public:
	std::wstring GetText() const override
	{
		return L"";
	}

	std::optional<int> GetIconIndex() const override
	{
		return std::nullopt;
	}

	bool CanRename() const override
	{
		return true;
	}

	bool CanRemove() const override
	{
		return true;
	}
};

class TreeViewDelegateMock : public TreeViewDelegate
{
public:
	MOCK_METHOD(void, OnNodeMiddleClicked, (TreeViewNode * targetNode, const MouseEvent &event),
		(override));
	MOCK_METHOD(bool, OnNodeRenamed, (TreeViewNode * targetNode, const std::wstring &name),
		(override));
	MOCK_METHOD(void, OnNodeRemoved, (TreeViewNode * targetNode, RemoveMode removeMode),
		(override));
	MOCK_METHOD(void, OnNodeCopied, (TreeViewNode * targetNode), (override));
	MOCK_METHOD(void, OnNodeCut, (TreeViewNode * targetNode), (override));
	MOCK_METHOD(void, OnPaste, (TreeViewNode * targetNode), (override));
	MOCK_METHOD(void, OnSelectionChanged, (TreeViewNode * selectedNode), (override));
	MOCK_METHOD(void, OnShowContextMenu, (TreeViewNode * targetNode, const POINT &ptScreen),
		(override));
	MOCK_METHOD(void, OnBeginDrag, (TreeViewNode * targetNode), (override));
	MOCK_METHOD(void, OnBeginRightButtonDrag, (TreeViewNode * targetNode), (override));
};

}

class TreeViewTest : public Test
{
protected:
	void SetUp() override
	{
		m_parentWindow.reset(CreateWindow(WC_STATIC, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_parentWindow, nullptr);

		m_treeViewWindow = CreateWindow(WC_TREEVIEW, L"", WS_POPUP, 0, 0, 1000, 1000,
			m_parentWindow.get(), nullptr, GetModuleHandle(nullptr), nullptr);
		ASSERT_NE(m_treeViewWindow, nullptr);

		m_treeView = std::make_unique<TreeView>(m_treeViewWindow, &m_keyboardState);
		m_treeView->SetAdapter(&m_adapter);
		m_treeView->SetDelegate(&m_delegate);
	}

	static void SimulateMiddleClick(HWND hwnd, const POINT &ptClient)
	{
		SendMessage(hwnd, WM_MBUTTONDOWN, 0, MAKELPARAM(ptClient.x, ptClient.y));
		SendMessage(hwnd, WM_MBUTTONUP, 0, MAKELPARAM(ptClient.x, ptClient.y));
	}

	KeyboardStateFake m_keyboardState;
	wil::unique_hwnd m_parentWindow;
	HWND m_treeViewWindow = nullptr;
	TreeViewAdapter m_adapter;
	TreeViewDelegateMock m_delegate;
	std::unique_ptr<TreeView> m_treeView;
};

TEST_F(TreeViewTest, MoveNodeCollapsesParent)
{
	auto *node1 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	auto *childNode = m_adapter.AddNode(node1, std::make_unique<FakeTreeViewNode>());
	auto *node2 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	m_treeView->ExpandNode(node1);

	// Since node1 no longer has any children after the move, it should be collapsed.
	m_adapter.MoveNode(childNode, node2, 0);
	EXPECT_FALSE(m_treeView->IsNodeExpanded(node1));
}

TEST_F(TreeViewTest, MoveNodeDoesntCollapseParent)
{
	auto *node1 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	auto *childNode1 = m_adapter.AddNode(node1, std::make_unique<FakeTreeViewNode>());
	m_adapter.AddNode(node1, std::make_unique<FakeTreeViewNode>());
	auto *node2 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	m_treeView->ExpandNode(node1);

	// node1 still has a child, so it shouldn't be collapsed.
	m_adapter.MoveNode(childNode1, node2, 0);
	EXPECT_TRUE(m_treeView->IsNodeExpanded(node1));
}

TEST_F(TreeViewTest, MoveSelectedNode)
{
	auto *node1 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	auto *node2 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	m_treeView->SelectNode(node2);

	// The node being moved is selected, so it should still be selected after the move and no
	// selection change notification should be generated.
	EXPECT_CALL(m_delegate, OnSelectionChanged(_)).Times(0);
	m_adapter.MoveNode(node2, node1, 0);
	EXPECT_EQ(m_treeView->GetSelectedNode(), node2);
}

TEST_F(TreeViewTest, MoveNonSelectedNode)
{
	auto *node1 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	auto *node2 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	m_treeView->SelectNode(node1);

	// The node being moved isn't selected, so the selection shouldn't change and no selection
	// change notification should be generated.
	EXPECT_CALL(m_delegate, OnSelectionChanged(_)).Times(0);
	m_adapter.MoveNode(node2, node1, 0);
	EXPECT_EQ(m_treeView->GetSelectedNode(), node1);
}

TEST_F(TreeViewTest, ExpandNodeWithNoChildren)
{
	// The node has no children, so attempting to expand it should have no effect.
	const auto *node = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	m_treeView->ExpandNode(node);
	EXPECT_FALSE(m_treeView->IsNodeExpanded(node));
}

TEST_F(TreeViewTest, ItemPosition)
{
	const auto *node1 =
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	const auto *node2 =
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());

	auto node1Rect = m_treeView->GetNodeRect(node1);
	POINT node1Origin = { node1Rect.left, node1Rect.top };
	EXPECT_EQ(m_treeView->MaybeGetNodeAtPoint(node1Origin), node1);
	EXPECT_EQ(m_treeView->MaybeGetNextVisibleNode(node1Origin), node2);
}

TEST_F(TreeViewTest, GhostedNode)
{
	const auto *node = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());

	m_treeView->SetNodeGhosted(node, true);
	EXPECT_TRUE(m_treeView->IsNodeGhosted(node));

	m_treeView->SetNodeGhosted(node, false);
	EXPECT_FALSE(m_treeView->IsNodeGhosted(node));
}

TEST_F(TreeViewTest, HighlightedNode)
{
	const auto *node = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());

	m_treeView->SetNodeHighlighted(node, true);
	EXPECT_TRUE(m_treeView->IsNodeHighlighted(node));

	m_treeView->SetNodeHighlighted(node, false);
	EXPECT_FALSE(m_treeView->IsNodeHighlighted(node));
}

TEST_F(TreeViewTest, InsertMark)
{
	m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	const auto *node2 =
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());

	m_treeView->ShowInsertMark(node2, InsertMarkPosition::Before);
	m_treeView->RemoveInsertMark();
}

TEST_F(TreeViewTest, MiddleClick)
{
	auto *node = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());

	auto rect = m_treeView->GetNodeRect(node);
	POINT pt = { rect.left, rect.top };
	EXPECT_CALL(m_delegate, OnNodeMiddleClicked(node, MouseEvent(pt, false, false)));
	SimulateMiddleClick(m_treeViewWindow, pt);
}

class TreeViewKeyPressTest : public TreeViewTest
{
protected:
	void SetUp() override
	{
		TreeViewTest::SetUp();

		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
		m_node2 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
		m_treeView->SelectNode(m_node2);
	}

	TreeViewNode *m_node2 = nullptr;
};

TEST_F(TreeViewKeyPressTest, Copy)
{
	EXPECT_CALL(m_delegate, OnNodeCopied(m_node2));
	m_keyboardState.SetCtrlDown(true);
	SendSimulatedKeyPress(m_treeViewWindow, 'C');
}

TEST_F(TreeViewKeyPressTest, Cut)
{
	EXPECT_CALL(m_delegate, OnNodeCut(m_node2));
	m_keyboardState.SetCtrlDown(true);
	SendSimulatedKeyPress(m_treeViewWindow, 'X');
}

TEST_F(TreeViewKeyPressTest, Paste)
{
	EXPECT_CALL(m_delegate, OnPaste(m_node2));
	m_keyboardState.SetCtrlDown(true);
	SendSimulatedKeyPress(m_treeViewWindow, 'V');
}

TEST_F(TreeViewKeyPressTest, Delete)
{
	EXPECT_CALL(m_delegate, OnNodeRemoved(m_node2, RemoveMode::Standard));
	SendSimulatedKeyPress(m_treeViewWindow, VK_DELETE);

	EXPECT_CALL(m_delegate, OnNodeRemoved(m_node2, RemoveMode::Permanent));
	m_keyboardState.SetShiftDown(true);
	SendSimulatedKeyPress(m_treeViewWindow, VK_DELETE);
}
