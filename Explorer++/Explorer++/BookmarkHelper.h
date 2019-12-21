// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkItem.h"
#include "BookmarkTree.h"
#include "CoreInterface.h"
#include "TabContainer.h"

namespace BookmarkHelper
{
	enum SortMode_t
	{
		SM_NAME = 1,
		SM_LOCATION = 2,
		SM_DATE_ADDED = 3,
		SM_DATE_MODIFIED = 4
	};

	/* These GUID's are statically defined, so that they will
	always be the same across separate instances of the process. */
	static TCHAR *ROOT_GUID = _T("00000000-0000-0000-0000-000000000001");
	static TCHAR *TOOLBAR_GUID = _T("00000000-0000-0000-0000-000000000002");
	static TCHAR *MENU_GUID = _T("00000000-0000-0000-0000-000000000003");
	static TCHAR *OTHER_GUID = _T("00000000-0000-0000-0000-000000000004");

	bool IsFolder(const std::unique_ptr<BookmarkItem> &bookmarkItem);
	bool IsBookmark(const std::unique_ptr<BookmarkItem> &bookmarkItem);

	int CALLBACK Sort(SortMode_t SortMode, const BookmarkItem *firstItem, const BookmarkItem *secondItem);

	void AddBookmarkItem(BookmarkTree *bookmarkTree, BookmarkItem::Type type, HMODULE resoureceModule,
		HWND parentWindow, TabContainer *tabContainer, IExplorerplusplus *coreInterface);
	void EditBookmarkItem(BookmarkItem *bookmarkItem, BookmarkTree *bookmarkTree, HMODULE resoureceModule,
		HWND parentWindow, IExplorerplusplus *coreInterface);
	void OpenBookmarkItemInNewTab(const BookmarkItem *bookmarkItem, IExplorerplusplus *expp);
}