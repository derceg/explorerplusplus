// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "Config.h"
#include "ItemData.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "SelectColumnsDialog.h"
#include "SetFileAttributesDialog.h"
#include "ShellNavigationController.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/Helper.h"
#include "../Helper/IconFetcher.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ShellHelper.h"
#include <boost/format.hpp>

const std::vector<unsigned int> COMMON_REAL_FOLDER_COLUMNS =
{ CM_NAME, CM_TYPE, CM_SIZE, CM_DATEMODIFIED,
CM_AUTHORS, CM_TITLE };

const std::vector<unsigned int> COMMON_CONTROL_PANEL_COLUMNS =
{ CM_NAME, CM_VIRTUALCOMMENTS };

const std::vector<unsigned int> COMMON_MY_COMPUTER_COLUMNS =
{ CM_NAME, CM_TYPE, CM_TOTALSIZE,
CM_FREESPACE, CM_VIRTUALCOMMENTS,
CM_FILESYSTEM };

const std::vector<unsigned int> COMMON_NETWORK_CONNECTIONS_COLUMNS =
{ CM_NAME, CM_TYPE, CM_NETWORKADAPTER_STATUS,
CM_OWNER };

const std::vector<unsigned int> COMMON_NETWORK_COLUMNS =
{ CM_NAME, CM_VIRTUALCOMMENTS };

const std::vector<unsigned int> COMMON_PRINTERS_COLUMNS =
{ CM_NAME, CM_NUMPRINTERDOCUMENTS, CM_PRINTERSTATUS,
CM_PRINTERCOMMENTS, CM_PRINTERLOCATION };

const std::vector<unsigned int> COMMON_RECYCLE_BIN_COLUMNS =
{ CM_NAME, CM_ORIGINALLOCATION, CM_DATEDELETED,
CM_SIZE, CM_TYPE, CM_DATEMODIFIED };

std::vector<unsigned int> GetColumnHeaderMenuList(const std::wstring &directory);

LRESULT CALLBACK ShellBrowser::ListViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *shellBrowser = reinterpret_cast<ShellBrowser *>(dwRefData);
	return shellBrowser->ListViewProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK ShellBrowser::ListViewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

LRESULT CALLBACK ShellBrowser::ListViewParentProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *shellBrowser = reinterpret_cast<ShellBrowser *>(dwRefData);
	return shellBrowser->ListViewParentProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK ShellBrowser::ListViewParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

			case LVN_ITEMCHANGED:
				OnListViewItemChanged(reinterpret_cast<NMLISTVIEW *>(lParam));
				break;

			case LVN_KEYDOWN:
				OnListViewKeyDown(reinterpret_cast<NMLVKEYDOWN *>(lParam));
				break;

			case LVN_COLUMNCLICK:
				ColumnClicked(reinterpret_cast<NMLISTVIEW *>(lParam)->iSubItem);
				break;
			}
		}
		else if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == ListView_GetHeader(m_hListView))
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_RCLICK:
			{
				DWORD messagePos = GetMessagePos();
				OnListViewHeaderRightClick(MAKEPOINTS(messagePos));
			}
			break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void ShellBrowser::OnListViewMButtonDown(const POINT *pt)
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

void ShellBrowser::OnListViewMButtonUp(const POINT *pt)
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

void ShellBrowser::OnListViewGetDisplayInfo(LPARAM lParam)
{
	NMLVDISPINFO	*pnmv = nullptr;
	LVITEM			*plvItem = nullptr;

	pnmv = (NMLVDISPINFO *)lParam;
	plvItem = &pnmv->item;

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

		m_iconFetcher->QueueIconTask(itemInfo.pidlComplete.get(), [this, internalIndex] (int iconIndex) {
			ProcessIconResult(internalIndex, iconIndex);
		});
	}

	plvItem->mask |= LVIF_DI_SETITEM;
}

