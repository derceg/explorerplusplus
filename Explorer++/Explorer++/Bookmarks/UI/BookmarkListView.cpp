// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Bookmarks/UI/BookmarkListView.h"
#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/BookmarkTree.h"
#include "BrowserWindow.h"
#include "Config.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/DropSourceImpl.h"
#include "../Helper/HeaderHelper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/WindowHelper.h"
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <glog/logging.h>
#include <utility>

BookmarkListView::BookmarkListView(HWND hListView, HINSTANCE resourceInstance,
	BookmarkTree *bookmarkTree, BrowserWindow *browserWindow, CoreInterface *coreInterface,
	const IconResourceLoader *iconResourceLoader, IconFetcher *iconFetcher,
	ThemeManager *themeManager, const std::vector<Column> &initialColumns) :
	BookmarkDropTargetWindow(hListView, bookmarkTree),
	m_hListView(hListView),
	m_resourceInstance(resourceInstance),
	m_bookmarkTree(bookmarkTree),
	m_browserWindow(browserWindow),
	m_coreInterface(coreInterface),
	m_iconResourceLoader(iconResourceLoader),
	m_themeManager(themeManager),
	m_columns(initialColumns),
	m_sortColumn(BookmarkHelper::ColumnType::Default),
	m_sortAscending(true),
	m_bookmarkContextMenu(bookmarkTree, resourceInstance, browserWindow, coreInterface,
		iconResourceLoader, themeManager)
{
	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP,
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

	SetUpListViewImageList(iconFetcher);

	InsertColumns(initialColumns);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hListView,
		std::bind_front(&BookmarkListView::WndProc, this)));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(GetParent(m_hListView),
		std::bind_front(&BookmarkListView::ParentWndProc, this)));

	m_connections.push_back(m_bookmarkTree->bookmarkItemAddedSignal.AddObserver(
		std::bind_front(&BookmarkListView::OnBookmarkItemAdded, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemUpdatedSignal.AddObserver(
		std::bind_front(&BookmarkListView::OnBookmarkItemUpdated, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemMovedSignal.AddObserver(
		std::bind_front(&BookmarkListView::OnBookmarkItemMoved, this)));
	m_connections.push_back(m_bookmarkTree->bookmarkItemPreRemovalSignal.AddObserver(
		std::bind_front(&BookmarkListView::OnBookmarkItemPreRemoval, this)));
}

void BookmarkListView::SetUpListViewImageList(IconFetcher *iconFetcher)
{
	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(m_hListView);
	int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);

	m_bookmarkIconManager = std::make_unique<BookmarkIconManager>(m_iconResourceLoader, iconFetcher,
		iconWidth, iconHeight);

	ListView_SetImageList(m_hListView, m_bookmarkIconManager->GetImageList(), LVSIL_SMALL);
}

void BookmarkListView::InsertColumns(const std::vector<Column> &columns)
{
	for (const auto &column :
		columns | boost::adaptors::filtered(IsColumnActive) | boost::adaptors::indexed(0))
	{
		InsertColumn(column.value(), static_cast<int>(column.index()));
	}
}

void BookmarkListView::InsertColumn(const Column &column, int index)
{
	std::wstring columnText = GetColumnText(column.columnType);

	LVCOLUMN lvColumn;
	lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
	lvColumn.pszText = columnText.data();
	lvColumn.cx = column.width;
	int insertedIndex = ListView_InsertColumn(m_hListView, index, &lvColumn);

	HWND header = ListView_GetHeader(m_hListView);

	HDITEM hdItem;
	hdItem.mask = HDI_LPARAM;
	hdItem.lParam = static_cast<LPARAM>(column.columnType);
	Header_SetItem(header, insertedIndex, &hdItem);
}

std::wstring BookmarkListView::GetColumnText(BookmarkHelper::ColumnType columnType)
{
	UINT resourceId = GetColumnTextResourceId(columnType);
	return ResourceHelper::LoadString(m_resourceInstance, resourceId);
}

std::vector<BookmarkListView::Column> BookmarkListView::GetColumns()
{
	int index = 0;

	for (auto &column : m_columns | boost::adaptors::filtered(IsColumnActive))
	{
		column.width = ListView_GetColumnWidth(m_hListView, index++);
	}

	return m_columns;
}

bool BookmarkListView::IsColumnActive(const Column &column)
{
	return column.active;
}

std::optional<BookmarkHelper::ColumnType> BookmarkListView::GetColumnTypeByIndex(int index) const
{
	HWND hHeader = ListView_GetHeader(m_hListView);

	HDITEM hdItem;
	hdItem.mask = HDI_LPARAM;
	BOOL res = Header_GetItem(hHeader, index, &hdItem);

	if (!res)
	{
		return std::nullopt;
	}

	return static_cast<BookmarkHelper::ColumnType>(hdItem.lParam);
}

UINT BookmarkListView::GetColumnTextResourceId(BookmarkHelper::ColumnType columnType)
{
	switch (columnType)
	{
	case BookmarkHelper::ColumnType::Name:
		return IDS_BOOKMARKS_COLUMN_NAME;

	case BookmarkHelper::ColumnType::Location:
		return IDS_BOOKMARKS_COLUMN_LOCATION;

	case BookmarkHelper::ColumnType::DateCreated:
		return IDS_BOOKMARKS_COLUMN_DATE_CREATED;

	case BookmarkHelper::ColumnType::DateModified:
		return IDS_BOOKMARKS_COLUMN_DATE_MODIFIED;

	default:
		LOG(FATAL) << "Bookmark column string resource not found";
		__assume(0);
	}
}

LRESULT BookmarkListView::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_GETDLGCODE:
		switch (wParam)
		{
		// This key press is used to open the selected bookmarks.
		case VK_RETURN:
			return DLGC_WANTALLKEYS;
		}
		break;

	case WM_CONTEXTMENU:
		if (reinterpret_cast<HWND>(wParam) == m_hListView)
		{
			OnShowContextMenu({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
			return 0;
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT BookmarkListView::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hListView)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_DBLCLK:
				OnDblClk(reinterpret_cast<NMITEMACTIVATE *>(lParam));
				break;

			case LVN_GETDISPINFO:
				OnGetDispInfo(reinterpret_cast<NMLVDISPINFO *>(lParam));
				break;

			case LVN_BEGINLABELEDIT:
				return OnBeginLabelEdit(reinterpret_cast<NMLVDISPINFO *>(lParam));

			case LVN_ENDLABELEDIT:
				return OnEndLabelEdit(reinterpret_cast<NMLVDISPINFO *>(lParam));

			case LVN_KEYDOWN:
				OnKeyDown(reinterpret_cast<NMLVKEYDOWN *>(lParam));
				break;

			case LVN_BEGINDRAG:
				OnBeginDrag();
				break;
			}
		}
		else if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == ListView_GetHeader(m_hListView))
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case HDN_ITEMCLICK:
				OnHeaderItemClick(reinterpret_cast<NMHEADER *>(lParam));
				break;

			case NM_RCLICK:
			{
				POINT pt;
				DWORD messagePos = GetMessagePos();
				POINTSTOPOINT(pt, MAKEPOINTS(messagePos));
				OnHeaderRClick(pt);
				return TRUE;
			}
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void BookmarkListView::NavigateToBookmarkFolder(BookmarkItem *bookmarkFolder,
	const BookmarkHistoryEntry *entry)
{
	DCHECK(bookmarkFolder->IsFolder());

	m_currentBookmarkFolder = bookmarkFolder;

	ListView_DeleteAllItems(m_hListView);

	int position = 0;

	for (auto &childItem : bookmarkFolder->GetChildren())
	{
		InsertBookmarkItemIntoListView(childItem.get(), position);

		position++;
	}

	m_navigationCompletedSignal(bookmarkFolder, entry);
}

boost::signals2::connection BookmarkListView::AddNavigationCompletedObserver(
	const BookmarkNavigationCompletedSignal::slot_type &observer,
	boost::signals2::connect_position position)
{
	return m_navigationCompletedSignal.connect(observer, position);
}

int BookmarkListView::InsertBookmarkItemIntoListView(BookmarkItem *bookmarkItem, int position)
{
	DCHECK(position >= 0 && position <= ListView_GetItemCount(m_hListView));

	TCHAR szName[256];
	StringCchCopy(szName, std::size(szName), bookmarkItem->GetName().c_str());

	int iconIndex = m_bookmarkIconManager->GetBookmarkItemIconIndex(bookmarkItem,
		std::bind_front(&BookmarkListView::OnBookmarkIconAvailable, this, bookmarkItem->GetGUID()));

	int sortedPosition;

	if (m_sortColumn == BookmarkHelper::ColumnType::Default)
	{
		sortedPosition = position;
	}
	else
	{
		sortedPosition = GetItemSortedPosition(bookmarkItem);
	}

	LVITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iItem = sortedPosition;
	lvi.iSubItem = 0;
	lvi.iImage = iconIndex;
	lvi.pszText = szName;
	lvi.lParam = reinterpret_cast<LPARAM>(bookmarkItem);
	int iItem = ListView_InsertItem(m_hListView, &lvi);

	return iItem;
}

void BookmarkListView::OnBookmarkIconAvailable(std::wstring_view guid, int iconIndex)
{
	auto index = GetBookmarkItemIndexUsingGuid(guid);

	if (!index)
	{
		return;
	}

	LVITEM item;
	item.mask = LVIF_IMAGE;
	item.iItem = *index;
	item.iSubItem = 0;
	item.iImage = iconIndex;
	ListView_SetItem(m_hListView, &item);
}

BookmarkItem *BookmarkListView::GetBookmarkItemFromListView(int iItem)
{
	return const_cast<BookmarkItem *>(std::as_const(*this).GetBookmarkItemFromListView(iItem));
}

const BookmarkItem *BookmarkListView::GetBookmarkItemFromListView(int iItem) const
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iItem = iItem;
	lvi.iSubItem = 0;
	ListView_GetItem(m_hListView, &lvi);

	return reinterpret_cast<BookmarkItem *>(lvi.lParam);
}

