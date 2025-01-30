// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Explorer++/ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "ShellTestHelper.h"
#include "TabNavigationMock.h"
#include "../Explorer++/ShellBrowser/HistoryEntry.h"
#include "../Explorer++/ShellBrowser/PreservedHistoryEntry.h"
#include "../Helper/ShellHelper.h"
#include <gtest/gtest.h>
#include <ShlObj.h>

using namespace testing;

class ShellNavigationControllerTest : public Test
{
protected:
	ShellNavigationControllerTest() : m_shellBrowser(&m_tabNavigation)
	{
	}

	ShellNavigationController *GetNavigationController() const
	{
		return m_shellBrowser.GetNavigationController();
	}

	TabNavigationMock m_tabNavigation;
	ShellBrowserFake m_shellBrowser;
};

TEST_F(ShellNavigationControllerTest, Refresh)
{
	auto *navigationController = GetNavigationController();

	// Shouldn't be able to refresh when no navigation has occurred yet.
	navigationController->Refresh();
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 0);

	m_shellBrowser.NavigateToPath(L"C:\\Fake");

	navigationController->Refresh();

	// Refreshing shouldn't result in a history entry being added.
	EXPECT_FALSE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 1);
}

TEST_F(ShellNavigationControllerTest, NavigateToSameFolder)
{
	m_shellBrowser.NavigateToPath(L"C:\\Fake");
	m_shellBrowser.NavigateToPath(L"C:\\Fake");

	auto *navigationController = GetNavigationController();

	// Navigating to the same location should be treated as an implicit refresh. No history entry
	// should be added.
	EXPECT_FALSE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 1);
}

TEST_F(ShellNavigationControllerTest, BackForward)
{
	m_shellBrowser.NavigateToPath(L"C:\\Fake1");

	auto *navigationController = GetNavigationController();

	EXPECT_FALSE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 1);

	m_shellBrowser.NavigateToPath(L"C:\\Fake2");

	EXPECT_TRUE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);

	navigationController->GoBack();

	EXPECT_FALSE(navigationController->CanGoBack());
	EXPECT_TRUE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 0);

	navigationController->GoForward();

	EXPECT_TRUE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 1);

	m_shellBrowser.NavigateToPath(L"C:\\Fake3");

	EXPECT_TRUE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 3);

	// Go back to the first entry.
	navigationController->GoToOffset(-2);

	EXPECT_FALSE(navigationController->CanGoBack());
	EXPECT_TRUE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 3);

	m_shellBrowser.NavigateToPath(L"C:\\Fake4");

	// Performing a new navigation should have cleared the forward history.
	EXPECT_TRUE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);
}

TEST_F(ShellNavigationControllerTest, RetrieveHistory)
{
	m_shellBrowser.NavigateToPath(L"C:\\Fake1");

	auto *navigationController = GetNavigationController();

	auto history = navigationController->GetBackHistory();
	EXPECT_TRUE(history.empty());

	history = navigationController->GetForwardHistory();
	EXPECT_TRUE(history.empty());

	m_shellBrowser.NavigateToPath(L"C:\\Fake2");

	history = navigationController->GetBackHistory();
	EXPECT_EQ(history.size(), 1U);

	history = navigationController->GetForwardHistory();
	EXPECT_TRUE(history.empty());

	m_shellBrowser.NavigateToPath(L"C:\\Fake3");

	history = navigationController->GetBackHistory();
	EXPECT_EQ(history.size(), 2U);

	history = navigationController->GetForwardHistory();
	EXPECT_TRUE(history.empty());

	navigationController->GoBack();

	history = navigationController->GetBackHistory();
	EXPECT_EQ(history.size(), 1U);

	history = navigationController->GetForwardHistory();
	EXPECT_EQ(history.size(), 1U);
}

