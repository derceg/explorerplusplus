// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "GlobalTabEventDispatcher.h"
#include "BrowserWindowMock.h"
#include "ShellBrowserFake.h"
#include "TabNavigationMock.h"
#include <gtest/gtest.h>

using namespace testing;

class GlobalTabEventDispatcherTest : public Test
{
protected:
	GlobalTabEventDispatcherTest() :
		m_tab1(std::make_unique<ShellBrowserFake>(&m_tabNavigation1), &m_browser1),
		m_tab2(std::make_unique<ShellBrowserFake>(&m_tabNavigation2), &m_browser2)
	{
	}

	GlobalTabEventDispatcher m_dispatcher;

	NiceMock<BrowserWindowMock> m_browser1;
	NiceMock<TabNavigationMock> m_tabNavigation1;
	Tab m_tab1;

	NiceMock<BrowserWindowMock> m_browser2;
	NiceMock<TabNavigationMock> m_tabNavigation2;
	Tab m_tab2;

	MockFunction<void(const Tab &tab)> m_tabSelectedCallback;
	MockFunction<void(const Tab &tab, int index)> m_tabPreRemovalCallback;
};

TEST_F(GlobalTabEventDispatcherTest, Signals)
{
	m_dispatcher.AddSelectedObserver(m_tabSelectedCallback.AsStdFunction());
	EXPECT_CALL(m_tabSelectedCallback, Call(Ref(m_tab1)));

	m_dispatcher.AddPreRemovalObserver(m_tabPreRemovalCallback.AsStdFunction());
	EXPECT_CALL(m_tabPreRemovalCallback, Call(Ref(m_tab2), 0));

	m_dispatcher.NotifySelected(m_tab1);
	m_dispatcher.NotifyPreRemoval(m_tab2, 0);
}

TEST_F(GlobalTabEventDispatcherTest, SignalsFilteredByBrowser)
{
	// The observer here should only be triggered when a tab event in m_browser1 occurs. That is,
	// only when m_tab1 is selected.
	m_dispatcher.AddSelectedObserver(m_tabSelectedCallback.AsStdFunction(), &m_browser1);
	EXPECT_CALL(m_tabSelectedCallback, Call(Ref(m_tab1)));

	// Likewise, the observer here should only be triggered when a tab event in m_browser2 occurs.
	m_dispatcher.AddPreRemovalObserver(m_tabPreRemovalCallback.AsStdFunction(), &m_browser2);
	EXPECT_CALL(m_tabPreRemovalCallback, Call(Ref(m_tab2), 0));

	m_dispatcher.NotifySelected(m_tab1);
	m_dispatcher.NotifySelected(m_tab2);

	m_dispatcher.NotifyPreRemoval(m_tab1, 0);
	m_dispatcher.NotifyPreRemoval(m_tab2, 0);
}
