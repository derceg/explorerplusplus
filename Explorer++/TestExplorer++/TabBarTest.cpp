// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabBar.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "ControlTestHelper.h"
#include "PidlTestHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabBarView.h"
#include "TabViewTestApi.h"
#include <gtest/gtest.h>

using namespace testing;

class TabBarTest : public BrowserTestBase
{
protected:
	TabBarTest() : m_browser(AddBrowser()), m_tabBar(m_browser->GetActiveTabBar())
	{
	}

	BrowserWindowFake *const m_browser;
	TabBar *const m_tabBar;
};

TEST_F(TabBarTest, TabSettingsOnCreation)
{
	std::wstring customName = L"Custom name";

	TabSettings tabSettings1;
	tabSettings1.lockState = Tab::LockState::Locked;
	tabSettings1.name = customName;
	const auto *tab1 = m_browser->AddTab(L"c:\\", tabSettings1);
	EXPECT_EQ(tab1->GetLockState(), Tab::LockState::Locked);
	EXPECT_EQ(tab1->GetName(), customName);

	TabSettings tabSettings2;
	tabSettings2.index = 0;
	const auto *tab2 = m_browser->AddTab(L"d:\\", tabSettings2);
	EXPECT_THAT(m_tabBar->GetTabByIndex(0), Ref(*tab2));

	TabSettings tabSettings3;
	tabSettings3.selected = true;
	const auto *tab3 = m_browser->AddTab(L"e:\\", tabSettings3);
	EXPECT_TRUE(m_tabBar->IsTabSelected(*tab3));
}

TEST_F(TabBarTest, OpenNewTabNextToCurrentOptionInitialTab)
{
	m_config.openNewTabNextToCurrent = true;

	// When the openNewTabNextToCurrent option is set and a new tab is created, that tab will be
	// created to the right of the selected tab. When the first tab is created, there will be no
	// selected tab, but that should still be a safe operation.
	m_browser->AddTab(L"c:\\");
}

TEST_F(TabBarTest, OpenNewTabNextToCurrentOption)
{
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"d:\\");
	m_browser->AddTab(L"e:\\");

	m_tabBar->SelectTabAtIndex(0);

	m_config.openNewTabNextToCurrent = true;

	// Since the openNewTabNextToCurrent option was set, this tab should be opened to the right of
	// the selected tab (i.e. to the right of the first tab).
	auto *tab4 = m_browser->AddTab(L"f:\\");
	EXPECT_EQ(m_tabBar->GetTabIndex(*tab4), 1);
}

TEST_F(TabBarTest, TabText)
{
	auto *tab = m_browser->AddTab(L"c:\\path\\to\\folder");

	const auto *tabViewItem = m_tabBar->GetView()->GetTabAtIndex(0);
	EXPECT_EQ(tabViewItem->GetText(), L"folder");

	// The tab's text should be updated when a navigation occurs.
	auto pidl = CreateSimplePidlForTest(L"c:\\windows\\system32");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());
	tab->GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);
	EXPECT_EQ(tabViewItem->GetText(), L"system32");

	// The text should also be updated if a custom name is set.
	std::wstring customName = L"Custom name";
	tab->SetCustomName(customName);
	EXPECT_EQ(tabViewItem->GetText(), customName);
}

TEST_F(TabBarTest, TabIconReturnedAfterDestruction)
{
	int tabId1 = m_browser->AddTabAndReturnId(L"c:\\");
	m_tabBar->CloseTab(m_tabBar->GetTab(tabId1));

	// When the tab is added to the control, its icon will be requested. That will cause the tab to
	// initiate an asynchronous request. This verifies that completing the request after the tab has
	// been closed is safe.
	m_iconFetcher.CompletePendingRequests();
}

TEST_F(TabBarTest, TabTooltip)
{
	std::wstring path = L"c:\\path\\to\\folder";
	auto *tab = m_browser->AddTab(path);

	const auto *tabViewItem = m_tabBar->GetView()->GetTabAtIndex(0);
	EXPECT_THAT(tabViewItem->GetTooltipText(), StrCaseEq(path));

	std::wstring updatedPath = L"c:\\windows\\system32";
	auto pidl = CreateSimplePidlForTest(updatedPath);
	auto navigateParams = NavigateParams::Normal(pidl.Raw());
	tab->GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);
	EXPECT_THAT(tabViewItem->GetTooltipText(), StrCaseEq(updatedPath));
}

