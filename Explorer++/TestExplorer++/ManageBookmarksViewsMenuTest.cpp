// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "Bookmarks/UI/ManageBookmarksViewsMenu.h"
#include "Bookmarks/UI/BookmarkColumnModel.h"
#include "Bookmarks/UI/BookmarkListPresenter.h"
#include "BrowserTestBase.h"
#include "IconFetcherFake.h"
#include "LabelEditHandler.h"
#include "ListView.h"
#include "MenuTestHost.h"
#include "MenuViewTestHelper.h"
#include "NoOpMenuHelpTextHost.h"
#include <gtest/gtest.h>
#include <wil/resource.h>

class ManageBookmarksViewsMenuTest : public BrowserTestBase
{
protected:
	void SetUp() override
	{
		m_parentWindow.reset(CreateWindow(WC_STATIC, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
			GetModuleHandle(nullptr), nullptr));
		ASSERT_NE(m_parentWindow, nullptr);

		HWND listViewWindow = CreateWindow(WC_LISTVIEW, L"",
			WS_POPUP | LVS_REPORT | LVS_EDITLABELS | LVS_SHAREIMAGELISTS, 0, 0, 0, 0,
			m_parentWindow.get(), nullptr, GetModuleHandle(nullptr), nullptr);
		ASSERT_NE(listViewWindow, nullptr);

		auto listView = std::make_unique<ListView>(listViewWindow,
			m_platformContext.GetKeyboardState(), LabelEditHandler::CreateForTest,
			NoOpMenuHelpTextHost::GetInstance(), &m_acceleratorManager, &m_resourceLoader);

		m_bookmarkListPresenter = std::make_unique<BookmarkListPresenter>(std::move(listView),
			&m_bookmarkTree, BookmarkColumnModel(), std::nullopt, SortDirection::Ascending,
			&m_browserList, &m_config, &m_acceleratorManager, &m_resourceLoader, &m_iconFetcher,
			&m_platformContext);

		m_menu = std::make_unique<ManageBookmarksViewsMenu>(m_menuHost.GetView(),
			&m_acceleratorManager, m_bookmarkListPresenter.get(), &m_resourceLoader);
	}

	IconFetcherFake m_iconFetcher;

	wil::unique_hwnd m_parentWindow;
	std::unique_ptr<BookmarkListPresenter> m_bookmarkListPresenter;

	MenuTestHost m_menuHost;
	std::unique_ptr<ManageBookmarksViewsMenu> m_menu;
};

TEST_F(ManageBookmarksViewsMenuTest, ItemStatesWithDefaultSort)
{
	m_bookmarkListPresenter->SetSortDetails(std::nullopt, SortDirection::Ascending);

	auto *sortByMenuView =
		m_menuHost.GetView()->GetSubMenuViewForTesting(ManageBookmarksViewsMenu::Item::SortByPopup);
	MenuViewTestHelper::ExpectItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByDefault, true);
	MenuViewTestHelper::ExpectItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByName, false);
	MenuViewTestHelper::ExpectItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByLocation, false);
	MenuViewTestHelper::ExpectItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByDateCreated, false);
	MenuViewTestHelper::ExpectItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByDateModified, false);

	MenuViewTestHelper::ExpectItemEnabled(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortAscending, false);
	MenuViewTestHelper::ExpectItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortAscending, false);
	MenuViewTestHelper::ExpectItemEnabled(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortDescending, false);
	MenuViewTestHelper::ExpectItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortDescending, false);
}

TEST_F(ManageBookmarksViewsMenuTest, ItemStatesWithSortColumn)
{
	m_bookmarkListPresenter->SetSortDetails(BookmarkColumn::DateCreated, SortDirection::Descending);

	auto *sortByMenuView =
		m_menuHost.GetView()->GetSubMenuViewForTesting(ManageBookmarksViewsMenu::Item::SortByPopup);
	MenuViewTestHelper::ExpectItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByDefault, false);
	MenuViewTestHelper::ExpectItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByName, false);
	MenuViewTestHelper::ExpectItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByLocation, false);
	MenuViewTestHelper::ExpectItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByDateCreated, true);
	MenuViewTestHelper::ExpectItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByDateModified, false);

	MenuViewTestHelper::ExpectItemEnabled(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortAscending, true);
	MenuViewTestHelper::ExpectItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortAscending, false);
	MenuViewTestHelper::ExpectItemEnabled(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortDescending, true);
	MenuViewTestHelper::ExpectItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortDescending, true);
}

TEST_F(ManageBookmarksViewsMenuTest, SelectDefaultSort)
{
	m_bookmarkListPresenter->SetSortDetails(BookmarkColumn::DateCreated, SortDirection::Descending);

	m_menuHost.SelectItem(ManageBookmarksViewsMenu::Item::SortByDefault, false, false);
	EXPECT_EQ(m_bookmarkListPresenter->GetSortColumn(), std::nullopt);
	EXPECT_EQ(m_bookmarkListPresenter->GetSortDirection(), +SortDirection::Ascending);
}

TEST_F(ManageBookmarksViewsMenuTest, SelectSortColumn)
{
	m_bookmarkListPresenter->SetSortDetails(BookmarkColumn::Name, SortDirection::Descending);

	m_menuHost.SelectItem(ManageBookmarksViewsMenu::Item::SortByLocation, false, false);
	EXPECT_EQ(m_bookmarkListPresenter->GetSortColumn(), +BookmarkColumn::Location);
	EXPECT_EQ(m_bookmarkListPresenter->GetSortDirection(), +SortDirection::Ascending);
}

TEST_F(ManageBookmarksViewsMenuTest, SelectSortDirection)
{
	m_bookmarkListPresenter->SetSortDetails(BookmarkColumn::DateModified, SortDirection::Ascending);

	m_menuHost.SelectItem(ManageBookmarksViewsMenu::Item::SortDescending, false, false);
	EXPECT_EQ(m_bookmarkListPresenter->GetSortColumn(), +BookmarkColumn::DateModified);
	EXPECT_EQ(m_bookmarkListPresenter->GetSortDirection(), +SortDirection::Descending);
}
