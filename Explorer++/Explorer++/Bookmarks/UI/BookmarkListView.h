// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/BookmarkNavigatorInterface.h"
#include "Bookmarks/UI/BookmarkContextMenu.h"
#include "Bookmarks/UI/BookmarkDropTargetWindow.h"
#include "ResourceHelper.h"
#include "SignalWrapper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <optional>

class BookmarkTree;
__interface IExplorerplusplus;

class BookmarkListView : public BookmarkNavigatorInterface, private BookmarkDropTargetWindow
{
public:
	struct Column
	{
		BookmarkHelper::ColumnType columnType;
		int width;
		bool active;
	};

	BookmarkListView(HWND hListView, HMODULE resourceModule, BookmarkTree *bookmarkTree,
		IExplorerplusplus *expp, const std::vector<Column> &initialColumns);

	void NavigateToBookmarkFolder(BookmarkItem *bookmarkFolder, bool addHistoryEntry) override;
	boost::signals2::connection AddNavigationCompletedObserver(
		const BookmarkNavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;

	int GetLastSelectedItemIndex() const;
	RawBookmarkItems GetSelectedBookmarkItems();
	void SelectItem(const BookmarkItem *bookmarkItem);
	void CreateNewFolder();
	bool CanDelete();
	void DeleteSelection();

	std::vector<Column> GetColumns();
	void ToggleColumn(BookmarkHelper::ColumnType columnType);
	wil::unique_hmenu BuildColumnsMenu();
	BookmarkHelper::ColumnType GetSortColumn() const;
	void SetSortColumn(BookmarkHelper::ColumnType sortColumn);
	bool GetSortAscending() const;
	void SetSortAscending(bool sortAscending);

private:
	static inline const UINT_PTR PARENT_SUBCLASS_ID = 0;

	static inline const double FOLDER_CENTRAL_RECT_INDENT_PERCENTAGE = 0.2;

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void InsertColumns(const std::vector<Column> &columns);
	void InsertColumn(const Column &column, int index);
	std::wstring GetColumnText(BookmarkHelper::ColumnType columnType);
	UINT GetColumnTextResourceId(BookmarkHelper::ColumnType columnType);
	static bool IsColumnActive(const Column &column);
	std::optional<BookmarkHelper::ColumnType> GetColumnTypeByIndex(int index) const;

	int InsertBookmarkItemIntoListView(BookmarkItem *bookmarkItem, int position);
	std::wstring GetBookmarkItemColumnInfo(const BookmarkItem *bookmarkItem, BookmarkHelper::ColumnType columnType);
	static std::wstring FormatDate(const FILETIME *date);

	BookmarkItem *GetBookmarkItemFromListView(int iItem);

	void SortItems();
	static int CALLBACK SortBookmarksStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	int CALLBACK SortBookmarks(LPARAM lParam1, LPARAM lParam2);

	void OnDblClk(const NMITEMACTIVATE *itemActivate);
	void OnRClick(const NMITEMACTIVATE *itemActivate);
	void ShowBackgroundContextMenu(const POINT &ptScreen);
	void OnMenuItemSelected(int menuItemId);
	void OnNewBookmark();
	void OnGetDispInfo(NMLVDISPINFO *dispInfo);
	BOOL OnBeginLabelEdit(const NMLVDISPINFO *dispInfo);
	BOOL OnEndLabelEdit(const NMLVDISPINFO *dispInfo);
	void OnKeyDown(const NMLVKEYDOWN *keyDown);
	void OnBeginDrag();
	void OnRename();

	void OnHeaderRClick(const POINT &pt);
	void OnHeaderContextMenuItemSelected(int menuItemId);
	void UpdateHeader();
	void ClearColumnSortArrow(BookmarkHelper::ColumnType columnType);
	void SetColumnSortArrow(BookmarkHelper::ColumnType columnType, bool sortAscending);

	void OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index);
	void OnBookmarkItemUpdated(BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType);
	void OnBookmarkItemMoved(BookmarkItem *bookmarkItem, const BookmarkItem *oldParent,
		size_t oldIndex, const BookmarkItem *newParent, size_t newIndex);
	void OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem);

	void RemoveBookmarkItem(const BookmarkItem *bookmarkItem);
	std::optional<int> GetBookmarkItemIndex(const BookmarkItem *bookmarkItem) const;
	BookmarkHelper::ColumnType MapPropertyTypeToColumnType(BookmarkItem::PropertyType propertyType) const;
	Column &GetColumnByType(BookmarkHelper::ColumnType columnType);
	std::optional<int> GetColumnHeaderIndexByType(BookmarkHelper::ColumnType columnType) const;
	int GetColumnIndexByType(BookmarkHelper::ColumnType columnType) const;

	DropLocation GetDropLocation(const POINT &pt) override;
	int FindNextItemIndex(const POINT &ptClient);
	void UpdateUiForDropLocation(const DropLocation &dropLocation) override;
	void ResetDropUiState() override;
	void RemoveInsertionMark();
	void RemoveDropHighlight();

	HWND m_hListView;
	HMODULE m_resourceModule;
	IExplorerplusplus *m_expp;
	DpiCompatibility m_dpiCompat;
	wil::unique_himagelist m_imageList;
	IconImageListMapping m_imageListMappings;
	std::vector<Column> m_columns;

	BookmarkTree *m_bookmarkTree;
	BookmarkItem *m_currentBookmarkFolder;
	BookmarkHelper::ColumnType m_sortColumn;
	bool m_sortAscending;
	std::optional<BookmarkHelper::ColumnType> m_previousSortColumn;
	BookmarkContextMenu m_bookmarkContextMenu;

	BookmarkNavigationCompletedSignal m_navigationCompletedSignal;

	std::optional<int> m_previousDropItem;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};