// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TreeView.h"
#include "KeyboardStateFake.h"
#include "LabelEditHandler.h"
#include "MouseEvent.h"
#include "TreeViewAdapter.h"
#include "TreeViewNodeFake.h"
#include "../Helper/Helper.h"
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wil/resource.h>
#include <memory>

using namespace testing;

namespace
{

class TreeViewAdapterMock : public TreeViewAdapter
{
public:
	using TreeViewAdapter::AddNode;
	using TreeViewAdapter::MoveNode;

	MOCK_METHOD(void, OnNodeExpanding, (TreeViewNode * node), (override));
	MOCK_METHOD(void, OnNodeCollapsing, (TreeViewNode * node), (override));
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

class TreeViewTestBase : public Test
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
	}

	static void SimulateMiddleClick(HWND hwnd, const POINT &ptClient)
	{
		SendMessage(hwnd, WM_MBUTTONDOWN, 0, MAKELPARAM(ptClient.x, ptClient.y));
		SendMessage(hwnd, WM_MBUTTONUP, 0, MAKELPARAM(ptClient.x, ptClient.y));
	}

	KeyboardStateFake m_keyboardState;
	wil::unique_hwnd m_parentWindow;
	HWND m_treeViewWindow = nullptr;
	TreeViewDelegateMock m_delegate;
	std::unique_ptr<TreeView> m_treeView;
};

class TreeViewTest : public TreeViewTestBase
{
protected:
	void SetUp() override
	{
		TreeViewTestBase::SetUp();

		m_treeView = std::make_unique<TreeView>(m_treeViewWindow, &m_keyboardState,
			LabelEditHandler::CreateForTest);
		m_treeView->SetAdapter(&m_adapter);
		m_treeView->SetDelegate(&m_delegate);
	}

	TreeViewAdapterMock m_adapter;
};

TEST_F(TreeViewTest, MoveNodeCollapsesParent)
{
	auto *node1 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
	auto *childNode = m_adapter.AddNode(node1, std::make_unique<TreeViewNodeFake>());
	auto *node2 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
	m_treeView->ExpandNode(node1);

	// Since node1 no longer has any children after the move, it should be collapsed.
	m_adapter.MoveNode(childNode, node2, 0);
	EXPECT_FALSE(m_treeView->IsNodeExpanded(node1));
}

TEST_F(TreeViewTest, MoveNodeDoesntCollapseParent)
{
	auto *node1 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
	auto *childNode1 = m_adapter.AddNode(node1, std::make_unique<TreeViewNodeFake>());
	m_adapter.AddNode(node1, std::make_unique<TreeViewNodeFake>());
	auto *node2 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
	m_treeView->ExpandNode(node1);

	// node1 still has a child, so it shouldn't be collapsed.
	m_adapter.MoveNode(childNode1, node2, 0);
	EXPECT_TRUE(m_treeView->IsNodeExpanded(node1));
}

TEST_F(TreeViewTest, MoveSelectedNode)
{
	auto *node1 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
	auto *node2 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
	m_treeView->SelectNode(node2);

	// The node being moved is selected, so it should still be selected after the move and no
	// selection change notification should be generated.
	EXPECT_CALL(m_delegate, OnSelectionChanged(_)).Times(0);
	m_adapter.MoveNode(node2, node1, 0);
	EXPECT_EQ(m_treeView->GetSelectedNode(), node2);
}

TEST_F(TreeViewTest, MoveNonSelectedNode)
{
	auto *node1 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
	auto *node2 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
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
	auto *node = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
	m_treeView->ExpandNode(node);
	EXPECT_FALSE(m_treeView->IsNodeExpanded(node));
}

TEST_F(TreeViewTest, ItemPosition)
{
	const auto *node1 =
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
	const auto *node2 =
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());

	auto node1Rect = m_treeView->GetNodeRect(node1, TreeView::NodeRectType::EntireLine);
	POINT node1Origin = { node1Rect.left, node1Rect.top };
	EXPECT_EQ(m_treeView->MaybeGetNodeAtPoint(node1Origin, TreeView::HitTestScope::IconOrText),
		nullptr);
	EXPECT_EQ(m_treeView->MaybeGetNodeAtPoint(node1Origin, TreeView::HitTestScope::Row), node1);
	EXPECT_EQ(m_treeView->MaybeGetNextVisibleNode(node1Origin), node2);

	auto node1TextRect = m_treeView->GetNodeRect(node1, TreeView::NodeRectType::Text);
	POINT node1TextOrigin = { node1TextRect.left, node1TextRect.top };
	EXPECT_EQ(m_treeView->MaybeGetNodeAtPoint(node1TextOrigin, TreeView::HitTestScope::IconOrText),
		node1);
	EXPECT_EQ(m_treeView->MaybeGetNodeAtPoint(node1TextOrigin, TreeView::HitTestScope::Row), node1);
}

