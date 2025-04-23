// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SearchTabsModel.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "GeneratorTestHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellBrowserEvents.h"
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

	void NavigateTab(Tab *tab, const std::wstring &path)
	{
		auto pidl = CreateSimplePidlForTest(path);
		auto navigateParams = NavigateParams::Normal(pidl.Raw());
		tab->GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);
	}

	ShellBrowserEvents m_shellBrowserEvents;
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
	auto *tab1 = browser1->AddTab();
	browser1->ActivateTabAtIndex(0);
	NavigateTab(tab1, L"c:\\windows\\system32");
	tab1->SetCustomName(L"Custom name");

	auto *browser2 = AddBrowser();
	auto *tab2 = browser2->AddTab();
	browser2->ActivateTabAtIndex(0);
	NavigateTab(tab2, L"j:\\documents");

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

TEST_F(SearchTabsModelTest, TabUpdates)
{
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
		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(4));
		EXPECT_CALL(callback, Call());
		EXPECT_CALL(check, Call(5));

		// The callback should be invoked twice here - once for the removal of each tab.
		EXPECT_CALL(callback, Call());
		EXPECT_CALL(callback, Call());

		EXPECT_CALL(check, Call(6));
	}

	auto *browser = AddBrowser();

	auto *tab1 = browser->AddTab();
	check.Call(1);

	browser->AddTab();
	check.Call(2);

	browser->ActivateTabAtIndex(1);
	check.Call(3);

	tab1->SetCustomName(L"Updated name");
	check.Call(4);

	tab1->ClearCustomName();
	check.Call(5);

	RemoveBrowser(browser);
	check.Call(6);
}

TEST_F(SearchTabsModelTest, ShellBrowserUpdates)
{
	auto *browser = AddBrowser();
	auto *tab = browser->AddTab();

	MockFunction<void()> callback;
	m_model.updatedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call());
	m_shellBrowserEvents.NotifyDirectoryPropertiesChanged(tab->GetShellBrowser());
}

TEST_F(SearchTabsModelTest, NavigationUpdates)
{
	auto *browser = AddBrowser();
	auto *tab = browser->AddTab();

	MockFunction<void()> callback;
	m_model.updatedSignal.AddObserver(callback.AsStdFunction());

	EXPECT_CALL(callback, Call());
	NavigateTab(tab, L"c:\\users");
}

TEST_F(SearchTabsModelTest, SearchTermUpdates)
{
	auto *browser1 = AddBrowser();
	auto *tab1 = browser1->AddTab();
	NavigateTab(tab1, L"c:\\windows");

	auto *browser2 = AddBrowser();
	auto *tab2 = browser2->AddTab();
	NavigateTab(tab2, L"f:\\path\\to\\project");

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
