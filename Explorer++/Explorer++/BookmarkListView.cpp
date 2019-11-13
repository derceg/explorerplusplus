// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkListView.h"
#include "MainResource.h"
#include "../Helper/Macros.h"

CBookmarkListView::CBookmarkListView(HWND hListView) :
	m_hListView(hListView),
	m_uIDCounter(0)
{
	SetWindowTheme(hListView, L"Explorer", NULL);
	ListView_SetExtendedListViewStyleEx(hListView,
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT,
		LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT);

	UINT dpi = m_dpiCompat.GetDpiForWindow(hListView);
	std::tie(m_imageList, m_imageListMappings) = CreateIconImageList(16, dpi, {Icon::Folder, Icon::Bookmarks});
	ListView_SetImageList(hListView, m_imageList.get(), LVSIL_SMALL);
}

void CBookmarkListView::InsertBookmarksIntoListView(const CBookmarkFolder &BookmarkFolder)
{
	ListView_DeleteAllItems(m_hListView);
	m_uIDCounter = 0;
	m_mapID.clear();

	int iItem = 0;

	for (auto itr = BookmarkFolder.begin(); itr != BookmarkFolder.end(); ++itr)
	{
		if (itr->type() == typeid(CBookmarkFolder))
		{
			const CBookmarkFolder &CurrentBookmarkFolder = boost::get<CBookmarkFolder>(*itr);
			InsertBookmarkFolderIntoListView(CurrentBookmarkFolder, iItem);
		}
		else
		{
			const CBookmark &CurrentBookmark = boost::get<CBookmark>(*itr);
			InsertBookmarkIntoListView(CurrentBookmark, iItem);
		}

		++iItem;
	}
}

int CBookmarkListView::InsertBookmarkFolderIntoListView(const CBookmarkFolder &BookmarkFolder, int iPosition)
{
	return InsertBookmarkItemIntoListView(BookmarkFolder.GetName(),
		BookmarkFolder.GetGUID(), true, iPosition);
}

int CBookmarkListView::InsertBookmarkIntoListView(const CBookmark &Bookmark, int iPosition)
{
	return InsertBookmarkItemIntoListView(Bookmark.GetName(),
		Bookmark.GetGUID(), false, iPosition);
}

int CBookmarkListView::InsertBookmarkItemIntoListView(const std::wstring &strName,
	const GUID &guid, bool bFolder, int iPosition)
{
	assert(iPosition >= 0 && iPosition <= ListView_GetItemCount(m_hListView));

	TCHAR szName[256];
	StringCchCopy(szName, SIZEOF_ARRAY(szName), strName.c_str());

	int iImage;

	if (bFolder)
	{
		iImage = m_imageListMappings.at(Icon::Folder);
	}
	else
	{
		iImage = m_imageListMappings.at(Icon::Bookmarks);
	}

	LVITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iItem = iPosition;
	lvi.iSubItem = 0;
	lvi.iImage = iImage;
	lvi.pszText = szName;
	lvi.lParam = m_uIDCounter;
	int iItem = ListView_InsertItem(m_hListView, &lvi);

	m_mapID.insert(std::make_pair(m_uIDCounter, guid));
	++m_uIDCounter;

	return iItem;
}

VariantBookmark &CBookmarkListView::GetBookmarkItemFromListView(CBookmarkFolder &ParentBookmarkFolder, int iItem)
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iItem = iItem;
	lvi.iSubItem = 0;
	ListView_GetItem(m_hListView, &lvi);

	VariantBookmark &variantBookmark = GetBookmarkItemFromListViewlParam(ParentBookmarkFolder, lvi.lParam);

	return variantBookmark;
}

VariantBookmark &CBookmarkListView::GetBookmarkItemFromListViewlParam(CBookmarkFolder &ParentBookmarkFolder, LPARAM lParam)
{
	auto itr = m_mapID.find(static_cast<UINT>(lParam));
	VariantBookmark &variantBookmark = NBookmarkHelper::GetBookmarkItem(ParentBookmarkFolder, itr->second);

	return variantBookmark;
}