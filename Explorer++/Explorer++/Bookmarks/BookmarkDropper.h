// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include <optional>

class BookmarkTree;

class BookmarkDropper
{
public:
	BookmarkDropper(IDataObject *dataObject, BookmarkTree *bookmarkTree);

	void SetBlockDrop(bool blockDrop);
	DWORD GetDropEffect(const BookmarkItem *targetFolder, size_t index);
	DWORD PerformDrop(BookmarkItem *targetFolder, size_t index);

private:
	enum class ExtractionSource
	{
		CustomFormat,
		HDrop
	};

	struct ExtractedInfo
	{
		BookmarkItems bookmarkItems;
		std::optional<ExtractionSource> extractionSource;
	};

	static bool CanDropBookmarkItemAtLocation(const BookmarkItem *bookmarkItem,
		const BookmarkItem *targetFolder, size_t index);
	ExtractedInfo &GetExtractedInfo();
	ExtractedInfo ExtractBookmarkItems();
	BookmarkItems ExtractBookmarkItemsFromCustomFormat();
	BookmarkItems ExtractBookmarkItemsFromHDrop();

	IDataObject *m_dataObject;
	BookmarkTree *m_bookmarkTree;
	std::optional<ExtractedInfo> m_extractedInfo;
	bool m_blockDrop;
};
