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
		m_browser1(AddBrowser()),
		m_tab1(m_browser1->AddTab(L"c:\\")),
		m_tab2(m_browser1->AddTab(L"c:\\")),
		m_browser2(AddBrowser()),
		m_tab3(m_browser2->AddTab(L"c:\\"))
	{
	}

	// Takes a set of tabs, ordered by descending activation point (i.e. more recently activated
	// first).
	void VerifyActivationPointOrdering(const std::vector<const Tab *> &orderedTabs)
	{
		for (size_t i = 1; i < orderedTabs.size(); i++)
		{
			EXPECT_GT(m_tabList.GetTabLastActivationPoint(orderedTabs[i - 1]),
				m_tabList.GetTabLastActivationPoint(orderedTabs[i]));
		}
	}

	BrowserWindowFake *const m_browser1;
	Tab *const m_tab1;
	Tab *const m_tab2;

	BrowserWindowFake *const m_browser2;
	Tab *const m_tab3;
};

TEST_F(TabListTest, GetAll)
{
	EXPECT_THAT(GeneratorToVector(m_tabList.GetAll()),
		UnorderedElementsAre(m_tab1, m_tab2, m_tab3));
}

TEST_F(TabListTest, Add)
{
	auto *tab4 = m_browser2->AddTab(L"c:\\");

	EXPECT_THAT(GeneratorToVector(m_tabList.GetAll()),
		UnorderedElementsAre(m_tab1, m_tab2, m_tab3, tab4));

	auto *browser3 = AddBrowser();
	auto *tab5 = browser3->AddTab(L"c:\\");

	EXPECT_THAT(GeneratorToVector(m_tabList.GetAll()),
		UnorderedElementsAre(m_tab1, m_tab2, m_tab3, tab4, tab5));
}

TEST_F(TabListTest, Remove)
{
	m_browser1->GetActiveTabContainer()->CloseTab(*m_tab1);
	EXPECT_THAT(GeneratorToVector(m_tabList.GetAll()), UnorderedElementsAre(m_tab2, m_tab3));

	RemoveBrowser(m_browser2);
	EXPECT_THAT(GeneratorToVector(m_tabList.GetAll()), UnorderedElementsAre(m_tab2));
}

TEST_F(TabListTest, GetById)
{
	EXPECT_EQ(m_tabList.GetById(m_tab1->GetId()), m_tab1);
	EXPECT_EQ(m_tabList.GetById(m_tab2->GetId()), m_tab2);
	EXPECT_EQ(m_tabList.GetById(m_tab3->GetId()), m_tab3);
}

TEST_F(TabListTest, MaybeGetById)
{
	EXPECT_EQ(m_tabList.MaybeGetById(m_tab1->GetId()), m_tab1);
	EXPECT_EQ(m_tabList.MaybeGetById(m_tab2->GetId()), m_tab2);
	EXPECT_EQ(m_tabList.MaybeGetById(m_tab3->GetId()), m_tab3);
	EXPECT_EQ(m_tabList.MaybeGetById(1000), nullptr);
}

TEST_F(TabListTest, GetTabLastActivationPoint)
{
	VerifyActivationPointOrdering({ m_tab3, m_tab1, m_tab2 });

	m_browser1->GetActiveTabContainer()->SelectTab(*m_tab2);
	VerifyActivationPointOrdering({ m_tab2, m_tab3, m_tab1 });

	m_browser1->GetActiveTabContainer()->SelectTab(*m_tab1);
	VerifyActivationPointOrdering({ m_tab1, m_tab2, m_tab3 });
}

TEST_F(TabListTest, GetAllByLastActivation)
{
	EXPECT_THAT(GeneratorToVector(m_tabList.GetAllByLastActivation()),
		ElementsAre(m_tab3, m_tab1, m_tab2));

	m_browser1->GetActiveTabContainer()->SelectTab(*m_tab2);
	EXPECT_THAT(GeneratorToVector(m_tabList.GetAllByLastActivation()),
		ElementsAre(m_tab2, m_tab3, m_tab1));

	m_browser1->GetActiveTabContainer()->SelectTab(*m_tab1);
	EXPECT_THAT(GeneratorToVector(m_tabList.GetAllByLastActivation()),
		ElementsAre(m_tab1, m_tab2, m_tab3));
}

TEST_F(TabListTest, GetForBrowser)
{
	EXPECT_THAT(GeneratorToVector(m_tabList.GetForBrowser(m_browser1)),
		UnorderedElementsAre(m_tab1, m_tab2));
	EXPECT_THAT(GeneratorToVector(m_tabList.GetForBrowser(m_browser2)),
		UnorderedElementsAre(m_tab3));
}
