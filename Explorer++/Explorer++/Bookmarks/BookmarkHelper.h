// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "NavigationHelper.h"
#include "../Helper/FileOperations.h"
#include <optional>

class AcceleratorManager;
class BookmarkTree;
class BrowserWindow;
class ClipboardStore;
class ResourceLoader;
class TabContainer;

using RawBookmarkItems = std::vector<BookmarkItem *>;

namespace BookmarkHelper
{

bool IsFolder(const std::unique_ptr<BookmarkItem> &bookmarkItem);
bool IsBookmark(const std::unique_ptr<BookmarkItem> &bookmarkItem);

void BookmarkAllTabs(BookmarkTree *bookmarkTree, const ResourceLoader *resourceLoader,
	HWND parentWindow, BrowserWindow *browser, ClipboardStore *clipboardStore,
	const AcceleratorManager *acceleratorManager);
BookmarkItem *AddBookmarkItem(BookmarkTree *bookmarkTree, BookmarkItem::Type type,
	BookmarkItem *defaultParentSelection, std::optional<size_t> suggestedIndex, HWND parentWindow,
	BrowserWindow *browser, ClipboardStore *clipboardStore,
	const AcceleratorManager *acceleratorManager, const ResourceLoader *resourceLoader,
	std::optional<std::wstring> customDialogTitle = std::nullopt);
void EditBookmarkItem(BookmarkItem *bookmarkItem, BookmarkTree *bookmarkTree,
	ClipboardStore *clipboardStore, const AcceleratorManager *acceleratorManager,
	const ResourceLoader *resourceLoader, HWND parentWindow);
void RemoveBookmarks(BookmarkTree *bookmarkTree, const RawBookmarkItems &bookmarkItems);

void OpenBookmarkItemWithDisposition(const BookmarkItem *bookmarkItem,
	OpenFolderDisposition disposition, BrowserWindow *browser);

bool CopyBookmarkItems(ClipboardStore *clipboardStore, BookmarkTree *bookmarkTree,
	const RawBookmarkItems &bookmarkItems, ClipboardAction action);
void PasteBookmarkItems(ClipboardStore *clipboardStore, BookmarkTree *bookmarkTree,
	BookmarkItem *parentFolder, size_t index);

bool IsAncestor(const BookmarkItem *bookmarkItem, const BookmarkItem *possibleAncestor);

}
