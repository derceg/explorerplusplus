// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsTracker.h"
#include "BrowserWindowMock.h"
#include "FakeSystemClock.h"
#include "FrequentLocationsModel.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "ShellTestHelper.h"
#include "TabEvents.h"
#include "TabNavigationMock.h"
#include <gtest/gtest.h>

using namespace std::chrono_literals;
using namespace testing;

class FrequentLocationsTrackerTest : public Test
{
protected:
	FrequentLocationsTrackerTest() :
		m_frequentLocationsModel(&m_systemClock),
		m_frequentLocationsTracker(&m_frequentLocationsModel, &m_navigationEvents),
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

	FakeSystemClock m_systemClock;
	FrequentLocationsModel m_frequentLocationsModel;
	FrequentLocationsTracker m_frequentLocationsTracker;

	NiceMock<BrowserWindowMock> m_browser1;
	NiceMock<TabNavigationMock> m_tabNavigation1;
	Tab m_tab1;
	Tab m_tab2;

	NiceMock<BrowserWindowMock> m_browser2;
	NiceMock<TabNavigationMock> m_tabNavigation2;
	Tab m_tab3;
};

TEST_F(FrequentLocationsTrackerTest, CheckNavigationsAdded)
{
	auto path1 = CreateSimplePidlForTest(L"c:\\path1");
	NavigateTab(m_tab1, path1);

	auto path2 = CreateSimplePidlForTest(L"c:\\path2");
	NavigateTab(m_tab2, path2);

	auto path3 = CreateSimplePidlForTest(L"c:\\path3");
	NavigateTab(m_tab2, path3);

	std::vector<LocationVisitInfo> expectedVisits = { { path3, 1, SystemClock::TimePoint(2s) },
		{ path2, 1, SystemClock::TimePoint(1s) }, { path1, 1, SystemClock::TimePoint(0s) } };
	EXPECT_THAT(m_frequentLocationsModel.GetVisits(), ElementsAreArray(expectedVisits));
}
