// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/Bookmark.h"
#include "../Helper/BookmarkItem.h"

namespace NBookmarkHelper
{
	enum SortMode_t
	{
		SM_NAME = 1,
		SM_LOCATION = 2,
		SM_VISIT_DATE = 3,
		SM_VISIT_COUNT = 4,
		SM_ADDED = 5,
		SM_LAST_MODIFIED = 6
	};

	/* These GUID's are statically defined, so that they will
	always be the same across separate instances of the process. */
	static TCHAR *ROOT_GUID = _T("00000000-0000-0000-0000-000000000001");
	static TCHAR *TOOLBAR_GUID = _T("00000000-0000-0000-0000-000000000002");
	static TCHAR *MENU_GUID = _T("00000000-0000-0000-0000-000000000003");

	bool IsFolder(const std::unique_ptr<BookmarkItem> &bookmarkItem);
	bool IsBookmark(const std::unique_ptr<BookmarkItem> &bookmarkItem);

	int CALLBACK Sort(SortMode_t SortMode, const VariantBookmark &BookmarkItem1, const VariantBookmark &BookmarkItem2);

	VariantBookmark &GetBookmarkItem(CBookmarkFolder &ParentBookmarkFolder, const std::wstring &guid);
}