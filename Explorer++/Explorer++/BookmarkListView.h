#pragma once

#include "BookmarkHelper.h"
#include "../Helper/Bookmark.h"
#include <unordered_map>

class CBookmarkListView
{
public:

	CBookmarkListView(HWND hListView);
	~CBookmarkListView();

	void							InsertBookmarksIntoListView(const CBookmarkFolder &BookmarkFolder);
	int								InsertBookmarkFolderIntoListView(const CBookmarkFolder &BookmarkFolder, int iPosition);
	int								InsertBookmarkIntoListView(const CBookmark &Bookmark, int iPosition);
	NBookmarkHelper::variantBookmark_t	GetBookmarkItemFromListView(CBookmarkFolder &ParentBookmarkFolder, int iItem);
	NBookmarkHelper::variantBookmark_t	GetBookmarkItemFromListViewlParam(CBookmarkFolder &ParentBookmarkFolder, LPARAM lParam);

private:

	int								InsertBookmarkItemIntoListView(const std::wstring &strName, const GUID &guid, bool bFolder, int iPosition);

	HWND							m_hListView;
	HIMAGELIST						m_himl;

	std::unordered_map<UINT, GUID>	m_mapID;
	UINT							m_uIDCounter;
};