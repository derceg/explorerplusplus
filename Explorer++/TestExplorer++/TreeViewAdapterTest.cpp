// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TreeViewAdapter.h"
#include "TreeViewNodeFake.h"
#include <gtest/gtest.h>

using namespace testing;

namespace
{

class TreeViewAdapterFake : public TreeViewAdapter
{
public:
	TreeViewNodeFake *AddNode(TreeViewNode *parentNode, const std::wstring &text = L"")
	{
		auto node = std::make_unique<TreeViewNodeFake>(text);
		auto *rawNode = node.get();
		TreeViewAdapter::AddNode(parentNode, std::move(node));
		return rawNode;
	}

	using TreeViewAdapter::RemoveNode;

protected:
	std::weak_ordering CompareItems(const TreeViewNode *first,
		const TreeViewNode *second) const override
	{
		const auto *firstFake = GetFakeFromNode(first);
		const auto *secondFake = GetFakeFromNode(second);

		return firstFake->GetText() <=> secondFake->GetText();
	}

private:
	const TreeViewNodeFake *GetFakeFromNode(const TreeViewNode *node) const
	{
		return static_cast<const TreeViewNodeFake *>(node);
	}
};

}

class TreeViewAdapterTest : public Test
{
protected:
	TreeViewAdapterFake m_adapter;
};

TEST_F(TreeViewAdapterTest, MaybeGetNodeById)
{
	auto *node1 = m_adapter.AddNode(m_adapter.GetRoot());
	auto *node2 = m_adapter.AddNode(node1);
	auto *node3 = m_adapter.AddNode(node2);

	for (const auto *node : m_adapter.GetRoot()->GetNodesDepthFirst())
	{
		EXPECT_EQ(m_adapter.MaybeGetNodeById(node->GetId()), node);
	}

	int node1Id = node1->GetId();
	int node2Id = node2->GetId();
	int node3Id = node3->GetId();

	m_adapter.RemoveNode(node1);
	EXPECT_EQ(m_adapter.MaybeGetNodeById(node1Id), nullptr);
	EXPECT_EQ(m_adapter.MaybeGetNodeById(node2Id), nullptr);
	EXPECT_EQ(m_adapter.MaybeGetNodeById(node3Id), nullptr);
}

TEST_F(TreeViewAdapterTest, UpdateSortedPosition)
{
	auto *parentNode = m_adapter.GetRoot();
	auto *node1 = m_adapter.AddNode(parentNode, L"A");
	auto *node2 = m_adapter.AddNode(parentNode, L"B");
	auto *node3 = m_adapter.AddNode(parentNode, L"C");

	node2->SetText(L"M");
	EXPECT_THAT(parentNode->GetChildren(),
		ElementsAre(Pointer(node1), Pointer(node3), Pointer(node2)));
}
