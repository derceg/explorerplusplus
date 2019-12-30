// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkItem.h"

class BookmarkClipboard
{
public:

	BookmarkClipboard();

	std::unique_ptr<BookmarkItem> ReadBookmark();
	bool WriteBookmark(const std::unique_ptr<BookmarkItem> &bookmarkItem);

	static UINT GetClipboardFormat();

private:

	static const WCHAR CLIPBOARD_FORMAT_STRING[];
};