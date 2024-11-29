// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkContextMenu.h"
#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/UI/BookmarkDropTargetWindow.h"
#include "Bookmarks/UI/BookmarkMenu.h"
#include <boost/signals2.hpp>
#include <memory>
#include <vector>

class BookmarkIconManager;
class BookmarksToolbarView;
class BookmarkTree;
class BrowserWindow;
class CoreInterface;
class IconFetcher;
class IconResourceLoader;
struct MouseEvent;
class ThemeManager;

class BookmarksToolbar : private BookmarkDropTargetWindow
{
public:
	static BookmarksToolbar *Create(BookmarksToolbarView *view, BrowserWindow *browserWindow,
		CoreInterface *coreInterface, const IconResourceLoader *iconResourceLoader,
		IconFetcher *iconFetcher, BookmarkTree *bookmarkTree, ThemeManager *themeManager);

	BookmarksToolbar(const BookmarksToolbar &) = delete;
	BookmarksToolbar(BookmarksToolbar &&) = delete;
	BookmarksToolbar &operator=(const BookmarksToolbar &) = delete;
	BookmarksToolbar &operator=(BookmarksToolbar &&) = delete;

	BookmarksToolbarView *GetView() const;

	void ShowOverflowMenu(const POINT &ptScreen);

private:
	// When an item is dragged over a folder on the bookmarks toolbar, the drop target should be set
	// to the folder only if the dragged item is over the main part of the button for the folder.
	// This is to allow the dragged item to be positioned before or after the folder if the item is
	// currently over the left or right edge of the button.
	// This is especially important when there's no horizontal padding between buttons, as there
	// would be no space before or after the button that would allow you to correctly set the
	// position.
	// The constant here represents how far the left/right edges of the button are indented, as a
	// percentage of the total size of the button, in order to determine whether an item is over the
	// main portion of the button.
	static constexpr double FOLDER_CENTRAL_RECT_INDENT_PERCENTAGE = 0.2;

	BookmarksToolbar(BookmarksToolbarView *view, BrowserWindow *browserWindow,
		CoreInterface *coreInterface, const IconResourceLoader *iconResourceLoader,
		IconFetcher *iconFetcher, BookmarkTree *bookmarkTree, ThemeManager *themeManager);

	void Initialize(IconFetcher *iconFetcher);
	void AddBookmarkItems();
	void AddBookmarkItem(BookmarkItem *bookmarkItem, size_t index);

	void OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index);
	void OnBookmarkItemUpdated(BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType);
	void OnBookmarkItemMoved(BookmarkItem *bookmarkItem, const BookmarkItem *oldParent,
		size_t oldIndex, const BookmarkItem *newParent, size_t newIndex);
	void OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem);

	void OnBookmarkClicked(BookmarkItem *bookmarkItem, const MouseEvent &event);
	void OnBookmarkFolderClicked(BookmarkItem *bookmarkItem, const MouseEvent &event);
	void OnButtonMiddleClicked(const BookmarkItem *bookmarkItem, const MouseEvent &event);
	void OnButtonRightClicked(BookmarkItem *bookmarkItem, const MouseEvent &event);

	void OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow, const POINT &pt);
	void OnToolbarContextMenuItemSelected(HWND sourceWindow, int menuItemId);

	void OnNewBookmarkItem(BookmarkItem::Type type, size_t targetIndex);
	void OnPaste(size_t targetIndex);

	void OnWindowDestroyed();

	void OnButtonDragStarted(const BookmarkItem *bookmarkItem);

	// BookmarkDropTargetWindow
	DropLocation GetDropLocation(const POINT &pt) override;
	void UpdateUiForDropLocation(const DropLocation &dropLocation) override;
	void ResetDropUiState() override;

	void RemoveDropHighlight();

	BookmarksToolbarView *m_view = nullptr;
	BrowserWindow *m_browserWindow = nullptr;
	CoreInterface *m_coreInterface = nullptr;
	const IconResourceLoader *const m_iconResourceLoader;
	BookmarkTree *m_bookmarkTree = nullptr;
	ThemeManager *const m_themeManager;

	std::unique_ptr<BookmarkIconManager> m_bookmarkIconManager;
	BookmarkContextMenu m_contextMenu;
	BookmarkMenu m_bookmarkMenu;

	std::optional<POINT> m_contextMenuLocation;

	// Drag and drop
	BookmarkItem *m_dropTargetFolder = nullptr;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
