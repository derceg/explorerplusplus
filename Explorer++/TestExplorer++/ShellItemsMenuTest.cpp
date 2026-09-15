// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellItemsMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "MenuTestHost.h"
#include "MenuViewTestHelper.h"
#include "PidlTestHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellIconLoaderFake.h"
#include <gtest/gtest.h>

class ShellItemsMenuTest : public BrowserTestBase
{
protected:
	ShellItemsMenuTest() : m_browser(AddBrowser()), m_tab(m_browser->AddTab(L"c:\\"))
	{
	}

	std::vector<PidlAbsolute> BuildPidlCollection(int size)
	{
		std::vector<PidlAbsolute> pidls;

		for (int i = 0; i < size; i++)
		{
			pidls.push_back(CreateSimplePidlForTest(std::format(L"c:\\fake{}", i)));
		}

		return pidls;
	}

	void CheckIdRange(UINT startId, UINT endId, UINT expectedStartId, UINT expectedEndId)
	{
		MenuTestHost menuHost;
		ShellItemsMenu menu(menuHost.GetView(), &m_acceleratorManager,
			{ CreateSimplePidlForTest(L"c:\\") }, m_browser, &m_shellIconLoader, startId, endId);
		EXPECT_EQ(menu.GetIdRange(), MenuBase::IdRange(expectedStartId, expectedEndId));
	}

	ShellIconLoaderFake m_shellIconLoader;

	BrowserWindowFake *const m_browser;
	Tab *const m_tab;
};

TEST_F(ShellItemsMenuTest, CheckItems)
{
	MenuTestHost menuHost;
	auto pidls = BuildPidlCollection(3);
	ShellItemsMenu menu(menuHost.GetView(), &m_acceleratorManager, pidls, m_browser,
		&m_shellIconLoader);

	MenuViewTestHelper::CheckShellItemDetails(menuHost.GetView(), pidls);
}

TEST_F(ShellItemsMenuTest, CheckItemBitmapsAssigned)
{
	MenuTestHost menuHost;
	auto *menuView = menuHost.GetView();
	auto pidls = BuildPidlCollection(3);
	ShellItemsMenu menu(menuView, &m_acceleratorManager, pidls, m_browser, &m_shellIconLoader);

	menuView->OnPopupWillShowForTesting(USER_DEFAULT_SCREEN_DPI);

	for (int i = 0; i < menuView->GetNumItems(); i++)
	{
		EXPECT_NE(menuView->GetItemBitmapForTesting(menuView->GetItemIdForTesting(i)), nullptr);
	}
}

TEST_F(ShellItemsMenuTest, MaxItems)
{
	MenuTestHost menuHost;
	auto pidls = BuildPidlCollection(3);
	ShellItemsMenu menu(menuHost.GetView(), &m_acceleratorManager, pidls, m_browser,
		&m_shellIconLoader, 1, 2);

	// The menu only has a single ID it can assign from the provided range of [1,2). So, although 3
	// items were passed in, only the first item should be added to the menu.
	MenuViewTestHelper::CheckShellItemDetails(menuHost.GetView(), { pidls[0] });
}

TEST_F(ShellItemsMenuTest, GetIdRange)
{
	CheckIdRange(20, 100, 20, 100);

	// 0 isn't a valid start ID, so the final ID range should start from 1.
	CheckIdRange(0, 46, 1, 46);

	// 0 isn't a valid end ID either, so the end ID should be set to the start ID.
	CheckIdRange(11, 0, 11, 11);

	CheckIdRange(0, 0, 1, 1);

	// The end ID should always be greater or equal to the start ID.
	CheckIdRange(200, 148, 200, 200);
}

TEST_F(ShellItemsMenuTest, RebuildMenu)
{
	MenuTestHost menuHost;
	auto pidls = BuildPidlCollection(3);
	ShellItemsMenu menu(menuHost.GetView(), &m_acceleratorManager, pidls, m_browser,
		&m_shellIconLoader);

	auto updatedPidls = BuildPidlCollection(5);
	menu.RebuildMenu(updatedPidls);

	MenuViewTestHelper::CheckShellItemDetails(menuHost.GetView(), updatedPidls);
}

TEST_F(ShellItemsMenuTest, OpenOnClick)
{
	MenuTestHost menuHost;
	auto pidl = CreateSimplePidlForTest(L"c:\\users");
	ShellItemsMenu menu(menuHost.GetView(), &m_acceleratorManager, { pidl }, m_browser,
		&m_shellIconLoader);

	menuHost.SelectItemAtIndex(0, false, false);

	auto *navigationController = m_tab->GetShellBrowser()->GetNavigationController();
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);
	EXPECT_EQ(navigationController->GetCurrentEntry()->GetPidl(), pidl);
}

TEST_F(ShellItemsMenuTest, OpenOnMiddleClick)
{
	MenuTestHost menuHost;
	auto pidl = CreateSimplePidlForTest(L"c:\\users");
	ShellItemsMenu menu(menuHost.GetView(), &m_acceleratorManager, { pidl }, m_browser,
		&m_shellIconLoader);

	menuHost.MiddleClickItemAtIndex(0, false, false);

	auto *tabContainer = m_browser->GetActiveTabContainer();
	ASSERT_EQ(tabContainer->GetNumTabs(), 2);

	auto &tab2 = tabContainer->GetTabByIndex(1);
	EXPECT_EQ(tab2.GetShellBrowser()->GetDirectory(), pidl);
}
