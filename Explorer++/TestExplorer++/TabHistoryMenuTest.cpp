// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabHistoryMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "MenuViewFake.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellIconLoaderFake.h"
#include "ShellTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class TabHistoryMenuTest : public BrowserTestBase
{
protected:
	TabHistoryMenuTest() : m_browser(AddBrowser()), m_tab(m_browser->AddTab(L"c:\\fake1"))
	{
		NavigateTab(m_tab, L"c:\\fake2");
		NavigateTab(m_tab, L"c:\\fake3");
	}

	ShellIconLoaderFake m_shellIconLoader;
	BrowserWindowFake *const m_browser;
	Tab *const m_tab;
};

TEST_F(TabHistoryMenuTest, BackHistory)
{
	MenuViewFake menuView;
	TabHistoryMenu menu(&menuView, &m_acceleratorManager, m_browser, &m_shellIconLoader,
		TabHistoryMenu::MenuType::Back);

	ASSERT_EQ(menuView.GetItemCount(), 2);
	EXPECT_EQ(menuView.GetItemText(menuView.GetItemId(0)), L"fake2");
	EXPECT_EQ(menuView.GetItemText(menuView.GetItemId(1)), L"fake1");
}

TEST_F(TabHistoryMenuTest, ForwardHistory)
{
	// Go back to fake1.
	m_tab->GetShellBrowser()->GetNavigationController()->GoBack();
	m_tab->GetShellBrowser()->GetNavigationController()->GoBack();

	MenuViewFake menuView;
	TabHistoryMenu menu(&menuView, &m_acceleratorManager, m_browser, &m_shellIconLoader,
		TabHistoryMenu::MenuType::Forward);

	ASSERT_EQ(menuView.GetItemCount(), 2);
	EXPECT_EQ(menuView.GetItemText(menuView.GetItemId(0)), L"fake2");
	EXPECT_EQ(menuView.GetItemText(menuView.GetItemId(1)), L"fake3");
}

TEST_F(TabHistoryMenuTest, BackSelection)
{
	MenuViewFake menuView;
	TabHistoryMenu menu(&menuView, &m_acceleratorManager, m_browser, &m_shellIconLoader,
		TabHistoryMenu::MenuType::Back);

	// Go back to fake2.
	menuView.SelectItem(menuView.GetItemId(0), false, false);

	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentIndex(), 1);
}

TEST_F(TabHistoryMenuTest, BackSelectionMiddleClick)
{
	MenuViewFake menuView;
	TabHistoryMenu menu(&menuView, &m_acceleratorManager, m_browser, &m_shellIconLoader,
		TabHistoryMenu::MenuType::Back);

	// Open fake2 in a new tab.
	menuView.MiddleClickItem(menuView.GetItemId(0), false, false);

	// Since the item was opened in a new tab, the current index should remain unchanged.
	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentIndex(), 2);

	auto *tabContainer = m_browser->GetActiveTabContainer();
	ASSERT_EQ(tabContainer->GetNumTabs(), 2);
	EXPECT_EQ(tabContainer->GetTabByIndex(1).GetShellBrowser()->GetDirectory(),
		CreateSimplePidlForTest(L"c:\\fake2"));
}

TEST_F(TabHistoryMenuTest, ForwardSelection)
{
	// Go back to fake1.
	m_tab->GetShellBrowser()->GetNavigationController()->GoBack();
	m_tab->GetShellBrowser()->GetNavigationController()->GoBack();

	MenuViewFake menuView;
	TabHistoryMenu menu(&menuView, &m_acceleratorManager, m_browser, &m_shellIconLoader,
		TabHistoryMenu::MenuType::Forward);

	// Go forward to fake3.
	menuView.SelectItem(menuView.GetItemId(1), false, false);

	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentIndex(), 2);
}

TEST_F(TabHistoryMenuTest, ForwardSelectionMiddleClick)
{
	// Go back to fake1.
	m_tab->GetShellBrowser()->GetNavigationController()->GoBack();
	m_tab->GetShellBrowser()->GetNavigationController()->GoBack();

	MenuViewFake menuView;
	TabHistoryMenu menu(&menuView, &m_acceleratorManager, m_browser, &m_shellIconLoader,
		TabHistoryMenu::MenuType::Forward);

	// Open fake3 in a new tab.
	menuView.MiddleClickItem(menuView.GetItemId(1), false, false);

	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentIndex(), 0);

	auto *tabContainer = m_browser->GetActiveTabContainer();
	ASSERT_EQ(tabContainer->GetNumTabs(), 2);
	EXPECT_EQ(tabContainer->GetTabByIndex(1).GetShellBrowser()->GetDirectory(),
		CreateSimplePidlForTest(L"c:\\fake3"));
}

TEST_F(TabHistoryMenuTest, InvalidSelection)
{
	// Go back to fake2.
	m_tab->GetShellBrowser()->GetNavigationController()->GoBack();

	MenuViewFake menuView;
	TabHistoryMenu menu(&menuView, &m_acceleratorManager, m_browser, &m_shellIconLoader,
		TabHistoryMenu::MenuType::Forward);

	// Go back to fake1.
	m_tab->GetShellBrowser()->GetNavigationController()->GoBack();

	// This will erase the forward history (i.e. fake2 and fake3).
	NavigateTab(m_tab, L"c:\\fake4");

	menuView.SelectItem(menuView.GetItemId(0), false, false);

	// There was no forward history entry to navigate to, so the current index should remain
	// unchanged.
	EXPECT_EQ(m_tab->GetShellBrowser()->GetNavigationController()->GetCurrentIndex(), 1);
}
