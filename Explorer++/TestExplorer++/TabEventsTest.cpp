// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include <gtest/gtest.h>

using namespace testing;

class TabEventsTest : public BrowserTestBase
{
protected:
	TabEventsTest() : m_browser1(AddBrowser()), m_browser2(AddBrowser())
	{
	}

	BrowserWindowFake *const m_browser1;
	BrowserWindowFake *const m_browser2;
};

TEST_F(TabEventsTest, CreatedSignal)
{
	InSequence seq;

	const Tab *firstCreatedTab = nullptr;
	const Tab *secondCreatedTab = nullptr;

	MockFunction<void(const Tab &tab)> tabCreatedCallback;
	m_tabEvents.AddCreatedObserver(tabCreatedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(tabCreatedCallback, Call(_))
		.WillOnce([&firstCreatedTab](const auto &tab) { firstCreatedTab = &tab; });
	EXPECT_CALL(tabCreatedCallback, Call(_))
		.WillOnce([&secondCreatedTab](const auto &tab) { secondCreatedTab = &tab; });

	auto *tab1 = m_browser1->AddTab(L"c:\\");
	auto *tab2 = m_browser2->AddTab(L"c:\\");
	EXPECT_EQ(firstCreatedTab, tab1);
	EXPECT_EQ(secondCreatedTab, tab2);
}

TEST_F(TabEventsTest, UpdatedSignal)
{
	InSequence seq;

	auto *tab1 = m_browser1->AddTab(L"c:\\");
	auto *tab2 = m_browser2->AddTab(L"c:\\");

	MockFunction<void(const Tab &tab, Tab::PropertyType propertyType)> tabUpdatedCallback;
	m_tabEvents.AddUpdatedObserver(tabUpdatedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(tabUpdatedCallback, Call(Ref(*tab1), Tab::PropertyType::Name));
	EXPECT_CALL(tabUpdatedCallback, Call(Ref(*tab2), Tab::PropertyType::LockState));

	tab1->SetCustomName(L"Custom name");
	tab2->SetLockState(Tab::LockState::AddressLocked);
}

TEST_F(TabEventsTest, SelectedSignal)
{
	m_browser1->AddTab(L"c:\\");
	auto *tab2 = m_browser1->AddTab(L"c:\\");

	MockFunction<void(const Tab &tab)> tabSelectedCallback;
	m_tabEvents.AddSelectedObserver(tabSelectedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(tabSelectedCallback, Call(Ref(*tab2)));

	m_browser1->GetActiveTabBar()->SelectTab(*tab2);
}

TEST_F(TabEventsTest, MovedSignal)
{
	auto *tab1 = m_browser1->AddTab(L"c:\\");
	m_browser1->AddTab(L"c:\\");

	MockFunction<void(const Tab &tab, int fromIndex, int toIndex)> tabMovedCallback;
	m_tabEvents.AddMovedObserver(tabMovedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(tabMovedCallback, Call(Ref(*tab1), 0, 1));

	m_browser1->GetActiveTabBar()->MoveTab(*tab1, 1);
}

TEST_F(TabEventsTest, PreRemovalSignal)
{
	m_browser1->AddTab(L"c:\\");
	auto *tab2 = m_browser1->AddTab(L"c:\\");

	MockFunction<void(const Tab &tab, int index)> tabPreRemovalCallback;
	m_tabEvents.AddPreRemovalObserver(tabPreRemovalCallback.AsStdFunction(),
		TabEventScope::Global());
	EXPECT_CALL(tabPreRemovalCallback, Call(Ref(*tab2), 1));

	m_browser1->GetActiveTabBar()->CloseTab(*tab2);
}

TEST_F(TabEventsTest, RemovedSignal)
{
	auto *tab1 = m_browser1->AddTab(L"c:\\");
	m_browser1->AddTab(L"c:\\");

	MockFunction<void(const Tab &tab)> tabRemovedCallback;

	m_tabEvents.AddRemovedObserver(tabRemovedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(tabRemovedCallback, Call(Ref(*tab1)));

	m_browser1->GetActiveTabBar()->CloseTab(*tab1);
}

TEST_F(TabEventsTest, SignalsFilteredByBrowser)
{
	// The observer here should only be triggered when a tab event in m_browser1 occurs. That is,
	// only when the first tab is created.
	MockFunction<void(const Tab &tab)> tabCreatedCallback;
	m_tabEvents.AddCreatedObserver(tabCreatedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(*m_browser1));

	const Tab *createdTab = nullptr;
	EXPECT_CALL(tabCreatedCallback, Call(_))
		.WillOnce([&createdTab](const auto &tab) { createdTab = &tab; });

	auto *tab1 = m_browser1->AddTab(L"c:\\");
	m_browser2->AddTab(L"c:\\");
	EXPECT_EQ(createdTab, tab1);
}
