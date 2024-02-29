// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Tab.h"
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
	TabTest() : m_tab(nullptr), m_observer(&m_tab)
	{
	}

	Tab m_tab;
	TabObserverMock m_observer;
};

TEST_F(TabTest, CustomName)
{
	EXPECT_FALSE(m_tab.GetUseCustomName());

	std::wstring customName = L"Name";
	m_tab.SetCustomName(customName);
	EXPECT_TRUE(m_tab.GetUseCustomName());
	EXPECT_EQ(m_tab.GetName(), customName);

	m_tab.ClearCustomName();
	EXPECT_FALSE(m_tab.GetUseCustomName());
}

TEST_F(TabTest, EmptyName)
{
	// An empty string isn't counted as a valid name and should be ignored.
	m_tab.SetCustomName(L"");
	EXPECT_FALSE(m_tab.GetUseCustomName());
}

TEST_F(TabTest, Update)
{
	EXPECT_CALL(m_observer, OnTabUpdated(Ref(m_tab), Tab::PropertyType::Name));
	m_tab.SetCustomName(L"Name");

	EXPECT_CALL(m_observer, OnTabUpdated(Ref(m_tab), Tab::PropertyType::Name));
	m_tab.ClearCustomName();

	EXPECT_CALL(m_observer, OnTabUpdated(Ref(m_tab), Tab::PropertyType::LockState));
	m_tab.SetLockState(Tab::LockState::Locked);

	EXPECT_CALL(m_observer, OnTabUpdated(Ref(m_tab), Tab::PropertyType::LockState));
	m_tab.SetLockState(Tab::LockState::AddressLocked);

	EXPECT_CALL(m_observer, OnTabUpdated(Ref(m_tab), Tab::PropertyType::LockState));
	m_tab.SetLockState(Tab::LockState::NotLocked);
}
