// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabEvents.h"
#include "BrowserWindowMock.h"
#include "NavigationRequestTestHelper.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "ShellTestHelper.h"
#include "TabNavigationMock.h"
#include <gtest/gtest.h>

using namespace testing;

class TabEventsTest : public Test
{
protected:
	TabEventsTest() :
		m_tab1(std::make_unique<ShellBrowserFake>(&m_navigationEvents, &m_tabNavigation1),
			&m_browser1),
		m_tab2(std::make_unique<ShellBrowserFake>(&m_navigationEvents, &m_tabNavigation2),
			&m_browser2)
	{
	}

	TabEvents m_tabEvents;
	NavigationEvents m_navigationEvents;

	NiceMock<BrowserWindowMock> m_browser1;
	NiceMock<TabNavigationMock> m_tabNavigation1;
	Tab m_tab1;

	NiceMock<BrowserWindowMock> m_browser2;
	NiceMock<TabNavigationMock> m_tabNavigation2;
	Tab m_tab2;
};

class TabEventsTabSignalTest : public TabEventsTest
{
protected:
	StrictMock<MockFunction<void(const Tab &tab, bool selected)>> m_tabCreatedCallback;
	StrictMock<MockFunction<void(const Tab &tab)>> m_tabSelectedCallback;
	StrictMock<MockFunction<void(const Tab &tab, int fromIndex, int toIndex)>> m_tabMovedCallback;
	StrictMock<MockFunction<void(const Tab &tab, int index)>> m_tabPreRemovalCallback;
	StrictMock<MockFunction<void(const Tab &tab)>> m_tabRemovedCallback;
};

TEST_F(TabEventsTabSignalTest, Signals)
{
	InSequence seq;

	m_tabEvents.AddCreatedObserver(m_tabCreatedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(m_tabCreatedCallback, Call(Ref(m_tab1), true));
	EXPECT_CALL(m_tabCreatedCallback, Call(Ref(m_tab2), false));

	m_tabEvents.AddSelectedObserver(m_tabSelectedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(m_tabSelectedCallback, Call(Ref(m_tab1)));

	m_tabEvents.AddMovedObserver(m_tabMovedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(m_tabMovedCallback, Call(Ref(m_tab2), 1, 2));

	m_tabEvents.AddPreRemovalObserver(m_tabPreRemovalCallback.AsStdFunction(),
		TabEventScope::Global());
	EXPECT_CALL(m_tabPreRemovalCallback, Call(Ref(m_tab2), 0));

	m_tabEvents.AddRemovedObserver(m_tabRemovedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(m_tabRemovedCallback, Call(Ref(m_tab2)));
	EXPECT_CALL(m_tabRemovedCallback, Call(Ref(m_tab1)));

	m_tabEvents.NotifyCreated(m_tab1, true);
	m_tabEvents.NotifyCreated(m_tab2, false);
	m_tabEvents.NotifySelected(m_tab1);
	m_tabEvents.NotifyMoved(m_tab2, 1, 2);
	m_tabEvents.NotifyPreRemoval(m_tab2, 0);
	m_tabEvents.NotifyRemoved(m_tab2);
	m_tabEvents.NotifyRemoved(m_tab1);
}

TEST_F(TabEventsTabSignalTest, SignalsFilteredByBrowser)
{
	InSequence seq;

	// The observer here should only be triggered when a tab event in m_browser1 occurs. That is,
	// only when m_tab1 is created.
	m_tabEvents.AddCreatedObserver(m_tabCreatedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(&m_browser1));
	EXPECT_CALL(m_tabCreatedCallback, Call(Ref(m_tab1), false));

	m_tabEvents.AddSelectedObserver(m_tabSelectedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(&m_browser1));
	EXPECT_CALL(m_tabSelectedCallback, Call(Ref(m_tab1)));

	// Likewise, the observer here should only be triggered when a tab event in m_browser2 occurs.
	m_tabEvents.AddMovedObserver(m_tabMovedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(&m_browser2));
	EXPECT_CALL(m_tabMovedCallback, Call(Ref(m_tab2), 3, 4));

	m_tabEvents.AddPreRemovalObserver(m_tabPreRemovalCallback.AsStdFunction(),
		TabEventScope::ForBrowser(&m_browser2));
	EXPECT_CALL(m_tabPreRemovalCallback, Call(Ref(m_tab2), 0));

	m_tabEvents.AddRemovedObserver(m_tabRemovedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(&m_browser1));
	EXPECT_CALL(m_tabRemovedCallback, Call(Ref(m_tab1)));

	m_tabEvents.NotifyCreated(m_tab1, false);
	m_tabEvents.NotifyCreated(m_tab2, false);

	m_tabEvents.NotifySelected(m_tab1);
	m_tabEvents.NotifySelected(m_tab2);

	m_tabEvents.NotifyMoved(m_tab1, 1, 2);
	m_tabEvents.NotifyMoved(m_tab2, 3, 4);

	m_tabEvents.NotifyPreRemoval(m_tab1, 0);
	m_tabEvents.NotifyPreRemoval(m_tab2, 0);

	m_tabEvents.NotifyRemoved(m_tab1);
	m_tabEvents.NotifyRemoved(m_tab2);
}

class TabEventsNavigationSignalTest : public TabEventsTest
{
protected:
	TabEventsNavigationSignalTest()
	{
		m_tabEvents.NotifyCreated(m_tab1, false);
		m_tabEvents.NotifyCreated(m_tab2, false);
	}

	StrictMock<MockFunction<void(const Tab &tab, const NavigationRequest *request)>>
		m_tabNavigationStartedCallback;
	StrictMock<MockFunction<void(const Tab &tab, const NavigationRequest *request)>>
		m_tabNavigationCommittedCallback;
	StrictMock<MockFunction<void(const Tab &tab, const NavigationRequest *request)>>
		m_tabNavigationFailedCallback;
};

TEST_F(TabEventsNavigationSignalTest, Signals)
{
	InSequence seq;

	PidlAbsolute pidl1 = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams1 = NavigateParams::Normal(pidl1.Raw());

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"d:\\");
	auto navigateParams2 = NavigateParams::Normal(pidl2.Raw());

	PidlAbsolute pidl3 = CreateSimplePidlForTest(L"e:\\");
	auto navigateParams3 = NavigateParams::Normal(pidl3.Raw());

	PidlAbsolute pidl4 = CreateSimplePidlForTest(L"f:\\");
	auto navigateParams4 = NavigateParams::Normal(pidl4.Raw());

	m_tabEvents.AddNavigationStartedObserver(m_tabNavigationStartedCallback.AsStdFunction(),
		TabEventScope::Global());
	m_tabEvents.AddNavigationCommittedObserver(m_tabNavigationCommittedCallback.AsStdFunction(),
		TabEventScope::Global());
	m_tabEvents.AddNavigationFailedObserver(m_tabNavigationFailedCallback.AsStdFunction(),
		TabEventScope::Global());

	EXPECT_CALL(m_tabNavigationStartedCallback,
		Call(Ref(m_tab1), NavigateParamsMatch(navigateParams1)));
	EXPECT_CALL(m_tabNavigationCommittedCallback,
		Call(Ref(m_tab1), NavigateParamsMatch(navigateParams1)));
	EXPECT_CALL(m_tabNavigationStartedCallback,
		Call(Ref(m_tab2), NavigateParamsMatch(navigateParams2)));
	EXPECT_CALL(m_tabNavigationCommittedCallback,
		Call(Ref(m_tab2), NavigateParamsMatch(navigateParams2)));

	EXPECT_CALL(m_tabNavigationStartedCallback,
		Call(Ref(m_tab1), NavigateParamsMatch(navigateParams3)));
	EXPECT_CALL(m_tabNavigationFailedCallback,
		Call(Ref(m_tab1), NavigateParamsMatch(navigateParams3)));
	EXPECT_CALL(m_tabNavigationStartedCallback,
		Call(Ref(m_tab2), NavigateParamsMatch(navigateParams4)));
	EXPECT_CALL(m_tabNavigationFailedCallback,
		Call(Ref(m_tab2), NavigateParamsMatch(navigateParams4)));

	m_tab1.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams1);
	m_tab2.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams2);

	static_cast<ShellBrowserFake *>(m_tab1.GetShellBrowser())->SetShouldEnumerationSucceed(false);
	static_cast<ShellBrowserFake *>(m_tab2.GetShellBrowser())->SetShouldEnumerationSucceed(false);
	m_tab1.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams3);
	m_tab2.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams4);
}

