// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkClipboard.h"
#include "BookmarkDataExchange.h"
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

std::unique_ptr<BookmarkItem> BookmarkClipboard::ReadBookmark()
{
	Clipboard clipboard;
	auto data = clipboard.ReadCustomData(GetClipboardFormat());

	if (!data)
	{
		return nullptr;
	}

	return BookmarkDataExchange::DeserializeBookmarkItem(*data);
}

bool BookmarkClipboard::WriteBookmark(const std::unique_ptr<BookmarkItem> &bookmarkItem)
{
	BulkClipboardWriter clipboardWriter;
	bool textWritten;

	if (bookmarkItem->IsFolder())
	{
		textWritten = clipboardWriter.WriteText(bookmarkItem->GetName());
	}
	else
	{
		textWritten = clipboardWriter.WriteText(bookmarkItem->GetLocation());
	}

	std::string data = BookmarkDataExchange::SerializeBookmarkItem(bookmarkItem);
	bool bookmarkWritten = clipboardWriter.WriteCustomData(GetClipboardFormat(), data);

	return textWritten && bookmarkWritten;
}