void BookmarkListView::SortItems()
{
	ListView_SortItems(m_hListView, SortBookmarksStub, reinterpret_cast<LPARAM>(this));
}

int CALLBACK BookmarkListView::SortBookmarksStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	auto *listView = reinterpret_cast<BookmarkListView *>(lParamSort);

	return listView->SortBookmarks(lParam1, lParam2);
}

int CALLBACK BookmarkListView::SortBookmarks(LPARAM lParam1, LPARAM lParam2)
{
	auto firstItem = reinterpret_cast<BookmarkItem *>(lParam1);
	auto secondItem = reinterpret_cast<BookmarkItem *>(lParam2);

	int iRes = BookmarkHelper::Sort(m_sortColumn, firstItem, secondItem);

	// When using the default sort mode (in which items are sorted according to
	// their position within the parent folder), the items are always in
	// ascending order.
	if (!m_sortAscending && m_sortColumn != BookmarkHelper::ColumnType::Default)
	{
		iRes = -iRes;
	}

	return iRes;
}

int BookmarkListView::GetItemSortedPosition(const BookmarkItem *bookmarkItem) const
{
	int numItems = ListView_GetItemCount(m_hListView);
	int res = 1;
	int i = 0;

	while (res > 0 && i < numItems)
	{
		const BookmarkItem *currentItem = GetBookmarkItemFromListView(i);
		res = BookmarkHelper::Sort(m_sortColumn, bookmarkItem, currentItem);

		i++;
	}

	if (i < numItems || res < 0)
	{
		i--;
	}

	return i;
}

