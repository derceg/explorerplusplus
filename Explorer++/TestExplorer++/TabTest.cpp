// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Tab.h"
#include "BrowserWindowMock.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowserFake.h"
#include "TabNavigationMock.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class TabObserverMock
{
public:
	TabObserverMock(Tab *tab)
	{
		tab->AddTabUpdatedObserver(std::bind_front(&TabObserverMock::OnTabUpdated, this));
	}

	MOCK_METHOD(void, OnTabUpdated, (const Tab &tab, Tab::PropertyType propertyType));
};

class TabTest : public Test
{
protected:
	TabTest()
	{
		auto shellBrowser = std::make_unique<ShellBrowserFake>(&m_tabNavigation);
		m_shellBrowser = shellBrowser.get();

		m_tab = std::make_unique<Tab>(std::move(shellBrowser), &m_browser);

		m_observer = std::make_unique<TabObserverMock>(m_tab.get());
	}

	TabNavigationMock m_tabNavigation;
	ShellBrowserFake *m_shellBrowser = nullptr;
	BrowserWindowMock m_browser;
	std::unique_ptr<Tab> m_tab;
	std::unique_ptr<TabObserverMock> m_observer;
};

TEST_F(TabTest, CustomName)
{
	std::wstring folderName = L"fake";
	m_shellBrowser->NavigateToPath(L"c:\\" + folderName);

	EXPECT_FALSE(m_tab->GetUseCustomName());
	EXPECT_EQ(m_tab->GetName(), folderName);

	std::wstring customName = L"Name";
	m_tab->SetCustomName(customName);
	EXPECT_TRUE(m_tab->GetUseCustomName());
	EXPECT_EQ(m_tab->GetName(), customName);

	m_tab->ClearCustomName();
	EXPECT_FALSE(m_tab->GetUseCustomName());
	EXPECT_EQ(m_tab->GetName(), folderName);
}

TEST_F(TabTest, EmptyName)
{
	std::wstring folderName = L"fake";
	m_shellBrowser->NavigateToPath(L"c:\\" + folderName);

	// An empty string isn't counted as a valid name and should be ignored.
	m_tab->SetCustomName(L"");
	EXPECT_FALSE(m_tab->GetUseCustomName());
	EXPECT_EQ(m_tab->GetName(), folderName);
}

TEST_F(TabTest, LockState)
{
	EXPECT_EQ(m_shellBrowser->GetNavigationController()->GetNavigationTargetMode(),
		NavigationTargetMode::Normal);

	m_tab->SetLockState(Tab::LockState::Locked);
	EXPECT_EQ(m_shellBrowser->GetNavigationController()->GetNavigationTargetMode(),
		NavigationTargetMode::Normal);

	m_tab->SetLockState(Tab::LockState::AddressLocked);
	EXPECT_EQ(m_shellBrowser->GetNavigationController()->GetNavigationTargetMode(),
		NavigationTargetMode::ForceNewTab);
}

TEST_F(TabTest, Update)
{
	EXPECT_CALL(*m_observer, OnTabUpdated(Ref(*m_tab), Tab::PropertyType::Name));
	m_tab->SetCustomName(L"Name");

	EXPECT_CALL(*m_observer, OnTabUpdated(Ref(*m_tab), Tab::PropertyType::Name));
	m_tab->ClearCustomName();

	EXPECT_CALL(*m_observer, OnTabUpdated(Ref(*m_tab), Tab::PropertyType::LockState));
	m_tab->SetLockState(Tab::LockState::Locked);

	EXPECT_CALL(*m_observer, OnTabUpdated(Ref(*m_tab), Tab::PropertyType::LockState));
	m_tab->SetLockState(Tab::LockState::AddressLocked);

	EXPECT_CALL(*m_observer, OnTabUpdated(Ref(*m_tab), Tab::PropertyType::LockState));
	m_tab->SetLockState(Tab::LockState::NotLocked);
}

TEST_F(TabTest, GetBrowser)
{
	EXPECT_EQ(m_tab->GetBrowser(), &m_browser);
}