TEST_F(TabBarTest, GetTab)
{
	const auto *tab1 = m_browser->AddTab(L"c:\\");
	EXPECT_THAT(m_tabBar->GetTab(tab1->GetId()), Ref(*tab1));

	const auto *tab2 = m_browser->AddTab(L"d:\\");
	EXPECT_THAT(m_tabBar->GetTab(tab2->GetId()), Ref(*tab2));
}

TEST_F(TabBarTest, MaybeGetTab)
{
	const auto *tab1 = m_browser->AddTab(L"c:\\");
	EXPECT_EQ(m_tabBar->MaybeGetTab(tab1->GetId()), tab1);

	const auto tab2 = m_browser->AddTab(L"d:\\");
	EXPECT_EQ(m_tabBar->MaybeGetTab(tab2->GetId()), tab2);

	EXPECT_EQ(m_tabBar->MaybeGetTab(-1), nullptr);
}

TEST_F(TabBarTest, GetSetSelectedTab)
{
	// The first tab should always be selected.
	const auto *tab1 = m_browser->AddTab(L"c:\\");
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab1));

	const auto *tab2 = m_browser->AddTab(L"d:\\");
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab1));

	const auto *tab3 = m_browser->AddTab(L"e:\\");
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab1));

	m_tabBar->SelectTab(*tab3);
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab3));

	m_tabBar->SelectTab(*tab1);
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab1));

	m_tabBar->SelectTab(*tab2);
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab2));
}

TEST_F(TabBarTest, SelectAdjacentTab)
{
	const auto *tab1 = m_browser->AddTab(L"c:\\");
	const auto *tab2 = m_browser->AddTab(L"d:\\");
	const auto *tab3 = m_browser->AddTab(L"e:\\");

	m_tabBar->SelectTabAtIndex(0);

	m_tabBar->SelectAdjacentTab(TabBar::SelectionDirection::Next);
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab2));

	m_tabBar->SelectAdjacentTab(TabBar::SelectionDirection::Next);
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab3));

	// The last tab was selected, so selecting the next tab should result in the selection wrapping
	// around.
	m_tabBar->SelectAdjacentTab(TabBar::SelectionDirection::Next);
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab1));

	m_tabBar->SelectTabAtIndex(2);

	m_tabBar->SelectAdjacentTab(TabBar::SelectionDirection::Previous);
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab2));

	m_tabBar->SelectAdjacentTab(TabBar::SelectionDirection::Previous);
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab1));

	// The first tab was selected, so selecting the previous tab should result in the selection
	// wrapping around again.
	m_tabBar->SelectAdjacentTab(TabBar::SelectionDirection::Previous);
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab3));
}

TEST_F(TabBarTest, SelectTabAtIndex)
{
	const auto *tab1 = m_browser->AddTab(L"c:\\");
	const auto *tab2 = m_browser->AddTab(L"d:\\");
	const auto *tab3 = m_browser->AddTab(L"e:\\");

	m_tabBar->SelectTabAtIndex(1);
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab2));

	m_tabBar->SelectTabAtIndex(2);
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab3));

	m_tabBar->SelectTabAtIndex(0);
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab1));
}

TEST_F(TabBarTest, GetSelectedTabIndex)
{
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"d:\\");
	m_browser->AddTab(L"e:\\");

	m_tabBar->SelectTabAtIndex(1);
	EXPECT_EQ(m_tabBar->GetSelectedTabIndex(), 1);

	m_tabBar->SelectTabAtIndex(2);
	EXPECT_EQ(m_tabBar->GetSelectedTabIndex(), 2);

	m_tabBar->SelectTabAtIndex(0);
	EXPECT_EQ(m_tabBar->GetSelectedTabIndex(), 0);
}

