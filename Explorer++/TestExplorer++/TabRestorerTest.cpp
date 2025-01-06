// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabRestorer.h"
#include "BrowserList.h"
#include "BrowserWindowMock.h"
#include "GlobalTabEventDispatcher.h"
#include "ShellBrowserFake.h"
#include "TabNavigationMock.h"
#include <gtest/gtest.h>

using namespace testing;

class TabRestorerTest : public Test
{
protected:
	TabRestorerTest() : m_tabRestorer(&m_dispatcher, &m_browserList)
	{
	}

	Tab BuildTab()
	{
		return Tab(std::make_shared<ShellBrowserFake>(&m_tabNavigation), &m_browser);
	}

	Tab BuildTab(BrowserWindow *browser)
	{
		return Tab(std::make_shared<ShellBrowserFake>(&m_tabNavigation), browser);
	}

	GlobalTabEventDispatcher m_dispatcher;
	BrowserList m_browserList;
	TabRestorer m_tabRestorer;

	TabNavigationMock m_tabNavigation;
	BrowserWindowMock m_browser;
};

TEST_F(TabRestorerTest, InitialState)
{
	EXPECT_EQ(m_tabRestorer.GetClosedTabs().size(), 0u);
	EXPECT_TRUE(m_tabRestorer.IsEmpty());
}

TEST_F(TabRestorerTest, GetTabs)
{
	auto tab1 = BuildTab();
	m_dispatcher.NotifyPreRemoval(tab1, 1);

	auto tab2 = BuildTab();
	m_dispatcher.NotifyPreRemoval(tab2, 0);

	const auto &closedTabs = m_tabRestorer.GetClosedTabs();
	ASSERT_EQ(closedTabs.size(), 2u);

	// tab2 was closed most recently, so its entry should appear first.
	EXPECT_EQ(closedTabs.front()->id, tab2.GetId());
	EXPECT_EQ((*++closedTabs.begin())->id, tab1.GetId());

	for (const auto &closedTab : closedTabs)
	{
		EXPECT_EQ(closedTab.get(), m_tabRestorer.GetTabById(closedTab->id));
	}
}

TEST_F(TabRestorerTest, RestoreLastTab)
{
	auto tab1 = BuildTab();
	m_dispatcher.NotifyPreRemoval(tab1, 1);

	auto tab2 = BuildTab();
	m_dispatcher.NotifyPreRemoval(tab2, 0);

	m_tabRestorer.RestoreLastTab();

	// tab2 was closed most recently, so it should be restored first, leaving only tab1.
	ASSERT_EQ(m_tabRestorer.GetClosedTabs().size(), 1u);
	EXPECT_EQ(m_tabRestorer.GetClosedTabs().front()->id, tab1.GetId());

	m_tabRestorer.RestoreLastTab();

	EXPECT_EQ(m_tabRestorer.GetClosedTabs().size(), 0u);
}

TEST_F(TabRestorerTest, RestoreTabById)
{
	auto tab1 = BuildTab();
	m_dispatcher.NotifyPreRemoval(tab1, 1);

	auto tab2 = BuildTab();
	m_dispatcher.NotifyPreRemoval(tab2, 0);

	m_tabRestorer.RestoreTabById(tab1.GetId());

	ASSERT_EQ(m_tabRestorer.GetClosedTabs().size(), 1u);
	EXPECT_EQ(m_tabRestorer.GetClosedTabs().front()->id, tab2.GetId());

	m_tabRestorer.RestoreTabById(tab2.GetId());

	EXPECT_EQ(m_tabRestorer.GetClosedTabs().size(), 0u);
}

TEST_F(TabRestorerTest, BrowserUsedForRestore)
{
	BrowserWindowMock browser1;
	m_browserList.AddBrowser(&browser1);

	BrowserWindowMock browser2;
	m_browserList.AddBrowser(&browser2);

	auto tab1 = BuildTab(&browser1);
	m_dispatcher.NotifyPreRemoval(tab1, 0);

	auto tab2 = BuildTab(&browser1);
	m_dispatcher.NotifyPreRemoval(tab2, 0);

	MockFunction<void(int)> check;
	{
		InSequence seq;

		// At this point, browser1 still exists, so tab2 should preferentially be restored into it.
		EXPECT_CALL(browser1, CreateTabFromPreservedTab(m_tabRestorer.GetTabById(tab2.GetId())));

		EXPECT_CALL(check, Call(1));

		// At this point, browser1 has been removed, so tab1 should instead be restored into the
		// last active browser.
		EXPECT_CALL(browser2, CreateTabFromPreservedTab(m_tabRestorer.GetTabById(tab1.GetId())));
	}

	m_tabRestorer.RestoreLastTab();

	m_browserList.RemoveBrowser(&browser1);

	check.Call(1);

	m_tabRestorer.RestoreLastTab();
}

TEST_F(TabRestorerTest, ItemsChanged)
{
	MockFunction<void()> callback;
	m_tabRestorer.AddItemsChangedObserver(callback.AsStdFunction());

	// See https://google.github.io/googletest/gmock_cook_book.html#UsingCheckPoints.
	// This verifies that each of the functions below invokes the items changed callback.
	MockFunction<void(int)> check;
	{
		InSequence seq;

		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(1));
		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(2));
		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(3));
		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(4));
	}

	auto tab1 = BuildTab();
	m_dispatcher.NotifyPreRemoval(tab1, 1);
	check.Call(1);

	auto tab2 = BuildTab();
	m_dispatcher.NotifyPreRemoval(tab2, 0);
	check.Call(2);

	m_tabRestorer.RestoreTabById(tab1.GetId());
	check.Call(3);

	m_tabRestorer.RestoreLastTab();
	check.Call(4);
}