TEST_F(ShellNavigationControllerTest, GoUp)
{
	auto *navigationController = GetNavigationController();

	PidlAbsolute pidlFolder = CreateSimplePidlForTest(L"C:\\Fake");
	auto navigateParamsFolder = NavigateParams::Normal(pidlFolder.Raw());
	navigationController->Navigate(navigateParamsFolder);

	EXPECT_TRUE(navigationController->CanGoUp());

	navigationController->GoUp();

	auto entry = navigationController->GetCurrentEntry();
	ASSERT_NE(entry, nullptr);

	PidlAbsolute pidlParent = CreateSimplePidlForTest(L"C:\\");
	EXPECT_EQ(entry->GetPidl(), pidlParent);

	// The desktop folder is the root of the shell namespace.
	PidlAbsolute pidlDesktop;
	HRESULT hr = SHGetKnownFolderIDList(FOLDERID_Desktop, KF_FLAG_DEFAULT, nullptr,
		PidlOutParam(pidlDesktop));
	ASSERT_HRESULT_SUCCEEDED(hr);

	auto navigateParamsDesktop = NavigateParams::Normal(pidlDesktop.Raw());
	navigationController->Navigate(navigateParamsDesktop);

	EXPECT_FALSE(navigationController->CanGoUp());

	// This should have no effect.
	navigationController->GoUp();

	entry = navigationController->GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->GetPidl(), pidlDesktop);
}

TEST_F(ShellNavigationControllerTest, HistoryEntries)
{
	auto *navigationController = GetNavigationController();

	auto entry = navigationController->GetCurrentEntry();
	EXPECT_EQ(entry, nullptr);

	entry = navigationController->GetEntryAtIndex(0);
	EXPECT_EQ(entry, nullptr);

	PidlAbsolute pidl1 = CreateSimplePidlForTest(L"C:\\Fake1");
	auto navigateParams1 = NavigateParams::Normal(pidl1.Raw());
	navigationController->Navigate(navigateParams1);

	EXPECT_EQ(navigationController->GetCurrentIndex(), 0);

	entry = navigationController->GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->GetPidl(), pidl1);

	EXPECT_EQ(navigationController->GetIndexOfEntry(entry), 0);
	EXPECT_EQ(navigationController->GetEntryById(entry->GetId()), entry);

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"C:\\Fake2");
	auto navigateParams2 = NavigateParams::Normal(pidl2.Raw());
	navigationController->Navigate(navigateParams2);

	entry = navigationController->GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->GetPidl(), pidl2);

	EXPECT_EQ(navigationController->GetIndexOfEntry(entry), 1);
	EXPECT_EQ(navigationController->GetEntryById(entry->GetId()), entry);

	EXPECT_EQ(navigationController->GetCurrentIndex(), 1);
	EXPECT_EQ(navigationController->GetCurrentEntry(), navigationController->GetEntryAtIndex(1));

	entry = navigationController->GetEntryAtIndex(0);
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->GetPidl(), pidl1);
}

TEST_F(ShellNavigationControllerTest, SetNavigationMode)
{
	PidlAbsolute pidl1 = CreateSimplePidlForTest(L"C:\\Fake1");
	auto params = NavigateParams::Normal(pidl1.Raw());

	MockFunction<void(const NavigateParams &navigateParams)> navigationStartedCallback;
	m_shellBrowser.AddNavigationStartedObserver(navigationStartedCallback.AsStdFunction());

	EXPECT_CALL(navigationStartedCallback, Call(Eq(params)));

	// By default, all navigations should proceed in the current tab.
	EXPECT_CALL(m_tabNavigation, CreateNewTab).Times(0);

	auto *navigationController = GetNavigationController();
	EXPECT_EQ(navigationController->GetNavigationMode(), NavigationMode::Normal);

	navigationController->Navigate(params);

	navigationController->SetNavigationMode(NavigationMode::ForceNewTab);
	EXPECT_EQ(navigationController->GetNavigationMode(), NavigationMode::ForceNewTab);

	// The navigation is to the same directory, which is treated as an implicit refresh, so the
	// following fields are expected to be set.
	auto expectedParams = params;
	expectedParams.historyEntryType = HistoryEntryType::ReplaceCurrentEntry;
	expectedParams.overrideNavigationMode = true;

	// Although the navigation mode has been set, the navigation is an implicit refresh and should
	// always proceed in the same tab.
	EXPECT_CALL(navigationStartedCallback, Call(Eq(expectedParams)));
	EXPECT_CALL(m_tabNavigation, CreateNewTab).Times(0);

	navigationController->Navigate(params);

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"C:\\Fake2");
	params = NavigateParams::Normal(pidl2.Raw());

	// This is a navigation to a different directory, so the navigation mode above should now apply.
	EXPECT_CALL(navigationStartedCallback, Call(_)).Times(0);
	EXPECT_CALL(m_tabNavigation, CreateNewTab(Ref(params), _));

	navigationController->Navigate(params);

	PidlAbsolute pidl3 = CreateSimplePidlForTest(L"C:\\Fake3");
	params = NavigateParams::Normal(pidl3.Raw());
	params.overrideNavigationMode = true;

	// The navigation explicitly overrides the navigation mode, so this navigation should proceed in
	// the tab, even though a navigation mode was applied above.
	EXPECT_CALL(navigationStartedCallback, Call(Eq(params)));
	EXPECT_CALL(m_tabNavigation, CreateNewTab).Times(0);

	navigationController->Navigate(params);
}

