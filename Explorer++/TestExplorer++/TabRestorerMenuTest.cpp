// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabRestorerMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "MenuViewFake.h"
#include "MenuViewFakeTestHelper.h"
#include "ShellIconLoaderFake.h"
#include "TabRestorer.h"
#include <gtest/gtest.h>

using namespace testing;

class TabRestorerMenuTest : public BrowserTestBase
{
protected:
	TabRestorerMenuTest() : m_tabRestorer(&m_tabEvents, &m_browserList)
	{
	}

	TabRestorer m_tabRestorer;
	ShellIconLoaderFake m_shellIconLoader;

	MenuViewFake m_menuView;
};

TEST_F(TabRestorerMenuTest, CheckItems)
{
	auto *browser = AddBrowser();
	browser->AddTab(L"c:\\path1");

	PidlAbsolute pidl2;
	int tabId2 = browser->AddTabAndReturnId(L"c:\\path2", {}, &pidl2);

	PidlAbsolute pidl3;
	int tabId3 = browser->AddTabAndReturnId(L"c:\\path3", {}, &pidl3);

	auto *tabContainer = browser->GetActiveTabContainer();
	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTab(tabId2)));
	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTab(tabId3)));

	TabRestorerMenu menu(&m_menuView, &m_acceleratorManager, &m_tabRestorer, &m_shellIconLoader,
		&m_resourceLoader);

	MenuViewFakeTestHelper::CheckItemDetails(&m_menuView, { pidl3, pidl2 });
}

TEST_F(TabRestorerMenuTest, Selection)
{
	auto *browser = AddBrowser();
	browser->AddTab(L"c:\\path1");
	int tabId2 = browser->AddTabAndReturnId(L"c:\\path2");
	int tabId3 = browser->AddTabAndReturnId(L"c:\\path3");
	int tabId4 = browser->AddTabAndReturnId(L"c:\\path4");

	auto *tabContainer = browser->GetActiveTabContainer();
	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTab(tabId2)));
	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTab(tabId3)));
	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTab(tabId4)));

	TabRestorerMenu menu(&m_menuView, &m_acceleratorManager, &m_tabRestorer, &m_shellIconLoader,
		&m_resourceLoader);

	// Tabs are listed by most recently closed first, so the menu should contain:
	//
	// tab4
	// tab3
	// tab2
	//
	// and this call should select tab3.
	m_menuView.SelectItem(m_menuView.GetItemId(1), false, false);
	EXPECT_EQ(tabContainer->GetNumTabs(), 2);

	// tab3 was restored, so tab2 and tab4 should remain.
	const auto &closedTabs = m_tabRestorer.GetClosedTabs();
	EXPECT_THAT(closedTabs,
		ElementsAre(Pointee(Field(&PreservedTab::id, tabId4)),
			Pointee(Field(&PreservedTab::id, tabId2))));

	// The menu should rebuild itself when the set of recent tabs changes, so the menu should now
	// contain:
	//
	// tab4
	// tab2
	//
	// and this call should select tab2.
	//
	// Note that middle-clicking an item should have the same effect as left-clicking an item.
	m_menuView.MiddleClickItem(m_menuView.GetItemId(1), false, false);
	EXPECT_EQ(tabContainer->GetNumTabs(), 3);

	// tab2 was restored, so only tab4 should remain.
	EXPECT_THAT(closedTabs, ElementsAre(Pointee(Field(&PreservedTab::id, tabId4))));

	// This should restore tab4, leaving no tabs left to be restored.
	m_menuView.SelectItem(m_menuView.GetItemId(0), false, false);
	EXPECT_EQ(tabContainer->GetNumTabs(), 4);

	EXPECT_TRUE(closedTabs.empty());
}
