// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Explorer++/ShellBrowser/ShellNavigationController.h"
#include "IconFetcher.h"
#include "../Explorer++/ShellBrowser/HistoryEntry.h"
#include "../Explorer++/ShellBrowser/PreservedHistoryEntry.h"
#include "../Explorer++/ShellBrowser/ShellNavigator.h"
#include "../Explorer++/TabNavigationInterface.h"
#include "../Helper/ShellHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ShlObj.h>

using namespace testing;

class NavigatorFake : public ShellNavigator
{
public:
	HRESULT Navigate(NavigateParams &navigateParams) override
	{
		m_navigationStartedSignal(navigateParams);
		m_navigationCommittedSignal(navigateParams);
		m_navigationCompletedSignal(navigateParams);
		return S_OK;
	}

	boost::signals2::connection AddNavigationStartedObserver(
		const NavigationStartedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override
	{
		return m_navigationStartedSignal.connect(observer, position);
	}

	boost::signals2::connection AddNavigationCommittedObserver(
		const NavigationCommittedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override
	{
		return m_navigationCommittedSignal.connect(observer, position);
	}

	boost::signals2::connection AddNavigationCompletedObserver(
		const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override
	{
		return m_navigationCompletedSignal.connect(observer, position);
	}

	boost::signals2::connection AddNavigationFailedObserver(
		const NavigationFailedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override
	{
		return m_navigationFailedSignal.connect(observer, position);
	}

private:
	NavigationStartedSignal m_navigationStartedSignal;
	NavigationCommittedSignal m_navigationCommittedSignal;
	NavigationCompletedSignal m_navigationCompletedSignal;
	NavigationFailedSignal m_navigationFailedSignal;
};

class NavigatorMock : public ShellNavigator
{
public:
	NavigatorMock()
	{
		ON_CALL(*this, NavigateImpl)
			.WillByDefault(
				[this](NavigateParams &navigateParams) { return m_fake.Navigate(navigateParams); });

		ON_CALL(*this, AddNavigationStartedObserverImpl)
			.WillByDefault([this](const NavigationStartedSignal::slot_type &observer,
							   boost::signals2::connect_position position)
				{ return m_fake.AddNavigationStartedObserver(observer, position); });

		ON_CALL(*this, AddNavigationCommittedObserverImpl)
			.WillByDefault([this](const NavigationCommittedSignal::slot_type &observer,
							   boost::signals2::connect_position position)
				{ return m_fake.AddNavigationCommittedObserver(observer, position); });

		ON_CALL(*this, AddNavigationCompletedObserverImpl)
			.WillByDefault([this](const NavigationCompletedSignal::slot_type &observer,
							   boost::signals2::connect_position position)
				{ return m_fake.AddNavigationCompletedObserver(observer, position); });

		ON_CALL(*this, AddNavigationFailedObserverImpl)
			.WillByDefault([this](const NavigationFailedSignal::slot_type &observer,
							   boost::signals2::connect_position position)
				{ return m_fake.AddNavigationFailedObserver(observer, position); });
	}

	MOCK_METHOD(HRESULT, NavigateImpl, (NavigateParams & navigateParams));
	MOCK_METHOD(boost::signals2::connection, AddNavigationStartedObserverImpl,
		(const NavigationStartedSignal::slot_type &observer,
			boost::signals2::connect_position position));
	MOCK_METHOD(boost::signals2::connection, AddNavigationCommittedObserverImpl,
		(const NavigationCommittedSignal::slot_type &observer,
			boost::signals2::connect_position position));
	MOCK_METHOD(boost::signals2::connection, AddNavigationCompletedObserverImpl,
		(const NavigationCompletedSignal::slot_type &observer,
			boost::signals2::connect_position position));
	MOCK_METHOD(boost::signals2::connection, AddNavigationFailedObserverImpl,
		(const NavigationFailedSignal::slot_type &observer,
			boost::signals2::connect_position position));

	HRESULT Navigate(NavigateParams &navigateParams) override
	{
		return NavigateImpl(navigateParams);
	}

	boost::signals2::connection AddNavigationStartedObserver(
		const NavigationStartedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override
	{
		return AddNavigationStartedObserver(observer, position);
	}

	boost::signals2::connection AddNavigationCommittedObserver(
		const NavigationCommittedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override
	{
		return AddNavigationCommittedObserverImpl(observer, position);
	}

	boost::signals2::connection AddNavigationCompletedObserver(
		const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override
	{
		return AddNavigationCompletedObserverImpl(observer, position);
	}

	boost::signals2::connection AddNavigationFailedObserver(
		const NavigationFailedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override
	{
		return AddNavigationFailedObserverImpl(observer, position);
	}

private:
	NavigatorFake m_fake;
};

class TabNavigationMock : public TabNavigationInterface
{
public:
	MOCK_METHOD(void, CreateNewTab, (NavigateParams & navigateParams, bool selected), (override));
	MOCK_METHOD(void, SelectTabById, (int tabId), (override));
};

class IconFetcherMock : public IconFetcher
{
public:
	MOCK_METHOD(void, QueueIconTask, (std::wstring_view path, Callback callback), (override));
	MOCK_METHOD(void, QueueIconTask, (PCIDLIST_ABSOLUTE pidl, Callback callback), (override));
	MOCK_METHOD(void, ClearQueue, (), (override));
	MOCK_METHOD(int, GetCachedIconIndexOrDefault,
		(const std::wstring &itemPath, DefaultIconType defaultIconType), (const, override));
	MOCK_METHOD(std::optional<int>, GetCachedIconIndex, (const std::wstring &itemPath),
		(const, override));
};

class ShellNavigationControllerTest : public Test
{
protected:
	ShellNavigationControllerTest() :
		m_navigationController(&m_navigator, &m_tabNavigation, &m_iconFetcher)
	{
	}

	// Although the NavigationController can navigate to a path (by transforming
	// it into a pidl), it requires that the path exist. This function will
	// transform the path into a simple pidl, which doesn't require the path to
	// exist.
	HRESULT NavigateToFolder(const std::wstring &path,
		HistoryEntryType addHistoryType = HistoryEntryType::AddEntry,
		unique_pidl_absolute *outputPidl = nullptr)
	{
		unique_pidl_absolute pidl(SHSimpleIDListFromPath(path.c_str()));

		if (!pidl)
		{
			return E_FAIL;
		}

		auto navigateParams = NavigateParams::Normal(pidl.get(), addHistoryType);
		HRESULT hr = m_navigationController.Navigate(navigateParams);

		if (outputPidl)
		{
			*outputPidl = std::move(pidl);
		}

		return hr;
	}

	NavigatorMock m_navigator;
	TabNavigationMock m_tabNavigation;
	IconFetcherMock m_iconFetcher;
	ShellNavigationController m_navigationController;
};

class ShellNavigationControllerPreservedTest : public Test
{
protected:
	void SetUp(int currentEntry)
	{
		auto preservedEntry = CreatePreservedHistoryEntry(L"C:\\Fake1");
		m_preservedEntries.push_back(std::move(preservedEntry));

		preservedEntry = CreatePreservedHistoryEntry(L"C:\\Fake2");
		m_preservedEntries.push_back(std::move(preservedEntry));

		m_navigationController = std::make_unique<ShellNavigationController>(&m_navigator,
			&m_tabNavigation, &m_iconFetcher, m_preservedEntries, currentEntry);
	}

	std::unique_ptr<PreservedHistoryEntry> CreatePreservedHistoryEntry(const std::wstring &path)
	{
		unique_pidl_absolute pidl(SHSimpleIDListFromPath(path.c_str()));

		if (!pidl)
		{
			return nullptr;
		}

		std::wstring displayName;
		HRESULT hr = GetDisplayName(pidl.get(), SHGDN_INFOLDER, displayName);

		if (FAILED(hr))
		{
			return nullptr;
		}

		auto fullPathForDisplay = GetFolderPathForDisplay(pidl.get());

		if (!fullPathForDisplay)
		{
			return nullptr;
		}

		HistoryEntry entry(pidl.get(), displayName, *fullPathForDisplay);
		return std::make_unique<PreservedHistoryEntry>(entry);
	}

	NavigatorMock m_navigator;
	TabNavigationMock m_tabNavigation;
	IconFetcherMock m_iconFetcher;
	std::unique_ptr<ShellNavigationController> m_navigationController;

	std::vector<std::unique_ptr<PreservedHistoryEntry>> m_preservedEntries;
};

TEST_F(ShellNavigationControllerTest, Refresh)
{
	// Shouldn't be able to refresh when no navigation has occurred yet.
	HRESULT hr = m_navigationController.Refresh();
	ASSERT_HRESULT_FAILED(hr);

	hr = NavigateToFolder(L"C:\\Fake");
	ASSERT_HRESULT_SUCCEEDED(hr);

	hr = m_navigationController.Refresh();
	ASSERT_HRESULT_SUCCEEDED(hr);

	// Refreshing shouldn't result in a history entry being added.
	EXPECT_FALSE(m_navigationController.CanGoBack());
	EXPECT_FALSE(m_navigationController.CanGoForward());
	EXPECT_EQ(m_navigationController.GetNumHistoryEntries(), 1);
}

TEST_F(ShellNavigationControllerTest, NavigateToSameFolder)
{
	HRESULT hr = NavigateToFolder(L"C:\\Fake");
	ASSERT_HRESULT_SUCCEEDED(hr);

	hr = NavigateToFolder(L"C:\\Fake");
	ASSERT_HRESULT_SUCCEEDED(hr);

	// Navigating to the same location should be treated as an implicit refresh. No history entry
	// should be added.
	EXPECT_FALSE(m_navigationController.CanGoBack());
	EXPECT_FALSE(m_navigationController.CanGoForward());
	EXPECT_EQ(m_navigationController.GetNumHistoryEntries(), 1);
}

TEST_F(ShellNavigationControllerTest, BackForward)
{
	HRESULT hr = NavigateToFolder(L"C:\\Fake1");
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_FALSE(m_navigationController.CanGoBack());
	EXPECT_FALSE(m_navigationController.CanGoForward());
	EXPECT_EQ(m_navigationController.GetNumHistoryEntries(), 1);

	hr = NavigateToFolder(L"C:\\Fake2");
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_TRUE(m_navigationController.CanGoBack());
	EXPECT_FALSE(m_navigationController.CanGoForward());
	EXPECT_EQ(m_navigationController.GetNumHistoryEntries(), 2);

	hr = m_navigationController.GoBack();
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_FALSE(m_navigationController.CanGoBack());
	EXPECT_TRUE(m_navigationController.CanGoForward());
	EXPECT_EQ(m_navigationController.GetNumHistoryEntries(), 2);
	EXPECT_EQ(m_navigationController.GetCurrentIndex(), 0);

	hr = m_navigationController.GoForward();
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_TRUE(m_navigationController.CanGoBack());
	EXPECT_FALSE(m_navigationController.CanGoForward());
	EXPECT_EQ(m_navigationController.GetNumHistoryEntries(), 2);
	EXPECT_EQ(m_navigationController.GetCurrentIndex(), 1);

	hr = NavigateToFolder(L"C:\\Fake3");
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_TRUE(m_navigationController.CanGoBack());
	EXPECT_FALSE(m_navigationController.CanGoForward());
	EXPECT_EQ(m_navigationController.GetNumHistoryEntries(), 3);

	// Go back to the first entry.
	hr = m_navigationController.GoToOffset(-2);
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_FALSE(m_navigationController.CanGoBack());
	EXPECT_TRUE(m_navigationController.CanGoForward());
	EXPECT_EQ(m_navigationController.GetNumHistoryEntries(), 3);

	hr = NavigateToFolder(L"C:\\Fake4");
	ASSERT_HRESULT_SUCCEEDED(hr);

	// Performing a new navigation should have cleared the forward history.
	EXPECT_TRUE(m_navigationController.CanGoBack());
	EXPECT_FALSE(m_navigationController.CanGoForward());
	EXPECT_EQ(m_navigationController.GetNumHistoryEntries(), 2);
}

TEST_F(ShellNavigationControllerTest, RetrieveHistory)
{
	HRESULT hr = NavigateToFolder(L"C:\\Fake1");
	ASSERT_HRESULT_SUCCEEDED(hr);

	auto history = m_navigationController.GetBackHistory();
	EXPECT_TRUE(history.empty());

	history = m_navigationController.GetForwardHistory();
	EXPECT_TRUE(history.empty());

	hr = NavigateToFolder(L"C:\\Fake2");
	ASSERT_HRESULT_SUCCEEDED(hr);

	history = m_navigationController.GetBackHistory();
	EXPECT_EQ(history.size(), 1);

	history = m_navigationController.GetForwardHistory();
	EXPECT_TRUE(history.empty());

	hr = NavigateToFolder(L"C:\\Fake3");
	ASSERT_HRESULT_SUCCEEDED(hr);

	history = m_navigationController.GetBackHistory();
	EXPECT_EQ(history.size(), 2);

	history = m_navigationController.GetForwardHistory();
	EXPECT_TRUE(history.empty());

	hr = m_navigationController.GoBack();
	ASSERT_HRESULT_SUCCEEDED(hr);

	history = m_navigationController.GetBackHistory();
	EXPECT_EQ(history.size(), 1);

	history = m_navigationController.GetForwardHistory();
	EXPECT_EQ(history.size(), 1);
}

TEST_F(ShellNavigationControllerTest, GoUp)
{
	unique_pidl_absolute pidlFolder(SHSimpleIDListFromPath(L"C:\\Fake"));
	ASSERT_TRUE(pidlFolder);
	auto navigateParamsFolder = NavigateParams::Normal(pidlFolder.get());
	HRESULT hr = m_navigationController.Navigate(navigateParamsFolder);
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_TRUE(m_navigationController.CanGoUp());

	hr = m_navigationController.GoUp();
	EXPECT_HRESULT_SUCCEEDED(hr);

	auto entry = m_navigationController.GetCurrentEntry();
	ASSERT_NE(entry, nullptr);

	unique_pidl_absolute pidlParent(SHSimpleIDListFromPath(L"C:\\"));
	ASSERT_TRUE(pidlParent);
	EXPECT_TRUE(ArePidlsEquivalent(entry->GetPidl().get(), pidlParent.get()));

	// The desktop folder is the root of the shell namespace.
	unique_pidl_absolute pidlDesktop;
	hr = SHGetKnownFolderIDList(FOLDERID_Desktop, KF_FLAG_DEFAULT, nullptr,
		wil::out_param(pidlDesktop));
	ASSERT_HRESULT_SUCCEEDED(hr);

	auto navigateParamsDesktop = NavigateParams::Normal(pidlDesktop.get());
	hr = m_navigationController.Navigate(navigateParamsDesktop);
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_FALSE(m_navigationController.CanGoUp());

	hr = m_navigationController.GoUp();
	EXPECT_HRESULT_FAILED(hr);

	entry = m_navigationController.GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_TRUE(ArePidlsEquivalent(entry->GetPidl().get(), pidlDesktop.get()));
}

TEST_F(ShellNavigationControllerTest, HistoryEntries)
{
	auto entry = m_navigationController.GetCurrentEntry();
	EXPECT_EQ(entry, nullptr);

	entry = m_navigationController.GetEntryAtIndex(0);
	EXPECT_EQ(entry, nullptr);

	unique_pidl_absolute pidl1(SHSimpleIDListFromPath(L"C:\\Fake1"));
	ASSERT_TRUE(pidl1);

	auto navigateParams1 = NavigateParams::Normal(pidl1.get());
	HRESULT hr = m_navigationController.Navigate(navigateParams1);
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_EQ(m_navigationController.GetCurrentIndex(), 0);

	entry = m_navigationController.GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_TRUE(ArePidlsEquivalent(entry->GetPidl().get(), pidl1.get()));

	EXPECT_EQ(m_navigationController.GetIndexOfEntry(entry), 0);
	EXPECT_EQ(m_navigationController.GetEntryById(entry->GetId()), entry);

	unique_pidl_absolute pidl2(SHSimpleIDListFromPath(L"C:\\Fake2"));
	ASSERT_TRUE(pidl2);

	auto navigateParams2 = NavigateParams::Normal(pidl2.get());
	hr = m_navigationController.Navigate(navigateParams2);
	ASSERT_HRESULT_SUCCEEDED(hr);

	entry = m_navigationController.GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_TRUE(ArePidlsEquivalent(entry->GetPidl().get(), pidl2.get()));

	EXPECT_EQ(m_navigationController.GetIndexOfEntry(entry), 1);
	EXPECT_EQ(m_navigationController.GetEntryById(entry->GetId()), entry);

	EXPECT_EQ(m_navigationController.GetCurrentIndex(), 1);
	EXPECT_EQ(m_navigationController.GetCurrentEntry(), m_navigationController.GetEntryAtIndex(1));

	entry = m_navigationController.GetEntryAtIndex(0);
	ASSERT_NE(entry, nullptr);
	EXPECT_TRUE(ArePidlsEquivalent(entry->GetPidl().get(), pidl1.get()));
}

TEST_F(ShellNavigationControllerTest, SetNavigationMode)
{
	unique_pidl_absolute pidl1(SHSimpleIDListFromPath(L"C:\\Fake1"));
	ASSERT_TRUE(pidl1);

	auto params = NavigateParams::Normal(pidl1.get());

	EXPECT_CALL(m_navigator, NavigateImpl(Ref(params)));

	// By default, all navigations should proceed in the current tab.
	EXPECT_CALL(m_tabNavigation, CreateNewTab).Times(0);

	HRESULT hr = m_navigationController.Navigate(params);
	ASSERT_HRESULT_SUCCEEDED(hr);

	m_navigationController.SetNavigationMode(NavigationMode::ForceNewTab);

	// Although the navigation mode has been set, the navigation is to the same directory, which is
	// treated as an implicit refresh and should always proceed in the same tab.
	EXPECT_CALL(m_navigator, NavigateImpl(Ref(params)));
	EXPECT_CALL(m_tabNavigation, CreateNewTab).Times(0);

	hr = m_navigationController.Navigate(params);
	ASSERT_HRESULT_SUCCEEDED(hr);

	unique_pidl_absolute pidl2(SHSimpleIDListFromPath(L"C:\\Fake2"));
	ASSERT_TRUE(pidl2);

	params = NavigateParams::Normal(pidl2.get());

	// This is a navigation to a different directory, so the navigation mode above should now apply.
	EXPECT_CALL(m_navigator, NavigateImpl(_)).Times(0);
	EXPECT_CALL(m_tabNavigation, CreateNewTab(Ref(params), _));

	hr = m_navigationController.Navigate(params);
	ASSERT_HRESULT_SUCCEEDED(hr);

	unique_pidl_absolute pidl3(SHSimpleIDListFromPath(L"C:\\Fake3"));
	ASSERT_TRUE(pidl3);

	params = NavigateParams::Normal(pidl3.get());
	params.overrideNavigationMode = true;

	// The navigation explicitly overrides the navigation mode, so this navigation should proceed in
	// the tab, even though a navigation mode was applied above.
	EXPECT_CALL(m_navigator, NavigateImpl(Ref(params)));
	EXPECT_CALL(m_tabNavigation, CreateNewTab).Times(0);

	hr = m_navigationController.Navigate(params);
	ASSERT_HRESULT_SUCCEEDED(hr);
}

TEST_F(ShellNavigationControllerTest, SetNavigationModeFirstNavigation)
{
	m_navigationController.SetNavigationMode(NavigationMode::ForceNewTab);

	unique_pidl_absolute pidl1(SHSimpleIDListFromPath(L"C:\\Fake1"));
	ASSERT_TRUE(pidl1);

	auto params = NavigateParams::Normal(pidl1.get());

	// The first navigation in a tab should always take place within that tab, regardless of the
	// navigation mode in effect.
	EXPECT_CALL(m_navigator, NavigateImpl(Ref(params)));
	EXPECT_CALL(m_tabNavigation, CreateNewTab).Times(0);

	HRESULT hr = m_navigationController.Navigate(params);
	ASSERT_HRESULT_SUCCEEDED(hr);

	unique_pidl_absolute pidl2(SHSimpleIDListFromPath(L"C:\\Fake2"));
	ASSERT_TRUE(pidl2);

	auto params2 = NavigateParams::Normal(pidl2.get());

	// Subsequent navigations should then open in a new tab when necessary.
	EXPECT_CALL(m_navigator, NavigateImpl(Ref(params2))).Times(0);
	EXPECT_CALL(m_tabNavigation, CreateNewTab(Ref(params2), _));

	hr = m_navigationController.Navigate(params2);
	ASSERT_HRESULT_SUCCEEDED(hr);
}

TEST_F(ShellNavigationControllerTest, HistoryEntryTypes)
{
	unique_pidl_absolute pidl1;
	ASSERT_HRESULT_SUCCEEDED(NavigateToFolder(L"C:\\Fake1", HistoryEntryType::AddEntry, &pidl1));

	unique_pidl_absolute pidl2;
	ASSERT_HRESULT_SUCCEEDED(
		NavigateToFolder(L"C:\\Fake2", HistoryEntryType::ReplaceCurrentEntry, &pidl2));

	// The second navigation should have replaced the entry from the first navigation, so there
	// should only be a single entry.
	EXPECT_EQ(m_navigationController.GetNumHistoryEntries(), 1);
	EXPECT_EQ(m_navigationController.GetCurrentIndex(), 0);

	auto entry = m_navigationController.GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_TRUE(ArePidlsEquivalent(entry->GetPidl().get(), pidl2.get()));

	unique_pidl_absolute pidl3;
	ASSERT_HRESULT_SUCCEEDED(NavigateToFolder(L"C:\\Fake3", HistoryEntryType::AddEntry, &pidl3));

	EXPECT_EQ(m_navigationController.GetNumHistoryEntries(), 2);
	EXPECT_EQ(m_navigationController.GetCurrentIndex(), 1);

	entry = m_navigationController.GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_TRUE(ArePidlsEquivalent(entry->GetPidl().get(), pidl3.get()));

	unique_pidl_absolute pidl4;
	ASSERT_HRESULT_SUCCEEDED(NavigateToFolder(L"C:\\Fake4", HistoryEntryType::None, &pidl4));

	EXPECT_EQ(m_navigationController.GetNumHistoryEntries(), 2);
	EXPECT_EQ(m_navigationController.GetCurrentIndex(), 1);

	// No entry should have been added, so the current entry should still be the same as it was
	// previously.
	entry = m_navigationController.GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_TRUE(ArePidlsEquivalent(entry->GetPidl().get(), pidl3.get()));
}

TEST_F(ShellNavigationControllerTest, ReplacePreviousHistoryEntry)
{
	ASSERT_HRESULT_SUCCEEDED(NavigateToFolder(L"C:\\Fake1", HistoryEntryType::AddEntry));
	ASSERT_HRESULT_SUCCEEDED(NavigateToFolder(L"C:\\Fake2", HistoryEntryType::AddEntry));

	auto *entry = m_navigationController.GetEntryAtIndex(0);
	ASSERT_NE(entry, nullptr);
	int originalEntryId = entry->GetId();

	auto params = NavigateParams::History(entry);
	params.historyEntryType = HistoryEntryType::ReplaceCurrentEntry;
	ASSERT_HRESULT_SUCCEEDED(m_navigationController.Navigate(params));

	auto *updatedEntry = m_navigationController.GetEntryAtIndex(0);
	ASSERT_NE(updatedEntry, nullptr);

	// Navigating to the entry should have resulted in it being replaced, so the ID of the first
	// entry should have changed.
	EXPECT_NE(updatedEntry->GetId(), originalEntryId);
}

TEST_F(ShellNavigationControllerTest, HistoryEntryTypeFirstNavigation)
{
	unique_pidl_absolute pidl;
	ASSERT_HRESULT_SUCCEEDED(NavigateToFolder(L"C:\\Fake", HistoryEntryType::None, &pidl));

	// The first navigation in a tab should always result in a history entry being added, regardless
	// of what's requested.
	EXPECT_EQ(m_navigationController.GetNumHistoryEntries(), 1);
	EXPECT_EQ(m_navigationController.GetCurrentIndex(), 0);

	auto entry = m_navigationController.GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_TRUE(ArePidlsEquivalent(entry->GetPidl().get(), pidl.get()));
}

TEST_F(ShellNavigationControllerPreservedTest, FirstIndexIsCurrent)
{
	SetUp(0);

	EXPECT_EQ(m_navigationController->GetCurrentIndex(), 0);
	EXPECT_FALSE(m_navigationController->CanGoBack());
	EXPECT_TRUE(m_navigationController->CanGoForward());
	EXPECT_EQ(m_navigationController->GetNumHistoryEntries(), 2);
}

TEST_F(ShellNavigationControllerPreservedTest, SecondIndexIsCurrent)
{
	SetUp(1);

	EXPECT_EQ(m_navigationController->GetCurrentIndex(), 1);
	EXPECT_TRUE(m_navigationController->CanGoBack());
	EXPECT_FALSE(m_navigationController->CanGoForward());
	EXPECT_EQ(m_navigationController->GetNumHistoryEntries(), 2);
}

TEST_F(ShellNavigationControllerPreservedTest, CheckEntries)
{
	SetUp(0);

	for (size_t i = 0; i < m_preservedEntries.size(); i++)
	{
		auto entry = m_navigationController->GetEntryAtIndex(static_cast<int>(i));
		ASSERT_NE(entry, nullptr);
		EXPECT_TRUE(ArePidlsEquivalent(entry->GetPidl().get(), m_preservedEntries[i]->pidl.get()));
		EXPECT_EQ(entry->GetDisplayName(), m_preservedEntries[i]->displayName);
		EXPECT_EQ(entry->GetFullPathForDisplay(), m_preservedEntries[i]->fullPathForDisplay);
	}
}