TEST_F(ShellNavigationControllerTest, SetNavigationModeFirstNavigation)
{
	auto *navigationController = GetNavigationController();
	navigationController->SetNavigationMode(NavigationMode::ForceNewTab);

	PidlAbsolute pidl1 = CreateSimplePidlForTest(L"C:\\Fake1");
	auto params = NavigateParams::Normal(pidl1.Raw());

	MockFunction<void(const NavigateParams &navigateParams)> navigationStartedCallback;
	m_shellBrowser.AddNavigationStartedObserver(navigationStartedCallback.AsStdFunction());

	// The first navigation in a tab should always take place within that tab, regardless of the
	// navigation mode in effect.
	EXPECT_CALL(navigationStartedCallback, Call(Eq(params)));
	EXPECT_CALL(m_tabNavigation, CreateNewTab).Times(0);

	navigationController->Navigate(params);

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"C:\\Fake2");
	auto params2 = NavigateParams::Normal(pidl2.Raw());

	// Subsequent navigations should then open in a new tab when necessary.
	EXPECT_CALL(navigationStartedCallback, Call(Eq(params2))).Times(0);
	EXPECT_CALL(m_tabNavigation, CreateNewTab(Ref(params2), _));

	navigationController->Navigate(params2);
}

TEST_F(ShellNavigationControllerTest, HistoryEntryTypes)
{
	m_shellBrowser.NavigateToPath(L"C:\\Fake1", HistoryEntryType::AddEntry);

	PidlAbsolute pidl2;
	m_shellBrowser.NavigateToPath(L"C:\\Fake2", HistoryEntryType::ReplaceCurrentEntry, &pidl2);

	auto *navigationController = GetNavigationController();

	// The second navigation should have replaced the entry from the first navigation, so there
	// should only be a single entry.
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 1);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 0);

	auto entry = navigationController->GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->GetPidl(), pidl2);

	PidlAbsolute pidl3;
	m_shellBrowser.NavigateToPath(L"C:\\Fake3", HistoryEntryType::AddEntry, &pidl3);

	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 1);

	entry = navigationController->GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->GetPidl(), pidl3);

	m_shellBrowser.NavigateToPath(L"C:\\Fake4", HistoryEntryType::None);

	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 1);

	// No entry should have been added, so the current entry should still be the same as it was
	// previously.
	entry = navigationController->GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->GetPidl(), pidl3);
}

TEST_F(ShellNavigationControllerTest, ReplacePreviousHistoryEntry)
{
	m_shellBrowser.NavigateToPath(L"C:\\Fake1", HistoryEntryType::AddEntry);
	m_shellBrowser.NavigateToPath(L"C:\\Fake2", HistoryEntryType::AddEntry);

	auto *navigationController = GetNavigationController();

	auto *entry = navigationController->GetEntryAtIndex(0);
	ASSERT_NE(entry, nullptr);
	int originalEntryId = entry->GetId();

	auto params = NavigateParams::History(entry);
	params.historyEntryType = HistoryEntryType::ReplaceCurrentEntry;
	navigationController->Navigate(params);

	auto *updatedEntry = navigationController->GetEntryAtIndex(0);
	ASSERT_NE(updatedEntry, nullptr);

	// Navigating to the entry should have resulted in it being replaced, so the ID of the first
	// entry should have changed.
	EXPECT_NE(updatedEntry->GetId(), originalEntryId);
}

