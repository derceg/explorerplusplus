// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "SearchTabsListViewModel.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "LabelEditHandler.h"
#include "ListView.h"
#include "NoOpMenuHelpTextHost.h"
#include "SearchTabsModel.h"
#include <gtest/gtest.h>
#include <wil/resource.h>
#include <memory>

using namespace testing;

class SearchTabsListViewModelTest : public BrowserTestBase
{
protected:
	SearchTabsListViewModelTest() :
		m_model(&m_tabList, &m_tabEvents, &m_shellBrowserEvents, &m_navigationEvents)
	{
	}

	void SetUp() override
	{
		m_parentWindow.reset(CreateWindow(WC_STATIC, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_parentWindow, nullptr);

		HWND listViewWindow = CreateWindow(WC_LISTVIEW, L"",
			WS_POPUP | LVS_REPORT | LVS_EDITLABELS | LVS_SHAREIMAGELISTS, 0, 0, 0, 0,
			m_parentWindow.get(), nullptr, GetModuleHandle(nullptr), nullptr);
		ASSERT_NE(listViewWindow, nullptr);

		m_listView = std::make_unique<ListView>(listViewWindow,
			m_platformContext.GetKeyboardState(), LabelEditHandler::CreateForTest,
			NoOpMenuHelpTextHost::GetInstance(), &m_acceleratorManager, &m_resourceLoader);

		m_listViewModel =
			std::make_unique<SearchTabsListViewModel>(&m_model, &m_tabList, &m_iconFetcher);
		m_listView->SetModel(m_listViewModel.get());
	}

	std::vector<const Tab *> GetTabsForItems() const
	{
		std::vector<const Tab *> tabs;

		for (const auto *item : m_listViewModel->GetItems())
		{
			tabs.push_back(m_listViewModel->GetTabForItem(item));
		}

		return tabs;
	}

	SearchTabsModel m_model;
	std::unique_ptr<SearchTabsListViewModel> m_listViewModel;

	wil::unique_hwnd m_parentWindow;
	std::unique_ptr<ListView> m_listView;
};

TEST_F(SearchTabsListViewModelTest, ItemsAfterUpdate)
{
	auto *browser = AddBrowser();
	auto *tab1 = browser->AddTab(L"c:\\folder");
	browser->AddTab(L"d:\\");
	auto *tab3 = browser->AddTab(L"e:\\folder");

	m_model.SetSearchTerm(L"folder");
	EXPECT_THAT(GetTabsForItems(), UnorderedElementsAre(tab1, tab3));
}

TEST_F(SearchTabsListViewModelTest, DefaultSortOrder)
{
	auto *browser = AddBrowser();
	auto *tab1 = browser->AddTab(L"c:\\");
	auto *tab2 = browser->AddTab(L"d:\\");
	auto *tab3 = browser->AddTab(L"e:\\");

	auto *tabContainer = browser->GetActiveTabContainer();
	tabContainer->SelectTab(*tab1);
	tabContainer->SelectTab(*tab3);
	tabContainer->SelectTab(*tab2);

	EXPECT_THAT(GetTabsForItems(), ElementsAre(tab2, tab3, tab1));
}

TEST_F(SearchTabsListViewModelTest, SortByName)
{
	auto *browser = AddBrowser();
	auto *tab1 = browser->AddTab(L"c:\\");
	auto *tab2 = browser->AddTab(L"d:\\");
	auto *tab3 = browser->AddTab(L"e:\\");

	tab1->SetCustomName(L"B");
	tab2->SetCustomName(L"C");
	tab3->SetCustomName(L"A");

	m_listViewModel->SetSortDetails(
		SearchTabsColumnModel::SearchTabsColumnToColumnId(SearchTabsColumn::TabName),
		SortDirection::Ascending);
	EXPECT_THAT(GetTabsForItems(), ElementsAre(tab3, tab1, tab2));
}

TEST_F(SearchTabsListViewModelTest, SortByPath)
{
	auto *browser = AddBrowser();
	auto *tab1 = browser->AddTab(L"z:\\");
	auto *tab2 = browser->AddTab(L"a:\\");
	auto *tab3 = browser->AddTab(L"g:\\");

	m_listViewModel->SetSortDetails(
		SearchTabsColumnModel::SearchTabsColumnToColumnId(SearchTabsColumn::Path),
		SortDirection::Descending);
	EXPECT_THAT(GetTabsForItems(), ElementsAre(tab1, tab3, tab2));
}
