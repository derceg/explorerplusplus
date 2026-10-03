// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include <boost/signals2.hpp>
#include <vector>

class BrowserWindow;
class ResourceLoader;
class ShellBrowser;
enum class SortDirection;
enum class SortMode;

class SortGroupMenu : public MenuBase
{
public:
	enum class Mode
	{
		// This indicates that the menu will update the sort mode for the active ShellBrowser.
		Sort,

		// This indicates that the menu will update the group mode for the active ShellBrowser.
		Group
	};

	enum class Item
	{
		GroupByNone
	};

	SortGroupMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BrowserWindow *browser, Mode mode, const ResourceLoader *resourceLoader);

private:
	void OnPopupWillShow();
	void BuildMenu();
	void AddSortItem(SortMode sortMode);
	bool IsSortModeActive(SortMode sortMode) const;
	void AddSortDirectionItem(SortDirection sortDirection);
	static UINT GetSortDirectionMenuStringId(SortDirection sortDirection);
	static UINT GetSortDirectionMenuHelpStringId(SortDirection sortDirection);
	static UINT GetGroupSortDirectionMenuHelpStringId(SortDirection sortDirection);
	bool CanChangeSortDirection() const;
	bool IsSortDirectionActive(SortDirection sortDirection) const;

	void OnSortModeSelected(SortMode sortMode);
	void UpdateSortMode(SortMode sortMode);
	void UpdateGroupMode(SortMode groupMode);
	void OnSortDirectionSelected(SortDirection sortDirection);
	void OnSelectColumns();

	ShellBrowser *GetActiveShellBrowser();
	const ShellBrowser *GetActiveShellBrowser() const;

	BrowserWindow *const m_browser;
	const Mode m_mode;
	const ResourceLoader *const m_resourceLoader;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