TEST_F(TabBarTest, IsTabSelected)
{
	const auto *tab1 = m_browser->AddTab(L"c:\\");
	const auto *tab2 = m_browser->AddTab(L"d:\\");
	const auto *tab3 = m_browser->AddTab(L"e:\\");

	m_tabBar->SelectTab(*tab3);
	EXPECT_FALSE(m_tabBar->IsTabSelected(*tab1));
	EXPECT_FALSE(m_tabBar->IsTabSelected(*tab2));
	EXPECT_TRUE(m_tabBar->IsTabSelected(*tab3));

	m_tabBar->SelectTab(*tab1);
	EXPECT_TRUE(m_tabBar->IsTabSelected(*tab1));
	EXPECT_FALSE(m_tabBar->IsTabSelected(*tab2));
	EXPECT_FALSE(m_tabBar->IsTabSelected(*tab3));

	m_tabBar->SelectTab(*tab2);
	EXPECT_FALSE(m_tabBar->IsTabSelected(*tab1));
	EXPECT_TRUE(m_tabBar->IsTabSelected(*tab2));
	EXPECT_FALSE(m_tabBar->IsTabSelected(*tab3));
}

TEST_F(TabBarTest, GetTabByIndex)
{
	const auto *tab1 = m_browser->AddTab(L"c:\\");
	const auto *tab2 = m_browser->AddTab(L"d:\\");
	const auto *tab3 = m_browser->AddTab(L"e:\\");

	EXPECT_THAT(m_tabBar->GetTabByIndex(0), Ref(*tab1));
	EXPECT_THAT(m_tabBar->GetTabByIndex(1), Ref(*tab2));
	EXPECT_THAT(m_tabBar->GetTabByIndex(2), Ref(*tab3));
}

TEST_F(TabBarTest, GetTabIndex)
{
	const auto *tab1 = m_browser->AddTab(L"c:\\");
	const auto *tab2 = m_browser->AddTab(L"d:\\");
	const auto *tab3 = m_browser->AddTab(L"e:\\");

	EXPECT_EQ(m_tabBar->GetTabIndex(*tab1), 0);
	EXPECT_EQ(m_tabBar->GetTabIndex(*tab2), 1);
	EXPECT_EQ(m_tabBar->GetTabIndex(*tab3), 2);
}

TEST_F(TabBarTest, GetNumTabs)
{
	EXPECT_EQ(m_tabBar->GetNumTabs(), 0);

	m_browser->AddTab(L"c:\\");
	EXPECT_EQ(m_tabBar->GetNumTabs(), 1);

	m_browser->AddTab(L"d:\\");
	EXPECT_EQ(m_tabBar->GetNumTabs(), 2);

	m_browser->AddTab(L"e:\\");
	EXPECT_EQ(m_tabBar->GetNumTabs(), 3);

	EXPECT_TRUE(m_tabBar->CloseTab(m_tabBar->GetTabByIndex(0)));
	EXPECT_EQ(m_tabBar->GetNumTabs(), 2);

	EXPECT_TRUE(m_tabBar->CloseTab(m_tabBar->GetTabByIndex(0)));
	EXPECT_EQ(m_tabBar->GetNumTabs(), 1);
}

TEST_F(TabBarTest, MoveTab)
{
	const auto *tab1 = m_browser->AddTab(L"c:\\");
	const auto *tab2 = m_browser->AddTab(L"d:\\");
	const auto *tab3 = m_browser->AddTab(L"e:\\");
	const auto *tab4 = m_browser->AddTab(L"f:\\");

	m_tabBar->SelectTabAtIndex(0);

	m_tabBar->MoveTab(*tab2, 2);
	EXPECT_THAT(m_tabBar->GetAllTabsInOrder(), ElementsAre(tab1, tab3, tab2, tab4));

	// The first tab is selected, so it should still be selected after being moved.
	m_tabBar->MoveTab(*tab1, 3);
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab1));
	EXPECT_THAT(m_tabBar->GetAllTabsInOrder(), ElementsAre(tab3, tab2, tab4, tab1));

	m_tabBar->MoveTab(*tab4, 0);
	EXPECT_THAT(m_tabBar->GetAllTabsInOrder(), ElementsAre(tab4, tab3, tab2, tab1));

	// The selected tab should shift position, but should remain selected.
	m_tabBar->MoveTab(*tab2, 3);
	EXPECT_THAT(m_tabBar->GetSelectedTab(), Ref(*tab1));
	EXPECT_THAT(m_tabBar->GetAllTabsInOrder(), ElementsAre(tab4, tab3, tab1, tab2));
}

