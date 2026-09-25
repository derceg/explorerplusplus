// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/UI/BookmarkMenuBuilder.h"
#include "MenuBase.h"
#include "MenuDelegate.h"
#include "../Helper/WeakPtr.h"
#include <unordered_map>

class BookmarkIconManager;
class BookmarkItem;
class BookmarkTree;
class BrowserWindow;
class PlatformContext;
class ResourceLoader;

class BookmarksMenu : public MenuBase, private MenuDelegate
{
public:
	// Note that if an IncludePredicate is provided, it only applies to immediate children in the
	// folder. That is, the predicate doesn't apply recursively.
	BookmarksMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
		BookmarkTree *bookmarkTree, BookmarkItem *bookmarkFolder,
		BookmarkIconManager *bookmarkIconManager, BrowserWindow *browser, HWND parentWindow,
		PlatformContext *platformContext, const ResourceLoader *resourceLoader,
		BookmarkMenuBuilder::IncludePredicate includePredicate = {},
		UINT startId = DEFAULT_START_ID, UINT endId = DEFAULT_END_ID);

	UINT GetNextId() const;

private:
	enum class MenuItemType
	{
		// This item represents a bookmark/bookmark folder.
		BookmarkItem,

		// This is used when the parent folder contains no items. The associated BookmarkItem will
		// refer to the parent folder.
		EmptyItem
	};

	struct MenuItemEntry
	{
		MenuItemEntry(WeakPtr<BookmarkItem> bookmarkItem, MenuItemType menuItemType) :
			bookmarkItem(bookmarkItem),
			menuItemType(menuItemType)
		{
		}

		WeakPtr<BookmarkItem> bookmarkItem;
		MenuItemType menuItemType;
	};

	void BuildMenu(MenuView *menuView, BookmarkItem *bookmarkFolder,
		BookmarkMenuBuilder::IncludePredicate includePredicate = {});
	void AddEmptyItem(MenuView *menuView, BookmarkItem *bookmarkFolder);

	// MenuDelegate
	bool IsItemEnabled(UINT id) const override;
	void OnItemSelected(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown) override;
	void OnItemMiddleClicked(UINT id, bool isCtrlKeyDown, bool isShiftKeyDown) override;
	void OnItemRightClicked(UINT id, const POINT &ptScreen) override;
	MenuDragAction OnItemDragged(UINT id) override;
	wil::com_ptr_nothrow<IDropTarget> MaybeGetDropTargetForLocation(
		const MenuDropLocation &dropLocation) override;

	BookmarkItem *MaybeGetBookmarkItemForMenuItem(UINT id);
	const MenuItemEntry *GetEntryForMenuItem(UINT id) const;

	UINT m_idCounter;
	BookmarkTree *const m_bookmarkTree;
	BookmarkIconManager *const m_bookmarkIconManager;
	BrowserWindow *const m_browser;
	const HWND m_parentWindow;
	PlatformContext *const m_platformContext;
	const ResourceLoader *const m_resourceLoader;
	std::unordered_map<UINT, MenuItemEntry> m_idToBookmarkMap;
};
