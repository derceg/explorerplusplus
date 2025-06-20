// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabContainer.h"
#include "AcceleratorManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "Config.h"
#include "MainTabView.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "ShellBrowser/ShellBrowserFactory.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "ShellTestHelper.h"
#include "TabEvents.h"
#include "TabNavigationMock.h"
#include "Win32ResourceLoader.h"
#include "../Helper/CachedIcons.h"
#include <gtest/gtest.h>

using namespace testing;

namespace
{

class ShellBrowserFactoryFake : public ShellBrowserFactory
{
public:
	ShellBrowserFactoryFake(NavigationEvents *navigationEvents, TabNavigationMock *tabNavigation) :
		m_navigationEvents(navigationEvents),
		m_tabNavigation(tabNavigation)
	{
	}

	std::unique_ptr<ShellBrowser> Create(const PidlAbsolute &initialPidl,
		const FolderSettings &folderSettings, const FolderColumns *initialColumns) override
	{
		UNREFERENCED_PARAMETER(initialPidl);
		UNREFERENCED_PARAMETER(folderSettings);
		UNREFERENCED_PARAMETER(initialColumns);

		return std::make_unique<ShellBrowserFake>(m_navigationEvents, m_tabNavigation);
	}

	std::unique_ptr<ShellBrowser> CreateFromPreserved(
		const std::vector<std::unique_ptr<PreservedHistoryEntry>> &history, int currentEntry,
		const PreservedFolderState &preservedFolderState) override
	{
		UNREFERENCED_PARAMETER(preservedFolderState);

		return std::make_unique<ShellBrowserFake>(m_navigationEvents, m_tabNavigation, history,
			currentEntry);
	}

private:
	NavigationEvents *const m_navigationEvents;
	TabNavigationMock *const m_tabNavigation;
};

}

class TabContainerTest : public BrowserTestBase
{
protected:
	TabContainerTest() :
		m_resourceLoader(GetModuleHandle(nullptr), IconSet::Color, nullptr, nullptr),
		m_cachedIcons(10),
		m_browser(AddBrowser()),
		m_shellBrowserFactory(&m_navigationEvents, &m_tabNavigation),
		m_tabView(MainTabView::Create(m_browser->GetHWND(), &m_config, &m_resourceLoader)),
		m_tabContainer(TabContainer::Create(m_tabView, m_browser, &m_shellBrowserFactory,
			&m_tabEvents, &m_shellBrowserEvents, &m_navigationEvents, nullptr, &m_cachedIcons,
			&m_bookmarkTree, &m_acceleratorManager, &m_config, &m_resourceLoader))
	{
	}

	Tab &AddTab(const std::wstring &path, const TabSettings &tabSettings = {})
	{
		auto pidl = CreateSimplePidlForTest(path);
		auto navigateParams = NavigateParams::Normal(pidl.Raw());
		return m_tabContainer->CreateNewTab(navigateParams, tabSettings);
	}

	[[nodiscard]] int AddTabAndReturnId(const std::wstring &path,
		const TabSettings &tabSettings = {})
	{
		const auto &tab = AddTab(path, tabSettings);
		return tab.GetId();
	}

	BookmarkTree m_bookmarkTree;
	AcceleratorManager m_acceleratorManager;
	Config m_config;
	Win32ResourceLoader m_resourceLoader;
	CachedIcons m_cachedIcons;

	TabEvents m_tabEvents;
	ShellBrowserEvents m_shellBrowserEvents;
	NavigationEvents m_navigationEvents;

	BrowserWindowFake *const m_browser;
	TabNavigationMock m_tabNavigation;
	ShellBrowserFactoryFake m_shellBrowserFactory;
	MainTabView *const m_tabView;
	TabContainer *const m_tabContainer;
};

