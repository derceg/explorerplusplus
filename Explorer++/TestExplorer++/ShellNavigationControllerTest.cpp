// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Explorer++/ShellBrowser/ShellNavigationController.h"
#include "IconFetcherMock.h"
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
	ShellNavigationControllerTest() : m_shellBrowser(&m_tabNavigation, &m_iconFetcher)
	{
	}

	ShellNavigationController *GetNavigationController() const
	{
		return m_shellBrowser.GetNavigationController();
	}

	TabNavigationMock m_tabNavigation;
	IconFetcherMock m_iconFetcher;
	ShellBrowserFake m_shellBrowser;
};

TEST_F(ShellNavigationControllerTest, Refresh)
{
	auto *navigationController = GetNavigationController();

	// Shouldn't be able to refresh when no navigation has occurred yet.
	HRESULT hr = navigationController->Refresh();
	ASSERT_HRESULT_FAILED(hr);

	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake"));

	hr = navigationController->Refresh();
	ASSERT_HRESULT_SUCCEEDED(hr);

	// Refreshing shouldn't result in a history entry being added.
	EXPECT_FALSE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 1);
}

TEST_F(ShellNavigationControllerTest, NavigateToSameFolder)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake"));
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake"));

	auto *navigationController = GetNavigationController();

	// Navigating to the same location should be treated as an implicit refresh. No history entry
	// should be added.
	EXPECT_FALSE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 1);
}

TEST_F(ShellNavigationControllerTest, BackForward)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));

	auto *navigationController = GetNavigationController();

	EXPECT_FALSE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 1);

	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));

	EXPECT_TRUE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);

	ASSERT_HRESULT_SUCCEEDED(navigationController->GoBack());

	EXPECT_FALSE(navigationController->CanGoBack());
	EXPECT_TRUE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 0);

	ASSERT_HRESULT_SUCCEEDED(navigationController->GoForward());

	EXPECT_TRUE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 1);

	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	EXPECT_TRUE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 3);

	// Go back to the first entry.
	ASSERT_HRESULT_SUCCEEDED(navigationController->GoToOffset(-2));

	EXPECT_FALSE(navigationController->CanGoBack());
	EXPECT_TRUE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 3);

	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake4"));

	// Performing a new navigation should have cleared the forward history.
	EXPECT_TRUE(navigationController->CanGoBack());
	EXPECT_FALSE(navigationController->CanGoForward());
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);
}

