// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"

BOOL Explorerplusplus::CanCut() const
{
	return TestItemAttributes(SFGAO_CANMOVE);
}

BOOL Explorerplusplus::CanCopy() const
{
	return TestItemAttributes(SFGAO_CANCOPY);
}

BOOL Explorerplusplus::CanRename() const
{
	return TestItemAttributes(SFGAO_CANRENAME);
}

BOOL Explorerplusplus::CanDelete() const
{
	return TestItemAttributes(SFGAO_CANDELETE);
}

BOOL Explorerplusplus::CanShowFileProperties() const
{
	return TestItemAttributes(SFGAO_HASPROPSHEET);
}

/* Returns TRUE if all the specified attributes are set on the selected items. */
BOOL Explorerplusplus::TestItemAttributes(SFGAOF attributes) const
{
	SFGAOF commonAttributes = attributes;
	HRESULT hr = GetSelectionAttributes(&commonAttributes);

	if (SUCCEEDED(hr))
	{
		return (commonAttributes & attributes) == attributes;
	}

	return FALSE;
}

HRESULT Explorerplusplus::GetSelectionAttributes(SFGAOF *pItemAttributes) const
{
	HWND	hFocus;
	HRESULT	hr = E_FAIL;

	hFocus = GetFocus();

	if (hFocus == m_hActiveListView)
		hr = GetListViewSelectionAttributes(pItemAttributes);
	else if (hFocus == m_hTreeView)
		hr = GetTreeViewSelectionAttributes(pItemAttributes);

	return hr;
}

HRESULT Explorerplusplus::TestListViewItemAttributes(int item, SFGAOF attributes) const
{
	SFGAOF commonAttributes = attributes;
	HRESULT hr = GetListViewItemAttributes(item, &commonAttributes);

	if (SUCCEEDED(hr))
	{
		return (commonAttributes & attributes) == attributes;
	}

	return FALSE;
}

HRESULT Explorerplusplus::GetListViewSelectionAttributes(SFGAOF *pItemAttributes) const
{
	HRESULT hr = E_FAIL;

	/* TODO: This should probably check all selected files. */
	int iSelected = ListView_GetNextItem(m_hActiveListView, -1, LVNI_SELECTED);

	if (iSelected != -1)
	{
		hr = GetListViewItemAttributes(iSelected, pItemAttributes);
	}

	return hr;
}

HRESULT Explorerplusplus::GetListViewItemAttributes(int item, SFGAOF *pItemAttributes) const
{
	PIDLPointer pidlComplete(m_pActiveShellBrowser->QueryItemCompleteIdl(item));

	if (!pidlComplete)
	{
		return E_FAIL;
	}

	HRESULT hr = GetItemAttributes(pidlComplete.get(), pItemAttributes);

	return hr;
}

HRESULT Explorerplusplus::GetTreeViewSelectionAttributes(SFGAOF *pItemAttributes) const
{
	HTREEITEM		hItem;
	LPITEMIDLIST	pidl = NULL;
	HRESULT			hr = E_FAIL;

	hItem = TreeView_GetSelection(m_hTreeView);

	if (hItem != NULL)
	{
		pidl = m_pMyTreeView->BuildPath(hItem);

		hr = GetItemAttributes(pidl, pItemAttributes);

		CoTaskMemFree(pidl);
	}

	return hr;
}

BOOL Explorerplusplus::CanPaste() const
{
	HWND hFocus = GetFocus();

	std::list<FORMATETC> ftcList;
	CDropHandler::GetDropFormats(ftcList);

	BOOL bDataAvailable = FALSE;

	/* Check whether the drop source has the type of data
	that is needed for this drag operation. */
	for (const auto &ftc : ftcList)
	{
		if (IsClipboardFormatAvailable(ftc.cfFormat))
		{
			bDataAvailable = TRUE;
			break;
		}
	}

	if (hFocus == m_hActiveListView)
	{
		return bDataAvailable && m_pActiveShellBrowser->CanCreate();
	}
	else if (hFocus == m_hTreeView)
	{
		HTREEITEM		hItem;
		LPITEMIDLIST	pidl = NULL;
		SFGAOF			Attributes;
		HRESULT			hr;

		hItem = TreeView_GetSelection(m_hTreeView);

		if (hItem != NULL)
		{
			pidl = m_pMyTreeView->BuildPath(hItem);

			Attributes = SFGAO_FILESYSTEM;

			hr = GetItemAttributes(pidl, &Attributes);

			CoTaskMemFree(pidl);

			if (hr == S_OK)
				return bDataAvailable;
		}
	}

	return FALSE;
}