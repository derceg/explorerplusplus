// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkHelper.h"
#include "CoreInterface.h"
#include "ResourceHelper.h"
#include "../Helper/Bookmark.h"
#include "../Helper/DpiCompatibility.h"
#include <wil/resource.h>
#include <unordered_map>

class CBookmarkListView
{
public:

	CBookmarkListView(HWND hListView, IExplorerplusplus *expp);

	void							InsertBookmarksIntoListView(const CBookmarkFolder &BookmarkFolder);
	int								InsertBookmarkFolderIntoListView(const CBookmarkFolder &BookmarkFolder, int iPosition);
	int								InsertBookmarkIntoListView(const CBookmark &Bookmark, int iPosition);
	VariantBookmark					&GetBookmarkItemFromListView(CBookmarkFolder &ParentBookmarkFolder, int iItem);
	VariantBookmark					&GetBookmarkItemFromListViewlParam(CBookmarkFolder &ParentBookmarkFolder, LPARAM lParam);

private:

	int								InsertBookmarkItemIntoListView(const std::wstring &strName, const GUID &guid, bool bFolder, int iPosition);

	HWND m_hListView;
	DpiCompatibility m_dpiCompat;
	wil::unique_himagelist m_imageList;
	IconImageListMapping m_imageListMappings;

	std::unordered_map<UINT, GUID> m_mapID;
	UINT m_uIDCounter;
};