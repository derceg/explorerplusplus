// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkHelper.h"
#include "Bookmarks/BookmarkItem.h"

class ClipboardStore;

class BookmarkClipboard
{
public:
	BookmarkClipboard(ClipboardStore *clipboardStore);

	BookmarkItems ReadBookmarks();
	bool WriteBookmarks(const RawBookmarkItems &bookmarkItems);

	static UINT GetClipboardFormat();

private:
	// Previously, a cereal binary archive was used to store serialized bookmarks. Now, they're
	// represented using JSON. Hence v2 in the name below.
	static constexpr wchar_t CLIPBOARD_FORMAT_STRING[] = L"explorerplusplus/bookmarks-v2";

	ClipboardStore *const m_clipboardStore;
};