std::optional<int> ShellBrowser::GetCachedIconIndex(const ItemInfo_t &itemInfo)
{
	TCHAR filePath[MAX_PATH];
	HRESULT hr = GetDisplayName(itemInfo.pidlComplete.get(),
		filePath, SIZEOF_ARRAY(filePath), SHGDN_FORPARSING);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	auto cachedItr = m_cachedIcons->findByPath(filePath);

	if (cachedItr == m_cachedIcons->end())
	{
		return std::nullopt;
	}

	return cachedItr->iconIndex;
}

void ShellBrowser::ProcessIconResult(int internalIndex, int iconIndex)
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

LRESULT ShellBrowser::OnListViewGetInfoTip(NMLVGETINFOTIP *getInfoTip)
{
	if (m_config->showInfoTips)
	{
		int internalIndex = GetItemInternalIndex(getInfoTip->iItem);
		QueueInfoTipTask(internalIndex, getInfoTip->pszText);
	}

	StringCchCopy(getInfoTip->pszText, getInfoTip->cchTextMax, EMPTY_STRING);

	return 0;
}

void ShellBrowser::QueueInfoTipTask(int internalIndex, const std::wstring &existingInfoTip)
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

std::optional<ShellBrowser::InfoTipResult> ShellBrowser::GetInfoTipAsync(HWND listView, int infoTipResultId,
	int internalIndex, const BasicItemInfo_t &basicItemInfo, const Config &config, HINSTANCE instance, bool virtualFolder)
{
	std::wstring infoTip;

	/* Use Explorer infotips if the option is selected, or this is a
	virtual folder. Otherwise, show the modified date. */
	if ((config.infoTipType == InfoTipType::System) || virtualFolder)
	{
		TCHAR infoTipText[256];
		HRESULT hr = GetItemInfoTip(basicItemInfo.pidlComplete.get(), infoTipText, SIZEOF_ARRAY(infoTipText));

		if (FAILED(hr))
		{
			return std::nullopt;
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
			return std::nullopt;
		}

		infoTip = str(boost::wformat(_T("%s: %s")) % dateModified % fileModificationText);
	}

	PostMessage(listView, WM_APP_INFO_TIP_READY, infoTipResultId, 0);

	InfoTipResult result;
	result.itemInternalIndex = internalIndex;
	result.infoTip = infoTip;

	return result;
}

void ShellBrowser::ProcessInfoTipResult(int infoTipResultId)
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

void ShellBrowser::OnListViewItemChanged(const NMLISTVIEW *changeData)
{
	if (changeData->uChanged != LVIF_STATE)
	{
		return;
	}

	if (m_config->checkBoxSelection && (LVIS_STATEIMAGEMASK & changeData->uNewState) != 0)
	{
		bool checked = ((changeData->uNewState & LVIS_STATEIMAGEMASK) >> 12) == 2;
		NListView::ListView_SelectItem(m_hListView, changeData->iItem, checked);
	}

	bool previouslySelected = WI_IsFlagSet(changeData->uOldState, LVIS_SELECTED);
	bool currentlySelected = WI_IsFlagSet(changeData->uNewState, LVIS_SELECTED);

	if (previouslySelected == currentlySelected)
	{
		return;
	}

	if (m_bPerformingDrag)
	{
		return;
	}

	if (m_config->checkBoxSelection)
	{
		if (!previouslySelected && currentlySelected)
		{
			ListView_SetCheckState(m_hListView, changeData->iItem, TRUE);
		}
		else if (previouslySelected && !currentlySelected)
		{
			ListView_SetCheckState(m_hListView, changeData->iItem, FALSE);
		}
	}

	UpdateFileSelectionInfo(static_cast<int>(changeData->lParam), currentlySelected);

	listViewSelectionChanged.m_signal();
}