BookmarkHelper::ColumnType BookmarkListView::GetSortColumn() const
{
	return m_sortColumn;
}

void BookmarkListView::SetSortColumn(BookmarkHelper::ColumnType sortColumn)
{
	m_previousSortColumn = m_sortColumn;
	m_sortColumn = sortColumn;

	// It's only possible to drop items when using the default sort mode, since that's the only mode
	// in which the listview indexes match the bookmark item indexes.
	SetBlockDrop(sortColumn != BookmarkHelper::ColumnType::Default);

	SortItems();
	UpdateHeader();
}

bool BookmarkListView::GetSortAscending() const
{
	return m_sortAscending;
}

void BookmarkListView::SetSortAscending(bool sortAscending)
{
	m_previousSortColumn = m_sortColumn;
	m_sortAscending = sortAscending;

	SortItems();
	UpdateHeader();
}

void BookmarkListView::UpdateHeader()
{
	if (m_previousSortColumn)
	{
		ClearColumnSortArrow(*m_previousSortColumn);
	}

	SetColumnSortArrow(m_sortColumn, m_sortAscending);
}

void BookmarkListView::ClearColumnSortArrow(BookmarkHelper::ColumnType columnType)
{
	auto columnIndex = GetColumnHeaderIndexByType(columnType);

	if (!columnIndex)
	{
		return;
	}

	HWND header = ListView_GetHeader(m_hListView);
	HeaderHelper::AddOrRemoveFormatOptions(header, *columnIndex, HDF_SORTUP | HDF_SORTDOWN, false);
}

void BookmarkListView::SetColumnSortArrow(BookmarkHelper::ColumnType columnType, bool sortAscending)
{
	auto columnIndex = GetColumnHeaderIndexByType(columnType);

	if (!columnIndex)
	{
		return;
	}

	HWND header = ListView_GetHeader(m_hListView);

	int sortOption;

	if (sortAscending)
	{
		sortOption = HDF_SORTUP;
	}
	else
	{
		sortOption = HDF_SORTDOWN;
	}

	HeaderHelper::AddOrRemoveFormatOptions(header, *columnIndex, sortOption, true);
}

void BookmarkListView::OnDblClk(const NMITEMACTIVATE *itemActivate)
{
	if (itemActivate->iItem == -1)
	{
		return;
	}

	auto bookmarkItem = GetBookmarkItemFromListView(itemActivate->iItem);

	if (bookmarkItem->IsFolder())
	{
		NavigateToBookmarkFolder(bookmarkItem);
	}
	else
	{
		BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem,
			m_coreInterface->GetConfig()->openTabsInForeground
				? OpenFolderDisposition::ForegroundTab
				: OpenFolderDisposition::BackgroundTab,
			m_browserWindow);
	}
}

