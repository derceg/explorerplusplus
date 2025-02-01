// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FolderView.h"
#include "ShellBrowser/ShellBrowserImpl.h"

FolderView::FolderView(WeakPtr<ShellBrowserImpl> shellBrowserWeak) :
	m_shellBrowserWeak(shellBrowserWeak)
{
}

// IFolderView2
IFACEMETHODIMP FolderView::SetGroupBy(REFPROPERTYKEY key, BOOL ascending)
{
	UNREFERENCED_PARAMETER(key);
	UNREFERENCED_PARAMETER(ascending);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetGroupBy(PROPERTYKEY *key, BOOL *ascending)
{
	UNREFERENCED_PARAMETER(key);
	UNREFERENCED_PARAMETER(ascending);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetViewProperty(PCUITEMID_CHILD pidl, REFPROPERTYKEY propertyKey,
	REFPROPVARIANT propVariant)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(propertyKey);
	UNREFERENCED_PARAMETER(propVariant);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetViewProperty(PCUITEMID_CHILD pidl, REFPROPERTYKEY propertyKey,
	PROPVARIANT *propVariant)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(propertyKey);
	UNREFERENCED_PARAMETER(propVariant);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetTileViewProperties(PCUITEMID_CHILD pidl, LPCWSTR propList)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(propList);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetExtendedTileViewProperties(PCUITEMID_CHILD pidl, LPCWSTR propList)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(propList);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetText(FVTEXTTYPE type, LPCWSTR text)
{
	UNREFERENCED_PARAMETER(type);
	UNREFERENCED_PARAMETER(text);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetCurrentFolderFlags(DWORD mask, DWORD flags)
{
	UNREFERENCED_PARAMETER(mask);
	UNREFERENCED_PARAMETER(flags);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetCurrentFolderFlags(DWORD *flags)
{
	UNREFERENCED_PARAMETER(flags);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetSortColumnCount(int *columns)
{
	UNREFERENCED_PARAMETER(columns);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetSortColumns(const SORTCOLUMN *sortColumns, int numColumns)
{
	UNREFERENCED_PARAMETER(sortColumns);
	UNREFERENCED_PARAMETER(numColumns);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetSortColumns(SORTCOLUMN *sortColumns, int numColumns)
{
	UNREFERENCED_PARAMETER(sortColumns);
	UNREFERENCED_PARAMETER(numColumns);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetItem(int item, REFIID riid, void **ppv)
{
	UNREFERENCED_PARAMETER(item);
	UNREFERENCED_PARAMETER(riid);
	UNREFERENCED_PARAMETER(ppv);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetVisibleItem(int start, BOOL previous, int *item)
{
	UNREFERENCED_PARAMETER(start);
	UNREFERENCED_PARAMETER(previous);
	UNREFERENCED_PARAMETER(item);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetSelectedItem(int start, int *item)
{
	UNREFERENCED_PARAMETER(start);
	UNREFERENCED_PARAMETER(item);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetSelection(BOOL noneImpliesFolder, IShellItemArray **itemArray)
{
	UNREFERENCED_PARAMETER(noneImpliesFolder);
	UNREFERENCED_PARAMETER(itemArray);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetSelectionState(PCUITEMID_CHILD pidl, DWORD *flags)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(flags);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::InvokeVerbOnSelection(LPCSTR verb)
{
	UNREFERENCED_PARAMETER(verb);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetViewModeAndIconSize(FOLDERVIEWMODE viewMode, int imageSize)
{
	UNREFERENCED_PARAMETER(viewMode);
	UNREFERENCED_PARAMETER(imageSize);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetViewModeAndIconSize(FOLDERVIEWMODE *viewMode, int *imageSize)
{
	UNREFERENCED_PARAMETER(viewMode);
	UNREFERENCED_PARAMETER(imageSize);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetGroupSubsetCount(UINT numVisibleRows)
{
	UNREFERENCED_PARAMETER(numVisibleRows);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetGroupSubsetCount(UINT *numVisibleRows)
{
	UNREFERENCED_PARAMETER(numVisibleRows);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetRedraw(BOOL redrawOn)
{
	if (m_shellBrowserWeak)
	{
		SendMessage(m_shellBrowserWeak->GetListView(), WM_SETREDRAW, redrawOn, 0);
	}

	return S_OK;
}

IFACEMETHODIMP FolderView::IsMoveInSameFolder()
{
	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::DoRename()
{
	return E_NOTIMPL;
}

// IFolderView
IFACEMETHODIMP FolderView::GetCurrentViewMode(UINT *viewMode)
{
	UNREFERENCED_PARAMETER(viewMode);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetCurrentViewMode(UINT viewMode)
{
	UNREFERENCED_PARAMETER(viewMode);

	return E_NOTIMPL;
}

// Implementing this method is necessary for the "Give access to" submenu (which appears in the
// background context menu for a directory) to be set up correctly.
IFACEMETHODIMP FolderView::GetFolder(REFIID riid, void **ppv)
{
	if (!m_shellBrowserWeak)
	{
		return E_FAIL;
	}

	auto directory = m_shellBrowserWeak->GetDirectoryIdl();

	if (riid == IID_IShellItemArray)
	{
		wil::com_ptr_nothrow<IShellItemArray> shellItemArray;
		PCIDLIST_ABSOLUTE item = directory.get();
		RETURN_IF_FAILED(SHCreateShellItemArrayFromIDLists(1, &item, &shellItemArray));

		*ppv = shellItemArray.detach();

		return S_OK;
	}

	return SHBindToObject(nullptr, directory.get(), nullptr, riid, ppv);
}

IFACEMETHODIMP FolderView::Item(int itemIndex, PITEMID_CHILD *child)
{
	UNREFERENCED_PARAMETER(itemIndex);
	UNREFERENCED_PARAMETER(child);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::ItemCount(UINT flags, int *items)
{
	UNREFERENCED_PARAMETER(flags);
	UNREFERENCED_PARAMETER(items);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::Items(UINT flags, REFIID riid, void **ppv)
{
	UNREFERENCED_PARAMETER(flags);
	UNREFERENCED_PARAMETER(riid);
	UNREFERENCED_PARAMETER(ppv);

	// If any other error code is returned from this method, the "Give access to" submenu won't be
	// created.
	// Note that this interface will only be used when the background context menu is being shown,
	// so there may not be any items to return anyway (e.g. it's not possible to return anything if
	// a list of selected items is being requested, since there won't be any selected items).
	return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

IFACEMETHODIMP FolderView::GetSelectionMarkedItem(int *item)
{
	UNREFERENCED_PARAMETER(item);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetFocusedItem(int *item)
{
	UNREFERENCED_PARAMETER(item);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetItemPosition(PCUITEMID_CHILD child, POINT *pt)
{
	UNREFERENCED_PARAMETER(child);
	UNREFERENCED_PARAMETER(pt);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetSpacing(POINT *pt)
{
	UNREFERENCED_PARAMETER(pt);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetDefaultSpacing(POINT *pt)
{
	UNREFERENCED_PARAMETER(pt);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetAutoArrange()
{
	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SelectItem(int item, DWORD flags)
{
	UNREFERENCED_PARAMETER(item);
	UNREFERENCED_PARAMETER(flags);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SelectAndPositionItems(UINT numItems, PCUITEMID_CHILD_ARRAY items,
	POINT *pts, DWORD flags)
{
	UNREFERENCED_PARAMETER(pts);
	UNREFERENCED_PARAMETER(flags);

	// If the hosting tab was closed or navigated to a different folder, the request to select items
	// should be ignored.
	if (!m_shellBrowserWeak)
	{
		return E_FAIL;
	}

	if (WI_IsFlagSet(flags, SVSI_SELECT))
	{
		std::vector<PidlAbsolute> pidls;

		for (UINT i = 0; i < numItems; i++)
		{
			unique_pidl_absolute pidl(
				ILCombine(m_shellBrowserWeak->GetDirectoryIdl().get(), items[i]));
			pidls.emplace_back(pidl.get());
		}

		m_shellBrowserWeak->SelectItems(pidls);

		return S_OK;
	}

	return S_FALSE;
}

// IShellFolderView
IFACEMETHODIMP FolderView::Rearrange(LPARAM sort)
{
	UNREFERENCED_PARAMETER(sort);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetArrangeParam(LPARAM *sort)
{
	UNREFERENCED_PARAMETER(sort);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::ArrangeGrid()
{
	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::AutoArrange()
{
	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::AddObject(PUITEMID_CHILD pidl, UINT *item)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(item);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetObject(PITEMID_CHILD *pidl, UINT item)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(item);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::RemoveObject(PUITEMID_CHILD pidl, UINT *item)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(item);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetObjectCount(UINT *count)
{
	UNREFERENCED_PARAMETER(count);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetObjectCount(UINT count, UINT flags)
{
	UNREFERENCED_PARAMETER(count);
	UNREFERENCED_PARAMETER(flags);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::UpdateObject(PUITEMID_CHILD pidlOld, PUITEMID_CHILD pidlNew, UINT *item)
{
	UNREFERENCED_PARAMETER(pidlOld);
	UNREFERENCED_PARAMETER(pidlNew);
	UNREFERENCED_PARAMETER(item);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::RefreshObject(PUITEMID_CHILD pidl, UINT *item)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(item);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetSelectedCount(UINT *numSelected)
{
	UNREFERENCED_PARAMETER(numSelected);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetSelectedObjects(PCUITEMID_CHILD **pidlArray, UINT *numItems)
{
	UNREFERENCED_PARAMETER(pidlArray);
	UNREFERENCED_PARAMETER(numItems);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::IsDropOnSource(IDropTarget *dropTarget)
{
	UNREFERENCED_PARAMETER(dropTarget);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetDragPoint(POINT *pt)
{
	UNREFERENCED_PARAMETER(pt);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetDropPoint(POINT *pt)
{
	UNREFERENCED_PARAMETER(pt);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::MoveIcons(IDataObject *dataObject)
{
	UNREFERENCED_PARAMETER(dataObject);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetItemPos(PCUITEMID_CHILD pidl, POINT *pt)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(pt);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::IsBkDropTarget(IDropTarget *dropTarget)
{
	UNREFERENCED_PARAMETER(dropTarget);

	// This method is only used when pasting, in which case the background of the view is the drop
	// target.
	return S_OK;
}

IFACEMETHODIMP FolderView::SetClipboard(BOOL move)
{
	UNREFERENCED_PARAMETER(move);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetPoints(IDataObject *dataObject)
{
	UNREFERENCED_PARAMETER(dataObject);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::GetItemSpacing(ITEMSPACING *spacing)
{
	UNREFERENCED_PARAMETER(spacing);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetCallback(IShellFolderViewCB *callback,
	IShellFolderViewCB **oldCallback)
{
	UNREFERENCED_PARAMETER(callback);
	UNREFERENCED_PARAMETER(oldCallback);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::Select(UINT flags)
{
	UNREFERENCED_PARAMETER(flags);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::QuerySupport(UINT *support)
{
	UNREFERENCED_PARAMETER(support);

	return E_NOTIMPL;
}

IFACEMETHODIMP FolderView::SetAutomationObject(IDispatch *dispatch)
{
	UNREFERENCED_PARAMETER(dispatch);

	return E_NOTIMPL;
}

namespace winrt
{
template <>
bool is_guid_of<IFolderView2>(guid const &id) noexcept
{
	return is_guid_of<IFolderView2, IFolderView>(id);
}
}
