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
	m_menuView->SetDelegate(this);

	BuildMenu(resourceLoader);
}

ManageBookmarksViewsMenu::~ManageBookmarksViewsMenu() = default;

void ManageBookmarksViewsMenu::BuildMenu(const ResourceLoader *resourceLoader)
{
	auto *columnsSubMenuView =
		m_menuView->AppendSubMenu(IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SHOW_COLUMNS_POPUP,
			resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SHOW_COLUMNS_POPUP));
	m_columnsMenu = std::make_unique<ListViewColumnsMenu>(columnsSubMenuView, m_acceleratorManager,
		m_bookmarkListPresenter->GetColumnModel(), resourceLoader);

	auto *sortSubMenuView = m_menuView->AppendSubMenu(IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_POPUP,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_POPUP));
	BuildSortMenu(sortSubMenuView, resourceLoader);
}

void ManageBookmarksViewsMenu::BuildSortMenu(MenuView *sortMenuView,
	const ResourceLoader *resourceLoader)
{
	sortMenuView->AppendItem(IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_DEFAULT,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_DEFAULT));
	sortMenuView->AppendItem(IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_NAME,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_NAME));
	sortMenuView->AppendItem(IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LOCATION,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LOCATION));
	sortMenuView->AppendItem(IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_ADDED,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_ADDED));
	sortMenuView->AppendItem(IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LAST_MODIFIED,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LAST_MODIFIED));

	sortMenuView->AppendSeparator();

	sortMenuView->AppendItem(IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_ASCENDING,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_ASCENDING));
	sortMenuView->AppendItem(IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_DESCENDING,
		resourceLoader->LoadString(IDS_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_DESCENDING));

	UpdateSortMenuItemStates(sortMenuView);
}

void ManageBookmarksViewsMenu::UpdateSortMenuItemStates(MenuView *sortMenuView)
{
	auto sortColumn = m_bookmarkListPresenter->GetSortColumn();
	sortMenuView->CheckRadioItem(sortColumn ? GetMenuIdForSortColumn(*sortColumn)
											: IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_DEFAULT,
		true);

	if (!sortColumn)
	{
		sortMenuView->EnableItem(IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_ASCENDING, false);
		sortMenuView->EnableItem(IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_DESCENDING, false);
	}
	else
	{
		sortMenuView->CheckRadioItem(m_bookmarkListPresenter->GetSortDirection()
					== +SortDirection::Ascending
				? IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_ASCENDING
				: IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_DESCENDING,
			true);
	}
}

UINT ManageBookmarksViewsMenu::GetMenuIdForSortColumn(BookmarkColumn sortColumn)
{
	switch (sortColumn)
	{
	case BookmarkColumn::Name:
		return IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_NAME;

	case BookmarkColumn::Location:
		return IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LOCATION;

	case BookmarkColumn::DateCreated:
		return IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_ADDED;

	case BookmarkColumn::DateModified:
		return IDM_MANAGE_BOOKMARKS_VIEWS_MENU_SORT_BY_LAST_MODIFIED;
	}

	LOG(FATAL) << "Invalid BookmarkColumn value";
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