void BookmarkListView::OnShowContextMenu(const POINT &ptScreen)
{
	POINT finalPoint = ptScreen;

	bool keyboardGenerated = false;

	// If the context menu message was generated by the keyboard, there won't be any associated
	// position.
	if (ptScreen.x == -1 && ptScreen.y == -1)
	{
		keyboardGenerated = true;
	}

	auto rawBookmarkItems = GetSelectedBookmarkItems();

	if (rawBookmarkItems.empty())
	{
		if (keyboardGenerated)
		{
			finalPoint = { 0, 0 };
			ClientToScreen(m_hListView, &finalPoint);
		}

		ShowBackgroundContextMenu(finalPoint);
	}
	else
	{
		if (keyboardGenerated)
		{
			RECT itemRect;
			auto lastSelectedItemIndex = GetLastSelectedItemIndex();
			ListView_GetItemRect(m_hListView, *lastSelectedItemIndex, &itemRect, LVIR_BOUNDS);

			finalPoint = { itemRect.left, itemRect.bottom };
			ClientToScreen(m_hListView, &finalPoint);
		}

		m_bookmarkContextMenu.ShowMenu(m_hListView, m_currentBookmarkFolder, rawBookmarkItems,
			finalPoint);
	}
}

void BookmarkListView::ShowBackgroundContextMenu(const POINT &ptScreen)
{
	wil::unique_hmenu parentMenu(
		LoadMenu(m_resourceInstance, MAKEINTRESOURCE(IDR_BOOKMARK_LISTVIEW_CONTEXT_MENU)));

	if (!parentMenu)
	{
		return;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	int menuItemId = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RETURNCMD, ptScreen.x, ptScreen.y, 0,
		m_hListView, nullptr);

	if (menuItemId != 0)
	{
		OnMenuItemSelected(menuItemId);
	}
}

void BookmarkListView::OnMenuItemSelected(int menuItemId)
{
	switch (menuItemId)
	{
	case IDM_BOOKMARKS_NEW_BOOKMARK:
		OnNewBookmark();
		break;

	case IDM_BOOKMARKS_NEW_FOLDER:
		CreateNewFolder();
		break;

	default:
		DCHECK(false);
		break;
	}
}

void BookmarkListView::OnNewBookmark()
{
	size_t targetIndex;
	auto lastSelectedItemindex = GetLastSelectedItemIndex();

	if (lastSelectedItemindex)
	{
		targetIndex = *lastSelectedItemindex + 1;
	}
	else
	{
		targetIndex = m_currentBookmarkFolder->GetChildren().size();
	}

	auto bookmark = BookmarkHelper::AddBookmarkItem(m_bookmarkTree, BookmarkItem::Type::Bookmark,
		m_currentBookmarkFolder, targetIndex, m_hListView, m_themeManager, m_coreInterface,
		m_iconResourceLoader);

	if (!bookmark)
	{
		return;
	}

	if (bookmark->GetParent() != m_currentBookmarkFolder)
	{
		return;
	}

	SelectItem(bookmark);
}

std::optional<int> BookmarkListView::GetLastSelectedItemIndex() const
{
	return ListViewHelper::GetLastSelectedItemIndex(m_hListView);
}

RawBookmarkItems BookmarkListView::GetSelectedBookmarkItems()
{
	RawBookmarkItems bookmarksItems;
	int index = -1;

	while ((index = ListView_GetNextItem(m_hListView, index, LVNI_SELECTED)) != -1)
	{
		BookmarkItem *bookmarkItem = GetBookmarkItemFromListView(index);
		bookmarksItems.push_back(bookmarkItem);
	}

	return bookmarksItems;
}

void BookmarkListView::SelectItem(const BookmarkItem *bookmarkItem)
{
	auto index = GetBookmarkItemIndex(bookmarkItem);

	if (!index)
	{
		return;
	}

	SetFocus(m_hListView);
	ListViewHelper::SelectAllItems(m_hListView, false);
	ListViewHelper::SelectItem(m_hListView, *index, true);
}

void BookmarkListView::CreateNewFolder()
{
	auto bookmarkItem = std::make_unique<BookmarkItem>(std::nullopt,
		ResourceHelper::LoadString(m_resourceInstance, IDS_BOOKMARKS_NEWBOOKMARKFOLDER),
		std::nullopt);
	auto rawBookmarkItem = bookmarkItem.get();

	m_bookmarkTree->AddBookmarkItem(m_currentBookmarkFolder, std::move(bookmarkItem),
		m_currentBookmarkFolder->GetChildren().size());

	auto index = GetBookmarkItemIndex(rawBookmarkItem);
	CHECK(index);

	SetFocus(m_hListView);
	ListView_EditLabel(m_hListView, *index);
}

