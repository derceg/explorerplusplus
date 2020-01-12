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
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall		AddRef(void);
	ULONG __stdcall		Release(void);

private:

	struct BookmarkDropTarget
	{
		BookmarkItem *parentFolder;
		size_t position;
		int selectedButtonIndex;
	};

	CBookmarksToolbarDropHandler & operator = (const CBookmarksToolbarDropHandler &btdh);

	/* IDropTarget methods. */
	HRESULT __stdcall DragEnter(IDataObject *pDataObject, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
	HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
	HRESULT __stdcall DragLeave();
	HRESULT __stdcall Drop(IDataObject *pDataObject, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);

	DWORD GetDropEffect(IDataObject *pDataObject);

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
	void	ResetDragFlags();
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
	bool m_withinDrag;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;
	std::vector<boost::signals2::scoped_connection> m_connections;
};