TEST_F(TabContainerTest, TabSettingsOnCreation)
{
	std::wstring customName = L"Custom name";

	TabSettings tabSettings1;
	tabSettings1.lockState = Tab::LockState::Locked;
	tabSettings1.name = customName;
	const auto &tab1 = AddTab(L"c:\\", tabSettings1);
	EXPECT_EQ(tab1.GetLockState(), Tab::LockState::Locked);
	EXPECT_EQ(tab1.GetName(), customName);

	TabSettings tabSettings2;
	tabSettings2.index = 0;
	const auto &tab2 = AddTab(L"d:\\", tabSettings2);
	EXPECT_THAT(m_tabContainer->GetTabByIndex(0), Ref(tab2));

	TabSettings tabSettings3;
	tabSettings3.selected = true;
	const auto &tab3 = AddTab(L"e:\\", tabSettings3);
	EXPECT_TRUE(m_tabContainer->IsTabSelected(tab3));
}

TEST_F(TabContainerTest, OpenNewTabNextToCurrentOption)
{
	m_config.openNewTabNextToCurrent = true;

	// When the openNewTabNextToCurrent option is set and a new tab is created, that tab will be
	// created to the right of the selected tab. When the first tab is created, there will be no
	// selected tab, but that should still be a safe operation.
	AddTab(L"c:\\");
}

TEST_F(TabContainerTest, TabText)
{
	auto &tab = AddTab(L"c:\\path\\to\\folder");

	const auto *tabViewItem = m_tabView->GetTabAtIndex(0);
	EXPECT_EQ(tabViewItem->GetText(), L"folder");

	// The tab's text should be updated when a navigation occurs.
	auto pidl = CreateSimplePidlForTest(L"c:\\windows\\system32");
	auto navigateParams = NavigateParams::Normal(pidl.Raw());
	tab.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);
	EXPECT_EQ(tabViewItem->GetText(), L"system32");

	// The text should also be updated if a custom name is set.
	std::wstring customName = L"Custom name";
	tab.SetCustomName(customName);
	EXPECT_EQ(tabViewItem->GetText(), customName);
}

TEST_F(TabContainerTest, TabTooltip)
{
	std::wstring path = L"c:\\path\\to\\folder";
	auto &tab = AddTab(path);

	const auto *tabViewItem = m_tabView->GetTabAtIndex(0);
	EXPECT_THAT(tabViewItem->GetTooltipText(), StrCaseEq(path));

	std::wstring updatedPath = L"c:\\windows\\system32";
	auto pidl = CreateSimplePidlForTest(updatedPath);
	auto navigateParams = NavigateParams::Normal(pidl.Raw());
	tab.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);
	EXPECT_THAT(tabViewItem->GetTooltipText(), StrCaseEq(updatedPath));
}

TEST_F(TabContainerTest, GetTab)
{
	const auto &tab1 = AddTab(L"c:\\");
	EXPECT_THAT(m_tabContainer->GetTab(tab1.GetId()), Ref(tab1));

	const auto &tab2 = AddTab(L"d:\\");
	EXPECT_THAT(m_tabContainer->GetTab(tab2.GetId()), Ref(tab2));
}

TEST_F(TabContainerTest, GetTabOptional)
{
	const auto &tab1 = AddTab(L"c:\\");
	EXPECT_EQ(m_tabContainer->GetTabOptional(tab1.GetId()), &tab1);

	const auto &tab2 = AddTab(L"d:\\");
	EXPECT_EQ(m_tabContainer->GetTabOptional(tab2.GetId()), &tab2);

	EXPECT_EQ(m_tabContainer->GetTabOptional(-1), nullptr);
}

TEST_F(TabContainerTest, GetSetSelectedTab)
{
	// The first tab should always be selected.
	const auto &tab1 = AddTab(L"c:\\");
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab1));

	const auto &tab2 = AddTab(L"d:\\");
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab1));

	const auto &tab3 = AddTab(L"e:\\");
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab1));

	m_tabContainer->SelectTab(tab3);
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab3));

	m_tabContainer->SelectTab(tab1);
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab1));

	m_tabContainer->SelectTab(tab2);
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab2));
}

