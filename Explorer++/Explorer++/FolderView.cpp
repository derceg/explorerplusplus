// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FolderView.h"

wil::com_ptr_nothrow<FolderView> FolderView::Create(PCIDLIST_ABSOLUTE directory)
{
	wil::com_ptr_nothrow<FolderView> folderView;
	folderView.attach(new FolderView(directory));
	return folderView;
}

FolderView::FolderView(PCIDLIST_ABSOLUTE directory) :
	m_refCount(1),
	m_directory(ILCloneFull(directory))
{
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
	if (riid == IID_IShellItemArray)
	{
		wil::com_ptr_nothrow<IShellItemArray> shellItemArray;
		PCIDLIST_ABSOLUTE directory = m_directory.get();
		RETURN_IF_FAILED(SHCreateShellItemArrayFromIDLists(1, &directory, &shellItemArray));

		*ppv = shellItemArray.detach();

		return S_OK;
	}

	return E_NOINTERFACE;
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

IFACEMETHODIMP FolderView::SelectAndPositionItems(
	UINT numItems, PCUITEMID_CHILD_ARRAY items, POINT *pt, DWORD flags)
{
	UNREFERENCED_PARAMETER(numItems);
	UNREFERENCED_PARAMETER(items);
	UNREFERENCED_PARAMETER(pt);
	UNREFERENCED_PARAMETER(flags);

	return E_NOTIMPL;
}

// IUnknown
IFACEMETHODIMP FolderView::QueryInterface(REFIID riid, void **ppvObject)
{
	// clang-format off
	static const QITAB qit[] = {
		QITABENT(FolderView, IFolderView),
		{ nullptr }
	};
	// clang-format on

	return QISearch(this, qit, riid, ppvObject);
}

IFACEMETHODIMP_(ULONG) FolderView::AddRef()
{
	return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG) FolderView::Release()
{
	ULONG refCount = InterlockedDecrement(&m_refCount);

	if (refCount == 0)
	{
		delete this;
		return 0;
	}

	return refCount;
}