void ShellBrowser::UpdateFileSelectionInfo(int internalIndex, BOOL selected)
{
	ULARGE_INTEGER	ulFileSize;
	BOOL			isFolder;

	isFolder = (m_itemInfoMap.at(internalIndex).wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		== FILE_ATTRIBUTE_DIRECTORY;

	ulFileSize.LowPart = m_itemInfoMap.at(internalIndex).wfd.nFileSizeLow;
	ulFileSize.HighPart = m_itemInfoMap.at(internalIndex).wfd.nFileSizeHigh;

	if (selected)
	{
		if (isFolder)
			m_NumFoldersSelected++;
		else
			m_NumFilesSelected++;

		m_ulFileSelectionSize.QuadPart += ulFileSize.QuadPart;
	}
	else
	{
		if (isFolder)
			m_NumFoldersSelected--;
		else
			m_NumFilesSelected--;

		m_ulFileSelectionSize.QuadPart -= ulFileSize.QuadPart;
	}
}

void ShellBrowser::OnListViewKeyDown(const NMLVKEYDOWN *lvKeyDown)
{
	switch (lvKeyDown->wVKey)
	{
	case 'A':
		if (IsKeyDown(VK_CONTROL) &&
			!IsKeyDown(VK_SHIFT) &&
			!IsKeyDown(VK_MENU))
		{
			NListView::ListView_SelectAllItems(m_hListView, TRUE);
			SetFocus(m_hListView);
		}
		break;

	case 'I':
		if (IsKeyDown(VK_CONTROL) &&
			!IsKeyDown(VK_SHIFT) &&
			!IsKeyDown(VK_MENU))
		{
			NListView::ListView_InvertSelection(m_hListView);
			SetFocus(m_hListView);
		}
		break;

	case VK_BACK:
		if (IsKeyDown(VK_CONTROL) &&
			!IsKeyDown(VK_SHIFT) &&
			!IsKeyDown(VK_MENU))
		{
			TCHAR szRoot[MAX_PATH];
			HRESULT hr = GetDisplayName(m_directoryState.pidlDirectory.get(), szRoot, SIZEOF_ARRAY(szRoot), SHGDN_FORPARSING);

			if (SUCCEEDED(hr))
			{
				BOOL bRes = PathStripToRoot(szRoot);

				if (bRes)
				{
					m_navigationController->BrowseFolder(szRoot);
				}
			}
		}
		else
		{
			m_navigationController->GoUp();
		}
		break;
	}
}

ShellBrowser::ItemInfo_t &ShellBrowser::GetItemByIndex(int index)
{
	int internalIndex = GetItemInternalIndex(index);
	return m_itemInfoMap.at(internalIndex);
}

int ShellBrowser::GetItemInternalIndex(int item) const
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

BOOL ShellBrowser::GhostItem(int iItem)
{
	return GhostItemInternal(iItem, TRUE);
}

BOOL ShellBrowser::DeghostItem(int iItem)
{
	return GhostItemInternal(iItem, FALSE);
}

BOOL ShellBrowser::GhostItemInternal(int iItem, BOOL bGhost)
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

void ShellBrowser::ShowPropertiesForSelectedFiles() const
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

void ShellBrowser::OnListViewHeaderRightClick(const POINTS &cursorPos)
{
	wil::unique_hmenu headerPopupMenu(LoadMenu(m_hResourceModule, MAKEINTRESOURCE(IDR_HEADER_MENU)));
	HMENU headerMenu = GetSubMenu(headerPopupMenu.get(), 0);

	auto commonColumns = GetColumnHeaderMenuList(m_CurDir);

	std::unordered_map<int, UINT> menuItemMappings;
	int totalInserted = 0;
	int commonColumnPosition = 0;

	for (const auto &column : *m_pActiveColumns)
	{
		auto itr = std::find(commonColumns.begin(), commonColumns.end(), column.id);
		bool inCommonColumns = (itr != commonColumns.end());

		if (!column.bChecked && !inCommonColumns)
		{
			continue;
		}

		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING | MIIM_STATE | MIIM_ID;

		std::wstring columnText = ResourceHelper::LoadString(m_hResourceModule,
			LookupColumnNameStringIndex(column.id));

		if (column.bChecked)
		{
			mii.fState = MFS_CHECKED;
		}
		else
		{
			mii.fState = MFS_ENABLED;
		}

		int currentPosition;

		if (inCommonColumns)
		{
			// The common columns always appear first, whether they're checked
			// or not.
			currentPosition = commonColumnPosition;
			commonColumnPosition++;
		}
		else
		{
			currentPosition = totalInserted;
		}

		int id = totalInserted + 1;

		mii.dwTypeData = columnText.data();
		mii.wID = id;
		InsertMenuItem(headerMenu, currentPosition, TRUE, &mii);

		menuItemMappings.insert({ id, column.id });

		totalInserted++;
	}

	int cmd = TrackPopupMenu(headerMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERTICAL | TPM_RETURNCMD,
		cursorPos.x, cursorPos.y, 0, m_hListView, nullptr);

	if (cmd == 0)
	{
		return;
	}

	OnListViewHeaderMenuItemSelected(cmd, menuItemMappings);
}

std::vector<unsigned int> GetColumnHeaderMenuList(const std::wstring &directory)
{
	if (CompareVirtualFolders(directory.c_str(), CSIDL_DRIVES))
	{
		return COMMON_MY_COMPUTER_COLUMNS;
	}
	else if (CompareVirtualFolders(directory.c_str(), CSIDL_CONTROLS))
	{
		return COMMON_CONTROL_PANEL_COLUMNS;
	}
	else if (CompareVirtualFolders(directory.c_str(), CSIDL_BITBUCKET))
	{
		return COMMON_RECYCLE_BIN_COLUMNS;
	}
	else if (CompareVirtualFolders(directory.c_str(), CSIDL_CONNECTIONS))
	{
		return COMMON_NETWORK_CONNECTIONS_COLUMNS;
	}
	else if (CompareVirtualFolders(directory.c_str(), CSIDL_NETWORK))
	{
		return COMMON_NETWORK_COLUMNS;
	}
	else if (CompareVirtualFolders(directory.c_str(), CSIDL_PRINTERS))
	{
		return COMMON_PRINTERS_COLUMNS;
	}
	else
	{
		return COMMON_REAL_FOLDER_COLUMNS;
	}
}

void ShellBrowser::OnListViewHeaderMenuItemSelected(int menuItemId,
	const std::unordered_map<int, UINT> &menuItemMappings)
{
	if (menuItemId == IDM_HEADER_MORE)
	{
		OnShowMoreColumnsSelected();
	}
	else
	{
		OnColumnMenuItemSelected(menuItemId, menuItemMappings);
	}
}

void ShellBrowser::OnShowMoreColumnsSelected()
{
	SelectColumnsDialog selectColumnsDialog(
		m_hResourceModule, m_hListView, this, m_iconResourceLoader);
	selectColumnsDialog.ShowModalDialog();
}

void ShellBrowser::OnColumnMenuItemSelected(
	int menuItemId, const std::unordered_map<int, UINT> &menuItemMappings)
{
	auto currentColumns = ExportCurrentColumns();

	UINT columnId = menuItemMappings.at(menuItemId);
	auto itr = std::find_if(
		currentColumns.begin(), currentColumns.end(), [columnId](const Column_t &column) {
			return column.id == columnId;
		});

	if (itr == currentColumns.end())
	{
		return;
	}

	itr->bChecked = !itr->bChecked;

	ImportColumns(currentColumns);

	// If it was the first column that was changed, need to refresh all columns.
	if (menuItemId == 1)
	{
		m_navigationController->Refresh();
	}
}

void ShellBrowser::SetFileAttributesForSelection()
{
	std::list<NSetFileAttributesDialogExternal::SetFileAttributesInfo_t> sfaiList;
	int index = -1;

	while ((index = ListView_GetNextItem(m_hListView, index, LVNI_SELECTED)) != -1)
	{
		NSetFileAttributesDialogExternal::SetFileAttributesInfo_t sfai;

		const ItemInfo_t &item = GetItemByIndex(index);
		sfai.wfd = item.wfd;

		GetDisplayName(item.pidlComplete.get(), sfai.szFullFileName,
			static_cast<UINT>(std::size(sfai.szFullFileName)), SHGDN_FORPARSING);

		sfaiList.push_back(sfai);
	}

	SetFileAttributesDialog setFileAttributesDialog(m_hResourceModule, m_hListView, sfaiList);
	setFileAttributesDialog.ShowModalDialog();
}