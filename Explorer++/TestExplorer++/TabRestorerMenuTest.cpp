// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabRestorerMenu.h"
#include "AcceleratorManager.h"
#include "BrowserList.h"
#include "BrowserWindowMock.h"
#include "MenuViewFake.h"
#include "MenuViewFakeTestHelper.h"
#include "ResourceLoaderFake.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "ShellIconLoaderFake.h"
#include "ShellTestHelper.h"
#include "TabEvents.h"
#include "TabNavigationMock.h"
#include "TabRestorer.h"
#include <gtest/gtest.h>

using namespace testing;

class TabRestorerMenuTest : public Test
{
protected:
	TabRestorerMenuTest() : m_tabRestorer(&m_tabEvents, &m_browserList)
	{
	}

	Tab BuildTab()
	{
		return Tab(std::make_unique<ShellBrowserFake>(&m_navigationEvents, &m_tabNavigation),
			&m_browser, nullptr, &m_tabEvents);
	}

	void NavigateTab(Tab &tab, const PidlAbsolute &pidl)
	{
		auto navigateParams = NavigateParams::Normal(pidl.Raw());
		tab.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);
	}

	MenuViewFake m_menuView;
	AcceleratorManager m_acceleratorManager;

	TabEvents m_tabEvents;
	BrowserList m_browserList;
	TabRestorer m_tabRestorer;

	ShellIconLoaderFake m_shellIconLoader;
	ResourceLoaderFake m_resourceLoader;

	NavigationEvents m_navigationEvents;
	TabNavigationMock m_tabNavigation;
	BrowserWindowMock m_browser;
};

TEST_F(TabRestorerMenuTest, CheckItems)
{
	auto tab1 = BuildTab();
	auto path1 = CreateSimplePidlForTest(L"c:\\path1");
	NavigateTab(tab1, path1);
	m_tabEvents.NotifyPreRemoval(tab1, 0);

	auto tab2 = BuildTab();
	auto path2 = CreateSimplePidlForTest(L"c:\\path2");
	NavigateTab(tab2, path2);
	m_tabEvents.NotifyPreRemoval(tab2, 0);

	TabRestorerMenu menu(&m_menuView, &m_acceleratorManager, &m_tabRestorer, &m_shellIconLoader,
		&m_resourceLoader);

	MenuViewFakeTestHelper::CheckItemDetails(&m_menuView, { path2, path1 });
}

TEST_F(TabRestorerMenuTest, Selection)
{
	auto tab1 = BuildTab();
	auto path1 = CreateSimplePidlForTest(L"c:\\path1");
	NavigateTab(tab1, path1);
	m_tabEvents.NotifyPreRemoval(tab1, 0);

	auto tab2 = BuildTab();
	auto path2 = CreateSimplePidlForTest(L"c:\\path2");
	NavigateTab(tab2, path2);
	m_tabEvents.NotifyPreRemoval(tab2, 0);

	auto tab3 = BuildTab();
	auto path3 = CreateSimplePidlForTest(L"c:\\path3");
	NavigateTab(tab3, path3);
	m_tabEvents.NotifyPreRemoval(tab3, 0);

	TabRestorerMenu menu(&m_menuView, &m_acceleratorManager, &m_tabRestorer, &m_shellIconLoader,
		&m_resourceLoader);

	// Tabs are listed by most recently closed first, so the menu should contain:
	//
	// tab3
	// tab2
	// tab1
	//
	// and this call should select tab2.
	m_menuView.SelectItem(m_menuView.GetItemId(1), false, false);

	// tab2 was restored, so tab1 and tab3 should remain.
	ASSERT_EQ(m_tabRestorer.GetClosedTabs().size(), 2u);
	EXPECT_EQ(m_tabRestorer.GetClosedTabs().front()->id, tab3.GetId());
	EXPECT_EQ((*++m_tabRestorer.GetClosedTabs().begin())->id, tab1.GetId());

	// The menu should rebuild itself when the set of recent tabs changes, so the menu should now
	// contain:
	//
	// tab3
	// tab1
	//
	// and this call should select tab1.
	m_menuView.SelectItem(m_menuView.GetItemId(1), false, false);

	// tab1 was restored, so only tab3 should remain.
	ASSERT_EQ(m_tabRestorer.GetClosedTabs().size(), 1u);
	EXPECT_EQ(m_tabRestorer.GetClosedTabs().front()->id, tab3.GetId());

	// This should restore tab3, leaving no tabs left to be restored.
	m_menuView.SelectItem(m_menuView.GetItemId(0), false, false);

	EXPECT_EQ(m_tabRestorer.GetClosedTabs().size(), 0u);
}