TEST_F(TabContainerTest, SelectAdjacentTab)
{
	const auto &tab1 = AddTab(L"c:\\");
	const auto &tab2 = AddTab(L"d:\\");
	const auto &tab3 = AddTab(L"e:\\");

	m_tabContainer->SelectTabAtIndex(0);

	m_tabContainer->SelectAdjacentTab(TabContainer::SelectionDirection::Next);
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab2));

	m_tabContainer->SelectAdjacentTab(TabContainer::SelectionDirection::Next);
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab3));

	// The last tab was selected, so selecting the next tab should result in the selection wrapping
	// around.
	m_tabContainer->SelectAdjacentTab(TabContainer::SelectionDirection::Next);
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab1));

	m_tabContainer->SelectTabAtIndex(2);

	m_tabContainer->SelectAdjacentTab(TabContainer::SelectionDirection::Previous);
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab2));

	m_tabContainer->SelectAdjacentTab(TabContainer::SelectionDirection::Previous);
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab1));

	// The first tab was selected, so selecting the previous tab should result in the selection
	// wrapping around again.
	m_tabContainer->SelectAdjacentTab(TabContainer::SelectionDirection::Previous);
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab3));
}

TEST_F(TabContainerTest, SelectTabAtIndex)
{
	const auto &tab1 = AddTab(L"c:\\");
	const auto &tab2 = AddTab(L"d:\\");
	const auto &tab3 = AddTab(L"e:\\");

	m_tabContainer->SelectTabAtIndex(1);
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab2));

	m_tabContainer->SelectTabAtIndex(2);
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab3));

	m_tabContainer->SelectTabAtIndex(0);
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab1));
}

TEST_F(TabContainerTest, GetSelectedTabIndex)
{
	AddTab(L"c:\\");
	AddTab(L"d:\\");
	AddTab(L"e:\\");

	m_tabContainer->SelectTabAtIndex(1);
	EXPECT_EQ(m_tabContainer->GetSelectedTabIndex(), 1);

	m_tabContainer->SelectTabAtIndex(2);
	EXPECT_EQ(m_tabContainer->GetSelectedTabIndex(), 2);

	m_tabContainer->SelectTabAtIndex(0);
	EXPECT_EQ(m_tabContainer->GetSelectedTabIndex(), 0);
}

TEST_F(TabContainerTest, IsTabSelected)
{
	const auto &tab1 = AddTab(L"c:\\");
	const auto &tab2 = AddTab(L"d:\\");
	const auto &tab3 = AddTab(L"e:\\");

	m_tabContainer->SelectTab(tab3);
	EXPECT_FALSE(m_tabContainer->IsTabSelected(tab1));
	EXPECT_FALSE(m_tabContainer->IsTabSelected(tab2));
	EXPECT_TRUE(m_tabContainer->IsTabSelected(tab3));

	m_tabContainer->SelectTab(tab1);
	EXPECT_TRUE(m_tabContainer->IsTabSelected(tab1));
	EXPECT_FALSE(m_tabContainer->IsTabSelected(tab2));
	EXPECT_FALSE(m_tabContainer->IsTabSelected(tab3));

	m_tabContainer->SelectTab(tab2);
	EXPECT_FALSE(m_tabContainer->IsTabSelected(tab1));
	EXPECT_TRUE(m_tabContainer->IsTabSelected(tab2));
	EXPECT_FALSE(m_tabContainer->IsTabSelected(tab3));
}

TEST_F(TabContainerTest, GetTabByIndex)
{
	const auto &tab1 = AddTab(L"c:\\");
	const auto &tab2 = AddTab(L"d:\\");
	const auto &tab3 = AddTab(L"e:\\");

	EXPECT_THAT(m_tabContainer->GetTabByIndex(0), Ref(tab1));
	EXPECT_THAT(m_tabContainer->GetTabByIndex(1), Ref(tab2));
	EXPECT_THAT(m_tabContainer->GetTabByIndex(2), Ref(tab3));
}

TEST_F(TabContainerTest, GetTabIndex)
{
	const auto &tab1 = AddTab(L"c:\\");
	const auto &tab2 = AddTab(L"d:\\");
	const auto &tab3 = AddTab(L"e:\\");

	EXPECT_EQ(m_tabContainer->GetTabIndex(tab1), 0);
	EXPECT_EQ(m_tabContainer->GetTabIndex(tab2), 1);
	EXPECT_EQ(m_tabContainer->GetTabIndex(tab3), 2);
}

