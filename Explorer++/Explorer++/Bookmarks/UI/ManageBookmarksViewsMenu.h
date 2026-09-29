// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/UI/BookmarkColumn.h"
#include "MenuBase.h"
#include "../Helper/SortDirection.h"
#include <memory>

class BookmarkListPresenter;
class ListViewColumnsMenu;
class ResourceLoader;

class ManageBookmarksViewsMenu : public MenuBase
{
public:
	enum class Item
	{
		ColumnsPopup,
		SortByPopup,
		SortByDefault,
		SortByName,
		SortByLocation,
		SortByDateCreated,
		SortByDateModified,
		SortAscending,
		SortDescending
	};

	ManageBookmarksViewsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BookmarkListPresenter *bookmarkListPresenter, const ResourceLoader *resourceLoader);
	~ManageBookmarksViewsMenu();

private:
	void BuildMenu(const ResourceLoader *resourceLoader);
	void BuildSortMenu(MenuView *sortMenuView, const ResourceLoader *resourceLoader);
	void AppendSortItem(MenuView *sortMenuView, Item itemKey, UINT stringId, BookmarkColumn column,
		const ResourceLoader *resourceLoader);
	void AppendSortDirectionItem(MenuView *sortMenuView, Item itemKey, UINT stringId,
		SortDirection sortDirection, const ResourceLoader *resourceLoader);
	bool HasSortColumn() const;

	void ResetSortColumn();
	void UpdateSortColumn(BookmarkColumn sortColumn);
	void UpdateSortDirection(SortDirection sortDirection);

	BookmarkListPresenter *const m_bookmarkListPresenter;
	std::unique_ptr<ListViewColumnsMenu> m_columnsMenu;
};
