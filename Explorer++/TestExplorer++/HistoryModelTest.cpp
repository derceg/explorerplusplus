// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "HistoryModel.h"
#include "ShellTestHelper.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>

using namespace testing;

TEST(HistoryModelTest, RepeatedNavigation)
{
	HistoryModel historyModel;
	const auto &history = historyModel.GetHistoryItems();

	PidlAbsolute pidl = CreateSimplePidlForTest(L"C:\\Fake");
	historyModel.AddHistoryItem(pidl);
	EXPECT_EQ(history.size(), 1U);

	MockFunction<void()> callback;
	historyModel.AddHistoryChangedObserver(callback.AsStdFunction());
	EXPECT_CALL(callback, Call()).Times(0);

	// A repeated navigation to the most recent entry should be ignored.
	historyModel.AddHistoryItem(pidl);
	EXPECT_EQ(history.size(), 1U);
}
