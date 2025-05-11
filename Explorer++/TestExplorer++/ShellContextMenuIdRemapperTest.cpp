// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ShellContextMenuIdRemapper.h"
#include "ShellContextMenuDelegateFake.h"
#include "../Helper/ShellContextMenuIdGenerator.h"
#include <gtest/gtest.h>

using namespace testing;

class ShellContextMenuIdRemapperTest : public Test
{
protected:
	ShellContextMenuIdRemapperTest() : m_idGenerator(1), m_idRemapper(&m_delegate, &m_idGenerator)
	{
	}

	ShellContextMenuIdGenerator m_idGenerator;
	ShellContextMenuDelegateFake m_delegate;
	ShellContextMenuIdRemapper m_idRemapper;
};

TEST_F(ShellContextMenuIdRemapperTest, GenerateUpdatedId)
{
	UINT updatedId1 = m_idRemapper.GenerateUpdatedId(100);
	UINT updatedId2 = m_idRemapper.GenerateUpdatedId(200);
	EXPECT_NE(updatedId1, updatedId2);

	// Generating an ID for the same item should yield the same result.
	UINT updatedId3 = m_idRemapper.GenerateUpdatedId(100);
	EXPECT_EQ(updatedId3, updatedId1);

	UINT updatedId4 = m_idRemapper.GenerateUpdatedId(200);
	EXPECT_EQ(updatedId4, updatedId2);
}

TEST_F(ShellContextMenuIdRemapperTest, GetOriginalId)
{
	UINT originalId = 100;
	UINT updatedId = m_idRemapper.GenerateUpdatedId(originalId);
	EXPECT_EQ(m_idRemapper.GetOriginalId(updatedId), originalId);
}

TEST_F(ShellContextMenuIdRemapperTest, GetUpdatedId)
{
	UINT originalId = 100;
	UINT updatedId = m_idRemapper.GenerateUpdatedId(originalId);
	EXPECT_EQ(m_idRemapper.GetUpdatedId(originalId), updatedId);
}
