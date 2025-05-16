// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Helper/ClipboardHelper.h"
#include "ShellTestHelper.h"
#include "SimulatedClipboardStore.h"
#include "../Helper/Clipboard.h"
#include <gtest/gtest.h>

using namespace testing;

TEST(ClipboardHelperTest, CopyItemPathsToClipboard)
{
	SimulatedClipboardStore clipboardStore;

	std::vector<PidlAbsolute> items;
	items.push_back(CreateSimplePidlForTest(L"c:\\item1"));
	items.push_back(CreateSimplePidlForTest(L"c:\\item2"));
	items.push_back(CreateSimplePidlForTest(L"c:\\item3"));
	CopyItemPathsToClipboard(&clipboardStore, items);

	Clipboard clipboard(&clipboardStore);
	auto clipboardText = clipboard.ReadText();
	ASSERT_TRUE(clipboardText.has_value());
	EXPECT_THAT(*clipboardText, StrCaseEq(L"c:\\item1\r\nc:\\item2\r\nc:\\item3"));
}