TEST_F(TreeViewTest, GhostedNode)
{
	auto node = std::make_unique<TreeViewNodeFake>();
	auto *rawNode = node.get();

	rawNode->SetIsGhosted(true);
	m_adapter.AddNode(m_adapter.GetRoot(), std::move(node));
	EXPECT_TRUE(m_treeView->IsNodeGhostedForTesting(rawNode));

	rawNode->SetIsGhosted(false);
	EXPECT_FALSE(m_treeView->IsNodeGhostedForTesting(rawNode));
}

TEST_F(TreeViewTest, HighlightedNode)
{
	const auto *node = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());

	m_treeView->SetNodeHighlighted(node, true);
	EXPECT_TRUE(m_treeView->IsNodeHighlighted(node));

	m_treeView->SetNodeHighlighted(node, false);
	EXPECT_FALSE(m_treeView->IsNodeHighlighted(node));
}

TEST_F(TreeViewTest, InsertMark)
{
	m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
	const auto *node2 =
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());

	m_treeView->ShowInsertMark(node2, InsertMarkPosition::Before);
	m_treeView->RemoveInsertMark();
}

TEST_F(TreeViewTest, MiddleClick)
{
	auto *node = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());

	auto rect = m_treeView->GetNodeRect(node, TreeView::NodeRectType::Text);
	POINT pt = { rect.left, rect.top };
	EXPECT_CALL(m_delegate, OnNodeMiddleClicked(node, MouseEvent(pt, false, false)));
	SimulateMiddleClick(m_treeViewWindow, pt);
}

TEST_F(TreeViewTest, MiddleClickOutsideIconText)
{
	auto *node = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());

	// Here, the node is middle-clicked, though outside of the icon/text bounds. So, no middle click
	// event should be generated.
	auto rect = m_treeView->GetNodeRect(node, TreeView::NodeRectType::EntireLine);
	POINT pt = { rect.left, rect.top };
	EXPECT_CALL(m_delegate, OnNodeMiddleClicked(_, _)).Times(0);
	SimulateMiddleClick(m_treeViewWindow, pt);
}

TEST_F(TreeViewTest, ExpandCollapseCallbacks)
{
	auto *node = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
	m_adapter.AddNode(node, std::make_unique<TreeViewNodeFake>());

	EXPECT_CALL(m_adapter, OnNodeExpanding(node));
	m_treeView->ExpandNode(node);

	EXPECT_CALL(m_adapter, OnNodeCollapsing(node));
	m_treeView->CollapseNode(node);
}

