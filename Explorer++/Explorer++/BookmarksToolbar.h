// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkContextMenu.h"
#include "BookmarkItem.h"
#include "BookmarkMenu.h"
#include "BookmarkTree.h"
#include "CoreInterface.h"
#include "Navigation.h"
#include "ResourceHelper.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <boost/signals2.hpp>
#include <wil/resource.h>
#include <optional>

class BookmarksToolbarInterface
{
public:

	virtual BookmarkItem *GetBookmarkItemFromToolbarIndex(int index) = 0;
	virtual int FindNextButtonIndex(const POINT &ptClient) = 0;
};

class CBookmarksToolbarDropHandler : public IDropTarget
{
public:

	CBookmarksToolbarDropHandler(BookmarksToolbarInterface *bookmarksToolbarInterface,
		HWND hToolbar, BookmarkTree *bookmarkTree);
	~CBookmarksToolbarDropHandler();

	/* IUnknown methods. */
	HRESULT __stdcall QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall AddRef(void);
	ULONG __stdcall Release(void);

	bool IsWithinDrag() const;

private:

	struct BookmarkDropTarget
	{
		BookmarkItem *parentFolder;
		size_t position;
		int selectedButtonIndex;
	};

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

	CBookmarksToolbarDropHandler & operator = (const CBookmarksToolbarDropHandler &btdh);

	/* IDropTarget methods. */
	HRESULT __stdcall DragEnter(IDataObject *pDataObject, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
	HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
	HRESULT __stdcall DragLeave();
	HRESULT __stdcall Drop(IDataObject *pDataObject, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);

	DWORD GetDropEffect(IDataObject *pDataObject);

	BookmarkItems CreateBookmarkItemsFromDroppedFiles(IDataObject *dataObject);

	BookmarkDropTarget GetDropTarget(const POINT &pt);
	void ResetToolbarState();
	void RemoveInsertionMark();
	void SetButtonPressedState(int index, bool pressed);

	ULONG m_ulRefCount;

	BookmarksToolbarInterface *m_bookmarksToolbarInterface;
	HWND m_hToolbar;
	BookmarkTree *m_bookmarkTree;

	IDragSourceHelper *m_pDragSourceHelper;
	IDropTargetHelper *m_pDropTargetHelper;

	bool m_withinDrag;
	DWORD m_cachedDropEffect;
	std::optional<int> m_previousDropButton;
};

class CBookmarksToolbar : private BookmarksToolbarInterface
{
public:

	CBookmarksToolbar(HWND hToolbar, HINSTANCE instance, IExplorerplusplus *pexpp,
		Navigation *navigation, BookmarkTree *bookmarkTree, UINT uIDStart, UINT uIDEnd);
	~CBookmarksToolbar();

private:

	CBookmarksToolbar & operator = (const CBookmarksToolbar &bt);

	static const UINT_PTR SUBCLASS_ID = 0;
	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	static LRESULT CALLBACK	BookmarksToolbarProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK	BookmarksToolbarProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	static LRESULT CALLBACK	BookmarksToolbarParentProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK	BookmarksToolbarParentProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	void	InitializeToolbar();

	void	InsertBookmarkItems();
	void	InsertBookmarkItem(BookmarkItem *bookmarkItem, int position);

	void	RemoveBookmarkItem(const BookmarkItem *bookmarkItem);

	void	OnLButtonDown(const POINT &pt);
	void	OnMouseMove(int keys, const POINT &pt);
	void	StartDrag(DragType dragType, const POINT &pt);
	void	OnLButtonUp();
	void	OnMButtonUp(const POINT &pt);
	bool	OnCommand(WPARAM wParam, LPARAM lParam);
	bool	OnButtonClick(int command);
	BOOL	OnRightClick(const NMMOUSE *nmm);
	void	ShowBookmarkFolderMenu(BookmarkItem *bookmarkItem, int command, int index);
	void	OnBookmarkMenuItemClicked(const BookmarkItem *bookmarkItem);
	void	OnNewBookmarkItem(BookmarkItem::Type type);
	void	OnPaste();
	int		FindNextButtonIndex(const POINT &ptClient);
	void	OnEditBookmarkItem(BookmarkItem *bookmarkItem);
	bool	OnGetInfoTip(NMTBGETINFOTIP *infoTip);

	void	OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow, const POINT &pt);

	std::optional<int>	GetBookmarkItemIndex(const BookmarkItem *bookmarkItem) const;

	BookmarkItem	*GetBookmarkItemFromToolbarIndex(int index);

	void	OnBookmarkItemAdded(BookmarkItem &bookmarkItem, size_t index);
	void	OnBookmarkItemUpdated(BookmarkItem &bookmarkItem, BookmarkItem::PropertyType propertyType);
	void	OnBookmarkItemMoved(BookmarkItem *bookmarkItem, const BookmarkItem *oldParent, size_t oldIndex,
		const BookmarkItem *newParent, size_t newIndex);
	void	OnBookmarkItemPreRemoval(BookmarkItem &bookmarkItem);

	HWND m_hToolbar;
	DpiCompatibility m_dpiCompat;
	wil::unique_himagelist m_imageList;
	IconImageListMapping m_imageListMappings;

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

	CBookmarksToolbarDropHandler *m_dropHandler;
	std::optional<POINT> m_leftButtonDownPoint;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};