TEST_F(TabBarTest, DuplicateTab)
{
	m_browser->AddTab(L"c:\\");

	PidlAbsolute pidl;
	const auto *tab2 = m_browser->AddTab(L"d:\\", {}, &pidl);

	m_browser->AddTab(L"e:\\");

	const auto &duplicatedTab = m_tabBar->DuplicateTab(*tab2);

	// The duplicate tab should be opened to the right of the original tab and be selected.
	EXPECT_EQ(m_tabBar->GetTabIndex(duplicatedTab), 2);
	EXPECT_TRUE(m_tabBar->IsTabSelected(duplicatedTab));

	EXPECT_EQ(duplicatedTab.GetShellBrowser()->GetDirectory(), pidl);
}

TEST_F(TabBarTest, DuplicateTabWithCustomName)
{
	auto *tab = m_browser->AddTab(L"c:\\");

	std::wstring customName = L"Custom name";
	tab->SetCustomName(customName);

	const auto &duplicatedTab = m_tabBar->DuplicateTab(*tab);
	EXPECT_EQ(duplicatedTab.GetName(), customName);
}

TEST_F(TabBarTest, DuplicateTabWithLock)
{
	auto *tab = m_browser->AddTab(L"c:\\");

	tab->SetLockState(Tab::LockState::Locked);

	const auto &duplicatedTab = m_tabBar->DuplicateTab(*tab);
	EXPECT_EQ(duplicatedTab.GetLockState(), Tab::LockState::Locked);
}

TEST_F(TabBarTest, DuplicateTabWithAddressLock)
{
	auto *tab = m_browser->AddTab(L"c:\\");

	tab->SetLockState(Tab::LockState::AddressLocked);

	const auto &duplicatedTab = m_tabBar->DuplicateTab(*tab);
	EXPECT_EQ(duplicatedTab.GetLockState(), Tab::LockState::AddressLocked);
}

TEST_F(TabBarTest, DuplicateTabWithFolderSettings)
{
	const auto *tab = m_browser->AddTab(L"c:\\");

	tab->GetShellBrowser()->SetSortMode(SortMode::Size);
	tab->GetShellBrowser()->SetSortDirection(SortDirection::Descending);
	tab->GetShellBrowser()->SetViewMode(ViewMode::Details);
	tab->GetShellBrowser()->SetAutoArrangeEnabled(false);

	const auto &duplicatedTab = m_tabBar->DuplicateTab(*tab);
	EXPECT_EQ(duplicatedTab.GetShellBrowser()->GetFolderSettings(),
		tab->GetShellBrowser()->GetFolderSettings());
}

TEST_F(TabBarTest, DuplicateTabWithColumns)
{
	const auto *tab = m_browser->AddTab(L"c:\\");

	FolderColumns folderColumns;
	folderColumns.realFolderColumns = { { ColumnType::Name, TRUE, 100 } };
	tab->GetShellBrowser()->SetAllColumnSets(folderColumns);

	const auto &duplicatedTab = m_tabBar->DuplicateTab(*tab);
	EXPECT_EQ(duplicatedTab.GetShellBrowser()->GetAllColumnSets(), folderColumns);
}

TEST_F(TabBarTest, DuplicateTabWithHistory)
{
	PidlAbsolute pidl;
	auto *tab = m_browser->AddTab(L"c:\\", {}, &pidl);
	NavigateTab(tab, L"d:\\");
	NavigateTab(tab, L"e:\\");
	tab->GetShellBrowser()->GetNavigationController()->GoBack();

	const auto &duplicatedTab = m_tabBar->DuplicateTab(*tab);

	// The history of the original tab should be duplicated.
	const auto *navigationController = duplicatedTab.GetShellBrowser()->GetNavigationController();
	ASSERT_EQ(navigationController->GetNumHistoryEntries(), 3);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 1);
	EXPECT_EQ(navigationController->GetEntryAtIndex(0)->GetPidl(),
		CreateSimplePidlForTest(L"c:\\"));
	EXPECT_EQ(navigationController->GetEntryAtIndex(1)->GetPidl(),
		CreateSimplePidlForTest(L"d:\\"));
	EXPECT_EQ(navigationController->GetEntryAtIndex(2)->GetPidl(),
		CreateSimplePidlForTest(L"e:\\"));
}

TEST_F(TabBarTest, CloseAllTabs)
{
	m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"d:\\");
	m_browser->AddTab(L"e:\\");

	m_tabBar->CloseAllTabs();
	EXPECT_EQ(m_tabBar->GetNumTabs(), 0);
}

