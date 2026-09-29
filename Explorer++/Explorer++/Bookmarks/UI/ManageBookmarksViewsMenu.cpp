// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/ManageBookmarksViewsMenu.h"
#include "Bookmarks/UI/BookmarkColumnModel.h"
#include "Bookmarks/UI/BookmarkListPresenter.h"
#include "ListViewColumnsMenu.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceLoader.h"

ManageBookmarksViewsMenu::ManageBookmarksViewsMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, BookmarkListPresenter *bookmarkListPresenter,
	const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_bookmarkListPresenter(bookmarkListPresenter)
{
	BuildMenu(resourceLoader);
}

ManageBookmarksViewsMenu::~ManageBookmarksViewsMenu() = default;

void ManageBookmarksViewsMenu::BuildMenu(const ResourceLoader *resourceLoader)
{
	auto *columnsSubMenuView = m_rootMenuView->AppendSubMenu(SubMenuItemParams(
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SHOW_COLUMNS_POPUP))
			.SetKey(Item::ColumnsPopup));
	m_columnsMenu = std::make_unique<ListViewColumnsMenu>(columnsSubMenuView, m_acceleratorManager,
		m_bookmarkListPresenter->GetColumnModel(), resourceLoader);

	auto *sortSubMenuView = m_rootMenuView->AppendSubMenu(
		SubMenuItemParams(resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_POPUP))
			.SetKey(Item::SortByPopup));
	BuildSortMenu(sortSubMenuView, resourceLoader);
}

void ManageBookmarksViewsMenu::BuildSortMenu(MenuView *sortMenuView,
	const ResourceLoader *resourceLoader)
{
	sortMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_DEFAULT))
			.SetKey(Item::SortByDefault)
			.SetCheckStyle(MenuItemCheckStyle::Radio)
			.SetCheckedCallback([this] { return !HasSortColumn(); })
			.SetSelectedCallback([this] { ResetSortColumn(); }));

	AppendSortItem(sortMenuView, Item::SortByName, IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_NAME,
		BookmarkColumn::Name, resourceLoader);
	AppendSortItem(sortMenuView, Item::SortByLocation,
		IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LOCATION, BookmarkColumn::Location, resourceLoader);
	AppendSortItem(sortMenuView, Item::SortByDateCreated,
		IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_ADDED, BookmarkColumn::DateCreated, resourceLoader);
	AppendSortItem(sortMenuView, Item::SortByDateModified,
		IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LAST_MODIFIED, BookmarkColumn::DateModified,
		resourceLoader);

	sortMenuView->AppendSeparator();

	AppendSortDirectionItem(sortMenuView, Item::SortAscending,
		IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_ASCENDING, SortDirection::Ascending, resourceLoader);
	AppendSortDirectionItem(sortMenuView, Item::SortDescending,
		IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_DESCENDING, SortDirection::Descending, resourceLoader);
}

void ManageBookmarksViewsMenu::AppendSortItem(MenuView *sortMenuView, Item itemKey, UINT stringId,
	BookmarkColumn column, const ResourceLoader *resourceLoader)
{
	sortMenuView->AppendItem(MenuItemParams(resourceLoader->LoadString(stringId))
			.SetKey(itemKey)
			.SetCheckStyle(MenuItemCheckStyle::Radio)
			.SetCheckedCallback(
				[this, column] { return m_bookmarkListPresenter->GetSortColumn() == column; })
			.SetSelectedCallback([this, column] { UpdateSortColumn(column); }));
}

void ManageBookmarksViewsMenu::AppendSortDirectionItem(MenuView *sortMenuView, Item itemKey,
	UINT stringId, SortDirection sortDirection, const ResourceLoader *resourceLoader)
{
	sortMenuView->AppendItem(MenuItemParams(resourceLoader->LoadString(stringId))
			.SetKey(itemKey)
			.SetCheckStyle(MenuItemCheckStyle::Radio)
			.SetEnabledCallback([this] { return HasSortColumn(); })
			.SetCheckedCallback(
				[this, sortDirection]
				{
					return HasSortColumn()
						&& m_bookmarkListPresenter->GetSortDirection() == sortDirection;
				})
			.SetSelectedCallback([this, sortDirection] { UpdateSortDirection(sortDirection); }));
}

bool ManageBookmarksViewsMenu::HasSortColumn() const
{
	return m_bookmarkListPresenter->GetSortColumn() != std::nullopt;
}

void ManageBookmarksViewsMenu::ResetSortColumn()
{
	m_bookmarkListPresenter->SetSortDetails(std::nullopt, SortDirection::Ascending);
}

void ManageBookmarksViewsMenu::UpdateSortColumn(BookmarkColumn sortColumn)
{
	if (m_bookmarkListPresenter->GetSortColumn() == sortColumn)
	{
		return;
	}

	m_bookmarkListPresenter->SetSortDetails(sortColumn, SortDirection::Ascending);
}

void ManageBookmarksViewsMenu::UpdateSortDirection(SortDirection sortDirection)
{
	m_bookmarkListPresenter->SetSortDetails(m_bookmarkListPresenter->GetSortColumn(),
		sortDirection);
}
