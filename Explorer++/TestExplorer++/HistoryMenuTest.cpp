// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "HistoryMenu.h"
#include "IconFetcherMock.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "TabNavigationMock.h"
#include <gtest/gtest.h>

using namespace testing;

class HistoryMenuTest : public Test
{
protected:
	HistoryMenuTest() : m_shellBrowser(&m_tabNavigation, &m_iconFetcher)
	{
	}

	TabNavigationMock m_tabNavigation;
	IconFetcherMock m_iconFetcher;
	ShellBrowserFake m_shellBrowser;
};

TEST_F(HistoryMenuTest, BackHistory)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	HistoryMenu menu(&m_shellBrowser, HistoryMenu::MenuType::Back);

	auto menuView = menu.GetMenuViewForTesting();

	EXPECT_EQ(menuView->GetItemCountForTesting(), 2);
	EXPECT_EQ(menuView->GetItemTextForTesting(0), L"Fake2");
	EXPECT_EQ(menuView->GetItemTextForTesting(1), L"Fake1");
}

TEST_F(HistoryMenuTest, ForwardHistory)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	// Go back to Fake1.
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());

	HistoryMenu menu(&m_shellBrowser, HistoryMenu::MenuType::Forward);

	auto menuView = menu.GetMenuViewForTesting();

	EXPECT_EQ(menuView->GetItemCountForTesting(), 2);
	EXPECT_EQ(menuView->GetItemTextForTesting(0), L"Fake2");
	EXPECT_EQ(menuView->GetItemTextForTesting(1), L"Fake3");
}

TEST_F(HistoryMenuTest, BackSelection)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	HistoryMenu menu(&m_shellBrowser, HistoryMenu::MenuType::Back);

	// Go back to Fake2.
	auto menuView = menu.GetMenuViewForTesting();
	menu.OnMenuItemSelected(menuView->GetItemIdForTesting(0), false, false);

	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 1);
}

TEST_F(HistoryMenuTest, ForwardSelection)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	// Go back to Fake1.
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());

	HistoryMenu menu(&m_shellBrowser, HistoryMenu::MenuType::Forward);

	// Go forward to Fake3.
	auto menuView = menu.GetMenuViewForTesting();
	menu.OnMenuItemSelected(menuView->GetItemIdForTesting(1), false, false);

	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 2);
}

TEST_F(HistoryMenuTest, InvalidSelection)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	// Go back to Fake2.
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());

	HistoryMenu menu(&m_shellBrowser, HistoryMenu::MenuType::Forward);

	// Go back to Fake1.
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());

	// This will erase the forward history (i.e. Fake2 and Fake3).
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake4"));

	auto menuView = menu.GetMenuViewForTesting();
	menu.OnMenuItemSelected(menuView->GetItemIdForTesting(0), false, false);

	// There was no forward history entry to navigate to, so the current index should remain
	// unchanged.
	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 1);
}