TEST_F(TabContainerTest, GetNumTabs)
{
	EXPECT_EQ(m_tabContainer->GetNumTabs(), 0);

	AddTab(L"c:\\");
	EXPECT_EQ(m_tabContainer->GetNumTabs(), 1);

	AddTab(L"d:\\");
	EXPECT_EQ(m_tabContainer->GetNumTabs(), 2);

	AddTab(L"e:\\");
	EXPECT_EQ(m_tabContainer->GetNumTabs(), 3);

	EXPECT_TRUE(m_tabContainer->CloseTab(m_tabContainer->GetTabByIndex(0)));
	EXPECT_EQ(m_tabContainer->GetNumTabs(), 2);

	EXPECT_TRUE(m_tabContainer->CloseTab(m_tabContainer->GetTabByIndex(0)));
	EXPECT_EQ(m_tabContainer->GetNumTabs(), 1);
}

TEST_F(TabContainerTest, MoveTab)
{
	const auto &tab1 = AddTab(L"c:\\");
	const auto &tab2 = AddTab(L"d:\\");
	const auto &tab3 = AddTab(L"e:\\");
	const auto &tab4 = AddTab(L"f:\\");

	m_tabContainer->SelectTabAtIndex(0);

	m_tabContainer->MoveTab(tab2, 2);
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(), ElementsAre(&tab1, &tab3, &tab2, &tab4));

	// The first tab is selected, so it should still be selected after being moved.
	m_tabContainer->MoveTab(tab1, 3);
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab1));
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(), ElementsAre(&tab3, &tab2, &tab4, &tab1));

	m_tabContainer->MoveTab(tab4, 0);
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(), ElementsAre(&tab4, &tab3, &tab2, &tab1));

	// The selected tab should shift position, but should remain selected.
	m_tabContainer->MoveTab(tab2, 3);
	EXPECT_THAT(m_tabContainer->GetSelectedTab(), Ref(tab1));
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(), ElementsAre(&tab4, &tab3, &tab1, &tab2));
}

TEST_F(TabContainerTest, CloseTab)
{
	int tabId1 = AddTabAndReturnId(L"c:\\");
	int tabId2 = AddTabAndReturnId(L"d:\\");
	int tabId3 = AddTabAndReturnId(L"e:\\");
	int tabId4 = AddTabAndReturnId(L"f:\\");

	m_tabContainer->SelectTabAtIndex(0);

	EXPECT_TRUE(m_tabContainer->CloseTab(m_tabContainer->GetTab(tabId3)));
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId1), Property(&Tab::GetId, tabId2),
			Property(&Tab::GetId, tabId4)));

	// The first tab is selected, so closing it should result in the selection shifting to the next
	// tab.
	EXPECT_TRUE(m_tabContainer->CloseTab(m_tabContainer->GetTab(tabId1)));
	EXPECT_EQ(m_tabContainer->GetSelectedTab().GetId(), tabId2);
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId2), Property(&Tab::GetId, tabId4)));

	EXPECT_TRUE(m_tabContainer->CloseTab(m_tabContainer->GetTab(tabId4)));
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(), ElementsAre(Property(&Tab::GetId, tabId2)));
}

TEST_F(TabContainerTest, CloseTabWhenLocked)
{
	int tabId1 = AddTabAndReturnId(L"c:\\");
	int tabId2 = AddTabAndReturnId(L"d:\\");

	// If a tab is locked, attempting to close it should have no effect.
	m_tabContainer->GetTab(tabId2).SetLockState(Tab::LockState::Locked);
	EXPECT_FALSE(m_tabContainer->CloseTab(m_tabContainer->GetTab(tabId2)));
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId1), Property(&Tab::GetId, tabId2)));

	m_tabContainer->GetTab(tabId2).SetLockState(Tab::LockState::AddressLocked);
	EXPECT_FALSE(m_tabContainer->CloseTab(m_tabContainer->GetTab(tabId2)));
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId1), Property(&Tab::GetId, tabId2)));
}

