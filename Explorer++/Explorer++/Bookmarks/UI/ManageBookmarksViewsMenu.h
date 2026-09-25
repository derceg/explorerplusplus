// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/UI/BookmarkColumn.h"
#include "MenuBase.h"
#include "MenuDelegate.h"
#include <memory>

class BookmarkListPresenter;
class ListViewColumnsMenu;
class ResourceLoader;

class ManageBookmarksViewsMenu : public MenuBase, private MenuDelegate
{
public:
	ManageBookmarksViewsMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BookmarkListPresenter *bookmarkListPresenter, const ResourceLoader *resourceLoader);
	~ManageBookmarksViewsMenu();

private:
	void BuildMenu(const ResourceLoader *resourceLoader);
	void BuildSortMenu(MenuView *sortMenuView, const ResourceLoader *resourceLoader);

	// MenuDelegate
	bool IsItemEnabled(UINT id) const override;
	bool IsItemChecked(UINT id) const override;
	void OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown) override;

	void UpdateSortColumn(BookmarkColumn sortColumn);

	BookmarkListPresenter *const m_bookmarkListPresenter;
	std::unique_ptr<ListViewColumnsMenu> m_columnsMenu;
};
