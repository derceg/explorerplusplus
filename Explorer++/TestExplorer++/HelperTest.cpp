// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/Helper.h"
#include <gtest/gtest.h>

using namespace testing;

TEST(BuildFileAttributesString, Empty)
{
	auto attributesString = BuildFileAttributesString(0);
	EXPECT_EQ(attributesString, L"---------");
}

TEST(BuildFileAttributesString, Simple)
{
	auto attributesString =
		BuildFileAttributesString(FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN);
	EXPECT_EQ(attributesString, L"AH-------");

	attributesString =
		BuildFileAttributesString(FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_COMPRESSED);
	EXPECT_EQ(attributesString, L"----DC---");
}

TEST(BuildFileAttributesString, All)
{
	auto attributesString = BuildFileAttributesString(FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN
		| FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY
		| FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE | FILE_ATTRIBUTE_ENCRYPTED
		| FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
	EXPECT_EQ(attributesString, L"AHRSDCPEI");
}

TEST(IsProcessRTL, CheckValue)
{
	// The layout will be LTR by default.
	EXPECT_EQ(IsProcessRTL(), false);

	SetProcessDefaultLayout(LAYOUT_RTL);
	EXPECT_EQ(IsProcessRTL(), true);
}

TEST(HelperTest, GetExpandedEnvironmentVariable)
{
	// Nested environment variable expansion.
	auto set = SetEnvironmentVariable(L"FOO", L"FOO_VALUE");
	ASSERT_TRUE(set);
	set = SetEnvironmentVariable(L"BAR", L"%FOO%/BAR_VALUE");
	ASSERT_TRUE(set);
	auto bar = GetExpandedEnvironmentVariable(L"BAR");
	ASSERT_TRUE(bar.has_value());
	EXPECT_EQ(bar.value(), L"FOO_VALUE/BAR_VALUE");
}

TEST(HelperTest, MoveVectorItem)
{
	std::vector<char> items = { 'a', 'b', 'c', 'd' };

	MoveVectorItem(items, 0, 2);
	EXPECT_THAT(items, ElementsAre('b', 'c', 'a', 'd'));

	MoveVectorItem(items, 0, 3);
	EXPECT_THAT(items, ElementsAre('c', 'a', 'd', 'b'));

	MoveVectorItem(items, 1, 2);
	EXPECT_THAT(items, ElementsAre('c', 'd', 'a', 'b'));

	MoveVectorItem(items, 3, 2);
	EXPECT_THAT(items, ElementsAre('c', 'd', 'b', 'a'));

	MoveVectorItem(items, 3, 0);
	EXPECT_THAT(items, ElementsAre('a', 'c', 'd', 'b'));
}
