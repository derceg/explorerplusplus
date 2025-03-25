// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "Tab.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class ShellBrowserEventsTest : public BrowserTestBase
{
protected:
	using CallBackMock = StrictMock<MockFunction<void(const ShellBrowser *shellBrowser)>>;

	ShellBrowserEventsTest() :
		m_browser1(AddBrowser()),
		m_tab1(m_browser1->AddTab()),
		m_tab2(m_browser1->AddTab()),
		m_browser2(AddBrowser()),
		m_tab3(m_browser2->AddTab())
	{
	}

	ShellBrowserEvents m_shellBrowserEvents;

	BrowserWindowFake *const m_browser1;
	Tab *const m_tab1;
	Tab *const m_tab2;

	BrowserWindowFake *const m_browser2;
	Tab *const m_tab3;

	CallBackMock m_itemsChangedCallback;
	CallBackMock m_directoryPropertiesChangedCallback;
	CallBackMock m_selectionChangedCallback;
};

TEST_F(ShellBrowserEventsTest, Signals)
{
	InSequence seq;

	m_shellBrowserEvents.AddItemsChangedObserver(m_itemsChangedCallback.AsStdFunction(),
		ShellBrowserEventScope::Global());
	EXPECT_CALL(m_itemsChangedCallback, Call(m_tab1->GetShellBrowser()));
	EXPECT_CALL(m_itemsChangedCallback, Call(m_tab2->GetShellBrowser()));
	EXPECT_CALL(m_itemsChangedCallback, Call(m_tab3->GetShellBrowser()));

	m_shellBrowserEvents.AddDirectoryPropertiesChangedObserver(
		m_directoryPropertiesChangedCallback.AsStdFunction(), ShellBrowserEventScope::Global());
	EXPECT_CALL(m_directoryPropertiesChangedCallback, Call(m_tab1->GetShellBrowser()));
	EXPECT_CALL(m_directoryPropertiesChangedCallback, Call(m_tab2->GetShellBrowser()));
	EXPECT_CALL(m_directoryPropertiesChangedCallback, Call(m_tab3->GetShellBrowser()));

	m_shellBrowserEvents.AddSelectionChangedObserver(m_selectionChangedCallback.AsStdFunction(),
		ShellBrowserEventScope::Global());
	EXPECT_CALL(m_selectionChangedCallback, Call(m_tab1->GetShellBrowser()));
	EXPECT_CALL(m_selectionChangedCallback, Call(m_tab2->GetShellBrowser()));
	EXPECT_CALL(m_selectionChangedCallback, Call(m_tab3->GetShellBrowser()));

	m_shellBrowserEvents.NotifyItemsChanged(m_tab1->GetShellBrowser());
	m_shellBrowserEvents.NotifyItemsChanged(m_tab2->GetShellBrowser());
	m_shellBrowserEvents.NotifyItemsChanged(m_tab3->GetShellBrowser());

	m_shellBrowserEvents.NotifyDirectoryPropertiesChanged(m_tab1->GetShellBrowser());
	m_shellBrowserEvents.NotifyDirectoryPropertiesChanged(m_tab2->GetShellBrowser());
	m_shellBrowserEvents.NotifyDirectoryPropertiesChanged(m_tab3->GetShellBrowser());

	m_shellBrowserEvents.NotifySelectionChanged(m_tab1->GetShellBrowser());
	m_shellBrowserEvents.NotifySelectionChanged(m_tab2->GetShellBrowser());
	m_shellBrowserEvents.NotifySelectionChanged(m_tab3->GetShellBrowser());
}

TEST_F(ShellBrowserEventsTest, SignalsFilteredByBrowser)
{
	InSequence seq;

	// Note that only a single signal is tested here. As each AddObserver() method functions in the
	// same way (that is, the filtering is always performed by a single method), all that's really
	// needed is to check that the filtering works correctly with a single signal.
	//
	// Testing multiple signals here would be redundant, since they all rely on the same filtering
	// method.
	m_shellBrowserEvents.AddItemsChangedObserver(m_itemsChangedCallback.AsStdFunction(),
		ShellBrowserEventScope::ForBrowser(*m_browser1));
	EXPECT_CALL(m_itemsChangedCallback, Call(m_tab1->GetShellBrowser()));
	EXPECT_CALL(m_itemsChangedCallback, Call(m_tab2->GetShellBrowser()));

	m_shellBrowserEvents.NotifyItemsChanged(m_tab1->GetShellBrowser());
	m_shellBrowserEvents.NotifyItemsChanged(m_tab2->GetShellBrowser());
	m_shellBrowserEvents.NotifyItemsChanged(m_tab3->GetShellBrowser());
}

TEST_F(ShellBrowserEventsTest, SignalsFilteredByShellBrowser)
{
	m_shellBrowserEvents.AddItemsChangedObserver(m_itemsChangedCallback.AsStdFunction(),
		ShellBrowserEventScope::ForShellBrowser(*m_tab1->GetShellBrowser()));
	EXPECT_CALL(m_itemsChangedCallback, Call(m_tab1->GetShellBrowser()));

	m_shellBrowserEvents.NotifyItemsChanged(m_tab1->GetShellBrowser());
	m_shellBrowserEvents.NotifyItemsChanged(m_tab2->GetShellBrowser());
	m_shellBrowserEvents.NotifyItemsChanged(m_tab3->GetShellBrowser());
}

TEST_F(ShellBrowserEventsTest, SignalsFilteredByActiveShellBrowser)
{
	m_browser1->ActivateTabAtIndex(1);
	m_browser2->ActivateTabAtIndex(0);

	m_shellBrowserEvents.AddItemsChangedObserver(m_itemsChangedCallback.AsStdFunction(),
		ShellBrowserEventScope::ForActiveShellBrowser(*m_browser1));
	EXPECT_CALL(m_itemsChangedCallback, Call(m_tab2->GetShellBrowser()));

	m_shellBrowserEvents.NotifyItemsChanged(m_tab1->GetShellBrowser());
	m_shellBrowserEvents.NotifyItemsChanged(m_tab2->GetShellBrowser());
	m_shellBrowserEvents.NotifyItemsChanged(m_tab3->GetShellBrowser());
}
