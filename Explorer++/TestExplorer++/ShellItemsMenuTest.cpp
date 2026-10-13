// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ShellItemsMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "MenuViewTestApi.h"
#include "MenuViewTestHelper.h"
#include "PidlTestHelper.h"
#include "RootMenuViewTestApi.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellIconLoaderFake.h"
#include "TestMenuView.h"
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

	ShellIconLoaderFake m_shellIconLoader;

	BrowserWindowFake *const m_browser;
	Tab *const m_tab;
};

TEST_F(ShellItemsMenuTest, CheckItems)
{
	TestMenuView menuView;
	auto pidls = BuildPidlCollection(3);
	ShellItemsMenu menu(&menuView, &m_acceleratorManager, pidls, m_browser, &m_shellIconLoader);

	MenuViewTestHelper::CheckShellItemDetails(&menuView, pidls);
}

TEST_F(ShellItemsMenuTest, CheckItemBitmapsAssigned)
{
	TestMenuView menuView;
	auto pidls = BuildPidlCollection(3);
	ShellItemsMenu menu(&menuView, &m_acceleratorManager, pidls, m_browser, &m_shellIconLoader);

	MenuViewTestApi::OnPopupWillShowForDpi(&menuView, USER_DEFAULT_SCREEN_DPI);

	for (int i = 0; i < menuView.GetNumItems(); i++)
	{
		EXPECT_NE(
			MenuViewTestApi::GetItemBitmap(&menuView, MenuViewTestApi::GetItemId(&menuView, i)),
			nullptr);
	}
}

TEST_F(ShellItemsMenuTest, RebuildMenu)
{
	TestMenuView menuView;
	auto pidls = BuildPidlCollection(3);
	ShellItemsMenu menu(&menuView, &m_acceleratorManager, pidls, m_browser, &m_shellIconLoader);

	auto updatedPidls = BuildPidlCollection(5);
	menu.RebuildMenu(updatedPidls);

	MenuViewTestHelper::CheckShellItemDetails(&menuView, updatedPidls);
}

TEST_F(ShellItemsMenuTest, OpenOnClick)
{
	TestMenuView menuView;
	auto pidl = CreateSimplePidlForTest(L"c:\\users");
	ShellItemsMenu menu(&menuView, &m_acceleratorManager, { pidl }, m_browser, &m_shellIconLoader);

	RootMenuViewTestApi::SelectItemAtIndex(&menuView, 0, false, false);

	auto *navigationController = m_tab->GetShellBrowser()->GetNavigationController();
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);
	EXPECT_EQ(navigationController->GetCurrentEntry()->GetPidl(), pidl);
}

TEST_F(ShellItemsMenuTest, OpenOnMiddleClick)
{
	TestMenuView menuView;
	auto pidl = CreateSimplePidlForTest(L"c:\\users");
	ShellItemsMenu menu(&menuView, &m_acceleratorManager, { pidl }, m_browser, &m_shellIconLoader);

	RootMenuViewTestApi::MiddleClickItemAtIndex(&menuView, 0, false, false);

	auto *tabContainer = m_browser->GetActiveTabContainer();
	ASSERT_EQ(tabContainer->GetNumTabs(), 2);

	auto &tab2 = tabContainer->GetTabByIndex(1);
	EXPECT_EQ(tab2.GetShellBrowser()->GetDirectory(), pidl);
}
