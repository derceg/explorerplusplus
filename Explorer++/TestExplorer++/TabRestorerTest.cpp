// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabRestorer.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class TabRestorerTest : public BrowserTestBase
{
protected:
	TabRestorerTest() : m_tabRestorer(&m_tabEvents, &m_browserList)
	{
	}

	TabRestorer m_tabRestorer;
};

TEST_F(TabRestorerTest, InitialState)
{
	EXPECT_EQ(m_tabRestorer.GetClosedTabs().size(), 0u);
	EXPECT_TRUE(m_tabRestorer.IsEmpty());
}

TEST_F(TabRestorerTest, GetTabs)
{
	auto *browser = AddBrowser();
	int tabId1 = browser->AddTabAndReturnId(L"c:\\");
	int tabId2 = browser->AddTabAndReturnId(L"d:\\");
	browser->AddTab(L"e:\\");

	auto *tabContainer = browser->GetActiveTabContainer();
	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTab(tabId1)));
	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTab(tabId2)));

	// tab2 was closed most recently, so its entry should appear first.
	const auto &closedTabs = m_tabRestorer.GetClosedTabs();
	EXPECT_THAT(closedTabs,
		ElementsAre(Pointee(Field(&PreservedTab::id, tabId2)),
			Pointee(Field(&PreservedTab::id, tabId1))));

	for (const auto &closedTab : closedTabs)
	{
		EXPECT_EQ(closedTab.get(), m_tabRestorer.GetTabById(closedTab->id));
	}

	EXPECT_NE(m_tabRestorer.RestoreTabById(tabId1), nullptr);
	EXPECT_THAT(closedTabs, ElementsAre(Pointee(Field(&PreservedTab::id, tabId2))));

	EXPECT_NE(m_tabRestorer.RestoreLastTab(), nullptr);
	EXPECT_TRUE(closedTabs.empty());
}

TEST_F(TabRestorerTest, RestoreLastTab)
{
	auto *browser = AddBrowser();
	browser->AddTab(L"c:\\");
	browser->AddTab(L"d:\\");
	browser->AddTab(L"e:\\");

	auto *tabContainer = browser->GetActiveTabContainer();
	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTabByIndex(1)));

	auto *restoredTab = m_tabRestorer.RestoreLastTab();
	ASSERT_NE(restoredTab, nullptr);

	// The tab should be restored to its original index. It should also be selected.
	ASSERT_EQ(restoredTab->GetBrowser(), browser);
	EXPECT_EQ(tabContainer->GetTabIndex(*restoredTab), 1);
	EXPECT_TRUE(tabContainer->IsTabSelected(*restoredTab));
	EXPECT_EQ(restoredTab->GetShellBrowser()->GetDirectory(), CreateSimplePidlForTest(L"d:\\"));
}

TEST_F(TabRestorerTest, RestoreLastTabIntoDifferentBrowser)
{
	auto *browser1 = AddBrowser();
	browser1->AddTab(L"c:\\");
	browser1->AddTab(L"d:\\");

	auto *browser2 = AddBrowser();
	browser2->AddTab(L"e:\\");

	EXPECT_TRUE(browser1->GetActiveTabContainer()->CloseTab(
		browser1->GetActiveTabContainer()->GetTabByIndex(0)));

	RemoveBrowser(browser1);
	browser1 = nullptr;

	auto *restoredTab = m_tabRestorer.RestoreLastTab();
	ASSERT_NE(restoredTab, nullptr);

	// The tab was restored into a different browser, so it should be added to the end, rather than
	// its original index.
	ASSERT_EQ(restoredTab->GetBrowser(), browser2);
	EXPECT_EQ(browser2->GetActiveTabContainer()->GetTabIndex(*restoredTab), 1);
	EXPECT_TRUE(browser2->GetActiveTabContainer()->IsTabSelected(*restoredTab));
	EXPECT_EQ(restoredTab->GetShellBrowser()->GetDirectory(), CreateSimplePidlForTest(L"c:\\"));
}

TEST_F(TabRestorerTest, RestoreWithoutBrowser)
{
	auto *browser = AddBrowser();
	browser->AddTab(L"c:\\");
	browser->AddTab(L"d:\\");

	auto *tabContainer = browser->GetActiveTabContainer();
	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTabByIndex(1)));

	RemoveBrowser(browser);
	browser = nullptr;

	// All browsers have been closed, so it's not possible to restore a tab.
	auto *restoredTab = m_tabRestorer.RestoreLastTab();
	EXPECT_EQ(restoredTab, nullptr);
}

TEST_F(TabRestorerTest, RestoreTabById)
{
	auto *browser = AddBrowser();
	browser->AddTab(L"c:\\");
	int tabId2 = browser->AddTabAndReturnId(L"d:\\");

	auto *tabContainer = browser->GetActiveTabContainer();
	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTab(tabId2)));

	auto *restoredTab = m_tabRestorer.RestoreTabById(tabId2);
	ASSERT_NE(restoredTab, nullptr);

	ASSERT_EQ(restoredTab->GetBrowser(), browser);
	EXPECT_EQ(tabContainer->GetTabIndex(*restoredTab), 1);
	EXPECT_TRUE(tabContainer->IsTabSelected(*restoredTab));
	EXPECT_EQ(restoredTab->GetShellBrowser()->GetDirectory(), CreateSimplePidlForTest(L"d:\\"));
}

