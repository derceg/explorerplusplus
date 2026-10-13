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
#include "MenuViewTestApi.h"
#include "MenuViewTestHelper.h"
#include "NoOpMenuHelpTextHost.h"
#include "RootMenuViewTestApi.h"
#include "TestMenuView.h"
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

		m_menu = std::make_unique<ManageBookmarksViewsMenu>(&m_menuView, &m_acceleratorManager,
			m_bookmarkListPresenter.get(), &m_resourceLoader);
	}

	IconFetcherFake m_iconFetcher;

	wil::unique_hwnd m_parentWindow;
	std::unique_ptr<BookmarkListPresenter> m_bookmarkListPresenter;

	TestMenuView m_menuView;
	std::unique_ptr<ManageBookmarksViewsMenu> m_menu;
};

TEST_F(ManageBookmarksViewsMenuTest, ItemStatesWithDefaultSort)
{
	m_bookmarkListPresenter->SetSortDetails(std::nullopt, SortDirection::Ascending);

	auto *sortByMenuView =
		MenuViewTestApi::GetSubMenuView(&m_menuView, ManageBookmarksViewsMenu::Item::SortByPopup);
	EXPECT_TRUE(MenuViewTestApi::IsItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByDefault));
	EXPECT_FALSE(
		MenuViewTestApi::IsItemChecked(sortByMenuView, ManageBookmarksViewsMenu::Item::SortByName));
	EXPECT_FALSE(MenuViewTestApi::IsItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByLocation));
	EXPECT_FALSE(MenuViewTestApi::IsItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByDateCreated));
	EXPECT_FALSE(MenuViewTestApi::IsItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByDateModified));

	EXPECT_FALSE(MenuViewTestApi::IsItemEnabled(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortAscending));
	EXPECT_FALSE(MenuViewTestApi::IsItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortAscending));
	EXPECT_FALSE(MenuViewTestApi::IsItemEnabled(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortDescending));
	EXPECT_FALSE(MenuViewTestApi::IsItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortDescending));
}

TEST_F(ManageBookmarksViewsMenuTest, ItemStatesWithSortColumn)
{
	m_bookmarkListPresenter->SetSortDetails(BookmarkColumn::DateCreated, SortDirection::Descending);

	auto *sortByMenuView =
		MenuViewTestApi::GetSubMenuView(&m_menuView, ManageBookmarksViewsMenu::Item::SortByPopup);
	EXPECT_FALSE(MenuViewTestApi::IsItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByDefault));
	EXPECT_FALSE(
		MenuViewTestApi::IsItemChecked(sortByMenuView, ManageBookmarksViewsMenu::Item::SortByName));
	EXPECT_FALSE(MenuViewTestApi::IsItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByLocation));
	EXPECT_TRUE(MenuViewTestApi::IsItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByDateCreated));
	EXPECT_FALSE(MenuViewTestApi::IsItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortByDateModified));

	EXPECT_TRUE(MenuViewTestApi::IsItemEnabled(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortAscending));
	EXPECT_FALSE(MenuViewTestApi::IsItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortAscending));
	EXPECT_TRUE(MenuViewTestApi::IsItemEnabled(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortDescending));
	EXPECT_TRUE(MenuViewTestApi::IsItemChecked(sortByMenuView,
		ManageBookmarksViewsMenu::Item::SortDescending));
}

TEST_F(ManageBookmarksViewsMenuTest, SelectDefaultSort)
{
	m_bookmarkListPresenter->SetSortDetails(BookmarkColumn::DateCreated, SortDirection::Descending);

	RootMenuViewTestApi::SelectItem(&m_menuView, ManageBookmarksViewsMenu::Item::SortByDefault,
		false, false);
	EXPECT_EQ(m_bookmarkListPresenter->GetSortColumn(), std::nullopt);
	EXPECT_EQ(m_bookmarkListPresenter->GetSortDirection(), SortDirection::Ascending);
}

TEST_F(ManageBookmarksViewsMenuTest, SelectSortColumn)
{
	m_bookmarkListPresenter->SetSortDetails(BookmarkColumn::Name, SortDirection::Descending);

	RootMenuViewTestApi::SelectItem(&m_menuView, ManageBookmarksViewsMenu::Item::SortByLocation,
		false, false);
	EXPECT_EQ(m_bookmarkListPresenter->GetSortColumn(), BookmarkColumn::Location);
	EXPECT_EQ(m_bookmarkListPresenter->GetSortDirection(), SortDirection::Ascending);
}

TEST_F(ManageBookmarksViewsMenuTest, SelectSortDirection)
{
	m_bookmarkListPresenter->SetSortDetails(BookmarkColumn::DateModified, SortDirection::Ascending);

	RootMenuViewTestApi::SelectItem(&m_menuView, ManageBookmarksViewsMenu::Item::SortDescending,
		false, false);
	EXPECT_EQ(m_bookmarkListPresenter->GetSortColumn(), BookmarkColumn::DateModified);
	EXPECT_EQ(m_bookmarkListPresenter->GetSortDirection(), SortDirection::Descending);
}
