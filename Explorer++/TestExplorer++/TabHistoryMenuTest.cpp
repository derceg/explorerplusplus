// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabHistoryMenu.h"
#include "BrowserWindowMock.h"
#include "IconFetcherMock.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "TabNavigationMock.h"
#include <gtest/gtest.h>

using namespace testing;

class TabHistoryMenuTest : public Test
{
protected:
	TabHistoryMenuTest() : m_shellBrowser(&m_tabNavigation, &m_iconFetcher)
	{
		ON_CALL(m_browserWindow, GetActiveShellBrowser)
			.WillByDefault([this]() { return &m_shellBrowser; });
	}

	TabNavigationMock m_tabNavigation;
	IconFetcherMock m_iconFetcher;
	ShellBrowserFake m_shellBrowser;
	BrowserWindowMock m_browserWindow;
};

TEST_F(TabHistoryMenuTest, BackHistory)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	TabHistoryMenu menu(&m_browserWindow, TabHistoryMenu::MenuType::Back);

	auto menuView = menu.GetMenuViewForTesting();

	EXPECT_EQ(menuView->GetItemCountForTesting(), 2);
	EXPECT_EQ(menuView->GetItemTextForTesting(menuView->GetItemIdForTesting(0)), L"Fake2");
	EXPECT_EQ(menuView->GetItemTextForTesting(menuView->GetItemIdForTesting(1)), L"Fake1");
}

TEST_F(TabHistoryMenuTest, ForwardHistory)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	// Go back to Fake1.
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());

	TabHistoryMenu menu(&m_browserWindow, TabHistoryMenu::MenuType::Forward);

	auto menuView = menu.GetMenuViewForTesting();

	EXPECT_EQ(menuView->GetItemCountForTesting(), 2);
	EXPECT_EQ(menuView->GetItemTextForTesting(menuView->GetItemIdForTesting(0)), L"Fake2");
	EXPECT_EQ(menuView->GetItemTextForTesting(menuView->GetItemIdForTesting(1)), L"Fake3");
}

TEST_F(TabHistoryMenuTest, BackSelection)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	TabHistoryMenu menu(&m_browserWindow, TabHistoryMenu::MenuType::Back);

	// Go back to Fake2.
	auto menuView = menu.GetMenuViewForTesting();
	menu.OnMenuItemSelected(menuView->GetItemIdForTesting(0), false, false);

	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 1);
}

TEST_F(TabHistoryMenuTest, BackSelectionMiddleClick)
{
	unique_pidl_absolute fake2;
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(
		m_shellBrowser.NavigateToPath(L"C:\\Fake2", HistoryEntryType::AddEntry, &fake2));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	TabHistoryMenu menu(&m_browserWindow, TabHistoryMenu::MenuType::Back);

	EXPECT_CALL(m_browserWindow,
		OpenItem(
			Matcher<PCIDLIST_ABSOLUTE>(Truly(std::bind_front(&ArePidlsEquivalent, fake2.get()))),
			OpenFolderDisposition::NewTabDefault));

	// Open Fake2 in a new tab.
	auto menuView = menu.GetMenuViewForTesting();
	menu.OnMenuItemMiddleClicked(menuView->GetItemIdForTesting(0), false, false);

	// Since the item was opened in a new tab, the current index should remain unchanged.
	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 2);
}

TEST_F(TabHistoryMenuTest, ForwardSelection)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	// Go back to Fake1.
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());

	TabHistoryMenu menu(&m_browserWindow, TabHistoryMenu::MenuType::Forward);

	// Go forward to Fake3.
	auto menuView = menu.GetMenuViewForTesting();
	menu.OnMenuItemSelected(menuView->GetItemIdForTesting(1), false, false);

	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 2);
}

TEST_F(TabHistoryMenuTest, ForwardSelectionMiddleClick)
{
	unique_pidl_absolute fake3;
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(
		m_shellBrowser.NavigateToPath(L"C:\\Fake3", HistoryEntryType::AddEntry, &fake3));

	// Go back to Fake1.
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());

	TabHistoryMenu menu(&m_browserWindow, TabHistoryMenu::MenuType::Forward);

	EXPECT_CALL(m_browserWindow,
		OpenItem(
			Matcher<PCIDLIST_ABSOLUTE>(Truly(std::bind_front(&ArePidlsEquivalent, fake3.get()))),
			OpenFolderDisposition::NewTabDefault));

	// Open Fake3 in a new tab.
	auto menuView = menu.GetMenuViewForTesting();
	menu.OnMenuItemMiddleClicked(menuView->GetItemIdForTesting(1), false, false);

	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 0);
}

TEST_F(TabHistoryMenuTest, InvalidSelection)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	// Go back to Fake2.
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());

	TabHistoryMenu menu(&m_browserWindow, TabHistoryMenu::MenuType::Forward);

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
