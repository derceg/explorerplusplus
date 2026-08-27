// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SessionRestorer.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "MainRebarStorage.h"
#include "PidlTestHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "WindowStorage.h"
#include "../Helper/WindowHelper.h"
#include <boost/range/combine.hpp>
#include <gtest/gtest.h>
#include <vector>

class SessionRestorerTest : public BrowserTestBase
{
protected:
	SessionRestorerTest() :
		m_sessionRestorer(&m_commandLineSettings, &m_config, &m_featureList,
			&m_browserWindowFactory)
	{
	}

	static void VerifyTabs(BrowserWindow *browser,
		const std::vector<std::wstring> &expectedTabDirectories)
	{
		auto *tabContainer = browser->GetActiveTabContainer();
		auto tabs = tabContainer->GetAllTabsInOrder();
		ASSERT_EQ(tabs.size(), expectedTabDirectories.size());

		// TODO: This should use std::views::zip once C++23 support is available.
		for (const auto &[tab, directory] : boost::combine(tabs, expectedTabDirectories))
		{
			EXPECT_EQ(tab->GetShellBrowser()->GetDirectory(), CreateSimplePidlForTest(directory));
		}
	}

	SessionRestorer m_sessionRestorer;
};

TEST_F(SessionRestorerTest, RestorePrevious)
{
	m_config.startupMode = StartupMode::PreviousTabs;

	WindowStorageData windowStorageData;

	{
		auto *browser = AddBrowser();

		browser->AddTab(L"c:\\");

		auto *tab2 = browser->AddTab(L"d:\\path");
		tab2->SetLockState(Tab::LockState::Locked);
		tab2->SetCustomName(L"Custom name");

		auto *tab3 = browser->AddTab(L"g:\\");
		tab3->SetLockState(Tab::LockState::AddressLocked);

		browser->GetActiveTabContainer()->SelectTab(*tab2);

		windowStorageData = browser->GetStorageData();

		RemoveBrowser(browser);
	}

	m_sessionRestorer.Restore({ windowStorageData });
	ASSERT_EQ(m_browserList.GetSize(), 1u);

	auto *restoredBrowser = m_browserList.GetLastActive();
	ASSERT_NE(restoredBrowser, nullptr);

	EXPECT_EQ(restoredBrowser->GetStorageData(), windowStorageData);
}

TEST_F(SessionRestorerTest, RestorePreviousWhenNoPreviousWindows)
{
	m_config.startupMode = StartupMode::PreviousTabs;

	// There are no windows being provided here, but a single new window should still be created.
	m_sessionRestorer.Restore({});
	EXPECT_EQ(m_browserList.GetSize(), 1u);
}

TEST_F(SessionRestorerTest, RestoreCustomFolders)
{
	m_config.startupMode = StartupMode::CustomFolders;
	m_config.startupFolders = { L"c:\\", L"d:\\", L"e:\\" };

	m_sessionRestorer.Restore({});
	ASSERT_EQ(m_browserList.GetSize(), 1u);

	auto *browser = m_browserList.GetLastActive();
	ASSERT_NE(browser, nullptr);

	VerifyTabs(browser, m_config.startupFolders);
}

TEST_F(SessionRestorerTest, RestoreCustomFoldersWithPreviousWindowState)
{
	m_config.startupMode = StartupMode::CustomFolders;
	m_config.startupFolders = { L"c:\\", L"d:\\", L"e:\\" };

	WindowStorageData sessionWindow;

	{
		auto *browser = AddBrowser();
		browser->SetWorkspaceBounds({ 11, 246, 1847, 862 });

		browser->AddTab(L"c:\\previous");

		sessionWindow = browser->GetStorageData();

		RemoveBrowser(browser);
	}

	m_sessionRestorer.Restore({ sessionWindow });
	ASSERT_EQ(m_browserList.GetSize(), 1u);

	auto *restoredBrowser = m_browserList.GetLastActive();
	ASSERT_NE(restoredBrowser, nullptr);

	// When a previous session window exists, its state information should be used when creating a
	// new window on startup.
	auto storageData = restoredBrowser->GetStorageData();
	EXPECT_EQ(storageData.bounds, sessionWindow.bounds);

	// On the other hand, the tabs from the previous session window should be ignored, so the only
	// tabs created should be for the configured startup folders.
	VerifyTabs(restoredBrowser, m_config.startupFolders);
}

TEST_F(SessionRestorerTest, RestoreCustomFoldersWhenEmpty)
{
	m_config.startupMode = StartupMode::CustomFolders;

	// There are no startup folders assigned, but a window should still be created.
	m_sessionRestorer.Restore({});
	EXPECT_EQ(m_browserList.GetSize(), 1u);
}

TEST_F(SessionRestorerTest, RestoreDefaultFolder)
{
	m_config.startupMode = StartupMode::DefaultFolder;

	m_sessionRestorer.Restore({});
	EXPECT_EQ(m_browserList.GetSize(), 1u);
}

TEST_F(SessionRestorerTest, CommandLineDirectoriesWithPreviousTabs)
{
	m_config.startupMode = StartupMode::PreviousTabs;

	m_commandLineSettings.directories = { L"c:\\", L"d:\\", L"e:\\" };

	WindowStorageData sessionWindow;
	sessionWindow.tabs = { { .directory = L"h:\\previous\\folder" } };

	m_sessionRestorer.Restore({ sessionWindow });
	ASSERT_EQ(m_browserList.GetSize(), 1u);

	auto *restoredBrowser = m_browserList.GetLastActive();
	ASSERT_NE(restoredBrowser, nullptr);

	// When loading a set of previous tabs, command line directories (if present) should be loaded
	// in addition to the previous tabs.
	VerifyTabs(restoredBrowser, { L"h:\\previous\\folder", L"c:\\", L"d:\\", L"e:\\" });
}

TEST_F(SessionRestorerTest, CommandLineDirectoriesWithCustomFolders)
{
	m_config.startupMode = StartupMode::CustomFolders;
	m_config.startupFolders = { L"c:\\startup-folder" };

	m_commandLineSettings.directories = { L"g:\\", L"h:\\documents" };

	m_sessionRestorer.Restore({});
	ASSERT_EQ(m_browserList.GetSize(), 1u);

	auto *restoredBrowser = m_browserList.GetLastActive();
	ASSERT_NE(restoredBrowser, nullptr);

	// When loading a set of custom folders, command line directories (if present) should replace
	// the custom folders. That is, the only tabs that should exist at this point should be the tabs
	// for the command line directories.
	VerifyTabs(restoredBrowser, m_commandLineSettings.directories);
}

TEST_F(SessionRestorerTest, CommandLineDirectoriesWithDefaultFolder)
{
	m_config.startupMode = StartupMode::DefaultFolder;

	m_commandLineSettings.directories = { L"d:\\", L"d:\\folder" };

	m_sessionRestorer.Restore({});
	ASSERT_EQ(m_browserList.GetSize(), 1u);

	auto *restoredBrowser = m_browserList.GetLastActive();
	ASSERT_NE(restoredBrowser, nullptr);

	// As with the above case, the only tabs that should exist at this point should be the tabs for
	// the command line directories.
	VerifyTabs(restoredBrowser, m_commandLineSettings.directories);
}
