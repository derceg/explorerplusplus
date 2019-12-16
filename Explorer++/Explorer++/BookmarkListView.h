// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkHelper.h"
#include "BookmarkTree.h"
#include "CoreInterface.h"
#include "ResourceHelper.h"
#include "../Helper/BookmarkItem.h"
#include "../Helper/DpiCompatibility.h"
#include <wil/resource.h>

class CBookmarkListView
{
public:

	CBookmarkListView(HWND hListView, IExplorerplusplus *expp);

	void InsertBookmarksIntoListView(BookmarkItem *bookmarkItem);
	BookmarkItem *GetBookmarkItemFromListView(int iItem);
	BookmarkItem *GetBookmarkItemFromListViewlParam(LPARAM lParam);

private:

	int InsertBookmarkItemIntoListView(BookmarkItem *bookmarkItem, int position);

	HWND m_hListView;
	DpiCompatibility m_dpiCompat;
	wil::unique_himagelist m_imageList;
	IconImageListMapping m_imageListMappings;
};