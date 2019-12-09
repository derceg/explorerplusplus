// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "Config.h"
#include "MainResource.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/ShellHelper.h"
#include <boost/format.hpp>

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
	case WM_MBUTTONDOWN:
	{
		POINT pt;
		POINTSTOPOINT(pt, MAKEPOINTS(lParam));
		OnListViewMButtonDown(&pt);
	}
	break;

	case WM_MBUTTONUP:
	{
		POINT pt;
		POINTSTOPOINT(pt, MAKEPOINTS(lParam));
		OnListViewMButtonUp(&pt);
	}
	break;

	case WM_APP_COLUMN_RESULT_READY:
		ProcessColumnResult(static_cast<int>(wParam));
		break;

	case WM_APP_THUMBNAIL_RESULT_READY:
		ProcessThumbnailResult(static_cast<int>(wParam));
		break;

	case WM_APP_INFO_TIP_READY:
		ProcessInfoTipResult(static_cast<int>(wParam));
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
				return OnListViewGetInfoTip(reinterpret_cast<NMLVGETINFOTIP *>(lParam));
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

void CShellBrowser::OnListViewMButtonDown(const POINT *pt)
{
	LV_HITTESTINFO ht;
	ht.pt = *pt;
	ListView_HitTest(m_hListView, &ht);

	if (ht.flags != LVHT_NOWHERE && ht.iItem != -1)
	{
		m_middleButtonItem = ht.iItem;

		ListView_SetItemState(m_hListView, ht.iItem, LVIS_FOCUSED, LVIS_FOCUSED);
	}
	else
	{
		m_middleButtonItem = -1;
	}
}

void CShellBrowser::OnListViewMButtonUp(const POINT *pt)
{
	LV_HITTESTINFO	ht;
	ht.pt = *pt;
	ListView_HitTest(m_hListView, &ht);

	if (ht.flags == LVHT_NOWHERE)
	{
		return;
	}

	// Only open an item if it was the one on which the middle mouse button was
	// initially clicked on.
	if (ht.iItem != m_middleButtonItem)
	{
		return;
	}

	const ItemInfo_t &itemInfo = GetItemByIndex(m_middleButtonItem);

	if (!WI_IsAnyFlagSet(itemInfo.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE))
	{
		return;
	}

	m_tabNavigation->CreateNewTab(itemInfo.pidlComplete.get(), false);
}

void CShellBrowser::OnListViewGetDisplayInfo(LPARAM lParam)
{
	NMLVDISPINFO	*pnmv = NULL;
	LVITEM			*plvItem = NULL;
	NMHDR			*nmhdr = NULL;

	pnmv = (NMLVDISPINFO *)lParam;
	plvItem = &pnmv->item;
	nmhdr = &pnmv->hdr;

	int internalIndex = static_cast<int>(plvItem->lParam);

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
		plvItem->iImage = GetIconThumbnail(internalIndex);
		plvItem->mask |= LVIF_DI_SETITEM;

		QueueThumbnailTask(internalIndex);

		return;
	}

	if (m_folderSettings.viewMode == +ViewMode::Details && (plvItem->mask & LVIF_TEXT) == LVIF_TEXT)
	{
		QueueColumnTask(internalIndex, plvItem->iSubItem);
	}

	if ((plvItem->mask & LVIF_IMAGE) == LVIF_IMAGE)
	{
		const ItemInfo_t &itemInfo = m_itemInfoMap.at(internalIndex);
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

		m_iconFetcher->QueueIconTask(itemInfo.pidlComplete.get(), [this, internalIndex] (PCIDLIST_ABSOLUTE pidl, int iconIndex) {
			UNREFERENCED_PARAMETER(pidl);

			ProcessIconResult(internalIndex, iconIndex);
		});
	}

	plvItem->mask |= LVIF_DI_SETITEM;
}

