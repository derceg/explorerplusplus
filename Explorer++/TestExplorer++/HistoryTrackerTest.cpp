// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "HistoryTracker.h"
#include "BrowserWindowMock.h"
#include "HistoryModel.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "ShellTestHelper.h"
#include "TabEvents.h"
#include "TabNavigationMock.h"
#include <gtest/gtest.h>

using namespace testing;

class HistoryTrackerTest : public Test
{
protected:
	HistoryTrackerTest() :
		m_historyTracker(&m_historyModel, &m_navigationEvents),
		m_tab1(std::make_unique<ShellBrowserFake>(&m_navigationEvents, &m_tabNavigation1),
			&m_browser1, nullptr, &m_tabEvents),
		m_tab2(std::make_unique<ShellBrowserFake>(&m_navigationEvents, &m_tabNavigation1),
			&m_browser1, nullptr, &m_tabEvents),
		m_tab3(std::make_unique<ShellBrowserFake>(&m_navigationEvents, &m_tabNavigation2),
			&m_browser2, nullptr, &m_tabEvents)
	{
	}

	void NavigateTab(Tab &tab, const PidlAbsolute &pidl)
	{
		auto navigateParams = NavigateParams::Normal(pidl.Raw());
		tab.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);
	}

	TabEvents m_tabEvents;
	NavigationEvents m_navigationEvents;

	HistoryModel m_historyModel;
	HistoryTracker m_historyTracker;

	NiceMock<BrowserWindowMock> m_browser1;
	NiceMock<TabNavigationMock> m_tabNavigation1;
	Tab m_tab1;
	Tab m_tab2;

	NiceMock<BrowserWindowMock> m_browser2;
	NiceMock<TabNavigationMock> m_tabNavigation2;
	Tab m_tab3;
};

TEST_F(HistoryTrackerTest, CheckNavigationsAdded)
{
	const auto &history = m_historyModel.GetHistoryItems();
	EXPECT_EQ(history.size(), 0U);

	MockFunction<void()> callback;
	m_historyModel.AddHistoryChangedObserver(callback.AsStdFunction());
	EXPECT_CALL(callback, Call()).Times(3);

	auto pidlFake1 = CreateSimplePidlForTest(L"C:\\Fake1");
	NavigateTab(m_tab1, pidlFake1);
	ASSERT_EQ(history.size(), 1U);
	EXPECT_EQ(history[0], pidlFake1);

	auto pidlFake2 = CreateSimplePidlForTest(L"C:\\Fake2");
	NavigateTab(m_tab2, pidlFake2);
	ASSERT_EQ(history.size(), 2U);
	EXPECT_EQ(history[0], pidlFake2);

	auto pidlFake3 = CreateSimplePidlForTest(L"C:\\Fake3");
	NavigateTab(m_tab3, pidlFake3);
	ASSERT_EQ(history.size(), 3U);
	EXPECT_EQ(history[0], pidlFake3);
}
