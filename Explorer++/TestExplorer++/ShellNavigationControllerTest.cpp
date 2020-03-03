// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "../Explorer++/ShellBrowser/HistoryEntry.h"
#include "../Explorer++/ShellBrowser/NavigatorInterface.h"
#include "../Explorer++/ShellBrowser/PreservedHistoryEntry.h"
#include "../Explorer++/ShellBrowser/ShellNavigationController.h"
#include "../Explorer++/TabNavigationInterface.h"
#include "../Helper/IconFetcher.h"
#include "../Helper/ShellHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ShlObj.h>

using namespace testing;

class NavigatorFake : public NavigatorInterface
{
public:

	HRESULT BrowseFolder(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry = true) override
	{
		m_navigationCompletedSignal(pidlDirectory, addHistoryEntry);

		return S_OK;
	}

	boost::signals2::connection AddNavigationCompletedObserver(const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override
	{
		return m_navigationCompletedSignal.connect(observer, position);
	}

private:

	NavigationCompletedSignal m_navigationCompletedSignal;

};

class NavigatorMock : public NavigatorInterface
{
public:

	NavigatorMock()
	{
		ON_CALL(*this, BrowseFolderImpl).WillByDefault([this] (PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry) {
			return m_fake.BrowseFolder(pidlDirectory, addHistoryEntry);
		});

		ON_CALL(*this, AddNavigationCompletedObserverImpl).WillByDefault([this] (const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position) {
			return m_fake.AddNavigationCompletedObserver(observer, position);
		});
	}

	MOCK_METHOD(HRESULT, BrowseFolderImpl, (PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry));
	MOCK_METHOD(boost::signals2::connection, AddNavigationCompletedObserverImpl, (const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position));

	HRESULT BrowseFolder(PCIDLIST_ABSOLUTE pidlDirectory, bool addHistoryEntry = true) override
	{
		return BrowseFolderImpl(pidlDirectory, addHistoryEntry);
	}

	boost::signals2::connection AddNavigationCompletedObserver(const NavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override
	{
		return AddNavigationCompletedObserverImpl(observer, position);
	}

private:

	NavigatorFake m_fake;
};

class TabNavigationMock : public TabNavigationInterface
{
public:

	MOCK_METHOD(HRESULT, CreateNewTab, (PCIDLIST_ABSOLUTE pidlDirectory, bool selected), (override));
};

class IconFetcherMock : public IconFetcherInterface
{
public:

	MOCK_METHOD(void, QueueIconTask, (std::wstring_view path, Callback callback), (override));
	MOCK_METHOD(void, QueueIconTask, (PCIDLIST_ABSOLUTE pidl, Callback callback), (override));
	MOCK_METHOD(void, ClearQueue, (), (override));
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
	HRESULT NavigateToFolder(const std::wstring &path)
	{
		unique_pidl_absolute pidl(SHSimpleIDListFromPath(path.c_str()));

		if (!pidl)
		{
			return E_FAIL;
		}

		return m_navigationController.BrowseFolder(pidl.get());
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

		TCHAR displayName[MAX_PATH];
		HRESULT hr = GetDisplayName(pidl.get(), displayName, static_cast<UINT>(std::size(displayName)), SHGDN_INFOLDER);

		if (FAILED(hr))
		{
			return nullptr;
		}

		HistoryEntry entry(pidl.get(), displayName);
		return std::make_unique<PreservedHistoryEntry>(entry);
	}

	NavigatorMock m_navigator;
	TabNavigationMock m_tabNavigation;
	IconFetcherMock m_iconFetcher;
	std::unique_ptr<ShellNavigationController> m_navigationController;