boost::optional<int> CShellBrowser::GetCachedIconIndex(const ItemInfo_t &itemInfo)
{
	TCHAR filePath[MAX_PATH];
	HRESULT hr = GetDisplayName(itemInfo.pidlComplete.get(),
		filePath, SIZEOF_ARRAY(filePath), SHGDN_FORPARSING);

	if (FAILED(hr))
	{
		return boost::none;
	}

	auto cachedItr = m_cachedIcons->findByPath(filePath);

	if (cachedItr == m_cachedIcons->end())
	{
		return boost::none;
	}

	return cachedItr->iconIndex;
}

void CShellBrowser::ProcessIconResult(int internalIndex, int iconIndex)
{
	auto index = LocateItemByInternalIndex(internalIndex);

	if (!index)
	{
		return;
	}

	LVITEM lvItem;
	lvItem.mask = LVIF_IMAGE | LVIF_STATE;
	lvItem.iItem = *index;
	lvItem.iSubItem = 0;
	lvItem.iImage = iconIndex;
	lvItem.stateMask = LVIS_OVERLAYMASK;
	lvItem.state = INDEXTOOVERLAYMASK(iconIndex >> 24);
	ListView_SetItem(m_hListView, &lvItem);
}

LRESULT CShellBrowser::OnListViewGetInfoTip(NMLVGETINFOTIP *getInfoTip)
{
	if (m_config->showInfoTips)
	{
		int internalIndex = GetItemInternalIndex(getInfoTip->iItem);
		QueueInfoTipTask(internalIndex, getInfoTip->pszText);
	}

	StringCchCopy(getInfoTip->pszText, getInfoTip->cchTextMax, EMPTY_STRING);

	return 0;
}

void CShellBrowser::QueueInfoTipTask(int internalIndex, const std::wstring &existingInfoTip)
{
	int infoTipResultId = m_infoTipResultIDCounter++;

	BasicItemInfo_t basicItemInfo = getBasicItemInfo(internalIndex);
	Config configCopy = *m_config;
	bool virtualFolder = InVirtualFolder();

	auto result = m_infoTipsThreadPool.push([this, infoTipResultId, internalIndex,
		basicItemInfo, configCopy, virtualFolder, existingInfoTip](int id) {
		UNREFERENCED_PARAMETER(id);

		auto result = GetInfoTipAsync(m_hListView, infoTipResultId, internalIndex, basicItemInfo, configCopy,
			m_hResourceModule, virtualFolder);

		// If the item name is truncated in the listview,
		// existingInfoTip will contain that value. Therefore, it's
		// important that the rest of the infotip is concatenated onto
		// that value if it's there.
		if (result && !existingInfoTip.empty())
		{
			result->infoTip = existingInfoTip + L"\n" + result->infoTip;
		}

		return result;
	});

	m_infoTipResults.insert({ infoTipResultId, std::move(result) });
}

boost::optional<CShellBrowser::InfoTipResult> CShellBrowser::GetInfoTipAsync(HWND listView, int infoTipResultId,
	int internalIndex, const BasicItemInfo_t &basicItemInfo, const Config &config, HINSTANCE instance, bool virtualFolder)
{
	std::wstring infoTip;

	/* Use Explorer infotips if the option is selected, or this is a
	virtual folder. Otherwise, show the modified date. */
	if ((config.infoTipType == INFOTIP_SYSTEM) || virtualFolder)
	{
		TCHAR infoTipText[256];
		HRESULT hr = GetItemInfoTip(basicItemInfo.pidlComplete.get(), infoTipText, SIZEOF_ARRAY(infoTipText));

		if (FAILED(hr))
		{
			return boost::none;
		}

		infoTip = infoTipText;
	}
	else
	{
		TCHAR dateModified[64];
		LoadString(instance, IDS_GENERAL_DATEMODIFIED, dateModified, SIZEOF_ARRAY(dateModified));

		TCHAR fileModificationText[256];
		BOOL fileTimeResult = CreateFileTimeString(&basicItemInfo.wfd.ftLastWriteTime, fileModificationText,
			SIZEOF_ARRAY(fileModificationText), config.globalFolderSettings.showFriendlyDates);

		if (!fileTimeResult)
		{
			return boost::none;
		}

		infoTip = str(boost::wformat(_T("%s: %s")) % dateModified % fileModificationText);
	}

	PostMessage(listView, WM_APP_INFO_TIP_READY, infoTipResultId, 0);

	InfoTipResult result;
	result.itemInternalIndex = internalIndex;
	result.infoTip = infoTip;

	return result;
}