TEST_F(TabEventsNavigationSignalTest, SignalsFilteredByBrowser)
{
	InSequence seq;

	PidlAbsolute pidl1 = CreateSimplePidlForTest(L"c:\\");
	auto navigateParams1 = NavigateParams::Normal(pidl1.Raw());

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"d:\\");
	auto navigateParams2 = NavigateParams::Normal(pidl2.Raw());

	PidlAbsolute pidl3 = CreateSimplePidlForTest(L"e:\\");
	auto navigateParams3 = NavigateParams::Normal(pidl3.Raw());

	PidlAbsolute pidl4 = CreateSimplePidlForTest(L"f:\\");
	auto navigateParams4 = NavigateParams::Normal(pidl4.Raw());

	m_tabEvents.AddNavigationStartedObserver(m_tabNavigationStartedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(&m_browser1));
	m_tabEvents.AddNavigationCommittedObserver(m_tabNavigationCommittedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(&m_browser2));
	m_tabEvents.AddNavigationFailedObserver(m_tabNavigationFailedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(&m_browser1));

	EXPECT_CALL(m_tabNavigationStartedCallback,
		Call(Ref(m_tab1), NavigateParamsMatch(navigateParams1)));
	EXPECT_CALL(m_tabNavigationCommittedCallback,
		Call(Ref(m_tab2), NavigateParamsMatch(navigateParams2)));
	EXPECT_CALL(m_tabNavigationStartedCallback,
		Call(Ref(m_tab1), NavigateParamsMatch(navigateParams3)));
	EXPECT_CALL(m_tabNavigationFailedCallback,
		Call(Ref(m_tab1), NavigateParamsMatch(navigateParams3)));

	m_tab1.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams1);
	m_tab2.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams2);

	static_cast<ShellBrowserFake *>(m_tab1.GetShellBrowser())->SetShouldEnumerationSucceed(false);
	static_cast<ShellBrowserFake *>(m_tab2.GetShellBrowser())->SetShouldEnumerationSucceed(false);
	m_tab1.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams3);
	m_tab2.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams4);
}
