// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabContextMenu.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "MainResource.h"
#include "MenuViewFake.h"
#include "PidlTestHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include <gtest/gtest.h>

using namespace testing;

class TabContextMenuTest : public BrowserTestBase
{
protected:
	struct HistoryEntrySnapshot
	{
		const Tab *const tab;
		const PidlAbsolute pidl;
		const int originalEntryId;
	};

	TabContextMenuTest() :
		m_browser(AddBrowser()),
		m_tabContainer(m_browser->GetActiveTabContainer())
	{
	}

	HistoryEntrySnapshot TakeHistoryEntrySnapshot(const Tab *tab)
	{
		auto *currentEntry = tab->GetShellBrowser()->GetNavigationController()->GetCurrentEntry();
		return { tab, currentEntry->GetPidl(), currentEntry->GetId() };
	}

	void VerifyTabRefreshed(const HistoryEntrySnapshot &snapshot)
	{
		auto *currentEntry =
			snapshot.tab->GetShellBrowser()->GetNavigationController()->GetCurrentEntry();
		EXPECT_NE(currentEntry->GetId(), snapshot.originalEntryId);
		EXPECT_EQ(currentEntry->GetPidl(), snapshot.pidl);
	}

	BrowserWindowFake *const m_browser;
	TabContainer *const m_tabContainer;
};

TEST_F(TabContextMenuTest, DuplicateTab)
{
	PidlAbsolute pidl;
	auto *tab1 = m_browser->AddTab(L"c:\\users", {}, &pidl);

	MenuViewFake menuView;
	TabContextMenu menu(&menuView, &m_acceleratorManager, tab1, m_tabContainer, &m_tabEvents,
		&m_resourceLoader);

	menuView.SelectItem(IDM_TAB_CONTEXT_MENU_DUPLICATE_TAB, false, false);
	ASSERT_EQ(m_tabContainer->GetNumTabs(), 2);

	const auto &tab2 = m_tabContainer->GetTabByIndex(1);
	EXPECT_EQ(tab2.GetShellBrowser()->GetDirectory(), pidl);
}

TEST_F(TabContextMenuTest, OpenParentInNewTab)
{
	auto *tab1 = m_browser->AddTab(L"c:\\windows\\system32");

	MenuViewFake menuView;
	TabContextMenu menu(&menuView, &m_acceleratorManager, tab1, m_tabContainer, &m_tabEvents,
		&m_resourceLoader);

	menuView.SelectItem(IDM_TAB_CONTEXT_MENU_OPEN_PARENT_IN_NEW_TAB, false, false);
	ASSERT_EQ(m_tabContainer->GetNumTabs(), 2);

	const auto &tab2 = m_tabContainer->GetTabByIndex(1);
	EXPECT_EQ(tab2.GetShellBrowser()->GetDirectory(), CreateSimplePidlForTest(L"c:\\windows"));
}

TEST_F(TabContextMenuTest, Refresh)
{
	auto *tab = m_browser->AddTab(L"c:\\");
	auto snapshot = TakeHistoryEntrySnapshot(tab);

	MenuViewFake menuView;
	TabContextMenu menu(&menuView, &m_acceleratorManager, tab, m_tabContainer, &m_tabEvents,
		&m_resourceLoader);

	menuView.SelectItem(IDM_TAB_CONTEXT_MENU_REFRESH, false, false);
	VerifyTabRefreshed(snapshot);
}

TEST_F(TabContextMenuTest, RefreshAll)
{
	auto *tab1 = m_browser->AddTab(L"c:\\");
	auto *tab2 = m_browser->AddTab(L"d:\\");
	auto *tab3 = m_browser->AddTab(L"e:\\");

	std::vector<HistoryEntrySnapshot> snapshots = { TakeHistoryEntrySnapshot(tab1),
		TakeHistoryEntrySnapshot(tab2), TakeHistoryEntrySnapshot(tab3) };

	MenuViewFake menuView;
	TabContextMenu menu(&menuView, &m_acceleratorManager, tab1, m_tabContainer, &m_tabEvents,
		&m_resourceLoader);

	menuView.SelectItem(IDM_TAB_CONTEXT_MENU_REFRESH_ALL, false, false);

	for (const auto &snapshot : snapshots)
	{
		VerifyTabRefreshed(snapshot);
	}
}

