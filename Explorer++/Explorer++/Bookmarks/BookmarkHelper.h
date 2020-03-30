// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include <optional>

class BookmarkTree;
__interface IExplorerplusplus;
class TabContainer;

using RawBookmarkItems = std::vector<BookmarkItem *>;

namespace BookmarkHelper
{
enum class ColumnType
{
	Default = 0,
	Name = 1,
	Location = 2,
	DateCreated = 3,
	DateModified = 4
};

bool IsFolder(const std::unique_ptr<BookmarkItem> &bookmarkItem);
bool IsBookmark(const std::unique_ptr<BookmarkItem> &bookmarkItem);

int CALLBACK Sort(
	ColumnType columnType, const BookmarkItem *firstItem, const BookmarkItem *secondItem);

void BookmarkAllTabs(BookmarkTree *bookmarkTree, HMODULE resoureceModule, HWND parentWindow,
	IExplorerplusplus *coreInterface);
BookmarkItem *AddBookmarkItem(BookmarkTree *bookmarkTree, BookmarkItem::Type type,
	BookmarkItem *defaultParentSelection, std::optional<size_t> suggestedIndex, HWND parentWindow,
	IExplorerplusplus *coreInterface, std::optional<std::wstring> customDialogTitle = std::nullopt);
void EditBookmarkItem(BookmarkItem *bookmarkItem, BookmarkTree *bookmarkTree,
	HMODULE resoureceModule, HWND parentWindow, IExplorerplusplus *coreInterface);
void OpenBookmarkItemInNewTab(const BookmarkItem *bookmarkItem, IExplorerplusplus *expp);

bool CopyBookmarkItems(BookmarkTree *bookmarkTree, const RawBookmarkItems &bookmarkItems, bool cut);
void PasteBookmarkItems(BookmarkTree *bookmarkTree, BookmarkItem *parentFolder, size_t index);

BookmarkItem *GetBookmarkItemById(BookmarkTree *bookmarkTree, std::wstring_view guid);

bool IsAncestor(BookmarkItem *bookmarkItem, BookmarkItem *possibleAncestor);
}