TEST_F(TabBarTest, CloseAllTabsWhenLocked)
{
	int tabId1 = m_browser->AddTabAndReturnId(L"c:\\");
	m_tabBar->GetTab(tabId1).SetLockState(Tab::LockState::Locked);

	int tabId2 = m_browser->AddTabAndReturnId(L"d:\\");
	m_tabBar->GetTab(tabId2).SetLockState(Tab::LockState::AddressLocked);

	// CloseAllTabs() should close every tab, regardless of the lock state of any individual tab.
	m_tabBar->CloseAllTabs();
	EXPECT_EQ(m_tabBar->GetNumTabs(), 0);
}

TEST_F(TabBarTest, CloseTab)
{
	int tabId1 = m_browser->AddTabAndReturnId(L"c:\\");
	int tabId2 = m_browser->AddTabAndReturnId(L"d:\\");
	int tabId3 = m_browser->AddTabAndReturnId(L"e:\\");
	int tabId4 = m_browser->AddTabAndReturnId(L"f:\\");

	m_tabBar->SelectTabAtIndex(0);

	EXPECT_TRUE(m_tabBar->CloseTab(m_tabBar->GetTab(tabId3)));
	ASSERT_THAT(m_tabBar->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId1), Property(&Tab::GetId, tabId2),
			Property(&Tab::GetId, tabId4)));

	// The first tab is selected, so closing it should result in the selection shifting to the next
	// tab.
	EXPECT_TRUE(m_tabBar->CloseTab(m_tabBar->GetTab(tabId1)));
	EXPECT_EQ(m_tabBar->GetSelectedTab().GetId(), tabId2);
	ASSERT_THAT(m_tabBar->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId2), Property(&Tab::GetId, tabId4)));

	m_tabBar->SelectTabAtIndex(1);

	// The last tab is selected, so closing it should result in the selection shifting to the
	// previous tab.
	EXPECT_TRUE(m_tabBar->CloseTab(m_tabBar->GetTab(tabId4)));
	EXPECT_EQ(m_tabBar->GetSelectedTab().GetId(), tabId2);
	EXPECT_THAT(m_tabBar->GetAllTabsInOrder(), ElementsAre(Property(&Tab::GetId, tabId2)));
}

TEST_F(TabBarTest, CloseTabWhenLocked)
{
	int tabId1 = m_browser->AddTabAndReturnId(L"c:\\");
	int tabId2 = m_browser->AddTabAndReturnId(L"d:\\");

	// If a tab is locked, attempting to close it should have no effect.
	m_tabBar->GetTab(tabId2).SetLockState(Tab::LockState::Locked);
	EXPECT_FALSE(m_tabBar->CloseTab(m_tabBar->GetTab(tabId2)));
	EXPECT_THAT(m_tabBar->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId1), Property(&Tab::GetId, tabId2)));

	m_tabBar->GetTab(tabId2).SetLockState(Tab::LockState::AddressLocked);
	EXPECT_FALSE(m_tabBar->CloseTab(m_tabBar->GetTab(tabId2)));
	EXPECT_THAT(m_tabBar->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId1), Property(&Tab::GetId, tabId2)));
}

TEST_F(TabBarTest, CloseButtonClick)
{
	int tabId1 = m_browser->AddTabAndReturnId(L"c:\\");
	m_browser->AddTab(L"d:\\");

	auto *view = m_tabBar->GetView();
	TabViewTestApi testApi(view);

	auto layout = testApi.GetTabContentLayout(1);
	SimulateClick(view->GetHWND(), { layout.closeButton.left, layout.closeButton.top });
	EXPECT_THAT(m_tabBar->GetAllTabsInOrder(), ElementsAre(Property(&Tab::GetId, tabId1)));
}

TEST_F(TabBarTest, GetAllTabs)
{
	const auto *tab1 = m_browser->AddTab(L"c:\\");
	const auto *tab2 = m_browser->AddTab(L"d:\\");
	const auto *tab3 = m_browser->AddTab(L"e:\\");

	EXPECT_THAT(m_tabBar->GetAllTabs(),
		UnorderedElementsAre(Pair(tab1->GetId(), Pointer(tab1)), Pair(tab2->GetId(), Pointer(tab2)),
			Pair(tab3->GetId(), Pointer(tab3))));
}

TEST_F(TabBarTest, GetAllTabsInOrder)
{
	const auto *tab1 = m_browser->AddTab(L"c:\\");
	const auto *tab2 = m_browser->AddTab(L"d:\\");
	const auto *tab3 = m_browser->AddTab(L"e:\\");

	EXPECT_THAT(m_tabBar->GetAllTabsInOrder(), ElementsAre(tab1, tab2, tab3));
}