TEST_F(ShellNavigationControllerTest, RetrieveHistory)
{
	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake1"));

	auto *navigationController = GetNavigationController();

	auto history = navigationController->GetBackHistory();
	EXPECT_TRUE(history.empty());

	history = navigationController->GetForwardHistory();
	EXPECT_TRUE(history.empty());

	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake2"));

	history = navigationController->GetBackHistory();
	EXPECT_EQ(history.size(), 1U);

	history = navigationController->GetForwardHistory();
	EXPECT_TRUE(history.empty());

	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake3"));

	history = navigationController->GetBackHistory();
	EXPECT_EQ(history.size(), 2U);

	history = navigationController->GetForwardHistory();
	EXPECT_TRUE(history.empty());

	ASSERT_HRESULT_SUCCEEDED(navigationController->GoBack());

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
	HRESULT hr = navigationController->Navigate(navigateParamsFolder);
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_TRUE(navigationController->CanGoUp());

	hr = navigationController->GoUp();
	EXPECT_HRESULT_SUCCEEDED(hr);

	auto entry = navigationController->GetCurrentEntry();
	ASSERT_NE(entry, nullptr);

	PidlAbsolute pidlParent = CreateSimplePidlForTest(L"C:\\");
	EXPECT_EQ(entry->GetPidl(), pidlParent);

	// The desktop folder is the root of the shell namespace.
	PidlAbsolute pidlDesktop;
	hr = SHGetKnownFolderIDList(FOLDERID_Desktop, KF_FLAG_DEFAULT, nullptr,
		PidlOutParam(pidlDesktop));
	ASSERT_HRESULT_SUCCEEDED(hr);

	auto navigateParamsDesktop = NavigateParams::Normal(pidlDesktop.Raw());
	hr = navigationController->Navigate(navigateParamsDesktop);
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_FALSE(navigationController->CanGoUp());

	hr = navigationController->GoUp();
	EXPECT_HRESULT_FAILED(hr);

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
	HRESULT hr = navigationController->Navigate(navigateParams1);
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_EQ(navigationController->GetCurrentIndex(), 0);

	entry = navigationController->GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->GetPidl(), pidl1);

	EXPECT_EQ(navigationController->GetIndexOfEntry(entry), 0);
	EXPECT_EQ(navigationController->GetEntryById(entry->GetId()), entry);

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"C:\\Fake2");
	auto navigateParams2 = NavigateParams::Normal(pidl2.Raw());
	hr = navigationController->Navigate(navigateParams2);
	ASSERT_HRESULT_SUCCEEDED(hr);

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

	EXPECT_CALL(navigationStartedCallback, Call(Ref(params)));

	// By default, all navigations should proceed in the current tab.
	EXPECT_CALL(m_tabNavigation, CreateNewTab).Times(0);

	auto *navigationController = GetNavigationController();
	EXPECT_EQ(navigationController->GetNavigationMode(), NavigationMode::Normal);

	HRESULT hr = navigationController->Navigate(params);
	ASSERT_HRESULT_SUCCEEDED(hr);

	navigationController->SetNavigationMode(NavigationMode::ForceNewTab);
	EXPECT_EQ(navigationController->GetNavigationMode(), NavigationMode::ForceNewTab);

	// Although the navigation mode has been set, the navigation is to the same directory, which is
	// treated as an implicit refresh and should always proceed in the same tab.
	EXPECT_CALL(navigationStartedCallback, Call(Ref(params)));
	EXPECT_CALL(m_tabNavigation, CreateNewTab).Times(0);

	hr = navigationController->Navigate(params);
	ASSERT_HRESULT_SUCCEEDED(hr);

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"C:\\Fake2");
	params = NavigateParams::Normal(pidl2.Raw());

	// This is a navigation to a different directory, so the navigation mode above should now apply.
	EXPECT_CALL(navigationStartedCallback, Call(_)).Times(0);
	EXPECT_CALL(m_tabNavigation, CreateNewTab(Ref(params), _));

	hr = navigationController->Navigate(params);
	ASSERT_HRESULT_SUCCEEDED(hr);

	PidlAbsolute pidl3 = CreateSimplePidlForTest(L"C:\\Fake3");
	params = NavigateParams::Normal(pidl3.Raw());
	params.overrideNavigationMode = true;

	// The navigation explicitly overrides the navigation mode, so this navigation should proceed in
	// the tab, even though a navigation mode was applied above.
	EXPECT_CALL(navigationStartedCallback, Call(Ref(params)));
	EXPECT_CALL(m_tabNavigation, CreateNewTab).Times(0);

	hr = navigationController->Navigate(params);
	ASSERT_HRESULT_SUCCEEDED(hr);
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
	EXPECT_CALL(navigationStartedCallback, Call(Ref(params)));
	EXPECT_CALL(m_tabNavigation, CreateNewTab).Times(0);

	HRESULT hr = navigationController->Navigate(params);
	ASSERT_HRESULT_SUCCEEDED(hr);

	PidlAbsolute pidl2 = CreateSimplePidlForTest(L"C:\\Fake2");
	auto params2 = NavigateParams::Normal(pidl2.Raw());

	// Subsequent navigations should then open in a new tab when necessary.
	EXPECT_CALL(navigationStartedCallback, Call(Ref(params2))).Times(0);
	EXPECT_CALL(m_tabNavigation, CreateNewTab(Ref(params2), _));

	hr = navigationController->Navigate(params2);
	ASSERT_HRESULT_SUCCEEDED(hr);
}

