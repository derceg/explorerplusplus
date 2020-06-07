// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "Plugins/AcceleratorParser.h"
#include <gtest/gtest.h>

TEST(AcceleratorParserTest, TestValidCases)
{
	auto res = Plugins::parseAccelerator(L"Ctrl+S");

	EXPECT_TRUE(res);

	EXPECT_EQ(res->modifiers, FVIRTKEY | FCONTROL);
	EXPECT_EQ(res->key, 'S');

	res = Plugins::parseAccelerator(L"Ctrl+Shift+N");

	EXPECT_TRUE(res);

	EXPECT_EQ(res->modifiers, FVIRTKEY | FCONTROL | FSHIFT);
	EXPECT_EQ(res->key, 'N');

	res = Plugins::parseAccelerator(L"Alt+G");

	EXPECT_TRUE(res);

	EXPECT_EQ(res->modifiers, FVIRTKEY | FALT);
	EXPECT_EQ(res->key, 'G');

	res = Plugins::parseAccelerator(L"Alt+Shift+P");

	EXPECT_TRUE(res);

	EXPECT_EQ(res->modifiers, FVIRTKEY | FALT | FSHIFT);
	EXPECT_EQ(res->key, 'P');
}

TEST(AcceleratorParserTest, TestInvalidCases)
{
	auto res = Plugins::parseAccelerator(L"");

	EXPECT_FALSE(res);

	res = Plugins::parseAccelerator(L"A");

	EXPECT_FALSE(res);

	res = Plugins::parseAccelerator(L"Shift");

	EXPECT_FALSE(res);
}