void CShellBrowser::ProcessInfoTipResult(int infoTipResultId)
{
	auto itr = m_infoTipResults.find(infoTipResultId);

	if (itr == m_infoTipResults.end())
	{
		return;
	}

	auto result = itr->second.get();
	m_infoTipResults.erase(itr);

	if (!result)
	{
		return;
	}

	auto index = LocateItemByInternalIndex(result->itemInternalIndex);

	if (!index)
	{
		return;
	}

	TCHAR infoTipText[256];
	StringCchCopy(infoTipText, SIZEOF_ARRAY(infoTipText), result->infoTip.c_str());

	LVSETINFOTIP infoTip;
	infoTip.cbSize = sizeof(infoTip);
	infoTip.dwFlags = 0;
	infoTip.iItem = *index;
	infoTip.iSubItem = 0;
	infoTip.pszText = infoTipText;
	ListView_SetInfoTip(m_hListView, &infoTip);
}

CShellBrowser::ItemInfo_t &CShellBrowser::GetItemByIndex(int index)
{
	int internalIndex = GetItemInternalIndex(index);
	return m_itemInfoMap.at(internalIndex);
}

int CShellBrowser::GetItemInternalIndex(int item) const
{
	LVITEM lvItem;
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = item;
	lvItem.iSubItem = 0;
	BOOL res = ListView_GetItem(m_hListView, &lvItem);

	if (!res)
	{
		throw std::runtime_error("Item lookup failed");
	}

	return static_cast<int>(lvItem.lParam);
}

BOOL CShellBrowser::GhostItem(int iItem)
{
	return GhostItemInternal(iItem, TRUE);
}

BOOL CShellBrowser::DeghostItem(int iItem)
{
	return GhostItemInternal(iItem, FALSE);
}

BOOL CShellBrowser::GhostItemInternal(int iItem, BOOL bGhost)
{
	LVITEM	lvItem;
	BOOL	bRet;

	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = iItem;
	lvItem.iSubItem = 0;
	bRet = ListView_GetItem(m_hListView, &lvItem);

	if (bRet)
	{
		/* If the file is hidden, prevent changes to its visibility state (i.e.
		hidden items will ALWAYS be ghosted). */
		if (m_itemInfoMap.at((int)lvItem.lParam).wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			return FALSE;

		if (bGhost)
		{
			ListView_SetItemState(m_hListView, iItem, LVIS_CUT, LVIS_CUT);
		}
		else
		{
			ListView_SetItemState(m_hListView, iItem, 0, LVIS_CUT);
		}
	}

	return TRUE;
}

void CShellBrowser::ShowPropertiesForSelectedFiles() const
{
	std::vector<unique_pidl_child> pidls;
	std::vector<PCITEMID_CHILD> rawPidls;

	int item = -1;

	while ((item = ListView_GetNextItem(m_hListView, item, LVNI_SELECTED)) != -1)
	{
		auto pidl = GetItemChildIdl(item);

		rawPidls.push_back(pidl.get());
		pidls.push_back(std::move(pidl));
	}

	if (rawPidls.empty())
	{
		return;
	}

	auto pidlDirectory = GetDirectoryIdl();
	ShowMultipleFileProperties(pidlDirectory.get(), rawPidls.data(), m_hOwner, static_cast<int>(rawPidls.size()));
}