TEST_F(TabContextMenuTest, LockTab)
{
	auto *tab = m_browser->AddTab(L"c:\\");

	MenuViewFake menuView;
	TabContextMenu menu(&menuView, &m_acceleratorManager, tab, m_tabContainer, &m_tabEvents,
		&m_resourceLoader);

	menuView.SelectItem(IDM_TAB_CONTEXT_MENU_LOCK_TAB, false, false);
	EXPECT_EQ(tab->GetLockState(), Tab::LockState::Locked);

	// Selecting the item again should toggle the lock state.
	menuView.SelectItem(IDM_TAB_CONTEXT_MENU_LOCK_TAB, false, false);
	EXPECT_EQ(tab->GetLockState(), Tab::LockState::NotLocked);
}

TEST_F(TabContextMenuTest, LockTabAndAddress)
{
	auto *tab = m_browser->AddTab(L"c:\\");

	MenuViewFake menuView;
	TabContextMenu menu(&menuView, &m_acceleratorManager, tab, m_tabContainer, &m_tabEvents,
		&m_resourceLoader);

	menuView.SelectItem(IDM_TAB_CONTEXT_MENU_LOCK_TAB_AND_ADDRESS, false, false);
	EXPECT_EQ(tab->GetLockState(), Tab::LockState::AddressLocked);

	menuView.SelectItem(IDM_TAB_CONTEXT_MENU_LOCK_TAB_AND_ADDRESS, false, false);
	EXPECT_EQ(tab->GetLockState(), Tab::LockState::NotLocked);
}

TEST_F(TabContextMenuTest, CloseTab)
{
	int tabId1 = m_browser->AddTabAndReturnId(L"c:\\");
	int tabId2 = m_browser->AddTabAndReturnId(L"d:\\");
	int tabId3 = m_browser->AddTabAndReturnId(L"e:\\");

	MenuViewFake menuView;
	TabContextMenu menu(&menuView, &m_acceleratorManager, &m_tabContainer->GetTab(tabId2),
		m_tabContainer, &m_tabEvents, &m_resourceLoader);

	menuView.SelectItem(IDM_TAB_CONTEXT_MENU_CLOSE_TAB, false, false);
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId1), Property(&Tab::GetId, tabId3)));
}

TEST_F(TabContextMenuTest, CloseOtherTabs)
{
	m_browser->AddTab(L"c:\\");
	int tabId2 = m_browser->AddTabAndReturnId(L"d:\\");
	m_browser->AddTab(L"e:\\");
	m_browser->AddTab(L"f:\\");

	MenuViewFake menuView;
	TabContextMenu menu(&menuView, &m_acceleratorManager, &m_tabContainer->GetTab(tabId2),
		m_tabContainer, &m_tabEvents, &m_resourceLoader);

	menuView.SelectItem(IDM_TAB_CONTEXT_MENU_CLOSE_OTHER_TABS, false, false);
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(), ElementsAre(Property(&Tab::GetId, tabId2)));
}

TEST_F(TabContextMenuTest, CloseTabsToRight)
{
	int tabId1 = m_browser->AddTabAndReturnId(L"c:\\");
	int tabId2 = m_browser->AddTabAndReturnId(L"d:\\");
	m_browser->AddTab(L"e:\\");
	m_browser->AddTab(L"f:\\");

	MenuViewFake menuView;
	TabContextMenu menu(&menuView, &m_acceleratorManager, &m_tabContainer->GetTab(tabId2),
		m_tabContainer, &m_tabEvents, &m_resourceLoader);

	menuView.SelectItem(IDM_TAB_CONTEXT_MENU_CLOSE_TABS_TO_RIGHT, false, false);
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId1), Property(&Tab::GetId, tabId2)));
}

TEST_F(TabContextMenuTest, SelectionAfterTabClosed)
{
	int tabId1 = m_browser->AddTabAndReturnId(L"c:\\");
	int tabId2 = m_browser->AddTabAndReturnId(L"d:\\");
	int tabId3 = m_browser->AddTabAndReturnId(L"e:\\");
	int tabId4 = m_browser->AddTabAndReturnId(L"f:\\");

	MenuViewFake menuView;
	TabContextMenu menu(&menuView, &m_acceleratorManager, &m_tabContainer->GetTab(tabId2),
		m_tabContainer, &m_tabEvents, &m_resourceLoader);

	EXPECT_TRUE(m_tabContainer->CloseTab(m_tabContainer->GetTab(tabId2)));

	// The tab associated with the context menu was closed, so selecting a menu item should have no
	// effect.
	menuView.SelectItem(IDM_TAB_CONTEXT_MENU_CLOSE_TABS_TO_RIGHT, false, false);
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId1), Property(&Tab::GetId, tabId3),
			Property(&Tab::GetId, tabId4)));
}
