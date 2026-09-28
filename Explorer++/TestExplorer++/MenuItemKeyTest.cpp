// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MenuItemKey.h"
#include <gtest/gtest.h>

namespace
{

enum class Enum1
{
	Value0 = 0,
	Value1 = 1
};

enum class Enum2
{
	Value0 = 0,
	Value1 = 1
};

}

TEST(MenuItemKeyTest, Equality)
{
	// The types and values here are the same, so these should compare equal.
	EXPECT_EQ(MenuItemKey(Enum1::Value0), MenuItemKey(Enum1::Value0));

	// The values here are different, so these should compare unequal.
	EXPECT_NE(MenuItemKey(Enum1::Value0), MenuItemKey(Enum1::Value1));

	// The underlying values here are the same, but the types are different, so these should also
	// compare unequal.
	EXPECT_NE(MenuItemKey(Enum1::Value0), MenuItemKey(Enum2::Value0));
}
