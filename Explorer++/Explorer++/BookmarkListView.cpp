/******************************************************************
*
* Project: Explorer++
* File: BookmarkListView.cpp
* License: GPL - See LICENSE in the top level directory
*
* Written by David Erceg
* www.explorerplusplus.com
*
*****************************************************************/

#include "stdafx.h"
#include "BookmarkListView.h"
#include "MainImages.h"
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

	m_himl = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 48);
	HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(m_himl, hBitmap, NULL);
	ListView_SetImageList(hListView, m_himl, LVSIL_SMALL);
	DeleteObject(hBitmap);
}

CBookmarkListView::~CBookmarkListView()
{
	ImageList_Destroy(m_himl);
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
		iImage = SHELLIMAGES_NEWTAB;
	}
	else
	{
		iImage = SHELLIMAGES_FAV;
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

NBookmarkHelper::variantBookmark_t CBookmarkListView::GetBookmarkItemFromListView(CBookmarkFolder &ParentBookmarkFolder, int iItem)
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iItem = iItem;
	lvi.iSubItem = 0;
	ListView_GetItem(m_hListView, &lvi);

	NBookmarkHelper::variantBookmark_t variantBookmark = GetBookmarkItemFromListViewlParam(ParentBookmarkFolder, lvi.lParam);

	return variantBookmark;
}

NBookmarkHelper::variantBookmark_t CBookmarkListView::GetBookmarkItemFromListViewlParam(CBookmarkFolder &ParentBookmarkFolder, LPARAM lParam)
{
	auto itr = m_mapID.find(static_cast<UINT>(lParam));
	NBookmarkHelper::variantBookmark_t variantBookmark = NBookmarkHelper::GetBookmarkItem(ParentBookmarkFolder, itr->second);

	return variantBookmark;
}