// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkListView.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/indexed.hpp>

CBookmarkListView::CBookmarkListView(HWND hListView, HMODULE resourceModule,
	BookmarkTree *bookmarkTree, IExplorerplusplus *expp, const std::vector<Column> &initialColumns) :
	m_hListView(hListView),
	m_resourceModule(resourceModule),
	m_bookmarkTree(bookmarkTree),
	m_expp(expp),
	m_columns(initialColumns),
	m_sortMode(BookmarkHelper::SortMode::Default),
	m_sortAscending(true)
{
	SetWindowTheme(hListView, L"Explorer", NULL);
	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT,
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT);

	UINT dpi = m_dpiCompat.GetDpiForWindow(hListView);
	int iconWidth = m_dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = m_dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	std::tie(m_imageList, m_imageListMappings) = ResourceHelper::CreateIconImageList(expp->GetIconResourceLoader(),
		iconWidth, iconHeight, {Icon::Folder, Icon::Bookmarks});
	ListView_SetImageList(hListView, m_imageList.get(), LVSIL_SMALL);

	InsertColumns(initialColumns);

	m_windowSubclasses.push_back(WindowSubclassWrapper(GetParent(m_hListView), ParentWndProcStub,
		PARENT_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));
}

void CBookmarkListView::InsertColumns(const std::vector<Column> &columns)
{
	for (const auto &column : columns | boost::adaptors::filtered(IsColumnActive) | boost::adaptors::indexed(0))
	{
		InsertColumn(column.value(), static_cast<int>(column.index()));
	}
}

void CBookmarkListView::InsertColumn(const Column &column, int index)
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

std::wstring CBookmarkListView::GetColumnText(ColumnType columnType)
{
	UINT resourceId = GetColumnTextResourceId(columnType);
	return ResourceHelper::LoadString(m_resourceModule, resourceId);
}

std::vector<CBookmarkListView::Column> CBookmarkListView::GetColumns()
{
	int index = 0;

	for (auto &column : m_columns | boost::adaptors::filtered(IsColumnActive))
	{
		column.width = ListView_GetColumnWidth(m_hListView, index++);
	}

	return m_columns;
}

bool CBookmarkListView::IsColumnActive(const Column &column)
{
	return column.active;
}

std::optional<CBookmarkListView::ColumnType> CBookmarkListView::GetColumnTypeByIndex(int index) const
{
	HWND hHeader = ListView_GetHeader(m_hListView);

	HDITEM hdItem;
	hdItem.mask = HDI_LPARAM;
	BOOL res = Header_GetItem(hHeader, index, &hdItem);

	if (!res)
	{
		return std::nullopt;
	}

	return static_cast<ColumnType>(hdItem.lParam);
}

UINT CBookmarkListView::GetColumnTextResourceId(ColumnType columnType)
{
	switch (columnType)
	{
	case ColumnType::Name:
		return IDS_BOOKMARKS_COLUMN_NAME;
		break;

	case ColumnType::Location:
		return IDS_BOOKMARKS_COLUMN_LOCATION;
		break;

	case ColumnType::DateCreated:
		return IDS_BOOKMARKS_COLUMN_DATE_CREATED;
		break;

	case ColumnType::DateModified:
		return IDS_BOOKMARKS_COLUMN_DATE_MODIFIED;
		break;
	}

	throw std::runtime_error("Bookmark column string resource not found");
}

LRESULT CALLBACK CBookmarkListView::ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CBookmarkListView *listView = reinterpret_cast<CBookmarkListView *>(dwRefData);
	return listView->ParentWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CBookmarkListView::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

			case NM_RCLICK:
				OnRClick(reinterpret_cast<NMITEMACTIVATE *>(lParam));
				return TRUE;
				break;

			case LVN_GETDISPINFO:
				OnGetDispInfo(reinterpret_cast<NMLVDISPINFO *>(lParam));
				break;

			case LVN_BEGINLABELEDIT:
				return OnBeginLabelEdit(reinterpret_cast<NMLVDISPINFO *>(lParam));
				break;

			case LVN_ENDLABELEDIT:
				return OnEndLabelEdit(reinterpret_cast<NMLVDISPINFO *>(lParam));
				break;

			case LVN_KEYDOWN:
				OnKeyDown(reinterpret_cast<NMLVKEYDOWN *>(lParam));
				break;
			}
		}
		else if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == ListView_GetHeader(m_hListView))
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_RCLICK:
			{
				POINT pt;
				DWORD messagePos = GetMessagePos();
				POINTSTOPOINT(pt, MAKEPOINTS(messagePos));
				OnHeaderRClick(pt);
				return TRUE;
			}
			break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void CBookmarkListView::NavigateToBookmarkFolder(BookmarkItem *bookmarkFolder)
{
	assert(bookmarkFolder->IsFolder());

	m_currentBookmarkFolder = bookmarkFolder;

	ListView_DeleteAllItems(m_hListView);

	int position = 0;

	for (auto &childItem : bookmarkFolder->GetChildren())
	{
		InsertBookmarkItemIntoListView(childItem.get(), position);

		position++;
	}

	navigationSignal.m_signal(bookmarkFolder);
}

