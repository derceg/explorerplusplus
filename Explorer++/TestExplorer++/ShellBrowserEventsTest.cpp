// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "BrowserWindowMock.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowserFake.h"
#include "Tab.h"
#include "TabNavigationMock.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class ShellBrowserEventsTest : public Test
{
protected:
	using CallBackMock = StrictMock<MockFunction<void(const ShellBrowser *shellBrowser)>>;

	ShellBrowserEventsTest() :
		m_tab1(std::make_unique<ShellBrowserFake>(&m_navigationEvents, &m_tabNavigation1),
			&m_browser1),
		m_tab2(std::make_unique<ShellBrowserFake>(&m_navigationEvents, &m_tabNavigation1),
			&m_browser1),
		m_tab3(std::make_unique<ShellBrowserFake>(&m_navigationEvents, &m_tabNavigation3),
			&m_browser2)
	{
	}

	ShellBrowserEvents m_shellBrowserEvents;
	NavigationEvents m_navigationEvents;

	NiceMock<BrowserWindowMock> m_browser1;
	NiceMock<TabNavigationMock> m_tabNavigation1;
	Tab m_tab1;
	Tab m_tab2;

	NiceMock<BrowserWindowMock> m_browser2;
	NiceMock<TabNavigationMock> m_tabNavigation3;
	Tab m_tab3;

	CallBackMock m_directoryContentsChangedCallback;
	CallBackMock m_directoryPropertiesChangedCallback;
};

TEST_F(ShellBrowserEventsTest, Signals)
{
	m_shellBrowserEvents.AddDirectoryContentsChangedObserver(
		m_directoryContentsChangedCallback.AsStdFunction(), ShellBrowserEventScope::Global());
	EXPECT_CALL(m_directoryContentsChangedCallback, Call(m_tab1.GetShellBrowser()));
	EXPECT_CALL(m_directoryContentsChangedCallback, Call(m_tab2.GetShellBrowser()));
	EXPECT_CALL(m_directoryContentsChangedCallback, Call(m_tab3.GetShellBrowser()));

	m_shellBrowserEvents.AddDirectoryPropertiesChangedObserver(
		m_directoryPropertiesChangedCallback.AsStdFunction(), ShellBrowserEventScope::Global());
	EXPECT_CALL(m_directoryPropertiesChangedCallback, Call(m_tab1.GetShellBrowser()));
	EXPECT_CALL(m_directoryPropertiesChangedCallback, Call(m_tab2.GetShellBrowser()));
	EXPECT_CALL(m_directoryPropertiesChangedCallback, Call(m_tab3.GetShellBrowser()));

	m_shellBrowserEvents.NotifyDirectoryContentsChanged(m_tab1.GetShellBrowser());
	m_shellBrowserEvents.NotifyDirectoryContentsChanged(m_tab2.GetShellBrowser());
	m_shellBrowserEvents.NotifyDirectoryContentsChanged(m_tab3.GetShellBrowser());

	m_shellBrowserEvents.NotifyDirectoryPropertiesChanged(m_tab1.GetShellBrowser());
	m_shellBrowserEvents.NotifyDirectoryPropertiesChanged(m_tab2.GetShellBrowser());
	m_shellBrowserEvents.NotifyDirectoryPropertiesChanged(m_tab3.GetShellBrowser());
}

TEST_F(ShellBrowserEventsTest, SignalsFilteredByBrowser)
{
	m_shellBrowserEvents.AddDirectoryContentsChangedObserver(
		m_directoryContentsChangedCallback.AsStdFunction(),
		ShellBrowserEventScope::ForBrowser(m_browser1));
	EXPECT_CALL(m_directoryContentsChangedCallback, Call(m_tab1.GetShellBrowser()));
	EXPECT_CALL(m_directoryContentsChangedCallback, Call(m_tab2.GetShellBrowser()));

	m_shellBrowserEvents.AddDirectoryPropertiesChangedObserver(
		m_directoryPropertiesChangedCallback.AsStdFunction(),
		ShellBrowserEventScope::ForBrowser(m_browser2));
	EXPECT_CALL(m_directoryPropertiesChangedCallback, Call(m_tab3.GetShellBrowser()));

	m_shellBrowserEvents.NotifyDirectoryContentsChanged(m_tab1.GetShellBrowser());
	m_shellBrowserEvents.NotifyDirectoryContentsChanged(m_tab2.GetShellBrowser());
	m_shellBrowserEvents.NotifyDirectoryContentsChanged(m_tab3.GetShellBrowser());

	m_shellBrowserEvents.NotifyDirectoryPropertiesChanged(m_tab1.GetShellBrowser());
	m_shellBrowserEvents.NotifyDirectoryPropertiesChanged(m_tab2.GetShellBrowser());
	m_shellBrowserEvents.NotifyDirectoryPropertiesChanged(m_tab3.GetShellBrowser());
}

TEST_F(ShellBrowserEventsTest, SignalsFilteredByShellBrowser)
{
	m_shellBrowserEvents.AddDirectoryContentsChangedObserver(
		m_directoryContentsChangedCallback.AsStdFunction(),
		ShellBrowserEventScope::ForShellBrowser(*m_tab1.GetShellBrowser()));
	EXPECT_CALL(m_directoryContentsChangedCallback, Call(m_tab1.GetShellBrowser()));

	m_shellBrowserEvents.AddDirectoryPropertiesChangedObserver(
		m_directoryPropertiesChangedCallback.AsStdFunction(),
		ShellBrowserEventScope::ForShellBrowser(*m_tab2.GetShellBrowser()));
	EXPECT_CALL(m_directoryPropertiesChangedCallback, Call(m_tab2.GetShellBrowser()));

	m_shellBrowserEvents.NotifyDirectoryContentsChanged(m_tab1.GetShellBrowser());
	m_shellBrowserEvents.NotifyDirectoryContentsChanged(m_tab2.GetShellBrowser());
	m_shellBrowserEvents.NotifyDirectoryContentsChanged(m_tab3.GetShellBrowser());

	m_shellBrowserEvents.NotifyDirectoryPropertiesChanged(m_tab1.GetShellBrowser());
	m_shellBrowserEvents.NotifyDirectoryPropertiesChanged(m_tab2.GetShellBrowser());
	m_shellBrowserEvents.NotifyDirectoryPropertiesChanged(m_tab3.GetShellBrowser());
}
