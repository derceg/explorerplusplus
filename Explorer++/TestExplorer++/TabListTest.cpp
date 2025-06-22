// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabList.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "GeneratorTestHelper.h"
#include <gtest/gtest.h>

using namespace testing;

class TabListTest : public BrowserTestBase
{
protected:
	TabListTest() :
		m_tabList(&m_tabEvents),
		m_browser1(AddBrowser()),
		m_tab1(m_browser1->AddTab(L"c:\\")),
		m_tab2(m_browser1->AddTab(L"c:\\")),
		m_browser2(AddBrowser()),
		m_tab3(m_browser2->AddTab(L"c:\\"))
	{
	}

	TabList m_tabList;

	BrowserWindowFake *const m_browser1;
	Tab *const m_tab1;
	Tab *const m_tab2;

	BrowserWindowFake *const m_browser2;
	Tab *const m_tab3;
};

TEST_F(TabListTest, Add)
{
	auto *browser3 = AddBrowser();
	auto *tab4 = browser3->AddTab(L"c:\\");

	EXPECT_THAT(GeneratorToVector(m_tabList.GetAll()),
		UnorderedElementsAre(m_tab1, m_tab2, m_tab3, tab4));

	// TODO: Removing a browser should also be tested, but at the moment, no tab removal events are
	// broadcast when a window is closed.
}

TEST_F(TabListTest, GetById)
{
	EXPECT_EQ(m_tabList.GetById(m_tab1->GetId()), m_tab1);
	EXPECT_EQ(m_tabList.GetById(m_tab2->GetId()), m_tab2);
	EXPECT_EQ(m_tabList.GetById(m_tab3->GetId()), m_tab3);
}

TEST_F(TabListTest, GetAllByLastActiveTime)
{
	EXPECT_THAT(GeneratorToVector(m_tabList.GetAllByLastActiveTime()),
		ElementsAre(m_tab3, m_tab1, m_tab2));

	m_browser1->GetActiveTabContainer()->SelectTabAtIndex(1);
	EXPECT_THAT(GeneratorToVector(m_tabList.GetAllByLastActiveTime()),
		ElementsAre(m_tab2, m_tab3, m_tab1));

	m_browser1->GetActiveTabContainer()->SelectTabAtIndex(0);
	EXPECT_THAT(GeneratorToVector(m_tabList.GetAllByLastActiveTime()),
		ElementsAre(m_tab1, m_tab2, m_tab3));
}

TEST_F(TabListTest, GetForBrowser)
{
	EXPECT_THAT(GeneratorToVector(m_tabList.GetForBrowser(m_browser1)),
		UnorderedElementsAre(m_tab1, m_tab2));
	EXPECT_THAT(GeneratorToVector(m_tabList.GetForBrowser(m_browser2)),
		UnorderedElementsAre(m_tab3));
}
