// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "GlobalTabEventDispatcher.h"
#include "BrowserWindowMock.h"
#include "NavigationRequestTestHelper.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "ShellTestHelper.h"
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
};

class GlobalTabEventDispatcherTabSignalTest : public GlobalTabEventDispatcherTest
{
protected:
	StrictMock<MockFunction<void(const Tab &tab, bool selected)>> m_tabCreatedCallback;
	StrictMock<MockFunction<void(const Tab &tab)>> m_tabSelectedCallback;
	StrictMock<MockFunction<void(const Tab &tab, int fromIndex, int toIndex)>> m_tabMovedCallback;
	StrictMock<MockFunction<void(const Tab &tab, int index)>> m_tabPreRemovalCallback;
	StrictMock<MockFunction<void(const Tab &tab)>> m_tabRemovedCallback;
};

TEST_F(GlobalTabEventDispatcherTabSignalTest, Signals)
{
	InSequence seq;

	m_dispatcher.AddCreatedObserver(m_tabCreatedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(m_tabCreatedCallback, Call(Ref(m_tab1), true));
	EXPECT_CALL(m_tabCreatedCallback, Call(Ref(m_tab2), false));

	m_dispatcher.AddSelectedObserver(m_tabSelectedCallback.AsStdFunction(),
		TabEventScope::Global());
	EXPECT_CALL(m_tabSelectedCallback, Call(Ref(m_tab1)));

	m_dispatcher.AddMovedObserver(m_tabMovedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(m_tabMovedCallback, Call(Ref(m_tab2), 1, 2));

	m_dispatcher.AddPreRemovalObserver(m_tabPreRemovalCallback.AsStdFunction(),
		TabEventScope::Global());
	EXPECT_CALL(m_tabPreRemovalCallback, Call(Ref(m_tab2), 0));

	m_dispatcher.AddRemovedObserver(m_tabRemovedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(m_tabRemovedCallback, Call(Ref(m_tab2)));
	EXPECT_CALL(m_tabRemovedCallback, Call(Ref(m_tab1)));

	m_dispatcher.NotifyCreated(m_tab1, true);
	m_dispatcher.NotifyCreated(m_tab2, false);
	m_dispatcher.NotifySelected(m_tab1);
	m_dispatcher.NotifyMoved(m_tab2, 1, 2);
	m_dispatcher.NotifyPreRemoval(m_tab2, 0);
	m_dispatcher.NotifyRemoved(m_tab2);
	m_dispatcher.NotifyRemoved(m_tab1);
}

TEST_F(GlobalTabEventDispatcherTabSignalTest, SignalsFilteredByBrowser)
{
	InSequence seq;

	// The observer here should only be triggered when a tab event in m_browser1 occurs. That is,
	// only when m_tab1 is created.
	m_dispatcher.AddCreatedObserver(m_tabCreatedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(&m_browser1));
	EXPECT_CALL(m_tabCreatedCallback, Call(Ref(m_tab1), false));

	m_dispatcher.AddSelectedObserver(m_tabSelectedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(&m_browser1));
	EXPECT_CALL(m_tabSelectedCallback, Call(Ref(m_tab1)));

	// Likewise, the observer here should only be triggered when a tab event in m_browser2 occurs.
	m_dispatcher.AddMovedObserver(m_tabMovedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(&m_browser2));
	EXPECT_CALL(m_tabMovedCallback, Call(Ref(m_tab2), 3, 4));

	m_dispatcher.AddPreRemovalObserver(m_tabPreRemovalCallback.AsStdFunction(),
		TabEventScope::ForBrowser(&m_browser2));
	EXPECT_CALL(m_tabPreRemovalCallback, Call(Ref(m_tab2), 0));

	m_dispatcher.AddRemovedObserver(m_tabRemovedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(&m_browser1));
	EXPECT_CALL(m_tabRemovedCallback, Call(Ref(m_tab1)));

	m_dispatcher.NotifyCreated(m_tab1, false);
	m_dispatcher.NotifyCreated(m_tab2, false);

	m_dispatcher.NotifySelected(m_tab1);
	m_dispatcher.NotifySelected(m_tab2);

	m_dispatcher.NotifyMoved(m_tab1, 1, 2);
	m_dispatcher.NotifyMoved(m_tab2, 3, 4);

	m_dispatcher.NotifyPreRemoval(m_tab1, 0);
	m_dispatcher.NotifyPreRemoval(m_tab2, 0);

	m_dispatcher.NotifyRemoved(m_tab1);
	m_dispatcher.NotifyRemoved(m_tab2);
}

class GlobalTabEventDispatcherNavigationSignalTest : public GlobalTabEventDispatcherTest
{
protected:
	GlobalTabEventDispatcherNavigationSignalTest()
	{
		m_dispatcher.NotifyCreated(m_tab1, false);
		m_dispatcher.NotifyCreated(m_tab2, false);
	}

	StrictMock<MockFunction<void(const Tab &tab, const NavigationRequest *request)>>
		m_tabNavigationStartedCallback;
};

TEST_F(GlobalTabEventDispatcherNavigationSignalTest, Signals)
{
	PidlAbsolute pidl1 = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams1 = NavigateParams::Normal(pidl1.Raw());

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"d:\\");
	auto navigateParams2 = NavigateParams::Normal(pidl2.Raw());

	m_dispatcher.AddNavigationStartedObserver(m_tabNavigationStartedCallback.AsStdFunction(),
		TabEventScope::Global());
	EXPECT_CALL(m_tabNavigationStartedCallback,
		Call(Ref(m_tab1), NavigateParamsMatch(navigateParams1)));
	EXPECT_CALL(m_tabNavigationStartedCallback,
		Call(Ref(m_tab2), NavigateParamsMatch(navigateParams2)));

	m_tab1.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams1);
	m_tab2.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams2);
}

TEST_F(GlobalTabEventDispatcherNavigationSignalTest, SignalsFilteredByBrowser)
{
	PidlAbsolute pidl1 = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams1 = NavigateParams::Normal(pidl1.Raw());

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"d:\\");
	auto navigateParams2 = NavigateParams::Normal(pidl2.Raw());

	m_dispatcher.AddNavigationStartedObserver(m_tabNavigationStartedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(&m_browser1));
	EXPECT_CALL(m_tabNavigationStartedCallback,
		Call(Ref(m_tab1), NavigateParamsMatch(navigateParams1)));

	m_tab1.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams1);
	m_tab2.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams2);
}
