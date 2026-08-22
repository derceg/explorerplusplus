// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "PidlTestHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include <boost/range/combine.hpp>
#include <gtest/gtest.h>

class BrowserWindowTest : public BrowserTestBase
{
};

TEST_F(BrowserWindowTest, InitializeWithStorageData)
{
	WindowStorageData storageData;
	storageData.tabs = {
		{ .directory = L"c:\\", .tabSettings = { .lockState = Tab::LockState::AddressLocked } },
		{ .directory = L"g:\\path", .tabSettings = { .lockState = Tab::LockState::Locked } },
		{ .directory = L"z:\\folder", .tabSettings = { .name = L"Folder" } },
	};
	storageData.selectedTab = 2;

	auto *browser = AddBrowser({ storageData });
	auto *tabContainer = browser->GetActiveTabContainer();
	EXPECT_EQ(tabContainer->GetSelectedTabIndex(), storageData.selectedTab);

	auto tabs = tabContainer->GetAllTabsInOrder();
	ASSERT_EQ(tabs.size(), storageData.tabs.size());

	// TODO: This should use std::views::zip once C++23 support is available.
	for (const auto &[tab, storageTab] : boost::combine(tabs, storageData.tabs))
	{
		EXPECT_EQ(tab->GetShellBrowser()->GetDirectory(),
			CreateSimplePidlForTest(storageTab.directory));
		EXPECT_EQ(tab->GetLockState(),
			storageTab.tabSettings.lockState ? *storageTab.tabSettings.lockState
											 : Tab::LockState::NotLocked);
		EXPECT_EQ(tab->GetUseCustomName(), !!storageTab.tabSettings.name);

		if (storageTab.tabSettings.name)
		{
			EXPECT_EQ(tab->GetName(), *storageTab.tabSettings.name);
		}
	}
}

// Note that this currently uses BrowserWindowFake. However, a test like this still has value, for
// two reasons:
//
// 1. If/when the real BrowserWindow implementation class is used in tests, this test should work
//    without changes.
// 2. Although BrowserWindowFake implements part of GetStorageData() itself, it also makes calls to
//    other implementation classes. So, this isn't just testing that the fake is assembling the data
//    correctly.
TEST_F(BrowserWindowTest, GetStorageData)
{
	auto *browser = AddBrowser();

	browser->AddTab(L"c:\\");

	auto *tab2 = browser->AddTab(L"d:\\path");
	tab2->SetLockState(Tab::LockState::Locked);
	tab2->SetCustomName(L"Custom name");

	auto *tab3 = browser->AddTab(L"g:\\");
	tab3->SetLockState(Tab::LockState::AddressLocked);

	auto *tabContainer = browser->GetActiveTabContainer();
	tabContainer->SelectTab(*tab2);

	auto storageData = browser->GetStorageData();
	EXPECT_EQ(storageData.selectedTab, tabContainer->GetSelectedTabIndex());

	auto tabs = tabContainer->GetAllTabsInOrder();
	ASSERT_EQ(storageData.tabs.size(), tabs.size());

	// TODO: This should use std::views::zip once C++23 support is available.
	for (const auto &[storageTab, tab] : boost::combine(storageData.tabs, tabs))
	{
		EXPECT_EQ(storageTab.pidl, tab->GetShellBrowser()->GetDirectory());
		EXPECT_EQ(storageTab.directory, tab->GetShellBrowser()->GetDirectoryPath());

		EXPECT_EQ(storageTab.tabSettings.lockState, tab->GetLockState());
		EXPECT_EQ(storageTab.tabSettings.name,
			tab->GetUseCustomName() ? std::optional{ tab->GetName() } : std::nullopt);
	}
}
