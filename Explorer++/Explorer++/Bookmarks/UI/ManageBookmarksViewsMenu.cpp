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
	auto *columnsSubMenuView =
		m_rootMenuView->AppendSubMenu(this, IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SHOW_COLUMNS_POPUP,
			resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SHOW_COLUMNS_POPUP));
	m_columnsMenu = std::make_unique<ListViewColumnsMenu>(columnsSubMenuView, m_acceleratorManager,
		m_bookmarkListPresenter->GetColumnModel(), resourceLoader);

	auto *sortSubMenuView =
		m_rootMenuView->AppendSubMenu(this, IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_POPUP,
			resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_POPUP));
	BuildSortMenu(sortSubMenuView, resourceLoader);
}

void ManageBookmarksViewsMenu::BuildSortMenu(MenuView *sortMenuView,
	const ResourceLoader *resourceLoader)
{
	sortMenuView->AppendRadioItem(this, IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_DEFAULT,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_DEFAULT));
	sortMenuView->AppendRadioItem(this, IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_NAME,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_NAME));
	sortMenuView->AppendRadioItem(this, IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LOCATION,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LOCATION));
	sortMenuView->AppendRadioItem(this, IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_ADDED,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_ADDED));
	sortMenuView->AppendRadioItem(this, IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LAST_MODIFIED,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LAST_MODIFIED));

	sortMenuView->AppendSeparator();

	sortMenuView->AppendRadioItem(this, IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_ASCENDING,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_ASCENDING));
	sortMenuView->AppendRadioItem(this, IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_DESCENDING,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_DESCENDING));
}

bool ManageBookmarksViewsMenu::IsItemEnabled(UINT id) const
{
	switch (id)
	{
	// If there is no sort column set, then the default sort order applies and the
	// ascending/descending options aren't relevant.
	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_ASCENDING:
	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_DESCENDING:
		return m_bookmarkListPresenter->GetSortColumn() != std::nullopt;
	}

	return true;
}

bool ManageBookmarksViewsMenu::IsItemChecked(UINT id) const
{
	switch (id)
	{
	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_DEFAULT:
		return m_bookmarkListPresenter->GetSortColumn() == std::nullopt;

	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_NAME:
		return m_bookmarkListPresenter->GetSortColumn() == +BookmarkColumn::Name;

	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LOCATION:
		return m_bookmarkListPresenter->GetSortColumn() == +BookmarkColumn::Location;

	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_ADDED:
		return m_bookmarkListPresenter->GetSortColumn() == +BookmarkColumn::DateCreated;

	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LAST_MODIFIED:
		return m_bookmarkListPresenter->GetSortColumn() == +BookmarkColumn::DateModified;

	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_ASCENDING:
		return m_bookmarkListPresenter->GetSortColumn() != std::nullopt
			&& m_bookmarkListPresenter->GetSortDirection() == +SortDirection::Ascending;

	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_DESCENDING:
		return m_bookmarkListPresenter->GetSortColumn() != std::nullopt
			&& m_bookmarkListPresenter->GetSortDirection() == +SortDirection::Descending;
	}

	return false;
}

void ManageBookmarksViewsMenu::OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown)
{
	UNREFERENCED_PARAMETER(isCtrlKeyDown);
	UNREFERENCED_PARAMETER(isShiftKeyDown);

	switch (id)
	{
	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_DEFAULT:
		m_bookmarkListPresenter->SetSortDetails(std::nullopt, SortDirection::Ascending);
		break;

	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_NAME:
		UpdateSortColumn(BookmarkColumn::Name);
		break;

	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LOCATION:
		UpdateSortColumn(BookmarkColumn::Location);
		break;

	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_ADDED:
		UpdateSortColumn(BookmarkColumn::DateCreated);
		break;

	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LAST_MODIFIED:
		UpdateSortColumn(BookmarkColumn::DateModified);
		break;

	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_ASCENDING:
		m_bookmarkListPresenter->SetSortDetails(m_bookmarkListPresenter->GetSortColumn(),
			SortDirection::Ascending);
		break;

	case IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_DESCENDING:
		m_bookmarkListPresenter->SetSortDetails(m_bookmarkListPresenter->GetSortColumn(),
			SortDirection::Descending);
		break;

	default:
		DCHECK(false);
		break;
	}
}

void ManageBookmarksViewsMenu::UpdateSortColumn(BookmarkColumn sortColumn)
{
	if (m_bookmarkListPresenter->GetSortColumn() == sortColumn)
	{
		return;
	}

	m_bookmarkListPresenter->SetSortDetails(sortColumn, SortDirection::Ascending);
}
