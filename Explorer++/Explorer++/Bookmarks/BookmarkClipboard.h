// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkDataExchange.h"
#include "Bookmarks/BookmarkItem.h"

class ClipboardStore;

class BookmarkClipboard
{
public:
	BookmarkClipboard(ClipboardStore *clipboardStore);

	BookmarkItems ReadBookmarks();
	bool WriteBookmarks(const OwnedRefBookmarkItems &bookmarkItems);

	static UINT GetClipboardFormat();

private:
	static inline const wchar_t CLIPBOARD_FORMAT_STRING[] = L"explorerplusplus/bookmarks";

	ClipboardStore *const m_clipboardStore;
};
