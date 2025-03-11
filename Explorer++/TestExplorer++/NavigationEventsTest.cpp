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

	StrictMock<
		MockFunction<void(const ShellBrowser *shellBrowser, const NavigationRequest *request)>>
		m_startedCallback;
	StrictMock<
		MockFunction<void(const ShellBrowser *shellBrowser, const NavigationRequest *request)>>
		m_willCommitCallback;
	StrictMock<
		MockFunction<void(const ShellBrowser *shellBrowser, const NavigationRequest *request)>>
		m_committedCallback;
	StrictMock<
		MockFunction<void(const ShellBrowser *shellBrowser, const NavigationRequest *request)>>
		m_failedCallback;
	StrictMock<
		MockFunction<void(const ShellBrowser *shellBrowser, const NavigationRequest *request)>>
		m_cancelledCallback;

	StrictMock<MockFunction<void(const ShellBrowser *shellBrowser)>> m_stoppedCallback;
};

TEST_F(NavigationEventsTest, Signals)
{
	InSequence seq;

	m_navigationEvents.AddStartedObserver(m_startedCallback.AsStdFunction(),
		NavigationEventScope::Global());
	EXPECT_CALL(m_startedCallback, Call(m_tab1.GetShellBrowser(), nullptr));
	EXPECT_CALL(m_startedCallback, Call(m_tab2.GetShellBrowser(), nullptr));
	EXPECT_CALL(m_startedCallback, Call(m_tab3.GetShellBrowser(), nullptr));

	m_navigationEvents.AddWillCommitObserver(m_willCommitCallback.AsStdFunction(),
		NavigationEventScope::Global());
	EXPECT_CALL(m_willCommitCallback, Call(m_tab1.GetShellBrowser(), nullptr));
	EXPECT_CALL(m_willCommitCallback, Call(m_tab2.GetShellBrowser(), nullptr));
	EXPECT_CALL(m_willCommitCallback, Call(m_tab3.GetShellBrowser(), nullptr));

	m_navigationEvents.AddCommittedObserver(m_committedCallback.AsStdFunction(),
		NavigationEventScope::Global());
	EXPECT_CALL(m_committedCallback, Call(m_tab1.GetShellBrowser(), nullptr));
	EXPECT_CALL(m_committedCallback, Call(m_tab2.GetShellBrowser(), nullptr));
	EXPECT_CALL(m_committedCallback, Call(m_tab3.GetShellBrowser(), nullptr));

	m_navigationEvents.AddFailedObserver(m_failedCallback.AsStdFunction(),
		NavigationEventScope::Global());
	EXPECT_CALL(m_failedCallback, Call(m_tab1.GetShellBrowser(), nullptr));
	EXPECT_CALL(m_failedCallback, Call(m_tab2.GetShellBrowser(), nullptr));
	EXPECT_CALL(m_failedCallback, Call(m_tab3.GetShellBrowser(), nullptr));

	m_navigationEvents.AddCancelledObserver(m_cancelledCallback.AsStdFunction(),
		NavigationEventScope::Global());
	EXPECT_CALL(m_cancelledCallback, Call(m_tab1.GetShellBrowser(), nullptr));
	EXPECT_CALL(m_cancelledCallback, Call(m_tab2.GetShellBrowser(), nullptr));
	EXPECT_CALL(m_cancelledCallback, Call(m_tab3.GetShellBrowser(), nullptr));

	m_navigationEvents.AddStoppedObserver(m_stoppedCallback.AsStdFunction(),
		NavigationEventScope::Global());
	EXPECT_CALL(m_stoppedCallback, Call(m_tab1.GetShellBrowser()));
	EXPECT_CALL(m_stoppedCallback, Call(m_tab2.GetShellBrowser()));
	EXPECT_CALL(m_stoppedCallback, Call(m_tab3.GetShellBrowser()));

	m_navigationEvents.NotifyStarted(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyStarted(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyStarted(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyWillCommit(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyWillCommit(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyWillCommit(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyCommitted(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyCommitted(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyCommitted(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyFailed(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyFailed(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyFailed(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyCancelled(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyCancelled(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyCancelled(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyStopped(m_tab1.GetShellBrowser());
	m_navigationEvents.NotifyStopped(m_tab2.GetShellBrowser());
	m_navigationEvents.NotifyStopped(m_tab3.GetShellBrowser());
}

TEST_F(NavigationEventsTest, SignalsFilteredByBrowser)
{
	InSequence seq;

	m_navigationEvents.AddStartedObserver(m_startedCallback.AsStdFunction(),
		NavigationEventScope::ForBrowser(m_browser1));
	EXPECT_CALL(m_startedCallback, Call(m_tab1.GetShellBrowser(), nullptr));
	EXPECT_CALL(m_startedCallback, Call(m_tab2.GetShellBrowser(), nullptr));

	m_navigationEvents.AddWillCommitObserver(m_willCommitCallback.AsStdFunction(),
		NavigationEventScope::ForBrowser(m_browser2));
	EXPECT_CALL(m_willCommitCallback, Call(m_tab3.GetShellBrowser(), nullptr));

	m_navigationEvents.AddCommittedObserver(m_committedCallback.AsStdFunction(),
		NavigationEventScope::ForBrowser(m_browser1));
	EXPECT_CALL(m_committedCallback, Call(m_tab1.GetShellBrowser(), nullptr));
	EXPECT_CALL(m_committedCallback, Call(m_tab2.GetShellBrowser(), nullptr));

	m_navigationEvents.AddFailedObserver(m_failedCallback.AsStdFunction(),
		NavigationEventScope::ForBrowser(m_browser2));
	EXPECT_CALL(m_failedCallback, Call(m_tab3.GetShellBrowser(), nullptr));

	m_navigationEvents.AddCancelledObserver(m_cancelledCallback.AsStdFunction(),
		NavigationEventScope::ForBrowser(m_browser1));
	EXPECT_CALL(m_cancelledCallback, Call(m_tab1.GetShellBrowser(), nullptr));
	EXPECT_CALL(m_cancelledCallback, Call(m_tab2.GetShellBrowser(), nullptr));

	m_navigationEvents.AddStoppedObserver(m_stoppedCallback.AsStdFunction(),
		NavigationEventScope::ForBrowser(m_browser2));
	EXPECT_CALL(m_stoppedCallback, Call(m_tab3.GetShellBrowser()));

	m_navigationEvents.NotifyStarted(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyStarted(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyStarted(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyWillCommit(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyWillCommit(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyWillCommit(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyCommitted(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyCommitted(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyCommitted(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyFailed(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyFailed(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyFailed(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyCancelled(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyCancelled(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyCancelled(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyStopped(m_tab1.GetShellBrowser());
	m_navigationEvents.NotifyStopped(m_tab2.GetShellBrowser());
	m_navigationEvents.NotifyStopped(m_tab3.GetShellBrowser());
}

TEST_F(NavigationEventsTest, SignalsFilteredByShellBrowser)
{
	InSequence seq;

	m_navigationEvents.AddStartedObserver(m_startedCallback.AsStdFunction(),
		NavigationEventScope::ForShellBrowser(*m_tab1.GetShellBrowser()));
	EXPECT_CALL(m_startedCallback, Call(m_tab1.GetShellBrowser(), nullptr));

	m_navigationEvents.AddWillCommitObserver(m_willCommitCallback.AsStdFunction(),
		NavigationEventScope::ForShellBrowser(*m_tab2.GetShellBrowser()));
	EXPECT_CALL(m_willCommitCallback, Call(m_tab2.GetShellBrowser(), nullptr));

	m_navigationEvents.AddCommittedObserver(m_committedCallback.AsStdFunction(),
		NavigationEventScope::ForShellBrowser(*m_tab1.GetShellBrowser()));
	EXPECT_CALL(m_committedCallback, Call(m_tab1.GetShellBrowser(), nullptr));

	m_navigationEvents.AddFailedObserver(m_failedCallback.AsStdFunction(),
		NavigationEventScope::ForShellBrowser(*m_tab3.GetShellBrowser()));
	EXPECT_CALL(m_failedCallback, Call(m_tab3.GetShellBrowser(), nullptr));

	m_navigationEvents.AddCancelledObserver(m_cancelledCallback.AsStdFunction(),
		NavigationEventScope::ForShellBrowser(*m_tab3.GetShellBrowser()));
	EXPECT_CALL(m_cancelledCallback, Call(m_tab3.GetShellBrowser(), nullptr));

	m_navigationEvents.AddStoppedObserver(m_stoppedCallback.AsStdFunction(),
		NavigationEventScope::ForShellBrowser(*m_tab2.GetShellBrowser()));
	EXPECT_CALL(m_stoppedCallback, Call(m_tab2.GetShellBrowser()));

	m_navigationEvents.NotifyStarted(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyStarted(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyStarted(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyWillCommit(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyWillCommit(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyWillCommit(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyCommitted(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyCommitted(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyCommitted(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyFailed(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyFailed(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyFailed(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyCancelled(m_tab1.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyCancelled(m_tab2.GetShellBrowser(), nullptr);
	m_navigationEvents.NotifyCancelled(m_tab3.GetShellBrowser(), nullptr);

	m_navigationEvents.NotifyStopped(m_tab1.GetShellBrowser());
	m_navigationEvents.NotifyStopped(m_tab2.GetShellBrowser());
	m_navigationEvents.NotifyStopped(m_tab3.GetShellBrowser());
}
