// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ShellContextMenuIdGenerator.h"
#include "ShellContextMenuDelegateFake.h"
#include <gtest/gtest.h>

TEST(ShellContextMenuIdGeneratorTest, MaybeGetDelegateForId)
{
	ShellContextMenuIdGenerator idGenerator(1);
	ShellContextMenuDelegateFake delegate1;
	ShellContextMenuDelegateFake delegate2;

	UINT id1 = idGenerator.GetNextId(&delegate1);
	UINT id2 = idGenerator.GetNextId(&delegate2);

	EXPECT_EQ(idGenerator.MaybeGetDelegateForId(id1), &delegate1);
	EXPECT_EQ(idGenerator.MaybeGetDelegateForId(id2), &delegate2);

	// These IDs haven't been generated, so no delegates should be returned.
	EXPECT_EQ(idGenerator.MaybeGetDelegateForId(1000), nullptr);
	EXPECT_EQ(idGenerator.MaybeGetDelegateForId(2000), nullptr);
}
