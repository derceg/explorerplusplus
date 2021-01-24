// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellHelper.h"
#include <wil/com.h>

// This isn't a complete implementation. There's only enough functionality to support the background
// context menu - in particular, the "Give access to" menu.
class FolderView : public IFolderView
{
public:
	static wil::com_ptr_nothrow<FolderView> Create(PCIDLIST_ABSOLUTE directory);

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
	IFACEMETHODIMP GetAutoArrange();
	IFACEMETHODIMP SelectItem(int item, DWORD flags);
	IFACEMETHODIMP SelectAndPositionItems(
		UINT numItems, PCUITEMID_CHILD_ARRAY items, POINT *pt, DWORD flags);

	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

private:
	FolderView(PCIDLIST_ABSOLUTE directory);

	ULONG m_refCount;
	unique_pidl_absolute m_directory;
};