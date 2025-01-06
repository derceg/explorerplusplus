// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabRestorerMenu.h"
#include "AcceleratorManager.h"
#include "BrowserList.h"
#include "BrowserWindowMock.h"
#include "GlobalTabEventDispatcher.h"
#include "PopupMenuView.h"
#include "PopupMenuViewTestHelper.h"
#include "ShellBrowserFake.h"
#include "ShellIconLoaderFake.h"
#include "ShellTestHelper.h"
#include "TabNavigationMock.h"
#include "TabRestorer.h"
#include "Win32ResourceLoader.h"
#include <gtest/gtest.h>

using namespace testing;

class TabRestorerMenuTest : public Test
{
protected:
	TabRestorerMenuTest() :
		m_tabRestorer(&m_dispatcher, &m_browserList),
		m_resourceLoader(GetModuleHandle(nullptr))
	{
	}

	Tab BuildTab()
	{
		return Tab(std::make_shared<ShellBrowserFake>(&m_tabNavigation), &m_browser);
	}

	void NavigateTab(Tab &tab, const PidlAbsolute &pidl)
	{
		auto navigateParams = NavigateParams::Normal(pidl.Raw());
		ASSERT_HRESULT_SUCCEEDED(tab.GetShellBrowser()->Navigate(navigateParams));
	}

	PopupMenuView m_popupMenu;
	AcceleratorManager m_acceleratorManager;

	GlobalTabEventDispatcher m_dispatcher;
	BrowserList m_browserList;
	TabRestorer m_tabRestorer;

	ShellIconLoaderFake m_shellIconLoader;
	Win32ResourceLoader m_resourceLoader;

	TabNavigationMock m_tabNavigation;
	BrowserWindowMock m_browser;
};

TEST_F(TabRestorerMenuTest, CheckItems)
{
	auto tab1 = BuildTab();
	auto path1 = CreateSimplePidlForTest(L"c:\\path1");
	NavigateTab(tab1, path1);
	m_dispatcher.NotifyPreRemoval(tab1, 0);

	auto tab2 = BuildTab();
	auto path2 = CreateSimplePidlForTest(L"c:\\path2");
	NavigateTab(tab2, path2);
	m_dispatcher.NotifyPreRemoval(tab2, 0);

	TabRestorerMenu menu(&m_popupMenu, &m_acceleratorManager, &m_tabRestorer, &m_shellIconLoader,
		&m_resourceLoader);

	PopupMenuViewTestHelper::CheckItemDetails(&m_popupMenu, { path2, path1 });
}

TEST_F(TabRestorerMenuTest, Selection)
{
	auto tab1 = BuildTab();
	auto path1 = CreateSimplePidlForTest(L"c:\\path1");
	NavigateTab(tab1, path1);
	m_dispatcher.NotifyPreRemoval(tab1, 0);

	auto tab2 = BuildTab();
	auto path2 = CreateSimplePidlForTest(L"c:\\path2");
	NavigateTab(tab2, path2);
	m_dispatcher.NotifyPreRemoval(tab2, 0);

	auto tab3 = BuildTab();
	auto path3 = CreateSimplePidlForTest(L"c:\\path3");
	NavigateTab(tab3, path3);
	m_dispatcher.NotifyPreRemoval(tab3, 0);

	TabRestorerMenu menu(&m_popupMenu, &m_acceleratorManager, &m_tabRestorer, &m_shellIconLoader,
		&m_resourceLoader);

	// Tabs are listed by most recently closed first, so the menu should contain:
	//
	// tab3
	// tab2
	// tab1
	//
	// and this call should select tab2.
	m_popupMenu.SelectItem(m_popupMenu.GetItemIdForTesting(1), false, false);

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
	m_popupMenu.SelectItem(m_popupMenu.GetItemIdForTesting(1), false, false);

	// tab1 was restored, so only tab3 should remain.
	ASSERT_EQ(m_tabRestorer.GetClosedTabs().size(), 1u);
	EXPECT_EQ(m_tabRestorer.GetClosedTabs().front()->id, tab3.GetId());

	// This should restore tab3, leaving no tabs left to be restored.
	m_popupMenu.SelectItem(m_popupMenu.GetItemIdForTesting(0), false, false);

	EXPECT_EQ(m_tabRestorer.GetClosedTabs().size(), 0u);
}
