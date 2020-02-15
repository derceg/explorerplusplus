// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkItem.h"
#include <optional>

class BookmarkTree;

class BookmarkDropInfo
{
public:

	BookmarkDropInfo(IDataObject *dataObject, BookmarkTree *bookmarkTree);

	DWORD GetDropEffect(BookmarkItem *parentFolder);
	DWORD PerformDrop(BookmarkItem *parentFolder, size_t position);

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

	static bool CanMoveBookmarkItemIntoFolder(BookmarkItem *bookmarkItem, BookmarkItem *parentFolder);
	ExtractedInfo &GetExtractedInfo();
	ExtractedInfo ExtractBookmarkItems();
	BookmarkItems ExtractBookmarkItemsFromCustomFormat();
	BookmarkItems ExtractBookmarkItemsFromHDrop();

	IDataObject *m_dataObject;
	BookmarkTree *m_bookmarkTree;
	std::optional<ExtractedInfo> m_extractedInfo;
};