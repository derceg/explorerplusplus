// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "HistoryService.h"
#include "ShellHelper.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>

using namespace testing;

TEST(HistoryServiceTest, RepeatedNavigation)
{
	HistoryService historyService;
	const auto &history = historyService.GetHistoryItems();

	PidlAbsolute pidl = CreateSimplePidlForTest(L"C:\\Fake");
	historyService.AddHistoryItem(pidl);
	EXPECT_EQ(history.size(), 1U);

	MockFunction<void()> callback;
	historyService.AddHistoryChangedObserver(callback.AsStdFunction());
	EXPECT_CALL(callback, Call()).Times(0);

	// A repeated navigation to the most recent entry should be ignored.
	historyService.AddHistoryItem(pidl);
	EXPECT_EQ(history.size(), 1U);
}