void BookmarkListView::OnGetDispInfo(NMLVDISPINFO *dispInfo)
{
	if (WI_IsFlagSet(dispInfo->item.mask, LVIF_TEXT))
	{
		auto bookmarkItem = GetBookmarkItemFromListView(dispInfo->item.iItem);

		auto columnType = GetColumnTypeByIndex(dispInfo->item.iSubItem);
		CHECK(columnType);

		std::wstring columnText = GetBookmarkItemColumnInfo(bookmarkItem, *columnType);

		StringCchCopy(dispInfo->item.pszText, dispInfo->item.cchTextMax, columnText.c_str());

		WI_SetFlag(dispInfo->item.mask, LVIF_DI_SETITEM);
	}
}

std::wstring BookmarkListView::GetBookmarkItemColumnInfo(const BookmarkItem *bookmarkItem,
	BookmarkHelper::ColumnType columnType)
{
	switch (columnType)
	{
	case BookmarkHelper::ColumnType::Name:
		return bookmarkItem->GetName();

	case BookmarkHelper::ColumnType::Location:
		if (bookmarkItem->IsBookmark())
		{
			return bookmarkItem->GetLocation();
		}
		else
		{
			return std::wstring();
		}

	case BookmarkHelper::ColumnType::DateCreated:
	{
		FILETIME dateCreated = bookmarkItem->GetDateCreated();
		return FormatDate(&dateCreated);
	}

	case BookmarkHelper::ColumnType::DateModified:
	{
		FILETIME dateModified = bookmarkItem->GetDateModified();
		return FormatDate(&dateModified);
	}

	default:
		LOG(FATAL) << "Bookmark column type not found";
	}
}

std::wstring BookmarkListView::FormatDate(const FILETIME *date)
{
	TCHAR formattedDate[256];
	BOOL res = CreateFileTimeString(date, formattedDate, std::size(formattedDate),
		m_coreInterface->GetConfig()->globalFolderSettings.showFriendlyDates);

	if (res)
	{
		return formattedDate;
	}

	return std::wstring();
}

BOOL BookmarkListView::OnBeginLabelEdit(const NMLVDISPINFO *dispInfo)
{
	auto bookmarkItem = GetBookmarkItemFromListView(dispInfo->item.iItem);

	if (m_bookmarkTree->IsPermanentNode(bookmarkItem))
	{
		return TRUE;
	}

	return FALSE;
}

BOOL BookmarkListView::OnEndLabelEdit(const NMLVDISPINFO *dispInfo)
{
	if (dispInfo->item.pszText == nullptr && lstrlen(dispInfo->item.pszText) == 0)
	{
		return FALSE;
	}

	auto bookmarkItem = GetBookmarkItemFromListView(dispInfo->item.iItem);

	if (m_bookmarkTree->IsPermanentNode(bookmarkItem))
	{
		return FALSE;
	}

	bookmarkItem->SetName(dispInfo->item.pszText);

	return TRUE;
}

void BookmarkListView::OnKeyDown(const NMLVKEYDOWN *keyDown)
{
	switch (keyDown->wVKey)
	{
	case VK_F2:
		OnRename();
		break;

	case 'A':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			ListViewHelper::SelectAllItems(m_hListView, true);
		}
		break;

	case VK_RETURN:
		OnEnterPressed();
		break;

	case VK_DELETE:
		DeleteSelection();
		break;
	}
}

void BookmarkListView::OnBeginDrag()
{
	auto dropSource = winrt::make_self<DropSourceImpl>();

	auto rawBookmarkItems = GetSelectedBookmarkItems();

	if (rawBookmarkItems.empty())
	{
		return;
	}

	OwnedRefBookmarkItems bookmarkItems;

	for (auto &rawBookmarkItem : rawBookmarkItems)
	{
		auto &ownedPtr = rawBookmarkItem->GetParent()->GetChildOwnedPtr(rawBookmarkItem);
		bookmarkItems.push_back(ownedPtr);
	}

	auto dataObject = BookmarkDataExchange::CreateDataObject(bookmarkItems);

	wil::com_ptr_nothrow<IDragSourceHelper> dragSourceHelper;
	HRESULT hr = CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_ALL,
		IID_PPV_ARGS(&dragSourceHelper));

	if (SUCCEEDED(hr))
	{
		dragSourceHelper->InitializeFromWindow(m_hListView, nullptr, dataObject.get());
	}

	DWORD effect;
	DoDragDrop(dataObject.get(), dropSource.get(), DROPEFFECT_MOVE, &effect);
}