TEST_F(TabRestorerTest, RestoreTabWithHistory)
{
	auto *browser = AddBrowser();
	browser->AddTab(L"c:\\");

	auto *tab2 = browser->AddTab(L"c:\\");
	NavigateTab(tab2, L"d:\\");
	NavigateTab(tab2, L"e:\\");
	tab2->GetShellBrowser()->GetNavigationController()->GoBack();

	auto *tabContainer = browser->GetActiveTabContainer();
	EXPECT_TRUE(tabContainer->CloseTab(*tab2));
	tab2 = nullptr;

	auto *restoredTab = m_tabRestorer.RestoreLastTab();
	ASSERT_NE(restoredTab, nullptr);

	auto *navigationController = restoredTab->GetShellBrowser()->GetNavigationController();
	ASSERT_EQ(navigationController->GetNumHistoryEntries(), 3);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 1);
	EXPECT_EQ(navigationController->GetEntryAtIndex(0)->GetPidl(),
		CreateSimplePidlForTest(L"c:\\"));
	EXPECT_EQ(navigationController->GetEntryAtIndex(1)->GetPidl(),
		CreateSimplePidlForTest(L"d:\\"));
	EXPECT_EQ(navigationController->GetEntryAtIndex(2)->GetPidl(),
		CreateSimplePidlForTest(L"e:\\"));
}

TEST_F(TabRestorerTest, RestoreTabWithCustomName)
{
	auto *browser = AddBrowser();
	browser->AddTab(L"c:\\");
	int tabId2 = browser->AddTabAndReturnId(L"d:\\");

	auto *tabContainer = browser->GetActiveTabContainer();

	std::wstring customName = L"CustomName";
	tabContainer->GetTab(tabId2).SetCustomName(customName);

	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTab(tabId2)));

	auto *restoredTab = m_tabRestorer.RestoreTabById(tabId2);
	ASSERT_NE(restoredTab, nullptr);

	EXPECT_TRUE(restoredTab->GetUseCustomName());
	EXPECT_EQ(restoredTab->GetName(), customName);
}

TEST_F(TabRestorerTest, RestoreTabWithColumns)
{
	auto *browser = AddBrowser();
	browser->AddTab(L"c:\\");
	int tabId2 = browser->AddTabAndReturnId(L"d:\\");

	auto *tabContainer = browser->GetActiveTabContainer();

	FolderColumns folderColumns;
	folderColumns.realFolderColumns = { { ColumnType::Name, TRUE, 100 } };
	tabContainer->GetTab(tabId2).GetShellBrowser()->SetAllColumnSets(folderColumns);

	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTab(tabId2)));

	auto *restoredTab = m_tabRestorer.RestoreTabById(tabId2);
	ASSERT_NE(restoredTab, nullptr);

	EXPECT_EQ(restoredTab->GetShellBrowser()->GetAllColumnSets(), folderColumns);
}

TEST_F(TabRestorerTest, StateDuringRestore)
{
	auto *browser = AddBrowser();
	browser->AddTab(L"c:\\");
	int tabId2 = browser->AddTabAndReturnId(L"d:\\");

	auto *tabContainer = browser->GetActiveTabContainer();
	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTab(tabId2)));

	MockFunction<void(Tab & tab)> callback;
	EXPECT_CALL(callback, Call(_))
		.WillOnce(
			[this, tabId2](Tab &tab)
			{
				UNREFERENCED_PARAMETER(tab);

				// This callback will be invoked when the restored tab is created. At this point,
				// RestoreLastTab() hasn't returned, but the restored tab should have already been
				// removed (otherwise it would be possible to attempt to restore the same tab
				// twice).
				EXPECT_EQ(m_tabRestorer.GetTabById(tabId2), nullptr);
			});

	m_tabEvents.AddCreatedObserver(callback.AsStdFunction(), TabEventScope::ForBrowser(*browser));

	EXPECT_NE(m_tabRestorer.RestoreLastTab(), nullptr);
}

TEST_F(TabRestorerTest, ItemsChangedSignal)
{
	auto *browser = AddBrowser();
	browser->AddTab(L"c:\\");
	int tabId2 = browser->AddTabAndReturnId(L"d:\\");
	int tabId3 = browser->AddTabAndReturnId(L"e:\\");

	MockFunction<void()> callback;
	m_tabRestorer.AddItemsChangedObserver(callback.AsStdFunction());

	// See https://google.github.io/googletest/gmock_cook_book.html#UsingCheckPoints.
	//
	// This verifies that each of the functions below invokes the items changed callback.
	MockFunction<void(int)> check;
	{
		InSequence seq;

		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(1));
		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(2));
		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(3));
		EXPECT_CALL(callback, Call());
	}

	auto *tabContainer = browser->GetActiveTabContainer();
	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTab(tabId2)));
	check.Call(1);

	EXPECT_TRUE(tabContainer->CloseTab(tabContainer->GetTab(tabId3)));
	check.Call(2);

	EXPECT_NE(m_tabRestorer.RestoreTabById(tabId3), nullptr);
	check.Call(3);

	EXPECT_NE(m_tabRestorer.RestoreLastTab(), nullptr);
}
