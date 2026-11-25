// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SessionRestorer.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "PidlTestHelper.h"
#include "RebarBandStorageData.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellNameParserFake.h"
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
			&m_browserWindowFactory, &m_shellNameParser)
	{
	}

	static void VerifyTabs(BrowserWindow *browser,
		const std::vector<std::wstring> &expectedTabDirectories)
	{
		auto *tabBar = browser->GetActiveTabBar();
		auto tabs = tabBar->GetAllTabsInOrder();
		ASSERT_EQ(tabs.size(), expectedTabDirectories.size());

		// TODO: This should use std::views::zip once C++23 support is available.
		for (const auto &[tab, directory] : boost::combine(tabs, expectedTabDirectories))
		{
			EXPECT_EQ(tab->GetShellBrowser()->GetDirectory(), CreateSimplePidlForTest(directory));
		}
	}

	ShellNameParserFake m_shellNameParser;
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

		browser->GetActiveTabBar()->SelectTab(*tab2);

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

TEST_F(SessionRestorerTest, CommandLineFilesToSelect)
{
	m_commandLineSettings.filesToSelect = { L"c:\\users", L"h:\\project\\documents" };

	m_sessionRestorer.Restore({});
	ASSERT_EQ(m_browserList.GetSize(), 1u);

	auto *restoredBrowser = m_browserList.GetLastActive();
	ASSERT_NE(restoredBrowser, nullptr);

	VerifyTabs(restoredBrowser, { L"c:\\", L"h:\\project" });

	// TODO: At the moment, there's no way to check that the requested files have been selected, but
	// it should be tested if possible.
}

TEST_F(SessionRestorerTest, NewStartupTabsUseDefaultFolderSettings)
{
	m_config.startupMode = StartupMode::CustomFolders;
	m_config.startupFolders = { L"c:\\startup" };
	m_config.defaultFolderSettings.viewMode = ViewMode::Details;
	m_config.defaultFolderSettings.showHidden = false;
	m_config.defaultFolderSettings.sortDirection = SortDirection::Descending;

	m_config.globalFolderSettings.folderColumns.realFolderColumns[0].width = 497;
	m_config.globalFolderSettings.folderColumns.realFolderColumns[1].width = 186;
	WindowStorageData createdWindow;
	BrowserWindowFactoryFake factory(
		[&](const WindowStorageData &storageData)
		{
			createdWindow = storageData;
			return nullptr;
		});
	SessionRestorer restorer(&m_commandLineSettings, &m_config, &m_featureList, &factory,
		&m_shellNameParser);

	for (int scenario = 0; scenario < 3; scenario++)
	{
		SCOPED_TRACE(scenario);
		m_commandLineSettings.directories.clear();
		m_commandLineSettings.filesToSelect.clear();
		if (scenario == 1)
		{
			m_commandLineSettings.directories = { L"d:\\command-line" };
		}
		else if (scenario == 2)
		{
			m_commandLineSettings.filesToSelect = { L"e:\\project\\selected.cpp" };
		}

		restorer.Restore({});
		ASSERT_EQ(createdWindow.tabs.size(), 1u);
		EXPECT_EQ(createdWindow.tabs[0].folderSettings, m_config.defaultFolderSettings);
		EXPECT_EQ(createdWindow.tabs[0].columns, m_config.globalFolderSettings.folderColumns);
	}
}

TEST_F(SessionRestorerTest, RestoredTabsKeepSavedFolderSettings)
{
	m_config.startupMode = StartupMode::PreviousTabs;
	m_config.defaultFolderSettings.viewMode = ViewMode::Details;
	m_commandLineSettings.directories = { L"d:\\new-tab" };
	WindowStorageData previousWindow;
	previousWindow.tabs = { { .directory = L"c:\\saved-tab" } };
	previousWindow.tabs[0].folderSettings.viewMode = ViewMode::LargeIcons;
	previousWindow.tabs[0].columns.realFolderColumns[0].width = 363;
	m_config.globalFolderSettings.folderColumns.realFolderColumns[0].width = 497;

	WindowStorageData createdWindow;
	BrowserWindowFactoryFake factory(
		[&](const WindowStorageData &storageData)
		{
			createdWindow = storageData;
			return nullptr;
		});
	SessionRestorer restorer(&m_commandLineSettings, &m_config, &m_featureList, &factory,
		&m_shellNameParser);
	restorer.Restore({ previousWindow });

	ASSERT_EQ(createdWindow.tabs.size(), 2u);
	EXPECT_EQ(createdWindow.tabs[0].folderSettings, previousWindow.tabs[0].folderSettings);
	EXPECT_EQ(createdWindow.tabs[1].folderSettings, m_config.defaultFolderSettings);
	EXPECT_EQ(createdWindow.tabs[0].columns, previousWindow.tabs[0].columns);
	EXPECT_EQ(createdWindow.tabs[1].columns, m_config.globalFolderSettings.folderColumns);
}