TEST_F(ShellNavigationControllerTest, HistoryEntryTypes)
{
	ASSERT_HRESULT_SUCCEEDED(
		m_shellBrowser.NavigateToPath(L"C:\\Fake1", HistoryEntryType::AddEntry));

	PidlAbsolute pidl2;
	ASSERT_HRESULT_SUCCEEDED(
		m_shellBrowser.NavigateToPath(L"C:\\Fake2", HistoryEntryType::ReplaceCurrentEntry, &pidl2));

	auto *navigationController = GetNavigationController();

	// The second navigation should have replaced the entry from the first navigation, so there
	// should only be a single entry.
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 1);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 0);

	auto entry = navigationController->GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->GetPidl(), pidl2);

	PidlAbsolute pidl3;
	ASSERT_HRESULT_SUCCEEDED(
		m_shellBrowser.NavigateToPath(L"C:\\Fake3", HistoryEntryType::AddEntry, &pidl3));

	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 2);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 1);

	entry = navigationController->GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->GetPidl(), pidl3);

	ASSERT_HRESULT_SUCCEEDED(m_shellBrowser.NavigateToPath(L"C:\\Fake4", HistoryEntryType::None));

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
	ASSERT_HRESULT_SUCCEEDED(
		m_shellBrowser.NavigateToPath(L"C:\\Fake1", HistoryEntryType::AddEntry));
	ASSERT_HRESULT_SUCCEEDED(
		m_shellBrowser.NavigateToPath(L"C:\\Fake2", HistoryEntryType::AddEntry));

	auto *navigationController = GetNavigationController();

	auto *entry = navigationController->GetEntryAtIndex(0);
	ASSERT_NE(entry, nullptr);
	int originalEntryId = entry->GetId();

	auto params = NavigateParams::History(entry);
	params.historyEntryType = HistoryEntryType::ReplaceCurrentEntry;
	ASSERT_HRESULT_SUCCEEDED(navigationController->Navigate(params));

	auto *updatedEntry = navigationController->GetEntryAtIndex(0);
	ASSERT_NE(updatedEntry, nullptr);

	// Navigating to the entry should have resulted in it being replaced, so the ID of the first
	// entry should have changed.
	EXPECT_NE(updatedEntry->GetId(), originalEntryId);
}

TEST_F(ShellNavigationControllerTest, HistoryEntryTypeFirstNavigation)
{
	PidlAbsolute pidl;
	ASSERT_HRESULT_SUCCEEDED(
		m_shellBrowser.NavigateToPath(L"C:\\Fake", HistoryEntryType::None, &pidl));

	auto *navigationController = GetNavigationController();

	// The first navigation in a tab should always result in a history entry being added, regardless
	// of what's requested.
	EXPECT_EQ(navigationController->GetNumHistoryEntries(), 1);
	EXPECT_EQ(navigationController->GetCurrentIndex(), 0);

	auto entry = navigationController->GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->GetPidl(), pidl);
}

class ShellNavigationControllerPreservedTest : public Test
{
protected:
	void SetUp(int currentEntry)
	{
		auto preservedEntry = CreatePreservedHistoryEntry(L"C:\\Fake1");
		m_preservedEntries.push_back(std::move(preservedEntry));

		preservedEntry = CreatePreservedHistoryEntry(L"C:\\Fake2");
		m_preservedEntries.push_back(std::move(preservedEntry));

		m_shellBrowser = std::make_unique<ShellBrowserFake>(&m_tabNavigation, &m_iconFetcher,
			m_preservedEntries, currentEntry);
	}

	ShellNavigationController *GetNavigationController() const
	{
		return m_shellBrowser->GetNavigationController();
	}

	TabNavigationMock m_tabNavigation;
	IconFetcherMock m_iconFetcher;
	std::unique_ptr<ShellBrowserFake> m_shellBrowser;

	std::vector<std::unique_ptr<PreservedHistoryEntry>> m_preservedEntries;

private:
	std::unique_ptr<PreservedHistoryEntry> CreatePreservedHistoryEntry(const std::wstring &path)
	{
		PidlAbsolute pidl = CreateSimplePidlForTest(path.c_str());

		std::wstring displayName;
		HRESULT hr = GetDisplayName(pidl.Raw(), SHGDN_INFOLDER, displayName);

		if (FAILED(hr))
		{
			return nullptr;
		}

		auto fullPathForDisplay = GetFolderPathForDisplay(pidl.Raw());

		if (!fullPathForDisplay)
		{
			return nullptr;
		}

		HistoryEntry entry(pidl.Raw(), displayName, *fullPathForDisplay);
		return std::make_unique<PreservedHistoryEntry>(entry);
	}
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
		EXPECT_EQ(entry->GetPidl(), m_preservedEntries[i]->pidl);
		EXPECT_EQ(entry->GetDisplayName(), m_preservedEntries[i]->displayName);
		EXPECT_EQ(entry->GetFullPathForDisplay(), m_preservedEntries[i]->fullPathForDisplay);
	}
}