TEST_F(TabContainerTest, GetAllTabs)
{
	const auto &tab1 = AddTab(L"c:\\");
	const auto &tab2 = AddTab(L"d:\\");
	const auto &tab3 = AddTab(L"e:\\");

	EXPECT_THAT(m_tabContainer->GetAllTabs(),
		UnorderedElementsAre(Pair(tab1.GetId(), Pointer(&tab1)), Pair(tab2.GetId(), Pointer(&tab2)),
			Pair(tab3.GetId(), Pointer(&tab3))));
}

TEST_F(TabContainerTest, GetAllTabsInOrder)
{
	const auto &tab1 = AddTab(L"c:\\");
	const auto &tab2 = AddTab(L"d:\\");
	const auto &tab3 = AddTab(L"e:\\");

	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(), ElementsAre(&tab1, &tab2, &tab3));
}

TEST_F(TabContainerTest, DoubleClickOnTab)
{
	int tabId1 = AddTabAndReturnId(L"c:\\");
	int tabId2 = AddTabAndReturnId(L"d:\\");
	int tabId3 = AddTabAndReturnId(L"e:\\");

	m_config.doubleClickTabClose = false;

	// This should have no effect, since the doubleClickTabClose is disabled.
	auto *tabViewItem = m_tabView->GetTabAtIndex(1);
	tabViewItem->OnDoubleClicked({ { 0, 0 }, false, false });
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId1), Property(&Tab::GetId, tabId2),
			Property(&Tab::GetId, tabId3)));

	m_config.doubleClickTabClose = true;

	tabViewItem = m_tabView->GetTabAtIndex(0);
	tabViewItem->OnDoubleClicked({ { 0, 0 }, false, false });
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId2), Property(&Tab::GetId, tabId3)));
}

TEST_F(TabContainerTest, MiddleClickOnTab)
{
	int tabId1 = AddTabAndReturnId(L"c:\\");
	int tabId2 = AddTabAndReturnId(L"d:\\");
	AddTab(L"e:\\");

	// Middle-clicking a tab should result in the tab being closed.
	auto *tabViewItem = m_tabView->GetTabAtIndex(2);
	tabViewItem->OnMiddleClicked({ { 0, 0 }, false, false });
	EXPECT_THAT(m_tabContainer->GetAllTabsInOrder(),
		ElementsAre(Property(&Tab::GetId, tabId1), Property(&Tab::GetId, tabId2)));
}

TEST_F(TabContainerTest, CreatedSignal)
{
	MockFunction<void(const Tab &tab)> callback;
	m_tabEvents.AddCreatedObserver(callback.AsStdFunction(), TabEventScope::ForBrowser(*m_browser));

	const Tab *callbackTab = nullptr;
	EXPECT_CALL(callback, Call(_))
		.WillOnce([&callbackTab](const auto &createdTab) { callbackTab = &createdTab; });
	const auto &tab = AddTab(L"c:\\");
	EXPECT_EQ(&tab, callbackTab);
}

TEST_F(TabContainerTest, SelectedSignal)
{
	const auto &tab1 = AddTab(L"c:\\");
	const auto &tab2 = AddTab(L"d:\\");

	MockFunction<void(const Tab &tab)> callback;
	m_tabEvents.AddSelectedObserver(callback.AsStdFunction(),
		TabEventScope::ForBrowser(*m_browser));

	MockFunction<void(int)> check;
	{
		InSequence seq;

		EXPECT_CALL(callback, Call(Ref(tab2)));
		EXPECT_CALL(check, Call(1));
		EXPECT_CALL(callback, Call(Ref(tab1)));
		EXPECT_CALL(check, Call(2));
		EXPECT_CALL(callback, Call(Ref(tab2)));
		EXPECT_CALL(check, Call(3));
		EXPECT_CALL(callback, Call(Ref(tab1)));
	}

	m_tabContainer->SelectTab(tab2);
	check.Call(1);
	m_tabContainer->SelectTabAtIndex(0);
	check.Call(2);
	m_tabContainer->SelectAdjacentTab(TabContainer::SelectionDirection::Next);
	check.Call(3);
	m_tabContainer->SelectAdjacentTab(TabContainer::SelectionDirection::Previous);
}