	std::vector<std::unique_ptr<PreservedHistoryEntry>> m_preservedEntries;
};

TEST_F(ShellNavigationControllerTest, Refresh) {
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

TEST_F(ShellNavigationControllerTest, BackForward) {
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

TEST_F(ShellNavigationControllerTest, RetrieveHistory) {
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

TEST_F(ShellNavigationControllerTest, GoUp) {
	unique_pidl_absolute pidlFolder(SHSimpleIDListFromPath(L"C:\\Fake"));
	ASSERT_TRUE(pidlFolder);
	HRESULT hr = m_navigationController.BrowseFolder(pidlFolder.get());
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_TRUE(m_navigationController.CanGoUp());

	hr = m_navigationController.GoUp();
	EXPECT_HRESULT_SUCCEEDED(hr);

	auto entry = m_navigationController.GetCurrentEntry();
	ASSERT_NE(entry, nullptr);

	unique_pidl_absolute pidlParent(SHSimpleIDListFromPath(L"C:\\"));
	ASSERT_TRUE(pidlParent);
	EXPECT_TRUE(CompareIdls(entry->GetPidl().get(), pidlParent.get()));

	// The desktop folder is the root of the shell namespace.
	unique_pidl_absolute pidlDesktop;
	hr = SHGetKnownFolderIDList(FOLDERID_Desktop, KF_FLAG_DEFAULT, nullptr, wil::out_param(pidlDesktop));
	ASSERT_HRESULT_SUCCEEDED(hr);

	hr = m_navigationController.BrowseFolder(pidlDesktop.get());
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_FALSE(m_navigationController.CanGoUp());

	hr = m_navigationController.GoUp();
	EXPECT_HRESULT_FAILED(hr);

	entry = m_navigationController.GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_TRUE(CompareIdls(entry->GetPidl().get(), pidlDesktop.get()));
}

TEST_F(ShellNavigationControllerTest, GetEntry) {
	auto entry = m_navigationController.GetCurrentEntry();
	EXPECT_EQ(entry, nullptr);

	entry = m_navigationController.GetEntryAtIndex(0);
	EXPECT_EQ(entry, nullptr);

	unique_pidl_absolute pidl1(SHSimpleIDListFromPath(L"C:\\Fake1"));
	ASSERT_TRUE(pidl1);

	HRESULT hr = m_navigationController.BrowseFolder(pidl1.get());
	ASSERT_HRESULT_SUCCEEDED(hr);

	EXPECT_EQ(m_navigationController.GetCurrentIndex(), 0);

	entry = m_navigationController.GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_TRUE(CompareIdls(entry->GetPidl().get(), pidl1.get()));

	unique_pidl_absolute pidl2(SHSimpleIDListFromPath(L"C:\\Fake2"));
	ASSERT_TRUE(pidl2);

	hr = m_navigationController.BrowseFolder(pidl2.get());
	ASSERT_HRESULT_SUCCEEDED(hr);

	entry = m_navigationController.GetCurrentEntry();
	ASSERT_NE(entry, nullptr);
	EXPECT_TRUE(CompareIdls(entry->GetPidl().get(), pidl2.get()));

	EXPECT_EQ(m_navigationController.GetCurrentIndex(), 1);
	EXPECT_EQ(m_navigationController.GetCurrentEntry(), m_navigationController.GetEntryAtIndex(1));

	entry = m_navigationController.GetEntryAtIndex(0);
	ASSERT_NE(entry, nullptr);
	EXPECT_TRUE(CompareIdls(entry->GetPidl().get(), pidl1.get()));
}

TEST_F(ShellNavigationControllerTest, NavigationMode) {
	unique_pidl_absolute pidl(SHSimpleIDListFromPath(L"C:\\Fake"));
	ASSERT_TRUE(pidl);

	EXPECT_CALL(m_navigator, BrowseFolderImpl(pidl.get(), _));

	// By default, all navigations should proceed in the current tab.
	EXPECT_CALL(m_tabNavigation, CreateNewTab)
		.Times(0);

	HRESULT hr = m_navigationController.BrowseFolder(pidl.get());
	ASSERT_HRESULT_SUCCEEDED(hr);

	m_navigationController.SetNavigationMode(ShellNavigationController::NavigationMode::ForceNewTab);

	EXPECT_CALL(m_navigator, BrowseFolderImpl)
		.Times(0);

	EXPECT_CALL(m_tabNavigation, CreateNewTab(pidl.get(), _));

	hr = m_navigationController.BrowseFolder(pidl.get());
	ASSERT_HRESULT_SUCCEEDED(hr);
}

TEST_F(ShellNavigationControllerTest, NavigationModeFirstNavigation) {
	m_navigationController.SetNavigationMode(ShellNavigationController::NavigationMode::ForceNewTab);

	unique_pidl_absolute pidl1(SHSimpleIDListFromPath(L"C:\\Fake1"));
	ASSERT_TRUE(pidl1);

	// The first navigation in a tab should always take place within that tab, regardless of the
	// navigation mode in effect.
	EXPECT_CALL(m_navigator, BrowseFolderImpl(pidl1.get(), _));
	EXPECT_CALL(m_tabNavigation, CreateNewTab).Times(0);

	HRESULT hr = m_navigationController.BrowseFolder(pidl1.get());
	ASSERT_HRESULT_SUCCEEDED(hr);

	unique_pidl_absolute pidl2(SHSimpleIDListFromPath(L"C:\\Fake2"));
	ASSERT_TRUE(pidl2);

	// Subsequent navigations should then open in a new tab when necessary.
	EXPECT_CALL(m_navigator, BrowseFolderImpl).Times(0);
	EXPECT_CALL(m_tabNavigation, CreateNewTab(pidl2.get(), _));

	hr = m_navigationController.BrowseFolder(pidl2.get());
	ASSERT_HRESULT_SUCCEEDED(hr);
}

TEST_F(ShellNavigationControllerPreservedTest, FirstIndexIsCurrent) {
	SetUp(0);

	EXPECT_EQ(m_navigationController->GetCurrentIndex(), 0);
	EXPECT_FALSE(m_navigationController->CanGoBack());
	EXPECT_TRUE(m_navigationController->CanGoForward());
	EXPECT_EQ(m_navigationController->GetNumHistoryEntries(), 2);
}

TEST_F(ShellNavigationControllerPreservedTest, SecondIndexIsCurrent) {
	SetUp(1);

	EXPECT_EQ(m_navigationController->GetCurrentIndex(), 1);
	EXPECT_TRUE(m_navigationController->CanGoBack());
	EXPECT_FALSE(m_navigationController->CanGoForward());
	EXPECT_EQ(m_navigationController->GetNumHistoryEntries(), 2);
}

TEST_F(ShellNavigationControllerPreservedTest, CheckEntries) {
	SetUp(0);

	for (size_t i = 0; i < m_preservedEntries.size(); i++)
	{
		auto entry = m_navigationController->GetEntryAtIndex(static_cast<int>(i));
		ASSERT_NE(entry, nullptr);
		EXPECT_TRUE(CompareIdls(entry->GetPidl().get(), m_preservedEntries[i]->pidl.get()));
	}
}