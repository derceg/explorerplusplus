// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SearchTabsModel.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "GeneratorTestHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellTestHelper.h"
#include "TabList.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class SearchTabsModelTest : public BrowserTestBase
{
protected:
	SearchTabsModelTest() :
		m_tabList(&m_tabEvents),
		m_model(&m_tabList, &m_tabEvents, &m_shellBrowserEvents, &m_navigationEvents)
	{
	}

	TabList m_tabList;
	SearchTabsModel m_model;
};

TEST_F(SearchTabsModelTest, SearchTerm)
{
	EXPECT_EQ(m_model.GetSearchTerm(), L"");

	std::wstring searchTerm = L"documents";
	m_model.SetSearchTerm(searchTerm);
	EXPECT_EQ(m_model.GetSearchTerm(), searchTerm);
}

TEST_F(SearchTabsModelTest, GetResults)
{
	EXPECT_THAT(GeneratorToVector(m_model.GetResults()), IsEmpty());

	auto *browser1 = AddBrowser();
	auto *tab1 = browser1->AddTab(L"c:\\windows\\system32");
	tab1->SetCustomName(L"Custom name");

	auto *browser2 = AddBrowser();
	auto *tab2 = browser2->AddTab(L"j:\\documents");

	// There is no search term, so all tabs should be returned, ordered by last active time.
	EXPECT_THAT(GeneratorToVector(m_model.GetResults()), ElementsAre(tab2, tab1));

	m_model.SetSearchTerm(L"c:\\windows");
	EXPECT_THAT(GeneratorToVector(m_model.GetResults()), ElementsAre(tab1));

	m_model.SetSearchTerm(L"docu");
	EXPECT_THAT(GeneratorToVector(m_model.GetResults()), ElementsAre(tab2));

	// The tab name can also be searched.
	m_model.SetSearchTerm(L"custom");
	EXPECT_THAT(GeneratorToVector(m_model.GetResults()), ElementsAre(tab1));

	m_model.SetSearchTerm(L"users");
	EXPECT_THAT(GeneratorToVector(m_model.GetResults()), IsEmpty());
}

TEST_F(SearchTabsModelTest, TabCreationTriggersUpdatedSignal)
{
	auto *browser = AddBrowser();

	MockFunction<void()> callback;
	m_model.updatedSignal.AddObserver(callback.AsStdFunction());

	// This can trigger multiple updates, due to the tab selection being set and a navigation
	// occurring. The callback should be triggered at least once.
	EXPECT_CALL(callback, Call()).Times(AtLeast(1));
	browser->AddTab(L"c:\\");
}

TEST_F(SearchTabsModelTest, TabSelectionTriggersUpdatedSignal)
{
	auto *browser = AddBrowser();
	browser->AddTab(L"c:\\");
	browser->AddTab(L"c:\\");

	MockFunction<void()> callback;
	m_model.updatedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call());
	browser->GetActiveTabContainer()->SelectTabAtIndex(1);
}

TEST_F(SearchTabsModelTest, TabUpdateTriggersUpdatedSignal)
{
	auto *browser = AddBrowser();
	auto *tab = browser->AddTab(L"c:\\");

	MockFunction<void()> callback;
	m_model.updatedSignal.AddObserver(callback.AsStdFunction());

	MockFunction<void(int)> check;
	{
		InSequence seq;

		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(1));
		EXPECT_CALL(callback, Call());
	}

	tab->SetCustomName(L"Updated name");
	check.Call(1);
	tab->ClearCustomName();
}

TEST_F(SearchTabsModelTest, TabMoveTriggersUpdatedSignal)
{
	auto *browser = AddBrowser();
	auto *tab1 = browser->AddTab(L"c:\\");
	browser->AddTab(L"c:\\");

	MockFunction<void()> callback;
	m_model.updatedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call());
	browser->GetActiveTabContainer()->MoveTab(*tab1, 1);
}

TEST_F(SearchTabsModelTest, TabRemovalTriggersUpdatedSignal)
{
	auto *browser = AddBrowser();
	browser->AddTab(L"c:\\");
	int tabId2 = browser->AddTabAndReturnId(L"c:\\");

	MockFunction<void()> callback;
	m_model.updatedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call());
	browser->GetActiveTabContainer()->CloseTab(browser->GetActiveTabContainer()->GetTab(tabId2));
}

TEST_F(SearchTabsModelTest, DirectoryPropertiesChangedTriggersUpdatedSignal)
{
	auto *browser = AddBrowser();
	auto *tab = browser->AddTab(L"c:\\");

	MockFunction<void()> callback;
	m_model.updatedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call());
	m_shellBrowserEvents.NotifyDirectoryPropertiesChanged(tab->GetShellBrowser());
}

TEST_F(SearchTabsModelTest, NavigationTriggersUpdatedSignal)
{
	auto *browser = AddBrowser();
	auto *tab = browser->AddTab(L"c:\\");

	MockFunction<void()> callback;
	m_model.updatedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call());
	NavigateTab(tab, L"c:\\users");
}

TEST_F(SearchTabsModelTest, SearchTermUpdateTriggersUpdatedSignal)
{
	auto *browser1 = AddBrowser();
	browser1->AddTab(L"c:\\windows");

	auto *browser2 = AddBrowser();
	browser2->AddTab(L"f:\\path\\to\\project");

	MockFunction<void()> callback;
	m_model.updatedSignal.AddObserver(callback.AsStdFunction());

	MockFunction<void(int)> check;
	{
		InSequence seq;

		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(1));
		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(2));
		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(3));
		EXPECT_CALL(check, Call(4));
	}

	m_model.SetSearchTerm(L"c:\\");
	check.Call(1);
	m_model.SetSearchTerm(L"project");
	check.Call(2);
	m_model.SetSearchTerm(L"h:\\");
	check.Call(3);

	// The search term doesn't change here, so no update should be triggered.
	m_model.SetSearchTerm(L"h:\\");
	check.Call(4);
}
