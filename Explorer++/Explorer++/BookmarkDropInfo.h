// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkItem.h"
#include "BookmarkTree.h"
#include <optional>

class BookmarkDropInfo
{
public:

	BookmarkDropInfo(IDataObject *dataObject, BookmarkTree *bookmarkTree);

	DWORD GetDropEffect(BookmarkItem *parentFolder);
	DWORD PerformDrop(BookmarkItem *parentFolder, size_t position);

private:

	DWORD DetermineDropEffect();

	BookmarkItems ExtractBookmarkItems();
	std::unique_ptr<BookmarkItem> ExtractBookmarkItemFromCustomFormat();
	BookmarkItems ExtractBookmarkItemsFromHDrop();

	IDataObject *m_dataObject;
	BookmarkTree *m_bookmarkTree;
	std::optional<DWORD> m_dropEffect;
};