// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "BookmarkClipboard.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/StringHelper.h"
#include "cereal/archives/binary.hpp"
#include "cereal/types/memory.hpp"

const WCHAR BookmarkClipboard::CLIPBOARD_FORMAT_STRING[] = L"explorerplusplus/bookmark";

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

	std::stringstream ss(*data);
	cereal::BinaryInputArchive inputArchive(ss);

	std::unique_ptr<BookmarkItem> bookmarkItem;
	inputArchive(bookmarkItem);

	return bookmarkItem;
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

	std::stringstream ss;
	cereal::BinaryOutputArchive outputArchive(ss);

	outputArchive(bookmarkItem);
	bool bookmarkWritten = clipboardWriter.WriteCustomData(GetClipboardFormat(), ss.str());

	return textWritten && bookmarkWritten;
}