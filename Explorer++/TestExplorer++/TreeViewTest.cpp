// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TreeView.h"
#include "TreeViewAdapter.h"
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
		return false;
	}

	bool CanRemove() const override
	{
		return false;
	}
};

class TreeViewDelegateMock : public TreeViewDelegate
{
public:
	MOCK_METHOD(bool, OnNodeRenamed, (TreeViewNode * targetNode, const std::wstring &name),
		(override));
	MOCK_METHOD(void, OnNodeRemoved, (TreeViewNode * targetNode), (override));
	MOCK_METHOD(void, OnNodeCopied, (TreeViewNode * targetNode), (override));
	MOCK_METHOD(void, OnNodeCut, (TreeViewNode * targetNode), (override));
	MOCK_METHOD(void, OnPaste, (TreeViewNode * targetNode), (override));
	MOCK_METHOD(void, OnSelectionChanged, (TreeViewNode * selectedNode), (override));
	MOCK_METHOD(void, OnShowContextMenu, (TreeViewNode * targetNode, const POINT &ptScreen),
		(override));
	MOCK_METHOD(void, OnBeginDrag, (TreeViewNode * targetNode), (override));
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

		m_treeViewWindow.reset(CreateWindow(WC_TREEVIEW, L"", WS_POPUP, 0, 0, 0, 0,
			m_parentWindow.get(), nullptr, GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_treeViewWindow, nullptr);

		m_treeView = std::make_unique<TreeView>(m_treeViewWindow.get());
		m_treeView->SetAdapter(&m_adapter);
		m_treeView->SetDelegate(&m_delegate);
	}

	wil::unique_hwnd m_parentWindow;
	wil::unique_hwnd m_treeViewWindow;
	TreeViewAdapter m_adapter;
	TreeViewDelegateMock m_delegate;
	std::unique_ptr<TreeView> m_treeView;
};

TEST_F(TreeViewTest, MoveNodeCollapsesParent)
{
	auto *topLevelNode1 =
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	auto *childNode = m_adapter.AddNode(topLevelNode1, std::make_unique<FakeTreeViewNode>());
	auto *topLevelNode2 =
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	m_treeView->ExpandNode(topLevelNode1);

	// Since topLevelNode1 no longer has any children after the move, it should be collapsed.
	m_adapter.MoveNode(childNode, topLevelNode2, 0);
	EXPECT_FALSE(m_treeView->IsNodeExpanded(topLevelNode1));
}

TEST_F(TreeViewTest, MoveNodeDoesntCollapseParent)
{
	auto *topLevelNode1 =
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	auto *childNode1 = m_adapter.AddNode(topLevelNode1, std::make_unique<FakeTreeViewNode>());
	m_adapter.AddNode(topLevelNode1, std::make_unique<FakeTreeViewNode>());
	auto *topLevelNode2 =
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	m_treeView->ExpandNode(topLevelNode1);

	// topLevelNode1 still has a child, so it shouldn't be collapsed.
	m_adapter.MoveNode(childNode1, topLevelNode2, 0);
	EXPECT_TRUE(m_treeView->IsNodeExpanded(topLevelNode1));
}

TEST_F(TreeViewTest, MoveSelectedNode)
{
	auto *topLevelNode1 =
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	auto *topLevelNode2 =
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	m_treeView->SelectNode(topLevelNode2);

	// The node being moved is selected, so it should still be selected after the move and no
	// selection change notification should be generated.
	EXPECT_CALL(m_delegate, OnSelectionChanged(_)).Times(0);
	m_adapter.MoveNode(topLevelNode2, topLevelNode1, 0);
	EXPECT_EQ(m_treeView->GetSelectedNode(), topLevelNode2);
}

TEST_F(TreeViewTest, MoveNonSelectedNode)
{
	auto *topLevelNode1 =
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	auto *topLevelNode2 =
		m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	m_treeView->SelectNode(topLevelNode1);

	// The node being moved isn't selected, so the selection shouldn't change and no selection
	// change notification should be generated.
	EXPECT_CALL(m_delegate, OnSelectionChanged(_)).Times(0);
	m_adapter.MoveNode(topLevelNode2, topLevelNode1, 0);
	EXPECT_EQ(m_treeView->GetSelectedNode(), topLevelNode1);
}

TEST_F(TreeViewTest, ExpandNodeWithNoChildren)
{
	// The node has no children, so attempting to expand it should have no effect.
	const auto *node = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());
	m_treeView->ExpandNode(node);
	EXPECT_FALSE(m_treeView->IsNodeExpanded(node));
}

TEST_F(TreeViewTest, HighlightedNode)
{
	const auto *node = m_adapter.AddNode(m_adapter.GetRoot(), std::make_unique<FakeTreeViewNode>());

	m_treeView->HighlightNode(node);
	EXPECT_TRUE(m_treeView->IsNodeHighlighted(node));

	m_treeView->UnhighlightNode(node);
	EXPECT_FALSE(m_treeView->IsNodeHighlighted(node));
}
