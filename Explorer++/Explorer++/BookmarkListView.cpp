// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkListView.h"
#include "MainResource.h"
#include "../Helper/Macros.h"

CBookmarkListView::CBookmarkListView(HWND hListView, IExplorerplusplus *expp) :
	m_hListView(hListView)
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
}

void CBookmarkListView::InsertBookmarksIntoListView(BookmarkItem *bookmarkItem)
{
	ListView_DeleteAllItems(m_hListView);

	int position = 0;

	for (auto &childItem : bookmarkItem->GetChildren())
	{
		InsertBookmarkItemIntoListView(childItem.get(), position);

		position++;
	}
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

BookmarkItem *CBookmarkListView::GetBookmarkItemFromListViewlParam(LPARAM lParam)
{
	return reinterpret_cast<BookmarkItem *>(lParam);
}