class TreeViewKeyPressTest : public TreeViewTest
{
protected:
	void SetUp() override
	{
		TreeViewTest::SetUp();

		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
		m_node2 = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
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

namespace
{

class TreeViewNodeLazyFake : public TreeViewNodeFake
{
public:
	bool GetMayLazyLoadChildren() const override
	{
		return m_mayLazyLoadChildren;
	}

	void SetLoaded(bool loaded)
	{
		m_loaded = loaded;

		UpdateMayLazyLoadChildren();
	}

	void SetChildrenMightExist(bool childrenMightExist)
	{
		m_childrenMightExist = childrenMightExist;

		UpdateMayLazyLoadChildren();
	}

private:
	void UpdateMayLazyLoadChildren()
	{
		bool mayLazyLoadChildren = ComputeMayLazyLoadChildren();

		if (mayLazyLoadChildren == m_mayLazyLoadChildren)
		{
			return;
		}

		m_mayLazyLoadChildren = mayLazyLoadChildren;

		NotifyUpdated(TreeViewNode::Property::MayLazyLoadChildren);
	}

	bool ComputeMayLazyLoadChildren()
	{
		if (m_loaded)
		{
			// It's only possible to lazy load children if they haven't been loaded yet.
			return false;
		}

		return m_childrenMightExist;
	}

	bool m_loaded = false;
	bool m_mayLazyLoadChildren = true;
	bool m_childrenMightExist = true;
};

class TreeViewAdapterLazyFake : public TreeViewAdapter
{
public:
	using TreeViewAdapter::AddNode;

	void OnNodeExpanding(TreeViewNode *node) override
	{
		if (m_shouldAddChildrenOnExpand)
		{
			AddNode(node, std::make_unique<TreeViewNodeLazyFake>());
			AddNode(node, std::make_unique<TreeViewNodeLazyFake>());
			AddNode(node, std::make_unique<TreeViewNodeLazyFake>());
		}

		auto *fakeNode = GetFakeFromNode(node);
		fakeNode->SetLoaded(true);
	}

	void OnNodeCollapsing(TreeViewNode *node) override
	{
		auto rawChildNodes = boost::copy_range<RawTreeViewNodes>(node->GetChildren()
			| boost::adaptors::transformed([](const auto &child) { return child.get(); }));

		for (auto *rawChildNode : rawChildNodes)
		{
			RemoveNode(rawChildNode);
		}

		auto *fakeNode = GetFakeFromNode(node);
		fakeNode->SetLoaded(false);
	}

	void SetShouldAddChildrenOnExpand(bool shouldAddChildrenOnExpand)
	{
		m_shouldAddChildrenOnExpand = shouldAddChildrenOnExpand;
	}

private:
	TreeViewNodeLazyFake *GetFakeFromNode(TreeViewNode *node)
	{
		return static_cast<TreeViewNodeLazyFake *>(node);
	}

	bool m_shouldAddChildrenOnExpand = false;
};

}

class TreeViewLazyLoadingTest : public TreeViewTestBase
{
protected:
	void SetUp() override
	{
		TreeViewTestBase::SetUp();

		m_treeView = std::make_unique<TreeView>(m_treeViewWindow, &m_keyboardState,
			LabelEditHandler::CreateForTest);
		m_treeView->SetAdapter(&m_adapter);
		m_treeView->SetDelegate(&m_delegate);
	}

	void VerifyItems()
	{
		auto viewNodes = m_treeView->GetAllNodesDepthFirstForTesting();

		ConstRawTreeViewNodes adapterNodes;

		for (const auto *currentNode : m_adapter.GetRoot()->GetNodesDepthFirst())
		{
			if (m_adapter.IsRoot(currentNode))
			{
				continue;
			}

			adapterNodes.push_back(currentNode);
		}

		EXPECT_EQ(viewNodes, adapterNodes);
	}

	TreeViewAdapterLazyFake m_adapter;
};

TEST_F(TreeViewLazyLoadingTest, ExpandNodeWithNoChildren)
{
	auto *node = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeLazyFake>());
	m_adapter.SetShouldAddChildrenOnExpand(false);

	// The node doesn't have any children, but it indicates it can lazy load children, so the
	// expander should still be shown.
	EXPECT_TRUE(m_treeView->IsExpanderShownForTesting(node));

	m_treeView->ExpandNode(node);
	EXPECT_FALSE(m_treeView->IsNodeExpanded(node));
	EXPECT_FALSE(m_treeView->IsExpanderShownForTesting(node));
}

TEST_F(TreeViewLazyLoadingTest, ExpandNodeWithChildren)
{
	auto *node = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeLazyFake>());

	m_adapter.SetShouldAddChildrenOnExpand(true);
	m_treeView->ExpandNode(node);
	EXPECT_FALSE(node->GetChildren().empty());
	VerifyItems();
}

TEST_F(TreeViewLazyLoadingTest, CollapseNodeAndRemoveChildren)
{
	auto *node = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<TreeViewNodeLazyFake>());
	m_adapter.SetShouldAddChildrenOnExpand(true);
	m_treeView->ExpandNode(node);

	m_treeView->CollapseNode(node);
	EXPECT_TRUE(node->GetChildren().empty());
	EXPECT_TRUE(m_treeView->IsExpanderShownForTesting(node));
	VerifyItems();
}

TEST_F(TreeViewLazyLoadingTest, MayLazyLoadChildrenUpdate)
{
	auto node = std::make_unique<TreeViewNodeLazyFake>();
	auto *rawNode = node.get();
	m_adapter.AddNode(m_adapter.GetRoot(), std::move(node));

	rawNode->SetChildrenMightExist(false);
	EXPECT_FALSE(m_treeView->IsExpanderShownForTesting(rawNode));

	rawNode->SetChildrenMightExist(true);
	EXPECT_TRUE(m_treeView->IsExpanderShownForTesting(rawNode));
}