TEST_F(TabBarTest, DoubleClickOnTab)
{
	int tabId1 = m_browser->AddTabAndReturnId(L"c:\\");
	int tabId2 = m_browser->AddTabAndReturnId(L"d:\\");
	int tabId3 = m_browser->AddTabAndReturnId(L"e:\\");

	m_config.doubleClickTabClose = false;

	// This should have no effect, since the doubleClickTabClose is disabled.
	auto *tabViewItem = m_tabBar->GetView()->GetTabAtIndex(1);
	tabViewItem->OnDoubleClicked({ { 0, 0 }, false, false });
	EXPECT_THAT(m_tabBar->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId1), Property(&Tab::GetId, tabId2),
			Property(&Tab::GetId, tabId3)));

	m_config.doubleClickTabClose = true;

	tabViewItem = m_tabBar->GetView()->GetTabAtIndex(0);
	tabViewItem->OnDoubleClicked({ { 0, 0 }, false, false });
	EXPECT_THAT(m_tabBar->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId2), Property(&Tab::GetId, tabId3)));
}

TEST_F(TabBarTest, MiddleClickOnTab)
{
	int tabId1 = m_browser->AddTabAndReturnId(L"c:\\");
	int tabId2 = m_browser->AddTabAndReturnId(L"d:\\");
	m_browser->AddTab(L"e:\\");

	// Middle-clicking a tab should result in the tab being closed.
	auto *tabViewItem = m_tabBar->GetView()->GetTabAtIndex(2);
	tabViewItem->OnMiddleClicked({ { 0, 0 }, false, false });
	EXPECT_THAT(m_tabBar->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId1), Property(&Tab::GetId, tabId2)));
}

TEST_F(TabBarTest, CreatedSignal)
{
	MockFunction<void(const Tab &tab)> callback;
	m_tabEvents.AddCreatedObserver(callback.AsStdFunction(), TabEventScope::ForBrowser(*m_browser));

	const Tab *callbackTab = nullptr;
	EXPECT_CALL(callback, Call(_))
		.WillOnce([&callbackTab](const auto &createdTab) { callbackTab = &createdTab; });
	const auto *tab = m_browser->AddTab(L"c:\\");
	EXPECT_EQ(tab, callbackTab);
}

TEST_F(TabBarTest, SelectedSignal)
{
	const auto *tab1 = m_browser->AddTab(L"c:\\");
	const auto *tab2 = m_browser->AddTab(L"d:\\");

	MockFunction<void(const Tab &tab)> callback;
	m_tabEvents.AddSelectedObserver(callback.AsStdFunction(),
		TabEventScope::ForBrowser(*m_browser));

	MockFunction<void(int)> check;
	{
		InSequence seq;

		EXPECT_CALL(callback, Call(Ref(*tab2)));
		EXPECT_CALL(check, Call(1));
		EXPECT_CALL(callback, Call(Ref(*tab1)));
		EXPECT_CALL(check, Call(2));
		EXPECT_CALL(callback, Call(Ref(*tab2)));
		EXPECT_CALL(check, Call(3));
		EXPECT_CALL(callback, Call(Ref(*tab1)));
	}

	m_tabBar->SelectTab(*tab2);
	check.Call(1);
	m_tabBar->SelectTabAtIndex(0);
	check.Call(2);
	m_tabBar->SelectAdjacentTab(TabBar::SelectionDirection::Next);
	check.Call(3);
	m_tabBar->SelectAdjacentTab(TabBar::SelectionDirection::Previous);
}

TEST_F(TabBarTest, SelectedSignalOnFirstTabCreation)
{
	MockFunction<void(const Tab &tab)> callback;
	m_tabEvents.AddSelectedObserver(callback.AsStdFunction(),
		TabEventScope::ForBrowser(*m_browser));

	// The first tab will be selected on creation. That should trigger a selection event.
	const Tab *callbackTab = nullptr;
	EXPECT_CALL(callback, Call(_))
		.WillOnce([&callbackTab](const auto &selectedTab) { callbackTab = &selectedTab; });
	const auto *tab = m_browser->AddTab(L"c:\\");
	EXPECT_EQ(tab, callbackTab);
}