void BookmarkListView::OnRename()
{
	int item = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);

	if (item != -1)
	{
		ListView_EditLabel(m_hListView, item);
	}
}

bool BookmarkListView::CanDelete()
{
	auto rawBookmarkItems = GetSelectedBookmarkItems();
	bool nonPermanentNodeSelected = false;

	for (BookmarkItem *bookmarkItem : rawBookmarkItems)
	{
		if (!m_bookmarkTree->IsPermanentNode(bookmarkItem))
		{
			nonPermanentNodeSelected = true;
			break;
		}
	}

	return nonPermanentNodeSelected;
}

void BookmarkListView::OnEnterPressed()
{
	RawBookmarkItems bookmarkItems = GetSelectedBookmarkItems();

	if (bookmarkItems.size() == 1 && bookmarkItems[0]->IsFolder())
	{
		NavigateToBookmarkFolder(bookmarkItems[0]);
	}
	else
	{
		OpenFolderDisposition disposition = m_coreInterface->GetConfig()->openTabsInForeground
			? OpenFolderDisposition::ForegroundTab
			: OpenFolderDisposition::BackgroundTab;

		for (BookmarkItem *bookmarkItem : bookmarkItems)
		{
			BookmarkHelper::OpenBookmarkItemWithDisposition(bookmarkItem, disposition,
				m_browserWindow);

			disposition = OpenFolderDisposition::BackgroundTab;
		}
	}
}

void BookmarkListView::DeleteSelection()
{
	auto rawBookmarkItems = GetSelectedBookmarkItems();

	for (BookmarkItem *bookmarkItem : rawBookmarkItems)
	{
		if (!m_bookmarkTree->IsPermanentNode(bookmarkItem))
		{
			m_bookmarkTree->RemoveBookmarkItem(bookmarkItem);
		}
	}
}

void BookmarkListView::OnHeaderItemClick(const NMHEADER *header)
{
	auto selectedColumn = GetColumnTypeByIndex(header->iItem);
	CHECK(selectedColumn);

	BookmarkHelper::ColumnType newSortColumn = m_sortColumn;
	bool newSortAscending = m_sortAscending;

	if (*selectedColumn == m_sortColumn)
	{
		if (m_sortAscending)
		{
			newSortAscending = false;
		}
		else
		{
			newSortColumn = BookmarkHelper::ColumnType::Default;
			newSortAscending = true;
		}
	}
	else
	{
		newSortColumn = *selectedColumn;
		newSortAscending = true;
	}

	if (newSortColumn != m_sortColumn)
	{
		SetSortColumn(newSortColumn);
	}

	if (newSortAscending != m_sortAscending)
	{
		SetSortAscending(newSortAscending);
	}
}

void BookmarkListView::OnHeaderRClick(const POINT &pt)
{
	auto menu = BuildColumnsMenu();

	if (!menu)
	{
		return;
	}

	/* The name column cannot be removed. */
	MenuHelper::EnableItem(menu.get(), static_cast<UINT>(BookmarkHelper::ColumnType::Name), FALSE);

	int cmd = TrackPopupMenu(menu.get(), TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_hListView,
		nullptr);

	if (cmd != 0)
	{
		OnHeaderContextMenuItemSelected(cmd);
	}
}

wil::unique_hmenu BookmarkListView::BuildColumnsMenu()
{
	wil::unique_hmenu menu(CreatePopupMenu());

	if (!menu)
	{
		return nullptr;
	}

	int index = 0;

	for (const auto &column : m_columns)
	{
		std::wstring columnText = GetColumnText(column.columnType);

		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE;
		mii.wID = static_cast<int>(column.columnType);
		mii.dwTypeData = columnText.data();
		mii.fState = 0;

		if (column.active)
		{
			mii.fState |= MFS_CHECKED;
		}

		InsertMenuItem(menu.get(), index++, TRUE, &mii);
	}

	return menu;
}

void BookmarkListView::OnHeaderContextMenuItemSelected(int menuItemId)
{
	auto columnType = static_cast<BookmarkHelper::ColumnType>(menuItemId);
	ToggleColumn(columnType);
}

void BookmarkListView::ToggleColumn(BookmarkHelper::ColumnType columnType)
{
	Column &column = GetColumnByType(columnType);
	int columnIndex = GetColumnIndexByType(columnType);

	bool nowActive = !column.active;

	if (nowActive)
	{
		InsertColumn(column, columnIndex);
	}
	else
	{
		column.width = ListView_GetColumnWidth(m_hListView, columnIndex);
		ListView_DeleteColumn(m_hListView, columnIndex);
	}

	column.active = nowActive;
}

void BookmarkListView::OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index)
{
	if (bookmarkItem.GetParent() == m_currentBookmarkFolder)
	{
		InsertBookmarkItemIntoListView(&bookmarkItem, static_cast<int>(index));
	}
}

