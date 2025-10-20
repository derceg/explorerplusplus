// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TreeViewAdapter.h"
#include "TreeViewNodeFake.h"
#include <gtest/gtest.h>

namespace
{

class TreeViewAdapterFake : public TreeViewAdapter
{
public:
	using TreeViewAdapter::AddNode;
	using TreeViewAdapter::RemoveNode;
};

}

TEST(TreeViewAdapterTest, MaybeGetNodeById)
{
	TreeViewAdapterFake adapter;
	auto *node1 = adapter.AddNode(adapter.GetRoot(), std::make_unique<TreeViewNodeFake>());
	auto *node2 = adapter.AddNode(node1, std::make_unique<TreeViewNodeFake>());
	auto *node3 = adapter.AddNode(node2, std::make_unique<TreeViewNodeFake>());

	for (const auto *node : adapter.GetRoot()->GetNodesDepthFirst())
	{
		EXPECT_EQ(adapter.MaybeGetNodeById(node->GetId()), node);
	}

	int node1Id = node1->GetId();
	int node2Id = node2->GetId();
	int node3Id = node3->GetId();

	adapter.RemoveNode(node1);
	EXPECT_EQ(adapter.MaybeGetNodeById(node1Id), nullptr);
	EXPECT_EQ(adapter.MaybeGetNodeById(node2Id), nullptr);
	EXPECT_EQ(adapter.MaybeGetNodeById(node3Id), nullptr);
}
