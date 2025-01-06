// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabHistoryMenu.h"
#include "AcceleratorManager.h"
#include "BrowserWindowMock.h"
#include "PopupMenuView.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "ShellIconLoaderFake.h"
#include "TabNavigationMock.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class TabHistoryMenuTest : public Test
{
protected:
	TabHistoryMenuTest() : m_shellBrowser(&m_tabNavigation)
	{
		ON_CALL(m_browserWindow, GetActiveShellBrowser)
			.WillByDefault([this]() { return &m_shellBrowser; });
	}

	TabNavigationMock m_tabNavigation;
	ShellBrowserFake m_shellBrowser;
	AcceleratorManager m_acceleratorManager;
	BrowserWindowMock m_browserWindow;
	ShellIconLoaderFake m_shellIconLoader;
};

TEST_F(TabHistoryMenuTest, BackHistory)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	PopupMenuView popupMenu;
	TabHistoryMenu menu(&popupMenu, &m_acceleratorManager, &m_browserWindow, &m_shellIconLoader,
		TabHistoryMenu::MenuType::Back);

	EXPECT_EQ(popupMenu.GetItemCountForTesting(), 2);
	EXPECT_EQ(popupMenu.GetItemTextForTesting(popupMenu.GetItemIdForTesting(0)), L"Fake2");
	EXPECT_EQ(popupMenu.GetItemTextForTesting(popupMenu.GetItemIdForTesting(1)), L"Fake1");
}

TEST_F(TabHistoryMenuTest, ForwardHistory)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	// Go back to Fake1.
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());

	PopupMenuView popupMenu;
	TabHistoryMenu menu(&popupMenu, &m_acceleratorManager, &m_browserWindow, &m_shellIconLoader,
		TabHistoryMenu::MenuType::Forward);

	EXPECT_EQ(popupMenu.GetItemCountForTesting(), 2);
	EXPECT_EQ(popupMenu.GetItemTextForTesting(popupMenu.GetItemIdForTesting(0)), L"Fake2");
	EXPECT_EQ(popupMenu.GetItemTextForTesting(popupMenu.GetItemIdForTesting(1)), L"Fake3");
}

TEST_F(TabHistoryMenuTest, BackSelection)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	PopupMenuView popupMenu;
	TabHistoryMenu menu(&popupMenu, &m_acceleratorManager, &m_browserWindow, &m_shellIconLoader,
		TabHistoryMenu::MenuType::Back);

	// Go back to Fake2.
	popupMenu.SelectItem(popupMenu.GetItemIdForTesting(0), false, false);

	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 1);
}

TEST_F(TabHistoryMenuTest, BackSelectionMiddleClick)
{
	PidlAbsolute fake2;
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(
		m_shellBrowser.NavigateToPath(L"C:\\Fake2", HistoryEntryType::AddEntry, &fake2));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	PopupMenuView popupMenu;
	TabHistoryMenu menu(&popupMenu, &m_acceleratorManager, &m_browserWindow, &m_shellIconLoader,
		TabHistoryMenu::MenuType::Back);

	EXPECT_CALL(m_browserWindow,
		OpenItem(TypedEq<PCIDLIST_ABSOLUTE>(fake2), OpenFolderDisposition::NewTabDefault));

	// Open Fake2 in a new tab.
	popupMenu.MiddleClickItem(popupMenu.GetItemIdForTesting(0), false, false);

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

	PopupMenuView popupMenu;
	TabHistoryMenu menu(&popupMenu, &m_acceleratorManager, &m_browserWindow, &m_shellIconLoader,
		TabHistoryMenu::MenuType::Forward);

	// Go forward to Fake3.
	popupMenu.SelectItem(popupMenu.GetItemIdForTesting(1), false, false);

	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 2);
}

TEST_F(TabHistoryMenuTest, ForwardSelectionMiddleClick)
{
	PidlAbsolute fake3;
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(
		m_shellBrowser.NavigateToPath(L"C:\\Fake3", HistoryEntryType::AddEntry, &fake3));

	// Go back to Fake1.
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());

	PopupMenuView popupMenu;
	TabHistoryMenu menu(&popupMenu, &m_acceleratorManager, &m_browserWindow, &m_shellIconLoader,
		TabHistoryMenu::MenuType::Forward);

	EXPECT_CALL(m_browserWindow,
		OpenItem(TypedEq<PCIDLIST_ABSOLUTE>(fake3), OpenFolderDisposition::NewTabDefault));

	// Open Fake3 in a new tab.
	popupMenu.MiddleClickItem(popupMenu.GetItemIdForTesting(1), false, false);

	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 0);
}

TEST_F(TabHistoryMenuTest, InvalidSelection)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	// Go back to Fake2.
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());

	PopupMenuView popupMenu;
	TabHistoryMenu menu(&popupMenu, &m_acceleratorManager, &m_browserWindow, &m_shellIconLoader,
		TabHistoryMenu::MenuType::Forward);

	// Go back to Fake1.
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.GetNavigationController()->GoBack());

	// This will erase the forward history (i.e. Fake2 and Fake3).
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake4"));

	popupMenu.SelectItem(popupMenu.GetItemIdForTesting(0), false, false);

	// There was no forward history entry to navigate to, so the current index should remain
	// unchanged.
	EXPECT_EQ(m_shellBrowser.GetNavigationController()->GetCurrentIndex(), 1);
}
