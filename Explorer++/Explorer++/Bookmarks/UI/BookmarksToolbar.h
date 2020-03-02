// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Bookmarks/BookmarkItem.h"
#include "Bookmarks/UI/BookmarkContextMenu.h"
#include "Bookmarks/UI/BookmarkDropTargetWindow.h"
#include "Bookmarks/UI/BookmarkMenu.h"
#include "Navigation.h"
#include "ResourceHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/DropHandler.h"
#include "../Helper/IconFetcher.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <boost/signals2.hpp>
#include <wil/com.h>
#include <wil/resource.h>
#include <optional>

class BookmarkTree;
__interface IExplorerplusplus;

class BookmarksToolbar : private BookmarkDropTargetWindow
{
public:
	BookmarksToolbar(HWND hToolbar, HINSTANCE instance, IExplorerplusplus *pexpp,
		Navigation *navigation, BookmarkTree *bookmarkTree, UINT uIDStart, UINT uIDEnd);

private:
	BookmarksToolbar &operator=(const BookmarksToolbar &bt);

	using SystemIconImageListMapping = std::unordered_map<int, int>;

	static inline const UINT_PTR SUBCLASS_ID = 0;
	static inline const UINT_PTR PARENT_SUBCLASS_ID = 0;

	// When an item is dragged over a folder on the bookmarks toolbar, the drop
	// target should be set to the folder only if the dragged item is over the
	// main part of the button for the folder. This is to allow the dragged item
	// to be positioned before or after the folder if the item is currently over
	// the left or right edge of the button.
	// This is especially important when there's no horizontal padding between
	// buttons, as there would be no space before or after the button that would
	// allow you to correctly set the position.
	// The constant here represents how far the left/right edges of the button
	// are indented, as a percentage of the total size of the button, in order
	// to determine whether an item is over the main portion of the button.
	static inline const double FOLDER_CENTRAL_RECT_INDENT_PERCENTAGE = 0.2;

	static LRESULT CALLBACK BookmarksToolbarProcStub(HWND hwnd, UINT uMsg, WPARAM wParam,
		LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK BookmarksToolbarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK BookmarksToolbarParentProcStub(HWND hwnd, UINT uMsg, WPARAM wParam,
		LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK BookmarksToolbarParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void InitializeToolbar();
	void SetUpToolbarImageList();

	void InsertBookmarkItems();
	void InsertBookmarkItem(BookmarkItem *bookmarkItem, int position);

	void RemoveBookmarkItem(const BookmarkItem *bookmarkItem);

	void OnLButtonDown(const POINT &pt);
	void OnMouseMove(int keys, const POINT &pt);
	void StartDrag(DragType dragType, const POINT &pt);
	void OnLButtonUp();
	void OnMButtonUp(const POINT &pt);
	bool OnCommand(WPARAM wParam, LPARAM lParam);
	bool OnButtonClick(int command);
	BOOL OnRightClick(const NMMOUSE *nmm);
	void ShowBookmarkFolderMenu(BookmarkItem *bookmarkItem, int command, int index);
	void OnBookmarkMenuItemClicked(const BookmarkItem *bookmarkItem);
	int FindNextButtonIndex(const POINT &ptClient);
	void OnEditBookmarkItem(BookmarkItem *bookmarkItem);
	bool OnGetInfoTip(NMTBGETINFOTIP *infoTip);

	// Toolbar context menu
	void OnToolbarContextMenuItemClicked(int menuItemId);
	void OnNewBookmarkItem(BookmarkItem::Type type, size_t targetIndex);
	void OnPaste(size_t targetIndex);

	void OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow, const POINT &pt);

	std::optional<int> GetBookmarkItemIndex(const BookmarkItem *bookmarkItem) const;
	std::optional<int> GetBookmarkItemIndexUsingGuid(std::wstring_view guid) const;

	BookmarkItem *GetBookmarkItemFromToolbarIndex(int index);

	void OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index);
	void OnBookmarkItemUpdated(BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType);
	void OnBookmarkItemMoved(BookmarkItem *bookmarkItem, const BookmarkItem *oldParent,
		size_t oldIndex, const BookmarkItem *newParent, size_t newIndex);
	void OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem);

	DropLocation GetDropLocation(const POINT &pt) override;
	void UpdateUiForDropLocation(const DropLocation &dropLocation) override;
	void ResetDropUiState() override;
	void SetButtonPressedState(int index, bool pressed);
	void RemoveInsertionMark();
	void RemoveDropHighlight();

	int GetIconForBookmark(const BookmarkItem *bookmark);
	void ProcessIconResult(std::wstring_view guid, int iconIndex);
	int AddSystemIconToImageList(int iconIndex);

	HWND m_hToolbar;
	DpiCompatibility m_dpiCompat;
	wil::unique_himagelist m_imageList;
	SystemIconImageListMapping m_imageListMappings;
	wil::com_ptr<IImageList> m_systemImageList;
	int m_defaultFolderIconSystemImageListIndex;
	int m_defaultFolderIconIndex;
	int m_bookmarkFolderIconIndex;
	IconFetcher m_iconFetcher;

	HINSTANCE m_instance;

	IExplorerplusplus *m_pexpp;
	Navigation *m_navigation;

	BookmarkTree *m_bookmarkTree;
	BookmarkContextMenu m_bookmarkContextMenu;
	BookmarkMenu m_bookmarkMenu;

	UINT m_uIDStart;
	UINT m_uIDEnd;
	UINT m_uIDCounter;

	std::optional<POINT> m_contextMenuLocation;

	// Drag and drop.
	std::optional<POINT> m_leftButtonDownPoint;
	std::optional<int> m_previousDropButton;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};