void BookmarkListView::OnBookmarkItemUpdated(BookmarkItem &bookmarkItem,
	BookmarkItem::PropertyType propertyType)
{
	if (bookmarkItem.GetParent() != m_currentBookmarkFolder)
	{
		return;
	}

	auto index = GetBookmarkItemIndex(&bookmarkItem);
	CHECK(index);

	BookmarkHelper::ColumnType columnType = MapPropertyTypeToColumnType(propertyType);
	Column &column = GetColumnByType(columnType);

	if (!column.active)
	{
		return;
	}

	int columnIndex = GetColumnIndexByType(columnType);
	std::wstring columnText = GetBookmarkItemColumnInfo(&bookmarkItem, columnType);
	ListView_SetItemText(m_hListView, *index, columnIndex, columnText.data());
}

void BookmarkListView::OnBookmarkItemMoved(BookmarkItem *bookmarkItem,
	const BookmarkItem *oldParent, size_t oldIndex, const BookmarkItem *newParent, size_t newIndex)
{
	UNREFERENCED_PARAMETER(oldIndex);

	if (oldParent == m_currentBookmarkFolder)
	{
		RemoveBookmarkItem(bookmarkItem);
	}

	if (newParent == m_currentBookmarkFolder)
	{
		InsertBookmarkItemIntoListView(bookmarkItem, static_cast<int>(newIndex));
	}
}

void BookmarkListView::OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem)
{
	if (bookmarkItem.GetParent() == m_currentBookmarkFolder)
	{
		RemoveBookmarkItem(&bookmarkItem);
	}
}

void BookmarkListView::RemoveBookmarkItem(const BookmarkItem *bookmarkItem)
{
	auto index = GetBookmarkItemIndex(bookmarkItem);
	CHECK(index);

	ListView_DeleteItem(m_hListView, *index);
}

std::optional<int> BookmarkListView::GetBookmarkItemIndex(const BookmarkItem *bookmarkItem) const
{
	LVFINDINFO findInfo;
	findInfo.flags = LVFI_PARAM;
	findInfo.lParam = reinterpret_cast<LPARAM>(bookmarkItem);
	int index = ListView_FindItem(m_hListView, -1, &findInfo);

	if (index == -1)
	{
		return std::nullopt;
	}

	return index;
}

std::optional<int> BookmarkListView::GetBookmarkItemIndexUsingGuid(std::wstring_view guid) const
{
	int numItems = ListView_GetItemCount(m_hListView);

	for (int i = 0; i < numItems; i++)
	{
		const BookmarkItem *bookmarkItem = GetBookmarkItemFromListView(i);

		if (bookmarkItem->GetGUID() == guid)
		{
			return i;
		}
	}

	return std::nullopt;
}

BookmarkHelper::ColumnType BookmarkListView::MapPropertyTypeToColumnType(
	BookmarkItem::PropertyType propertyType) const
{
	switch (propertyType)
	{
	case BookmarkItem::PropertyType::Name:
		return BookmarkHelper::ColumnType::Name;

	case BookmarkItem::PropertyType::Location:
		return BookmarkHelper::ColumnType::Location;

	case BookmarkItem::PropertyType::DateCreated:
		return BookmarkHelper::ColumnType::DateCreated;

	case BookmarkItem::PropertyType::DateModified:
		return BookmarkHelper::ColumnType::DateModified;

	default:
		LOG(FATAL) << "Bookmark column not found";
		__assume(0);
	}
}

BookmarkListView::Column &BookmarkListView::GetColumnByType(BookmarkHelper::ColumnType columnType)
{
	auto itr = std::find_if(m_columns.begin(), m_columns.end(),
		[columnType](const Column &column) { return column.columnType == columnType; });
	CHECK(itr != m_columns.end());
	return *itr;
}

// Returns the index of the specified column in the listview header. If the column isn't being
// shown, an empty value will be returned.
std::optional<int> BookmarkListView::GetColumnHeaderIndexByType(
	BookmarkHelper::ColumnType columnType) const
{
	auto itr = std::find_if(m_columns.begin(), m_columns.end(),
		[columnType](const Column &column) { return column.columnType == columnType; });

	if (itr == m_columns.end())
	{
		return std::nullopt;
	}

	if (!itr->active)
	{
		return std::nullopt;
	}

	auto columnIndex =
		std::count_if(m_columns.begin(), itr, [](const Column &column) { return column.active; });

	return static_cast<int>(columnIndex);
}

