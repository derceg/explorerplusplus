// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/WeakPtr.h"
#include "../Helper/WinRTBaseWrapper.h"
#include <ShlObj.h>

class ShellBrowserImpl;

// This isn't a complete implementation. There's only enough functionality to support some context
// menu items.
class FolderView :
	public winrt::implements<FolderView, IFolderView2, IShellFolderView, winrt::non_agile>
{
public:
	FolderView(WeakPtr<ShellBrowserImpl> shellBrowserWeak);

	// IFolderView2
	IFACEMETHODIMP SetGroupBy(REFPROPERTYKEY key, BOOL ascending);
	IFACEMETHODIMP GetGroupBy(PROPERTYKEY *key, BOOL *ascending);
	IFACEMETHODIMP SetViewProperty(PCUITEMID_CHILD pidl, REFPROPERTYKEY propertyKey,
		REFPROPVARIANT propVariant);
	IFACEMETHODIMP GetViewProperty(PCUITEMID_CHILD pidl, REFPROPERTYKEY propertyKey,
		PROPVARIANT *propVariant);
	IFACEMETHODIMP SetTileViewProperties(PCUITEMID_CHILD pidl, LPCWSTR propList);
	IFACEMETHODIMP SetExtendedTileViewProperties(PCUITEMID_CHILD pidl, LPCWSTR propList);
	IFACEMETHODIMP SetText(FVTEXTTYPE type, LPCWSTR text);
	IFACEMETHODIMP SetCurrentFolderFlags(DWORD mask, DWORD flags);
	IFACEMETHODIMP GetCurrentFolderFlags(DWORD *flags);
	IFACEMETHODIMP GetSortColumnCount(int *columns);
	IFACEMETHODIMP SetSortColumns(const SORTCOLUMN *sortColumns, int numColumns);
	IFACEMETHODIMP GetSortColumns(SORTCOLUMN *sortColumns, int numColumns);
	IFACEMETHODIMP GetItem(int item, REFIID riid, void **ppv);
	IFACEMETHODIMP GetVisibleItem(int start, BOOL previous, int *item);
	IFACEMETHODIMP GetSelectedItem(int start, int *item);
	IFACEMETHODIMP GetSelection(BOOL noneImpliesFolder, IShellItemArray **itemArray);
	IFACEMETHODIMP GetSelectionState(PCUITEMID_CHILD pidl, DWORD *flags);
	IFACEMETHODIMP InvokeVerbOnSelection(LPCSTR verb);
	IFACEMETHODIMP SetViewModeAndIconSize(FOLDERVIEWMODE viewMode, int imageSize);
	IFACEMETHODIMP GetViewModeAndIconSize(FOLDERVIEWMODE *viewMode, int *imageSize);
	IFACEMETHODIMP SetGroupSubsetCount(UINT numVisibleRows);
	IFACEMETHODIMP GetGroupSubsetCount(UINT *numVisibleRows);
	IFACEMETHODIMP IsMoveInSameFolder();
	IFACEMETHODIMP DoRename();

	// IFolderView
	IFACEMETHODIMP GetCurrentViewMode(UINT *viewMode);
	IFACEMETHODIMP SetCurrentViewMode(UINT viewMode);
	IFACEMETHODIMP GetFolder(REFIID riid, void **ppv);
	IFACEMETHODIMP Item(int itemIndex, PITEMID_CHILD *child);
	IFACEMETHODIMP ItemCount(UINT flags, int *items);
	IFACEMETHODIMP Items(UINT flags, REFIID riid, void **ppv);
	IFACEMETHODIMP GetSelectionMarkedItem(int *item);
	IFACEMETHODIMP GetFocusedItem(int *item);
	IFACEMETHODIMP GetItemPosition(PCUITEMID_CHILD child, POINT *pt);
	IFACEMETHODIMP GetSpacing(POINT *pt);
	IFACEMETHODIMP GetDefaultSpacing(POINT *pt);
	IFACEMETHODIMP SelectItem(int item, DWORD flags);
	IFACEMETHODIMP SelectAndPositionItems(UINT numItems, PCUITEMID_CHILD_ARRAY items, POINT *pts,
		DWORD flags);

	// IShellFolderView
	// Required for paste support (specifically, selecting items after they've been pasted).
	// Note that this interface has two methods that have identical signatures to methods in the
	// IFolderView/IFolderView2 interfaces: GetAutoArrange() and SetRedraw.
	IFACEMETHODIMP Rearrange(LPARAM sort) noexcept;
	IFACEMETHODIMP GetArrangeParam(LPARAM *sort) noexcept;
	IFACEMETHODIMP ArrangeGrid() noexcept;
	IFACEMETHODIMP AutoArrange() noexcept;
	IFACEMETHODIMP GetAutoArrange() noexcept;
	IFACEMETHODIMP AddObject(PUITEMID_CHILD pidl, UINT *item) noexcept;
	IFACEMETHODIMP GetObject(PITEMID_CHILD *pidl, UINT item) noexcept;
	IFACEMETHODIMP RemoveObject(PUITEMID_CHILD pidl, UINT *item) noexcept;
	IFACEMETHODIMP GetObjectCount(UINT *count) noexcept;
	IFACEMETHODIMP SetObjectCount(UINT count, UINT flags) noexcept;
	IFACEMETHODIMP UpdateObject(PUITEMID_CHILD pidlOld, PUITEMID_CHILD pidlNew,
		UINT *item) noexcept;
	IFACEMETHODIMP RefreshObject(PUITEMID_CHILD pidl, UINT *item) noexcept;
	IFACEMETHODIMP SetRedraw(BOOL redrawOn) noexcept;
	IFACEMETHODIMP GetSelectedCount(UINT *numSelected) noexcept;
	IFACEMETHODIMP GetSelectedObjects(PCUITEMID_CHILD **pidlArray, UINT *numItems) noexcept;
	IFACEMETHODIMP IsDropOnSource(IDropTarget *dropTarget) noexcept;
	IFACEMETHODIMP GetDragPoint(POINT *pt) noexcept;
	IFACEMETHODIMP GetDropPoint(POINT *pt) noexcept;
	IFACEMETHODIMP MoveIcons(IDataObject *dataObject) noexcept;
	IFACEMETHODIMP SetItemPos(PCUITEMID_CHILD pidl, POINT *pt) noexcept;
	IFACEMETHODIMP IsBkDropTarget(IDropTarget *dropTarget) noexcept;
	IFACEMETHODIMP SetClipboard(BOOL move) noexcept;
	IFACEMETHODIMP SetPoints(IDataObject *dataObject) noexcept;
	IFACEMETHODIMP GetItemSpacing(ITEMSPACING *spacing) noexcept;
	IFACEMETHODIMP SetCallback(IShellFolderViewCB *callback,
		IShellFolderViewCB **oldCallback) noexcept;
	IFACEMETHODIMP Select(UINT flags) noexcept;
	IFACEMETHODIMP QuerySupport(UINT *support) noexcept;
	IFACEMETHODIMP SetAutomationObject(IDispatch *dispatch) noexcept;

private:
	const WeakPtr<ShellBrowserImpl> m_shellBrowserWeak;
};

namespace winrt
{
template <>
bool is_guid_of<IFolderView2>(guid const &id) noexcept;
}
