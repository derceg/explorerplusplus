// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Tab.h"
#include "BrowserWindowMock.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "ShellTestHelper.h"
#include "TabEvents.h"
#include "TabNavigationMock.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class TabTest : public Test
{
protected:
	Tab BuildTab(const Tab::InitialData &initialData = {})
	{
		return Tab(std::make_unique<ShellBrowserFake>(&m_navigationEvents, &m_tabNavigation),
			&m_browser, nullptr, &m_tabEvents, initialData);
	}

	NavigationEvents m_navigationEvents;
	TabNavigationMock m_tabNavigation;
	BrowserWindowMock m_browser;
	TabEvents m_tabEvents;
};

TEST_F(TabTest, InitialData)
{
	Tab::InitialData initialData;
	initialData.useCustomName = true;
	initialData.customName = L"Custom tab name";
	initialData.lockState = Tab::LockState::AddressLocked;

	auto tab = BuildTab(initialData);
	EXPECT_EQ(tab.GetUseCustomName(), initialData.useCustomName);
	EXPECT_EQ(tab.GetName(), initialData.customName);
	EXPECT_EQ(tab.GetLockState(), initialData.lockState);
}

TEST_F(TabTest, CustomName)
{
	auto tab = BuildTab();

	std::wstring folderName = L"fake";
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\" + folderName);
	auto navigateParams = NavigateParams::Normal(pidl.Raw());
	tab.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);

	EXPECT_FALSE(tab.GetUseCustomName());
	EXPECT_EQ(tab.GetName(), folderName);

	std::wstring customName = L"Name";
	tab.SetCustomName(customName);
	EXPECT_TRUE(tab.GetUseCustomName());
	EXPECT_EQ(tab.GetName(), customName);

	tab.ClearCustomName();
	EXPECT_FALSE(tab.GetUseCustomName());
	EXPECT_EQ(tab.GetName(), folderName);
}

TEST_F(TabTest, EmptyName)
{
	auto tab = BuildTab();

	std::wstring folderName = L"fake";
	PidlAbsolute pidl = CreateSimplePidlForTest(L"c:\\" + folderName);
	auto navigateParams = NavigateParams::Normal(pidl.Raw());
	tab.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);

	// An empty string isn't counted as a valid name and should be ignored.
	tab.SetCustomName(L"");
	EXPECT_FALSE(tab.GetUseCustomName());
	EXPECT_EQ(tab.GetName(), folderName);
}

TEST_F(TabTest, InitialLockState)
{
	auto tab1 = BuildTab({ .lockState = Tab::LockState::NotLocked });
	EXPECT_EQ(tab1.GetShellBrowser()->GetNavigationController()->GetNavigationTargetMode(),
		NavigationTargetMode::Normal);

	auto tab2 = BuildTab({ .lockState = Tab::LockState::Locked });
	EXPECT_EQ(tab2.GetShellBrowser()->GetNavigationController()->GetNavigationTargetMode(),
		NavigationTargetMode::Normal);

	auto tab3 = BuildTab({ .lockState = Tab::LockState::AddressLocked });
	EXPECT_EQ(tab3.GetShellBrowser()->GetNavigationController()->GetNavigationTargetMode(),
		NavigationTargetMode::ForceNewTab);
}

TEST_F(TabTest, LockState)
{
	auto tab = BuildTab();

	EXPECT_EQ(tab.GetLockState(), Tab::LockState::NotLocked);
	EXPECT_FALSE(tab.IsLocked());
	EXPECT_EQ(tab.GetShellBrowser()->GetNavigationController()->GetNavigationTargetMode(),
		NavigationTargetMode::Normal);

	tab.SetLockState(Tab::LockState::Locked);
	EXPECT_EQ(tab.GetLockState(), Tab::LockState::Locked);
	EXPECT_TRUE(tab.IsLocked());
	EXPECT_EQ(tab.GetShellBrowser()->GetNavigationController()->GetNavigationTargetMode(),
		NavigationTargetMode::Normal);

	tab.SetLockState(Tab::LockState::AddressLocked);
	EXPECT_EQ(tab.GetLockState(), Tab::LockState::AddressLocked);
	EXPECT_TRUE(tab.IsLocked());
	EXPECT_EQ(tab.GetShellBrowser()->GetNavigationController()->GetNavigationTargetMode(),
		NavigationTargetMode::ForceNewTab);
}

TEST_F(TabTest, Update)
{
	auto tab = BuildTab();

	MockFunction<void(const Tab &tab, Tab::PropertyType propertyType)> tabUpdatedCallback;
	m_tabEvents.AddUpdatedObserver(tabUpdatedCallback.AsStdFunction(), TabEventScope::Global());

	InSequence seq;

	EXPECT_CALL(tabUpdatedCallback, Call(Ref(tab), Tab::PropertyType::Name)).Times(2);
	EXPECT_CALL(tabUpdatedCallback, Call(Ref(tab), Tab::PropertyType::LockState)).Times(3);

	tab.SetCustomName(L"Name");
	tab.ClearCustomName();

	tab.SetLockState(Tab::LockState::Locked);
	tab.SetLockState(Tab::LockState::AddressLocked);
	tab.SetLockState(Tab::LockState::NotLocked);
}

TEST_F(TabTest, NoUpdateOnCreation)
{
	MockFunction<void(const Tab &tab, Tab::PropertyType propertyType)> tabUpdatedCallback;
	m_tabEvents.AddUpdatedObserver(tabUpdatedCallback.AsStdFunction(), TabEventScope::Global());

	// Specifying a lock state during construction shouldn't result in an update notification being
	// triggered.
	EXPECT_CALL(tabUpdatedCallback, Call(_, _)).Times(0);

	auto tab1 = BuildTab({ .lockState = Tab::LockState::NotLocked });
	auto tab2 = BuildTab({ .lockState = Tab::LockState::Locked });
	auto tab3 = BuildTab({ .lockState = Tab::LockState::AddressLocked });
}

TEST_F(TabTest, GetBrowser)
{
	auto tab = BuildTab();

	EXPECT_EQ(tab.GetBrowser(), &m_browser);
}