// If the specified column is active, this function returns the index of the
// column (as it appears in the listview). If it's not active, the function will
// return the index the column would be shown at, if it were active.
int BookmarkListView::GetColumnIndexByType(BookmarkHelper::ColumnType columnType) const
{
	auto itr = std::find_if(m_columns.begin(), m_columns.end(),
		[columnType](const Column &column) { return column.columnType == columnType; });
	CHECK(itr != m_columns.end());

	auto columnIndex =
		std::count_if(m_columns.begin(), itr, [](const Column &column) { return column.active; });

	return static_cast<int>(columnIndex);
}

BookmarkListView::DropLocation BookmarkListView::GetDropLocation(const POINT &pt)
{
	POINT ptClient = pt;
	ScreenToClient(m_hListView, &ptClient);

	LVHITTESTINFO hitTestInfo;
	hitTestInfo.pt = ptClient;
	ListView_HitTest(m_hListView, &hitTestInfo);

	BookmarkItem *parentFolder = nullptr;
	size_t position;
	bool parentFolderSelected = false;

	if (WI_IsFlagSet(hitTestInfo.flags, LVHT_NOWHERE))
	{
		parentFolder = m_currentBookmarkFolder;
		position = FindNextItemIndex(ptClient);
	}
	else
	{
		auto bookmarkItem = GetBookmarkItemFromListView(hitTestInfo.iItem);

		RECT itemRect;
		ListView_GetItemRect(m_hListView, hitTestInfo.iItem, &itemRect, LVIR_BOUNDS);

		if (bookmarkItem->IsFolder())
		{
			RECT folderCentralRect = itemRect;
			int indent =
				static_cast<int>(FOLDER_CENTRAL_RECT_INDENT_PERCENTAGE * GetRectHeight(&itemRect));
			InflateRect(&folderCentralRect, 0, -indent);

			if (ptClient.y < folderCentralRect.top)
			{
				parentFolder = m_currentBookmarkFolder;
				position = hitTestInfo.iItem;
			}
			else if (ptClient.y > folderCentralRect.bottom)
			{
				parentFolder = m_currentBookmarkFolder;
				position = hitTestInfo.iItem + 1;
			}
			else
			{
				parentFolder = bookmarkItem;
				position = bookmarkItem->GetChildren().size();
				parentFolderSelected = true;
			}
		}
		else
		{
			parentFolder = m_currentBookmarkFolder;
			position = hitTestInfo.iItem;

			if (ptClient.y > (itemRect.top + GetRectHeight(&itemRect) / 2))
			{
				position++;
			}
		}
	}

	return { parentFolder, position, parentFolderSelected };
}

int BookmarkListView::FindNextItemIndex(const POINT &ptClient)
{
	int numItems = ListView_GetItemCount(m_hListView);
	int nextIndex = 0;

	for (int i = 0; i < numItems; i++)
	{
		RECT rc;
		ListView_GetItemRect(m_hListView, i, &rc, LVIR_BOUNDS);

		if (ptClient.y < rc.top)
		{
			break;
		}

		nextIndex = i + 1;
	}

	return nextIndex;
}

void BookmarkListView::UpdateUiForDropLocation(const DropLocation &dropLocation)
{
	RemoveDropHighlight();

	if (dropLocation.parentFolder == m_currentBookmarkFolder)
	{
		DWORD flags;
		int numItems = ListView_GetItemCount(m_hListView);
		size_t finalPosition = dropLocation.position;

		if (finalPosition == static_cast<size_t>(numItems))
		{
			finalPosition--;
			flags = LVIM_AFTER;
		}
		else
		{
			flags = 0;
		}

		LVINSERTMARK insertMark;
		insertMark.cbSize = sizeof(insertMark);
		insertMark.dwFlags = flags;
		insertMark.iItem = static_cast<int>(finalPosition);
		ListView_SetInsertMark(m_hListView, &insertMark);
	}
	else
	{
		RemoveInsertionMark();

		auto selectedItemIndex = GetBookmarkItemIndex(dropLocation.parentFolder);
		CHECK(selectedItemIndex);

		ListView_SetItemState(m_hListView, *selectedItemIndex, LVIS_DROPHILITED, LVIS_DROPHILITED);
		m_previousDropItem = *selectedItemIndex;
	}
}

void BookmarkListView::ResetDropUiState()
{
	RemoveInsertionMark();
	RemoveDropHighlight();
}

void BookmarkListView::RemoveInsertionMark()
{
	LVINSERTMARK insertMark;
	insertMark.cbSize = sizeof(insertMark);
	insertMark.dwFlags = 0;
	insertMark.iItem = -1;
	ListView_SetInsertMark(m_hListView, &insertMark);
}

void BookmarkListView::RemoveDropHighlight()
{
	if (m_previousDropItem)
	{
		ListView_SetItemState(m_hListView, *m_previousDropItem, 0, LVIS_DROPHILITED);
		m_previousDropItem.reset();
	}
}
