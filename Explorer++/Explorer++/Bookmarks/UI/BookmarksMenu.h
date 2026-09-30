// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "MenuBase.h"
#include "MenuItemCallbacks.h"
#include "../Helper/WeakPtr.h"
#include <functional>

class BookmarkIconManager;
class BookmarkItem;
class BookmarkTree;
class BrowserWindow;
class PlatformContext;
class ResourceLoader;

class BookmarksMenu : public MenuBase
{
public:
	using IncludePredicate = std::function<bool(const BookmarkItem *bookmarkItem)>;

	// Note that if an IncludePredicate is provided, it only applies to immediate children in the
	// folder. That is, the predicate doesn't apply recursively.
	BookmarksMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BookmarkTree *bookmarkTree, BookmarkItem *bookmarkFolder,
		BookmarkIconManager *bookmarkIconManager, BrowserWindow *browser, HWND parentWindow,
		PlatformContext *platformContext, const ResourceLoader *resourceLoader,
		IncludePredicate includePredicate = {});

private:
	enum class MenuItemType
	{
		// This item represents a bookmark/bookmark folder.
		BookmarkItem,

		// This is used when the parent folder contains no items.
		EmptyItem
	};

	void BuildMenu(MenuView *menuView, BookmarkItem *bookmarkFolder,
		IncludePredicate includePredicate = {});
	void AddEmptyItem(MenuView *menuView, BookmarkItem *bookmarkFolder);
	void AddBookmarkItem(MenuView *menuView, BookmarkItem *bookmarkItem);

	void OnBookmarkSelected(WeakPtr<BookmarkItem> bookmark, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	void OnBookmarkItemMiddleClicked(WeakPtr<BookmarkItem> bookmarkItem, bool isCtrlKeyDown,
		bool isShiftKeyDown);
	void OnBookmarkItemRightClicked(WeakPtr<BookmarkItem> bookmarkItem, const POINT &ptScreen);
	MenuDragAction OnBookmarkItemDragged(WeakPtr<BookmarkItem> bookmarkItem);
	wil::com_ptr_nothrow<IDropTarget> MaybeGetDropTarget(WeakPtr<BookmarkItem> bookmarkItem,
		MenuItemType itemType, MenuDropPosition position);

	BookmarkTree *const m_bookmarkTree;
	BookmarkIconManager *const m_bookmarkIconManager;
	BrowserWindow *const m_browser;
	const HWND m_parentWindow;
	PlatformContext *const m_platformContext;
	const ResourceLoader *const m_resourceLoader;
};