TEST_F(TabContainerTest, SelectedSignalOnFirstTabCreation)
{
	MockFunction<void(const Tab &tab)> callback;
	m_tabEvents.AddSelectedObserver(callback.AsStdFunction(),
		TabEventScope::ForBrowser(*m_browser));

	// The first tab will be selected on creation. That should trigger a selection event.
	const Tab *callbackTab = nullptr;
	EXPECT_CALL(callback, Call(_))
		.WillOnce([&callbackTab](const auto &selectedTab) { callbackTab = &selectedTab; });
	const auto &tab = AddTab(L"c:\\");
	EXPECT_EQ(&tab, callbackTab);
}

TEST_F(TabContainerTest, SelectedSignalOnSelectedTabCreation)
{
	AddTab(L"c:\\");

	MockFunction<void(const Tab &tab)> callback;
	m_tabEvents.AddSelectedObserver(callback.AsStdFunction(),
		TabEventScope::ForBrowser(*m_browser));

	const Tab *callbackTab = nullptr;
	EXPECT_CALL(callback, Call(_))
		.WillOnce([&callbackTab](const auto &selectedTab) { callbackTab = &selectedTab; });
	const auto &tab2 = AddTab(L"c:\\", TabSettings(_selected = true));
	EXPECT_EQ(&tab2, callbackTab);
}

TEST_F(TabContainerTest, SelectedSignalOnClose)
{
	int tabId1 = AddTabAndReturnId(L"c:\\");
	int tabId2 = AddTabAndReturnId(L"d:\\");

	m_tabContainer->SelectTab(m_tabContainer->GetTab(tabId2));

	MockFunction<void(const Tab &tab)> callback;
	m_tabEvents.AddSelectedObserver(callback.AsStdFunction(),
		TabEventScope::ForBrowser(*m_browser));

	// The second tab is selected, so closing it should result in the first tab being selected and a
	// signal being emitted.
	EXPECT_CALL(callback, Call(Property(&Tab::GetId, tabId1)));
	m_tabContainer->CloseTab(m_tabContainer->GetTab(tabId2));
}

TEST_F(TabContainerTest, MovedSignal)
{
	const auto &tab1 = AddTab(L"c:\\");
	AddTab(L"d:\\");
	AddTab(L"e:\\");

	MockFunction<void(const Tab &tab, int fromIndex, int toIndex)> callback;
	m_tabEvents.AddMovedObserver(callback.AsStdFunction(), TabEventScope::ForBrowser(*m_browser));

	EXPECT_CALL(callback, Call(Ref(tab1), 0, 2));
	m_tabContainer->MoveTab(tab1, 2);
}

TEST_F(TabContainerTest, PreRemovalSignal)
{
	AddTab(L"c:\\");
	int tabId2 = AddTabAndReturnId(L"d:\\");
	AddTab(L"e:\\");

	MockFunction<void(const Tab &tab, int index)> callback;
	m_tabEvents.AddPreRemovalObserver(callback.AsStdFunction(),
		TabEventScope::ForBrowser(*m_browser));

	EXPECT_CALL(callback, Call(Property(&Tab::GetId, tabId2), 1));
	m_tabContainer->CloseTab(m_tabContainer->GetTab(tabId2));
}

TEST_F(TabContainerTest, RemovedSignal)
{
	AddTab(L"c:\\");
	int tabId2 = AddTabAndReturnId(L"d:\\");
	AddTab(L"e:\\");

	MockFunction<void(const Tab &tab)> callback;
	m_tabEvents.AddRemovedObserver(callback.AsStdFunction(), TabEventScope::ForBrowser(*m_browser));

	EXPECT_CALL(callback, Call(Property(&Tab::GetId, tabId2)));
	m_tabContainer->CloseTab(m_tabContainer->GetTab(tabId2));
}
