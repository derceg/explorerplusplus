// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "../Helper/CachedIcons.h"

void CShellBrowser::QueueIconTask(int internalIndex)
{
	int iconResultID = m_iconResultIDCounter++;

	BasicItemInfo_t basicItemInfo = getBasicItemInfo(internalIndex);

	auto result = m_itemImageThreadPool.push([this, iconResultID, internalIndex, basicItemInfo](int id) {
		UNREFERENCED_PARAMETER(id);

		return FindIconAsync(m_hListView, iconResultID, internalIndex, basicItemInfo);
	});

	m_iconResults.insert({ iconResultID, std::move(result) });
}

boost::optional<CShellBrowser::IconResult_t> CShellBrowser::FindIconAsync(HWND listView, int iconResultId, int internalIndex,
	const BasicItemInfo_t &basicItemInfo)
{
	// Must use SHGFI_ICON here, rather than SHGFO_SYSICONINDEX, or else 
	// icon overlays won't be applied.
	SHFILEINFO shfi;
	DWORD_PTR res = SHGetFileInfo(reinterpret_cast<LPCTSTR>(basicItemInfo.pidlComplete.get()),
		0, &shfi, sizeof(SHFILEINFO), SHGFI_PIDL | SHGFI_ICON | SHGFI_OVERLAYINDEX);

	if (res == 0)
	{
		return boost::none;
	}

	DestroyIcon(shfi.hIcon);

	PostMessage(listView, WM_APP_ICON_RESULT_READY, iconResultId, 0);

	IconResult_t result;
	result.itemInternalIndex = internalIndex;
	result.iconIndex = shfi.iIcon;

	return result;
}

void CShellBrowser::ProcessIconResult(int iconResultId)
{
	auto itr = m_iconResults.find(iconResultId);

	if (itr == m_iconResults.end())
	{
		return;
	}

	auto result = itr->second.get();

	if (!result)
	{
		// Icon lookup failed.
		return;
	}

	auto index = LocateItemByInternalIndex(result->itemInternalIndex);

	if (!index)
	{
		return;
	}

	const ItemInfo_t &itemInfo = m_itemInfoMap.at(result->itemInternalIndex);

	TCHAR filePath[MAX_PATH];
	HRESULT hr = GetDisplayName(itemInfo.pidlComplete.get(), filePath, static_cast<UINT>(std::size(filePath)), SHGDN_FORPARSING);

	if (SUCCEEDED(hr))
	{
		m_cachedIcons->addOrUpdateFileIcon(filePath, result->iconIndex);
	}

	LVITEM lvItem;
	lvItem.mask = LVIF_IMAGE | LVIF_STATE;
	lvItem.iItem = *index;
	lvItem.iSubItem = 0;
	lvItem.iImage = result->iconIndex;
	lvItem.stateMask = LVIS_OVERLAYMASK;
	lvItem.state = INDEXTOOVERLAYMASK(result->iconIndex >> 24);
	ListView_SetItem(m_hListView, &lvItem);
}