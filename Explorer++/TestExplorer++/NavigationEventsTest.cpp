// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowser/NavigationEvents.h"
#include "BrowserWindowMock.h"
#include "ShellBrowserFake.h"
#include "TabNavigationMock.h"
#include <gtest/gtest.h>

using namespace testing;

class NavigationEventsTest : public Test
{
protected:
	NavigationEventsTest() :
		m_tab1(std::make_unique<ShellBrowserFake>(&m_navigationEvents, &m_tabNavigation1),
			&m_browser1),
		m_tab2(std::make_unique<ShellBrowserFake>(&m_navigationEvents, &m_tabNavigation1),
			&m_browser1),
		m_tab3(std::make_unique<ShellBrowserFake>(&m_navigationEvents, &m_tabNavigation3),
			&m_browser2)
	{
	}

	NavigationEvents m_navigationEvents;

	NiceMock<BrowserWindowMock> m_browser1;
	NiceMock<TabNavigationMock> m_tabNavigation1;
	Tab m_tab1;
	Tab m_tab2;

	NiceMock<BrowserWindowMock> m_browser2;
	NiceMock<TabNavigationMock> m_tabNavigation3;
	Tab m_tab3;

	StrictMock<MockFunction<void(const ShellBrowser *shellBrowser)>> m_stoppedCallback;
};

TEST_F(NavigationEventsTest, Signals)
{
	InSequence seq;

	m_navigationEvents.AddStoppedObserver(m_stoppedCallback.AsStdFunction(),
		NavigationEventScope::Global());
	EXPECT_CALL(m_stoppedCallback, Call(m_tab1.GetShellBrowser()));
	EXPECT_CALL(m_stoppedCallback, Call(m_tab2.GetShellBrowser()));
	EXPECT_CALL(m_stoppedCallback, Call(m_tab3.GetShellBrowser()));

	m_navigationEvents.NotifyStopped(m_tab1.GetShellBrowser());
	m_navigationEvents.NotifyStopped(m_tab2.GetShellBrowser());
	m_navigationEvents.NotifyStopped(m_tab3.GetShellBrowser());
}

TEST_F(NavigationEventsTest, SignalsFilteredByBrowser)
{
	InSequence seq;

	m_navigationEvents.AddStoppedObserver(m_stoppedCallback.AsStdFunction(),
		NavigationEventScope::ForBrowser(m_browser1));
	EXPECT_CALL(m_stoppedCallback, Call(m_tab1.GetShellBrowser()));
	EXPECT_CALL(m_stoppedCallback, Call(m_tab2.GetShellBrowser()));

	m_navigationEvents.NotifyStopped(m_tab1.GetShellBrowser());
	m_navigationEvents.NotifyStopped(m_tab2.GetShellBrowser());
	m_navigationEvents.NotifyStopped(m_tab3.GetShellBrowser());
}

TEST_F(NavigationEventsTest, SignalsFilteredByShellBrowser)
{
	InSequence seq;

	m_navigationEvents.AddStoppedObserver(m_stoppedCallback.AsStdFunction(),
		NavigationEventScope::ForShellBrowser(*m_tab2.GetShellBrowser()));
	EXPECT_CALL(m_stoppedCallback, Call(m_tab2.GetShellBrowser()));

	m_navigationEvents.NotifyStopped(m_tab1.GetShellBrowser());
	m_navigationEvents.NotifyStopped(m_tab2.GetShellBrowser());
	m_navigationEvents.NotifyStopped(m_tab3.GetShellBrowser());
}