int CBookmarkListView::InsertBookmarkItemIntoListView(BookmarkItem *bookmarkItem, int position)
{
	assert(position >= 0 && position <= ListView_GetItemCount(m_hListView));

	TCHAR szName[256];
	StringCchCopy(szName, SIZEOF_ARRAY(szName), bookmarkItem->GetName().c_str());

	int iImage;

	if (bookmarkItem->GetType() == BookmarkItem::Type::Folder)
	{
		iImage = m_imageListMappings.at(Icon::Folder);
	}
	else
	{
		iImage = m_imageListMappings.at(Icon::Bookmarks);
	}

	LVITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iItem = position;
	lvi.iSubItem = 0;
	lvi.iImage = iImage;
	lvi.pszText = szName;
	lvi.lParam = reinterpret_cast<LPARAM>(bookmarkItem);
	int iItem = ListView_InsertItem(m_hListView, &lvi);

	return iItem;
}

BookmarkItem *CBookmarkListView::GetBookmarkItemFromListView(int iItem)
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iItem = iItem;
	lvi.iSubItem = 0;
	ListView_GetItem(m_hListView, &lvi);

	return reinterpret_cast<BookmarkItem *>(lvi.lParam);
}

void CBookmarkListView::SortItems()
{
	ListView_SortItems(m_hListView, SortBookmarksStub, reinterpret_cast<LPARAM>(this));
}

int CALLBACK CBookmarkListView::SortBookmarksStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CBookmarkListView *listView = reinterpret_cast<CBookmarkListView *>(lParamSort);

	return listView->SortBookmarks(lParam1, lParam2);
}

int CALLBACK CBookmarkListView::SortBookmarks(LPARAM lParam1, LPARAM lParam2)
{
	auto firstItem = reinterpret_cast<BookmarkItem *>(lParam1);
	auto secondItem = reinterpret_cast<BookmarkItem *>(lParam2);

	int iRes = BookmarkHelper::Sort(m_sortMode, firstItem, secondItem);

	// When using the default sort mode (in which items are sorted according to
	// their position within the parent folder), the items are always in
	// ascending order.
	if (!m_sortAscending && m_sortMode != BookmarkHelper::SortMode::Default)
	{
		iRes = -iRes;
	}

	return iRes;
}

BookmarkHelper::SortMode CBookmarkListView::GetSortMode() const
{
	return m_sortMode;
}

void CBookmarkListView::SetSortMode(BookmarkHelper::SortMode sortMode)
{
	m_sortMode = sortMode;

	SortItems();
}

bool CBookmarkListView::GetSortAscending() const
{
	return m_sortAscending;
}

void CBookmarkListView::SetSortAscending(bool sortAscending)
{
	m_sortAscending = sortAscending;

	SortItems();
}

void CBookmarkListView::OnDblClk(const NMITEMACTIVATE *itemActivate)
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
		Tab &selectedTab = m_expp->GetTabContainer()->GetSelectedTab();
		selectedTab.GetShellBrowser()->GetNavigationController()->BrowseFolder(bookmarkItem->GetLocation().c_str());
	}
}

void CBookmarkListView::OnRClick(const NMITEMACTIVATE *itemActivate)
{
	HMENU hMenu = LoadMenu(m_resourceModule, MAKEINTRESOURCE(IDR_MANAGEBOOKMARKS_BOOKMARK_RCLICK_MENU));
	SetMenuDefaultItem(GetSubMenu(hMenu, 0), IDM_MB_BOOKMARK_OPEN, FALSE);

	POINT pt = itemActivate->ptAction;
	ClientToScreen(m_hListView, &pt);

	TrackPopupMenu(GetSubMenu(hMenu, 0), TPM_LEFTALIGN, pt.x, pt.y, 0, m_hListView, NULL);
	DestroyMenu(hMenu);
}

