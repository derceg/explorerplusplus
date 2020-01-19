// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkClipboard.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/StringHelper.h"

BookmarkClipboard::BookmarkClipboard()
{

}

UINT BookmarkClipboard::GetClipboardFormat()
{
	static UINT clipboardFormat = RegisterClipboardFormat(CLIPBOARD_FORMAT_STRING);
	return clipboardFormat;
}

BookmarkItems BookmarkClipboard::ReadBookmarks()
{
	Clipboard clipboard;
	auto data = clipboard.ReadCustomData(GetClipboardFormat());

	if (!data)
	{
		return {};
	}

	return BookmarkDataExchange::DeserializeBookmarkItems(*data);
}

bool BookmarkClipboard::WriteBookmarks(const OwnedRefBookmarkItems &bookmarkItems)
{
	BulkClipboardWriter clipboardWriter;

	for (auto &bookmarkItem : bookmarkItems)
	{
		if (bookmarkItem.get()->IsFolder())
		{
			clipboardWriter.WriteText(bookmarkItem.get()->GetName());
		}
		else
		{
			clipboardWriter.WriteText(bookmarkItem.get()->GetLocation());
		}
	}

	std::string data = BookmarkDataExchange::SerializeBookmarkItems(bookmarkItems);
	return clipboardWriter.WriteCustomData(GetClipboardFormat(), data);
}