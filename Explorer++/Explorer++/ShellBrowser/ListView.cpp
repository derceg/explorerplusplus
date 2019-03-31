// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "iShellView.h"
#include "MainResource.h"

LRESULT CALLBACK CShellBrowser::ListViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CShellBrowser *shellBrowser = reinterpret_cast<CShellBrowser *>(dwRefData);
	return shellBrowser->ListViewProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CShellBrowser::ListViewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_APP_COLUMN_RESULT_READY:
		ProcessColumnResult(static_cast<int>(wParam));
		break;

	case WM_APP_THUMBNAIL_RESULT_READY:
		ProcessThumbnailResult(static_cast<int>(wParam));
		break;

	case WM_APP_ICON_RESULT_READY:
		ProcessIconResult(static_cast<int>(wParam));
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CShellBrowser::ListViewParentProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CShellBrowser *shellBrowser = reinterpret_cast<CShellBrowser *>(dwRefData);
	return shellBrowser->ListViewParentProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CShellBrowser::ListViewParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hListView)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case LVN_GETDISPINFO:
				OnListViewGetDisplayInfo(lParam);
				break;

			case LVN_GETINFOTIP:
				OnListViewGetInfoTip(reinterpret_cast<NMLVGETINFOTIP *>(lParam));
				break;

			case LVN_COLUMNCLICK:
				ColumnClicked(reinterpret_cast<NMLISTVIEW *>(lParam)->iSubItem);
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void CShellBrowser::OnListViewGetDisplayInfo(LPARAM lParam)
{
	NMLVDISPINFO	*pnmv = NULL;
	LVITEM			*plvItem = NULL;
	NMHDR			*nmhdr = NULL;

	pnmv = (NMLVDISPINFO *)lParam;
	plvItem = &pnmv->item;
	nmhdr = &pnmv->hdr;

	/* Construct an image here using the items
	actual icon. This image will be shown initially.
	If the item also has a thumbnail image, this
	will be found later, and will overwrite any
	image settings made here.
	Note that the initial icon image MUST be drawn
	first, or else it may be possible for the
	thumbnail to be drawn before the initial
	image. */
	if (m_folderSettings.viewMode == +ViewMode::Thumbnails && (plvItem->mask & LVIF_IMAGE) == LVIF_IMAGE)
	{
		plvItem->iImage = GetIconThumbnail((int)plvItem->lParam);
		plvItem->mask |= LVIF_DI_SETITEM;

		QueueThumbnailTask(static_cast<int>(plvItem->lParam));

		return;
	}

	if (m_folderSettings.viewMode == +ViewMode::Details && (plvItem->mask & LVIF_TEXT) == LVIF_TEXT)
	{
		QueueColumnTask(static_cast<int>(plvItem->lParam), plvItem->iSubItem);
	}

	if ((plvItem->mask & LVIF_IMAGE) == LVIF_IMAGE)
	{
		const ItemInfo_t &itemInfo = m_itemInfoMap.at(static_cast<int>(plvItem->lParam));
		auto cachedIconIndex = GetCachedIconIndex(itemInfo);

		if (cachedIconIndex)
		{
			// The icon retrieval method specifies the
			// SHGFI_OVERLAYINDEX value. That means that cached icons
			// will have an overlay index stored in the upper eight bits
			// of the icon value. While setting the icon and
			// stateMask/state values in one go with ListView_SetItem()
			// works, there's no direct way to specify the
			// stateMask/state values here.
			// If you don't mask out the upper eight bits here, no icon
			// will be shown. You can call ListView_SetItem() at this
			// point, but that seemingly doesn't repaint the item
			// correctly (you have to call ListView_Update() to force
			// the item to be redrawn).
			// Rather than doing that, only the icon is set here. Any
			// overlay will be added by the icon retrieval task
			// (scheduled below).
			plvItem->iImage = (*cachedIconIndex & 0x0FFF);
		}
		else
		{
			if ((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
			{
				plvItem->iImage = m_iFolderIcon;
			}
			else
			{
				plvItem->iImage = m_iFileIcon;
			}
		}

		QueueIconTask(static_cast<int>(plvItem->lParam));
	}

	plvItem->mask |= LVIF_DI_SETITEM;
}

void CShellBrowser::OnListViewGetInfoTip(NMLVGETINFOTIP *getInfoTip)
{
	TCHAR szInfoTip[512];

	/* The pszText member of pGetInfoTip will contain the text of the
	item if its name is truncated in the listview. Always concatenate
	the rest of the info tip onto the name if it is there. */
	if (m_config->showInfoTips)
	{
		CreateFileInfoTip(getInfoTip->iItem, szInfoTip, SIZEOF_ARRAY(szInfoTip));

		if (lstrlen(getInfoTip->pszText) > 0)
		{
			StringCchCat(getInfoTip->pszText, getInfoTip->cchTextMax, _T("\n"));
		}

		StringCchCat(getInfoTip->pszText, getInfoTip->cchTextMax, szInfoTip);
	}
	else
	{
		StringCchCopy(getInfoTip->pszText, getInfoTip->cchTextMax, EMPTY_STRING);
	}
}

void CShellBrowser::CreateFileInfoTip(int iItem, TCHAR *szInfoTip, UINT cchMax)
{
	HRESULT	hr;

	/* Use Explorer infotips if the option is selected, or this is a
	virtual folder. Otherwise, show the modified date. */
	if ((m_config->infoTipType == INFOTIP_SYSTEM) || InVirtualFolder())
	{
		TCHAR szFullFileName[MAX_PATH];
		QueryFullItemName(iItem, szFullFileName, SIZEOF_ARRAY(szFullFileName));
		hr = GetItemInfoTip(szFullFileName, szInfoTip, cchMax);

		if (!SUCCEEDED(hr))
			StringCchCopy(szInfoTip, cchMax, EMPTY_STRING);
	}
	else
	{
		WIN32_FIND_DATA wfd;
		TCHAR			szDate[256];
		TCHAR			szDateModified[256];

		wfd = QueryFileFindData(iItem);

		CreateFileTimeString(&wfd.ftLastWriteTime,
			szDateModified, SIZEOF_ARRAY(szDateModified), m_config->globalFolderSettings.showFriendlyDates);

		LoadString(m_hResourceModule, IDS_GENERAL_DATEMODIFIED, szDate,
			SIZEOF_ARRAY(szDate));

		StringCchPrintf(szInfoTip, cchMax, _T("%s: %s"),
			szDate, szDateModified);
	}
}