void CBookmarkListView::OnGetDispInfo(NMLVDISPINFO *dispInfo)
{
	if (WI_IsFlagSet(dispInfo->item.mask, LVIF_TEXT))
	{
		auto bookmarkItem = GetBookmarkItemFromListView(dispInfo->item.iItem);

		auto columnType = GetColumnTypeByIndex(dispInfo->item.iSubItem);
		assert(columnType);

		std::wstring columnText = GetBookmarkItemColumnInfo(bookmarkItem, *columnType);

		StringCchCopy(dispInfo->item.pszText, dispInfo->item.cchTextMax, columnText.c_str());

		WI_SetFlag(dispInfo->item.mask, LVIF_DI_SETITEM);
	}
}

std::wstring CBookmarkListView::GetBookmarkItemColumnInfo(const BookmarkItem *bookmarkItem,
	ColumnType columnType)
{
	switch (columnType)
	{
	case ColumnType::Name:
		return bookmarkItem->GetName();
		break;

	case ColumnType::Location:
		if (bookmarkItem->IsBookmark())
		{
			return bookmarkItem->GetLocation();
		}
		else
		{
			return std::wstring();
		}
		break;

	case ColumnType::DateCreated:
	{
		FILETIME dateCreated = bookmarkItem->GetDateCreated();
		return FormatDate(&dateCreated);
	}
	break;

	case ColumnType::DateModified:
	{
		FILETIME dateModified = bookmarkItem->GetDateModified();
		return FormatDate(&dateModified);
	}
	break;
	}

	throw std::runtime_error("Bookmark column type not found");
}

std::wstring CBookmarkListView::FormatDate(const FILETIME *date)
{
	/* TODO: Friendly dates. */
	TCHAR formattedDate[256];
	BOOL res = CreateFileTimeString(date, formattedDate, std::size(formattedDate), FALSE);

	if (res)
	{
		return formattedDate;
	}

	return std::wstring();
}

BOOL CBookmarkListView::OnBeginLabelEdit(const NMLVDISPINFO *dispInfo)
{
	auto bookmarkItem = GetBookmarkItemFromListView(dispInfo->item.iItem);

	if (m_bookmarkTree->IsPermanentNode(bookmarkItem))
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CBookmarkListView::OnEndLabelEdit(const NMLVDISPINFO *dispInfo)
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

void CBookmarkListView::OnKeyDown(const NMLVKEYDOWN *keyDown)
{
	switch (keyDown->wVKey)
	{
	case VK_F2:
		OnRename();
		break;

	case 'A':
		if (IsKeyDown(VK_CONTROL) &&
			!IsKeyDown(VK_SHIFT) &&
			!IsKeyDown(VK_MENU))
		{
			NListView::ListView_SelectAllItems(m_hListView, TRUE);
		}
		break;

	/* TODO: */
	case VK_RETURN:
		break;

	case VK_DELETE:
		break;
	}
}

void CBookmarkListView::OnRename()
{
	int item = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);

	if (item != -1)
	{
		ListView_EditLabel(m_hListView, item);
	}
}

void CBookmarkListView::OnHeaderRClick(const POINT &pt)
{
	auto menu = BuildHeaderContextMenu();

	if (!menu)
	{
		return;
	}

	int cmd = TrackPopupMenu(menu.get(), TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, m_hListView, nullptr);

	if (cmd != 0)
	{
		OnHeaderContextMenuItemSelected(cmd);
	}
}

wil::unique_hmenu CBookmarkListView::BuildHeaderContextMenu()
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

		/* The name column cannot be removed. */
		if (column.columnType == ColumnType::Name)
		{
			mii.fState |= MFS_DISABLED;
		}

		InsertMenuItem(menu.get(), index++, TRUE, &mii);
	}

	return menu;
}

void CBookmarkListView::OnHeaderContextMenuItemSelected(int menuItemId)
{
	ColumnType columnType = static_cast<ColumnType>(menuItemId);

	auto itr = std::find_if(m_columns.begin(), m_columns.end(), [columnType] (const Column &column) {
		return column.columnType == columnType;
	});

	assert(itr != m_columns.end());

	auto index = std::count_if(m_columns.begin(), itr, [] (const Column &column) {
		return column.active;
	});

	bool nowActive = !itr->active;

	if (nowActive)
	{
		InsertColumn(*itr, static_cast<int>(index));

		// TODO: Update.
		/*for (const auto &childItem : bookmarkFolder->GetChildren())
		{
			TCHAR szColumn[256];
			GetBookmarkItemColumnInfo(childItem.get(), itr->ColumnType, szColumn, SIZEOF_ARRAY(szColumn));
			ListView_SetItemText(hListView, iBookmarkItem, iColumn, szColumn);

			++iBookmarkItem;
		}*/
	}
	else
	{
		itr->width = ListView_GetColumnWidth(m_hListView, index);
		ListView_DeleteColumn(m_hListView, index);
	}

	itr->active = nowActive;
}