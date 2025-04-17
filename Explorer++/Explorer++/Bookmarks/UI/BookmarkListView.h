// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/BookmarkNavigatorInterface.h"
#include "Bookmarks/UI/BookmarkDropTargetWindow.h"
#include "ResourceHelper.h"
#include "../Helper/WindowSubclass.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <optional>

class AcceleratorManager;
class BookmarkIconManager;
class BookmarkTree;
class BrowserWindow;
struct Config;
class IconFetcher;
class ResourceLoader;

class BookmarkListView : public BookmarkNavigatorInterface, private BookmarkDropTargetWindow
{
public:
	struct Column
	{
		BookmarkHelper::ColumnType columnType;
		int width;
		bool active;
	};

	BookmarkListView(HWND hListView, HINSTANCE resourceInstance, BookmarkTree *bookmarkTree,
		BrowserWindow *browser, const Config *config, const AcceleratorManager *acceleratorManager,
		const ResourceLoader *resourceLoader, IconFetcher *iconFetcher,
		const std::vector<Column> &initialColumns);

	void NavigateToBookmarkFolder(BookmarkItem *bookmarkFolder,
		const BookmarkHistoryEntry *entry = nullptr) override;
	boost::signals2::connection AddNavigationCompletedObserver(
		const BookmarkNavigationCompletedSignal::slot_type &observer,
		boost::signals2::connect_position position = boost::signals2::at_back) override;

	std::optional<int> GetLastSelectedItemIndex() const;
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
	static inline const double FOLDER_CENTRAL_RECT_INDENT_PERCENTAGE = 0.2;

	LRESULT WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void SetUpListViewImageList(IconFetcher *iconFetcher);

	void InsertColumns(const std::vector<Column> &columns);
	void InsertColumn(const Column &column, int index);
	std::wstring GetColumnText(BookmarkHelper::ColumnType columnType);
	UINT GetColumnTextResourceId(BookmarkHelper::ColumnType columnType);
	static bool IsColumnActive(const Column &column);
	std::optional<BookmarkHelper::ColumnType> GetColumnTypeByIndex(int index) const;

	int InsertBookmarkItemIntoListView(BookmarkItem *bookmarkItem, int position);
	void OnBookmarkIconAvailable(std::wstring_view guid, int iconIndex);
	std::wstring GetBookmarkItemColumnInfo(const BookmarkItem *bookmarkItem,
		BookmarkHelper::ColumnType columnType);
	std::wstring FormatDate(const FILETIME *date);

	BookmarkItem *GetBookmarkItemFromListView(int iItem);
	const BookmarkItem *GetBookmarkItemFromListView(int iItem) const;

	void SortItems();
	static int CALLBACK SortBookmarksStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	int CALLBACK SortBookmarks(LPARAM lParam1, LPARAM lParam2);
	int GetItemSortedPosition(const BookmarkItem *bookmarkItem) const;

	void OnDblClk(const NMITEMACTIVATE *itemActivate);
	void OnShowContextMenu(const POINT &ptScreen);
	void ShowBackgroundContextMenu(const POINT &ptScreen);
	void OnMenuItemSelected(int menuItemId);
	void OnNewBookmark();
	void OnGetDispInfo(NMLVDISPINFO *dispInfo);
	BOOL OnBeginLabelEdit(const NMLVDISPINFO *dispInfo);
	BOOL OnEndLabelEdit(const NMLVDISPINFO *dispInfo);
	void OnKeyDown(const NMLVKEYDOWN *keyDown);
	void OnBeginDrag();
	void OnRename();
	void OnEnterPressed();

	void OnHeaderItemClick(const NMHEADER *header);
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
	std::optional<int> GetBookmarkItemIndexUsingGuid(std::wstring_view guid) const;
	BookmarkHelper::ColumnType MapPropertyTypeToColumnType(
		BookmarkItem::PropertyType propertyType) const;
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
	HINSTANCE m_resourceInstance;
	BookmarkTree *const m_bookmarkTree;
	BrowserWindow *const m_browser;
	const Config *const m_config;
	const AcceleratorManager *const m_acceleratorManager;
	const ResourceLoader *const m_resourceLoader;
	std::unique_ptr<BookmarkIconManager> m_bookmarkIconManager;
	std::vector<Column> m_columns;

	BookmarkItem *m_currentBookmarkFolder = nullptr;
	BookmarkHelper::ColumnType m_sortColumn;
	bool m_sortAscending;
	std::optional<BookmarkHelper::ColumnType> m_previousSortColumn;

	BookmarkNavigationCompletedSignal m_navigationCompletedSignal;

	std::optional<int> m_previousDropItem;

	std::vector<std::unique_ptr<WindowSubclass>> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};