TEST_F(TabBarTest, SelectedSignalOnSelectedTabCreation)
{
	m_browser->AddTab(L"c:\\");

	MockFunction<void(const Tab &tab)> callback;
	m_tabEvents.AddSelectedObserver(callback.AsStdFunction(),
		TabEventScope::ForBrowser(*m_browser));

	const Tab *callbackTab = nullptr;
	EXPECT_CALL(callback, Call(_))
		.WillOnce([&callbackTab](const auto &selectedTab) { callbackTab = &selectedTab; });
	const auto *tab2 = m_browser->AddTab(L"c:\\", { .selected = true });
	EXPECT_EQ(tab2, callbackTab);
}

TEST_F(TabBarTest, SelectedSignalOnClose)
{
	int tabId1 = m_browser->AddTabAndReturnId(L"c:\\");
	int tabId2 = m_browser->AddTabAndReturnId(L"d:\\");

	m_tabBar->SelectTab(m_tabBar->GetTab(tabId2));

	MockFunction<void(const Tab &tab)> callback;
	m_tabEvents.AddSelectedObserver(callback.AsStdFunction(),
		TabEventScope::ForBrowser(*m_browser));

	// The second tab is selected, so closing it should result in the first tab being selected and a
	// signal being emitted.
	EXPECT_CALL(callback, Call(Property(&Tab::GetId, tabId1)));
	m_tabBar->CloseTab(m_tabBar->GetTab(tabId2));
}

TEST_F(TabBarTest, MovedSignal)
{
	const auto *tab1 = m_browser->AddTab(L"c:\\");
	m_browser->AddTab(L"d:\\");
	m_browser->AddTab(L"e:\\");

	MockFunction<void(const Tab &tab, int fromIndex, int toIndex)> callback;
	m_tabEvents.AddMovedObserver(callback.AsStdFunction(), TabEventScope::ForBrowser(*m_browser));

	EXPECT_CALL(callback, Call(Ref(*tab1), 0, 2));
	m_tabBar->MoveTab(*tab1, 2);
}

TEST_F(TabBarTest, PreRemovalSignal)
{
	m_browser->AddTab(L"c:\\");
	int tabId2 = m_browser->AddTabAndReturnId(L"d:\\");
	m_browser->AddTab(L"e:\\");

	MockFunction<void(const Tab &tab, int index)> callback;
	m_tabEvents.AddPreRemovalObserver(callback.AsStdFunction(),
		TabEventScope::ForBrowser(*m_browser));

	EXPECT_CALL(callback, Call(Property(&Tab::GetId, tabId2), 1));
	m_tabBar->CloseTab(m_tabBar->GetTab(tabId2));
}

TEST_F(TabBarTest, RemovedSignal)
{
	m_browser->AddTab(L"c:\\");
	int tabId2 = m_browser->AddTabAndReturnId(L"d:\\");
	m_browser->AddTab(L"e:\\");

	MockFunction<void(const Tab &tab)> callback;
	m_tabEvents.AddRemovedObserver(callback.AsStdFunction(), TabEventScope::ForBrowser(*m_browser));

	EXPECT_CALL(callback, Call(Property(&Tab::GetId, tabId2)));
	m_tabBar->CloseTab(m_tabBar->GetTab(tabId2));
}

TEST_F(TabBarTest, CloseAllTabsSignals)
{
	int tabId1 = m_browser->AddTabAndReturnId(L"c:\\");
	int tabId2 = m_browser->AddTabAndReturnId(L"d:\\");

	MockFunction<void(const Tab &tab, int index)> preRemovalCallback;
	m_tabEvents.AddPreRemovalObserver(preRemovalCallback.AsStdFunction(),
		TabEventScope::ForBrowser(*m_browser));

	MockFunction<void(const Tab &tab)> removedCallback;
	m_tabEvents.AddRemovedObserver(removedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(*m_browser));

	{
		InSequence seq;

		EXPECT_CALL(preRemovalCallback, Call(Property(&Tab::GetId, tabId1), 0));
		EXPECT_CALL(removedCallback, Call(Property(&Tab::GetId, tabId1)));
	}

	{
		InSequence seq;

		EXPECT_CALL(preRemovalCallback, Call(Property(&Tab::GetId, tabId2), 1));
		EXPECT_CALL(removedCallback, Call(Property(&Tab::GetId, tabId2)));
	}

	m_tabBar->CloseAllTabs();
}
