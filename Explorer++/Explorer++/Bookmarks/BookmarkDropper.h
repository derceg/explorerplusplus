// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include <wil/com.h>
#include <ShObjIdl.h>
#include <memory>
#include <optional>

class BookmarkTree;

class BookmarkDropper
{
public:
	BookmarkDropper(IDataObject *dataObject, DWORD allowedEffects, BookmarkTree *bookmarkTree);

	void SetBlockDrop(bool blockDrop);
	DWORD GetDropEffect(const BookmarkItem *targetFolder, size_t index);
	DWORD PerformDrop(BookmarkItem *targetFolder, size_t index);

private:
	enum class ExtractionSource
	{
		CustomFormat,
		Other
	};

	struct ExtractedInfo
	{
		BookmarkItems bookmarkItems;
		ExtractionSource extractionSource;
	};

	static bool CanDropBookmarkItemAtLocation(const BookmarkItem *bookmarkItem,
		const BookmarkItem *targetFolder, size_t index);
	ExtractedInfo &GetExtractedInfo();
	ExtractedInfo ExtractBookmarkItems();
	BookmarkItems ExtractBookmarkItemsFromCustomFormat();
	BookmarkItems MaybeExtractBookmarkItemsFromShellItems();
	std::unique_ptr<BookmarkItem> MaybeBuildBookmarkItemFromShellItem(IShellItem *shellItem);

	wil::com_ptr_nothrow<IDataObject> m_dataObject;
	DWORD m_allowedEffects;
	BookmarkTree *m_bookmarkTree;
	std::optional<ExtractedInfo> m_extractedInfo;
	bool m_blockDrop;
};