TEST_F(ShellNavigationControllerTest, HistoryEntryTypeFirstNavigation)
{
	PidlAbsolute pidl;
	m_shellBrowser.NavigateToPath(L"C:\\Fake", HistoryEntryType::None, &pidl);

	auto *navigationController = GetNavigationController();

	// The first navigation in a tab should always result in a history entry being added, regardless
	// of what's requested.
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 1);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 0);

	auto entry = navigationController->GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->GetPidl(), pidl);
}

TEST_F(ShellNavigationControllerTest, InitialNavigation)
{
	m_shellBrowser.SetNavigationMode(ShellBrowserFake::NavigationMode::Async);

	PidlAbsolute pidl;
	m_shellBrowser.NavigateToPath(L"C:\\Fake", HistoryEntryType::None, &pidl);

	auto *navigationController = GetNavigationController();

	// An initial entry should be added, regardless of the history entry type requested.
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 1);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 0);

	auto entry = navigationController->GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->GetInitialNavigationType(), HistoryEntry::InitialNavigationType::Initial);
	EXPECT_TRUE(entry->IsInitialEntry());
	EXPECT_EQ(entry->GetPidl(), pidl);
	int originalEntryId = entry->GetId();

	auto *request = m_shellBrowser.GetLastAsyncNavigationRequest();
	ASSERT_NE(request, nullptr);

	// This should result in the initial entry being replaced. There should still only be a single
	// entry.
	request->Complete();
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 1);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 0);

	auto updatedEntry = navigationController->GetCurrentEntry();
	ASSERT_NE(updatedEntry, nullptr);
	EXPECT_NE(updatedEntry->GetId(), originalEntryId);
	EXPECT_EQ(updatedEntry->GetInitialNavigationType(),
		HistoryEntry::InitialNavigationType::NonInitial);
	EXPECT_FALSE(updatedEntry->IsInitialEntry());
	EXPECT_EQ(updatedEntry->GetPidl(), pidl);
}

class ShellNavigationControllerPreservedTest : public Test
{
protected:
	void SetUp(int currentEntry)
	{
		auto preservedEntry =
			std::make_unique<PreservedHistoryEntry>(CreateSimplePidlForTest(L"C:\\Fake1"));
		m_preservedEntries.push_back(std::move(preservedEntry));

		preservedEntry =
			std::make_unique<PreservedHistoryEntry>(CreateSimplePidlForTest(L"C:\\Fake2"));
		m_preservedEntries.push_back(std::move(preservedEntry));

		m_shellBrowser =
			std::make_unique<ShellBrowserFake>(&m_tabNavigation, m_preservedEntries, currentEntry);
	}

	ShellNavigationController *GetNavigationController() const
	{
		return m_shellBrowser->GetNavigationController();
	}

	TabNavigationMock m_tabNavigation;
	std::unique_ptr<ShellBrowserFake> m_shellBrowser;

	std::vector<std::unique_ptr<PreservedHistoryEntry>> m_preservedEntries;
};

TEST_F(ShellNavigationControllerPreservedTest, FirstIndexIsCurrent)
{
	SetUp(0);

	auto *navigationController = GetNavigationController();

	EXPECT_EQ(navigationController->GetCurrentIndex(), 0);
	EXPECT_FALSE(navigationController->CanGoBack());
	EXPECT_TRUE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);
}

TEST_F(ShellNavigationControllerPreservedTest, SecondIndexIsCurrent)
{
	SetUp(1);

	auto *navigationController = GetNavigationController();

	EXPECT_EQ(navigationController->GetCurrentIndex(), 1);
	EXPECT_TRUE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);
}

TEST_F(ShellNavigationControllerPreservedTest, CheckEntries)
{
	SetUp(0);

	auto *navigationController = GetNavigationController();

	for (size_t i = 0; i < m_preservedEntries.size(); i++)
	{
		auto entry = navigationController->GetEntryAtIndex(static_cast<int>(i));
		ASSERT_NE(entry, nullptr);
		EXPECT_EQ(entry->GetPidl(), m_preservedEntries[i]->GetPidl());
	}
}
