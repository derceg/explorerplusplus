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
	TabEventsTest() :
		m_browser1(AddBrowser()),
		m_tab1(m_browser1->AddTab()),
		m_browser2(AddBrowser()),
		m_tab2(m_browser2->AddTab())
	{
	}

	BrowserWindowFake *const m_browser1;
	Tab *const m_tab1;

	BrowserWindowFake *const m_browser2;
	Tab *const m_tab2;

	StrictMock<MockFunction<void(const Tab &tab)>> m_tabCreatedCallback;
	StrictMock<MockFunction<void(const Tab &tab, Tab::PropertyType propertyType)>>
		m_tabUpdatedCallback;
	StrictMock<MockFunction<void(const Tab &tab)>> m_tabSelectedCallback;
	StrictMock<MockFunction<void(const Tab &tab, int fromIndex, int toIndex)>> m_tabMovedCallback;
	StrictMock<MockFunction<void(const Tab &tab, int index)>> m_tabPreRemovalCallback;
	StrictMock<MockFunction<void(const Tab &tab)>> m_tabRemovedCallback;
};

TEST_F(TabEventsTest, Signals)
{
	InSequence seq;

	m_tabEvents.AddCreatedObserver(m_tabCreatedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(m_tabCreatedCallback, Call(Ref(*m_tab1)));
	EXPECT_CALL(m_tabCreatedCallback, Call(Ref(*m_tab2)));

	m_tabEvents.AddUpdatedObserver(m_tabUpdatedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(m_tabUpdatedCallback, Call(Ref(*m_tab1), Tab::PropertyType::Name));
	EXPECT_CALL(m_tabUpdatedCallback, Call(Ref(*m_tab2), Tab::PropertyType::LockState));

	m_tabEvents.AddSelectedObserver(m_tabSelectedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(m_tabSelectedCallback, Call(Ref(*m_tab1)));

	m_tabEvents.AddMovedObserver(m_tabMovedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(m_tabMovedCallback, Call(Ref(*m_tab2), 1, 2));

	m_tabEvents.AddPreRemovalObserver(m_tabPreRemovalCallback.AsStdFunction(),
		TabEventScope::Global());
	EXPECT_CALL(m_tabPreRemovalCallback, Call(Ref(*m_tab2), 0));

	m_tabEvents.AddRemovedObserver(m_tabRemovedCallback.AsStdFunction(), TabEventScope::Global());
	EXPECT_CALL(m_tabRemovedCallback, Call(Ref(*m_tab2)));
	EXPECT_CALL(m_tabRemovedCallback, Call(Ref(*m_tab1)));

	m_tabEvents.NotifyCreated(*m_tab1);
	m_tabEvents.NotifyCreated(*m_tab2);
	m_tabEvents.NotifyUpdated(*m_tab1, Tab::PropertyType::Name);
	m_tabEvents.NotifyUpdated(*m_tab2, Tab::PropertyType::LockState);
	m_tabEvents.NotifySelected(*m_tab1);
	m_tabEvents.NotifyMoved(*m_tab2, 1, 2);
	m_tabEvents.NotifyPreRemoval(*m_tab2, 0);
	m_tabEvents.NotifyRemoved(*m_tab2);
	m_tabEvents.NotifyRemoved(*m_tab1);
}

TEST_F(TabEventsTest, SignalsFilteredByBrowser)
{
	// The observer here should only be triggered when a tab event in m_browser1 occurs. That is,
	// only when m_tab1 is created.
	m_tabEvents.AddCreatedObserver(m_tabCreatedCallback.AsStdFunction(),
		TabEventScope::ForBrowser(*m_browser1));
	EXPECT_CALL(m_tabCreatedCallback, Call(Ref(*m_tab1)));

	m_tabEvents.NotifyCreated(*m_tab1);
	m_tabEvents.NotifyCreated(*m_tab2);
}
