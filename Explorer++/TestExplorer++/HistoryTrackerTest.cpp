// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "HistoryTracker.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "HistoryModel.h"
#include <gtest/gtest.h>

using namespace testing;

class HistoryTrackerTest : public BrowserTestBase
{
protected:
	HistoryTrackerTest() : m_historyTracker(&m_historyModel, &m_navigationEvents)
	{
	}

	HistoryModel m_historyModel;
	HistoryTracker m_historyTracker;
};

TEST_F(HistoryTrackerTest, CheckNavigationsAdded)
{
	const auto &history = m_historyModel.GetHistoryItems();
	EXPECT_EQ(history.size(), 0U);

	MockFunction<void()> callback;
	m_historyModel.AddHistoryChangedObserver(callback.AsStdFunction());
	EXPECT_CALL(callback, Call()).Times(3);

	auto *browser1 = AddBrowser();

	PidlAbsolute pidl1;
	browser1->AddTab(L"c:\\fake1", {}, &pidl1);
	ASSERT_EQ(history.size(), 1U);
	EXPECT_EQ(history[0], pidl1);

	PidlAbsolute pidl2;
	browser1->AddTab(L"c:\\path2", {}, &pidl2);
	ASSERT_EQ(history.size(), 2U);
	EXPECT_EQ(history[0], pidl2);

	auto *browser2 = AddBrowser();

	PidlAbsolute pidl3;
	browser2->AddTab(L"c:\\path3", {}, &pidl3);
	ASSERT_EQ(history.size(), 3U);
	EXPECT_EQ(history[0], pidl3);
}
