// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkDataExchange.h"
#include "BookmarkItem.h"

class BookmarkClipboard
{
public:

	BookmarkItems ReadBookmarks();
	bool WriteBookmarks(const OwnedRefBookmarkItems &bookmarkItems);

	static UINT GetClipboardFormat();

private:

	static inline const WCHAR CLIPBOARD_FORMAT_STRING[] = L"explorerplusplus/bookmarks";
};