// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowserImpl.h"
#include "App.h"
#include "ColorRuleModel.h"
#include "ColumnHelper.h"
#include "Config.h"
#include "IconFetcher.h"
#include "ItemData.h"
#include "ItemNameEditControl.h"
#include "MainResource.h"
#include "NavigateParams.h"
#include "ResourceHelper.h"
#include "SelectColumnsDialog.h"
#include "SetFileAttributesDialog.h"
#include "ShellNavigationController.h"
#include "TabNavigationInterface.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/DragDropHelper.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ShellHelper.h"
#include <glog/logging.h>
#include <wil/common.h>
#include <format>

const std::vector<ColumnType> COMMON_REAL_FOLDER_COLUMNS = { ColumnType::Name, ColumnType::Type,
	ColumnType::Size, ColumnType::DateModified, ColumnType::Authors, ColumnType::Title };

const std::vector<ColumnType> COMMON_CONTROL_PANEL_COLUMNS = { ColumnType::Name,
	ColumnType::VirtualComments };

const std::vector<ColumnType> COMMON_MY_COMPUTER_COLUMNS = { ColumnType::Name, ColumnType::Type,
	ColumnType::TotalSize, ColumnType::FreeSpace, ColumnType::VirtualComments,
	ColumnType::FileSystem };

const std::vector<ColumnType> COMMON_NETWORK_CONNECTIONS_COLUMNS = { ColumnType::Name,
	ColumnType::Type, ColumnType::NetworkAdaptorStatus, ColumnType::Owner };

const std::vector<ColumnType> COMMON_NETWORK_COLUMNS = { ColumnType::Name,
	ColumnType::VirtualComments };

const std::vector<ColumnType> COMMON_PRINTERS_COLUMNS = { ColumnType::Name,
	ColumnType::PrinterNumDocuments, ColumnType::PrinterStatus, ColumnType::PrinterComments,
	ColumnType::PrinterLocation };

const std::vector<ColumnType> COMMON_RECYCLE_BIN_COLUMNS = { ColumnType::Name,
	ColumnType::OriginalLocation, ColumnType::DateDeleted, ColumnType::Size, ColumnType::Type,
	ColumnType::DateModified };

std::vector<ColumnType> GetColumnHeaderMenuList(const std::wstring &directory);

LRESULT ShellBrowserImpl::ListViewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (m_getDragImageMessage != 0 && uMsg == m_getDragImageMessage)
	{
		// The listview control has built-in handling for this message (DI_GETDRAGIMAGE). It will,
		// by default, build an image based on the item being dragged. However, that's undesirable
		// here. When using SHDoDragDrop(), the drag image will be set up by that method. If the
		// listview is allowed to process the DI_GETDRAGIMAGE message, it will set the default
		// image. So, returning FALSE here allows SHDoDragDrop() to set up the image itself.
		return FALSE;
	}

	switch (uMsg)
	{
	// NM_DBLCLK for the listview is sent both on double clicks (by default), as well as in the
	// situation when LVS_EX_ONECLICKACTIVATE is active (in which case it's sent on a single mouse
	// click). Navigation up should only occur on a double click, which is why WM_LBUTTONDBLCLK is
	// used.
	case WM_LBUTTONDBLCLK:
	{
		POINT pt;
		POINTSTOPOINT(pt, MAKEPOINTS(lParam));
		bool handled = OnListViewLeftButtonDoubleClick(&pt);

		if (handled)
		{
			return 0;
		}
	}
	break;

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
		OnListViewMButtonUp(&pt, static_cast<UINT>(wParam));
	}
	break;

	// Note that the specific HANDLE_WM_RBUTTONDOWN message cracker is used here, rather than the
	// more generic message cracker HANDLE_MSG because it's important that the listview control
	// itself receive this message. Returning 0 would prevent that from happening.
	case WM_RBUTTONDOWN:
		HANDLE_WM_RBUTTONDOWN(hwnd, wParam, lParam, OnRButtonDown);
		break;

	case WM_MOUSEWHEEL:
		if (OnMouseWheel(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), GET_WHEEL_DELTA_WPARAM(wParam),
				GET_KEYSTATE_WPARAM(wParam)))
		{
			return 0;
		}
		break;

	case WM_SETCURSOR:
		if (OnSetCursor(reinterpret_cast<HWND>(wParam)))
		{
			return TRUE;
		}
		break;

	case WM_CLIPBOARDUPDATE:
		OnClipboardUpdate();
		return 0;

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

LRESULT ShellBrowserImpl::ListViewParentProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hListView)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case LVN_BEGINDRAG:
				OnListViewBeginDrag(reinterpret_cast<NMLISTVIEW *>(lParam));
				break;

			case LVN_BEGINRDRAG:
				OnListViewBeginRightClickDrag(reinterpret_cast<NMLISTVIEW *>(lParam));
				break;

			case LVN_GETDISPINFO:
				OnListViewGetDisplayInfo(lParam);
				break;

			case LVN_GETINFOTIP:
				return OnListViewGetInfoTip(reinterpret_cast<NMLVGETINFOTIP *>(lParam));

			case LVN_GETEMPTYMARKUP:
				return OnListViewGetEmptyMarkup(reinterpret_cast<NMLVEMPTYMARKUP *>(lParam));

			case LVN_INSERTITEM:
				OnListViewItemInserted(reinterpret_cast<NMLISTVIEW *>(lParam));
				break;

			case LVN_ITEMCHANGED:
				OnListViewItemChanged(reinterpret_cast<NMLISTVIEW *>(lParam));
				break;

			case LVN_KEYDOWN:
				OnListViewKeyDown(reinterpret_cast<NMLVKEYDOWN *>(lParam));
				break;

			case LVN_COLUMNCLICK:
				ColumnClicked(reinterpret_cast<NMLISTVIEW *>(lParam)->iSubItem);
				break;

			case LVN_BEGINLABELEDIT:
				return OnListViewBeginLabelEdit(reinterpret_cast<NMLVDISPINFO *>(lParam));

			case LVN_ENDLABELEDIT:
				return OnListViewEndLabelEdit(reinterpret_cast<NMLVDISPINFO *>(lParam));

			case LVN_DELETEALLITEMS:
				// Respond to the notification in order to speed up calls to ListView_DeleteAllItems
				// per http://www.verycomputer.com/5_0c959e6a4fd713e2_1.htm
				return TRUE;

			case NM_CUSTOMDRAW:
				return OnListViewCustomDraw(reinterpret_cast<NMLVCUSTOMDRAW *>(lParam));
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

bool ShellBrowserImpl::OnListViewLeftButtonDoubleClick(const POINT *pt)
{
	if (!m_config->goUpOnDoubleClick)
	{
		return false;
	}

	LV_HITTESTINFO ht;
	ht.pt = *pt;
	ListView_HitTest(m_hListView, &ht);

	if (ht.flags != LVHT_NOWHERE)
	{
		return false;
	}

	m_navigationController->GoUp();

	return true;
}

void ShellBrowserImpl::OnListViewMButtonDown(const POINT *pt)
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

void ShellBrowserImpl::OnListViewMButtonUp(const POINT *pt, UINT keysDown)
{
	LV_HITTESTINFO ht;
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

	if (!WI_IsAnyFlagSet(itemInfo.wfd.dwFileAttributes,
			FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE))
	{
		return;
	}

	bool switchToNewTab = m_config->openTabsInForeground;

	if (WI_IsFlagSet(keysDown, MK_SHIFT))
	{
		switchToNewTab = !switchToNewTab;
	}

	auto navigateParams = NavigateParams::Normal(itemInfo.pidlComplete.Raw());
	m_tabNavigation->CreateNewTab(navigateParams, switchToNewTab);
}

void ShellBrowserImpl::OnRButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keyFlags)
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(doubleClick);

	// If shift is held down while right-clicking an item, it appears the listview control won't
	// select the item. Which is why the functionality is implemented here.
	if (WI_IsFlagSet(keyFlags, MK_SHIFT))
	{
		LVHITTESTINFO hitTestInfo = {};
		hitTestInfo.pt = { x, y };
		int itemAtPoint = ListView_HitTest(m_hListView, &hitTestInfo);

		if (itemAtPoint != -1
			&& ListView_GetItemState(m_hListView, itemAtPoint, LVIS_SELECTED) != LVIS_SELECTED)
		{
			ListViewHelper::SelectAllItems(m_hListView, false);
			ListViewHelper::FocusItem(m_hListView, itemAtPoint, true);
			ListViewHelper::SelectItem(m_hListView, itemAtPoint, true);
		}
	}
}

bool ShellBrowserImpl::OnMouseWheel(int xPos, int yPos, int delta, UINT keys)
{
	UNREFERENCED_PARAMETER(xPos);
	UNREFERENCED_PARAMETER(yPos);

	if (WI_IsFlagSet(keys, MK_CONTROL))
	{
		// Switch listview views. For each wheel delta (notch) the wheel is scrolled through, switch
		// the view once.
		for (int i = 0; i < abs(delta / WHEEL_DELTA); i++)
		{
			CycleViewMode(delta > 0);
		}

		return true;
	}
	else if (WI_IsFlagSet(keys, MK_SHIFT))
	{
		int offset = delta / WHEEL_DELTA;

		if (offset != 0)
		{
			GetNavigationController()->GoToOffset(offset);
		}

		return true;
	}

	return false;
}

bool ShellBrowserImpl::OnSetCursor(HWND target)
{
	if (target != m_hListView)
	{
		return false;
	}

	if (!m_navigationManager.HasAnyActiveNavigations())
	{
		return false;
	}

	SetCursor(m_progressCursor);

	return true;
}

void ShellBrowserImpl::OnListViewGetDisplayInfo(LPARAM lParam)
{
	NMLVDISPINFO *pnmv = nullptr;
	LVITEM *plvItem = nullptr;

	pnmv = (NMLVDISPINFO *) lParam;
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
	if (IsThumbnailsViewMode(m_folderSettings.viewMode)
		&& (plvItem->mask & LVIF_IMAGE) == LVIF_IMAGE)
	{
		const ItemInfo_t &itemInfo = m_itemInfoMap.at(internalIndex);
		auto cachedThumbnailIndex = GetCachedThumbnailIndex(itemInfo);

		if (cachedThumbnailIndex)
		{
			plvItem->iImage = *cachedThumbnailIndex;
		}
		else
		{
			plvItem->iImage = GetIconThumbnail(internalIndex);
		}

		plvItem->mask |= LVIF_DI_SETITEM;

		QueueThumbnailTask(internalIndex);

		return;
	}

	if (m_folderSettings.viewMode == +ViewMode::Details && (plvItem->mask & LVIF_TEXT) == LVIF_TEXT)
	{
		auto columnType = GetColumnTypeByIndex(plvItem->iSubItem);
		CHECK(columnType);

		QueueColumnTask(internalIndex, *columnType);
	}

	if ((plvItem->mask & LVIF_IMAGE) == LVIF_IMAGE)
	{
		const ItemInfo_t &itemInfo = m_itemInfoMap.at(internalIndex);
		auto cachedIconIndex = m_cachedIcons->MaybeGetIconIndex(itemInfo.parsingName);

		if (cachedIconIndex)
		{
			// Note that only the icon is set here. Any overlay will be added by the icon retrieval
			// task (scheduled below).
			plvItem->iImage = *cachedIconIndex;
		}
		else
		{
			if ((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				== FILE_ATTRIBUTE_DIRECTORY)
			{
				plvItem->iImage = m_iFolderIcon;
			}
			else
			{
				plvItem->iImage = m_iFileIcon;
			}
		}

		m_iconFetcher->QueueIconTask(itemInfo.pidlComplete.Raw(),
			[this, internalIndex](int iconIndex, int overlayIndex)
			{ ProcessIconResult(internalIndex, iconIndex, overlayIndex); });
	}

	plvItem->mask |= LVIF_DI_SETITEM;
}

void ShellBrowserImpl::ProcessIconResult(int internalIndex, int iconIndex, int overlayIndex)
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
	lvItem.state = INDEXTOOVERLAYMASK(overlayIndex);
	ListView_SetItem(m_hListView, &lvItem);
}

LRESULT ShellBrowserImpl::OnListViewGetInfoTip(NMLVGETINFOTIP *getInfoTip)
{
	if (m_config->showInfoTips)
	{
		int internalIndex = GetItemInternalIndex(getInfoTip->iItem);
		QueueInfoTipTask(internalIndex, getInfoTip->pszText);
	}

	StringCchCopy(getInfoTip->pszText, getInfoTip->cchTextMax, L"");

	return 0;
}

BOOL ShellBrowserImpl::OnListViewGetEmptyMarkup(NMLVEMPTYMARKUP *emptyMarkup)
{
	emptyMarkup->dwFlags = EMF_CENTERED;

	auto folderEmptyText =
		ResourceHelper::LoadString(m_resourceInstance, IDS_LISTVIEW_FOLDER_EMPTY);
	StringCchCopy(emptyMarkup->szMarkup, std::size(emptyMarkup->szMarkup), folderEmptyText.c_str());

	return TRUE;
}

void ShellBrowserImpl::QueueInfoTipTask(int internalIndex, const std::wstring &existingInfoTip)
{
	int infoTipResultId = m_infoTipResultIDCounter++;

	BasicItemInfo_t basicItemInfo = getBasicItemInfo(internalIndex);
	Config configCopy = *m_config;
	bool virtualFolder = InVirtualFolder();

	auto result = m_infoTipsThreadPool.push(
		[this, infoTipResultId, internalIndex, basicItemInfo, configCopy, virtualFolder,
			existingInfoTip](int id)
		{
			UNREFERENCED_PARAMETER(id);

			auto result = GetInfoTipAsync(m_hListView, infoTipResultId, internalIndex,
				basicItemInfo, configCopy, m_resourceInstance, virtualFolder);

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

std::optional<ShellBrowserImpl::InfoTipResult> ShellBrowserImpl::GetInfoTipAsync(HWND listView,
	int infoTipResultId, int internalIndex, const BasicItemInfo_t &basicItemInfo,
	const Config &config, HINSTANCE resourceInstance, bool virtualFolder)
{
	std::wstring infoTip;

	/* Use Explorer infotips if the option is selected, or this is a
	virtual folder. Otherwise, show the modified date. */
	if ((config.infoTipType == +InfoTipType::System) || virtualFolder)
	{
		std::wstring infoTipText;
		HRESULT hr = GetItemInfoTip(basicItemInfo.pidlComplete.get(), infoTipText);

		if (FAILED(hr))
		{
			return std::nullopt;
		}

		infoTip = infoTipText;
	}
	else
	{
		auto dateModified = ResourceHelper::LoadString(resourceInstance, IDS_GENERAL_DATEMODIFIED);

		TCHAR fileModificationText[256];
		BOOL fileTimeResult =
			CreateFileTimeString(&basicItemInfo.wfd.ftLastWriteTime, fileModificationText,
				std::size(fileModificationText), config.globalFolderSettings.showFriendlyDates);

		if (!fileTimeResult)
		{
			return std::nullopt;
		}

		infoTip = std::format(L"{}: {}", dateModified, fileModificationText);
	}

	PostMessage(listView, WM_APP_INFO_TIP_READY, infoTipResultId, 0);

	InfoTipResult result;
	result.itemInternalIndex = internalIndex;
	result.infoTip = infoTip;

	return result;
}

void ShellBrowserImpl::ProcessInfoTipResult(int infoTipResultId)
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
	StringCchCopy(infoTipText, std::size(infoTipText), result->infoTip.c_str());

	LVSETINFOTIP infoTip;
	infoTip.cbSize = sizeof(infoTip);
	infoTip.dwFlags = 0;
	infoTip.iItem = *index;
	infoTip.iSubItem = 0;
	infoTip.pszText = infoTipText;
	ListView_SetInfoTip(m_hListView, &infoTip);
}

void ShellBrowserImpl::OnListViewItemInserted(const NMLISTVIEW *itemData)
{
	if (m_folderSettings.showInGroups)
	{
		auto groupId = GetItemGroupId(itemData->iItem);

		if (groupId)
		{
			OnItemAddedToGroup(*groupId);
		}
	}
}

void ShellBrowserImpl::OnListViewItemChanged(const NMLISTVIEW *changeData)
{
	if (changeData->uChanged != LVIF_STATE)
	{
		return;
	}

	if (m_config->checkBoxSelection.get() && (LVIS_STATEIMAGEMASK & changeData->uNewState) != 0)
	{
		bool checked = ((changeData->uNewState & LVIS_STATEIMAGEMASK) >> 12) == 2;
		ListViewHelper::SelectItem(m_hListView, changeData->iItem, checked);
	}

	bool previouslySelected = WI_IsFlagSet(changeData->uOldState, LVIS_SELECTED);
	bool currentlySelected = WI_IsFlagSet(changeData->uNewState, LVIS_SELECTED);

	if (previouslySelected == currentlySelected)
	{
		return;
	}

	if (m_config->checkBoxSelection.get())
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

	m_app->GetShellBrowserEvents()->NotifySelectionChanged(this);
}

void ShellBrowserImpl::UpdateFileSelectionInfo(int internalIndex, BOOL selected)
{
	ULARGE_INTEGER ulFileSize;
	BOOL isFolder;

	isFolder = (m_itemInfoMap.at(internalIndex).wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		== FILE_ATTRIBUTE_DIRECTORY;

	ulFileSize.LowPart = m_itemInfoMap.at(internalIndex).wfd.nFileSizeLow;
	ulFileSize.HighPart = m_itemInfoMap.at(internalIndex).wfd.nFileSizeHigh;

	if (selected)
	{
		if (isFolder)
		{
			m_directoryState.numFoldersSelected++;
		}
		else
		{
			m_directoryState.numFilesSelected++;
		}

		m_directoryState.fileSelectionSize += ulFileSize.QuadPart;
	}
	else
	{
		if (isFolder)
		{
			m_directoryState.numFoldersSelected--;
		}
		else
		{
			m_directoryState.numFilesSelected--;
		}

		m_directoryState.fileSelectionSize -= ulFileSize.QuadPart;
	}
}

void ShellBrowserImpl::OnListViewKeyDown(const NMLVKEYDOWN *lvKeyDown)
{
	switch (lvKeyDown->wVKey)
	{
	case 'A':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			ListViewHelper::SelectAllItems(m_hListView, true);
			SetFocus(m_hListView);
		}
		break;

	case VK_INSERT:
	case 'C':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			CopySelectedItemsToClipboard(true);
		}
		break;

	case 'I':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			ListViewHelper::InvertSelection(m_hListView);
			SetFocus(m_hListView);
		}
		break;

	case 'X':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			CopySelectedItemsToClipboard(false);
		}
		break;

	case 'Z':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			m_fileActionHandler->Undo();
		}
		break;

	case VK_BACK:
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			TCHAR root[MAX_PATH];
			HRESULT hr = StringCchCopy(root, std::size(root), m_directoryState.directory.c_str());

			if (SUCCEEDED(hr))
			{
				BOOL bRes = PathStripToRoot(root);

				if (bRes)
				{
					m_navigationController->Navigate(root);
				}
			}
		}
		else
		{
			m_navigationController->GoUp();
		}
		break;

	case VK_DELETE:
		DeleteSelectedItems(IsKeyDown(VK_SHIFT));
		break;

	case VK_ESCAPE:
		m_navigationManager.StopLoading();
		break;
	}
}

const ShellBrowserImpl::ItemInfo_t &ShellBrowserImpl::GetItemByIndex(int index) const
{
	int internalIndex = GetItemInternalIndex(index);
	return m_itemInfoMap.at(internalIndex);
}

ShellBrowserImpl::ItemInfo_t &ShellBrowserImpl::GetItemByIndex(int index)
{
	int internalIndex = GetItemInternalIndex(index);
	return m_itemInfoMap.at(internalIndex);
}

int ShellBrowserImpl::GetItemInternalIndex(int item) const
{
	LVITEM lvItem;
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = item;
	lvItem.iSubItem = 0;
	BOOL res = ListView_GetItem(m_hListView, &lvItem);
	CHECK(res);

	return static_cast<int>(lvItem.lParam);
}

void ShellBrowserImpl::MarkItemAsCut(int item, bool cut)
{
	const auto &itemInfo = GetItemByIndex(item);

	// If the file is hidden, prevent changes to its visibility state.
	if (WI_IsFlagSet(itemInfo.wfd.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN))
	{
		return;
	}

	if (cut)
	{
		ListView_SetItemState(m_hListView, item, LVIS_CUT, LVIS_CUT);
	}
	else
	{
		ListView_SetItemState(m_hListView, item, 0, LVIS_CUT);
	}
}

void ShellBrowserImpl::ShowPropertiesForSelectedFiles() const
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

	auto pidlDirectory = GetDirectoryIdl();
	ShowMultipleFileProperties(pidlDirectory.get(), rawPidls, m_hOwner);
}

void ShellBrowserImpl::OnListViewHeaderRightClick(const POINTS &cursorPos)
{
	wil::unique_hmenu headerPopupMenu(
		LoadMenu(m_resourceInstance, MAKEINTRESOURCE(IDR_HEADER_MENU)));
	HMENU headerMenu = GetSubMenu(headerPopupMenu.get(), 0);

	auto commonColumns = GetColumnHeaderMenuList(m_directoryState.directory.c_str());

	std::unordered_map<int, ColumnType> menuItemMappings;
	int totalInserted = 0;
	int commonColumnPosition = 0;

	for (const auto &column : *m_pActiveColumns)
	{
		auto itr = std::find(commonColumns.begin(), commonColumns.end(), column.type);
		bool inCommonColumns = (itr != commonColumns.end());

		if (!column.checked && !inCommonColumns)
		{
			continue;
		}

		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING | MIIM_STATE | MIIM_ID;

		std::wstring columnName = GetColumnName(m_app->GetResourceLoader(), column.type);

		if (column.checked)
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

		mii.dwTypeData = columnName.data();
		mii.wID = id;
		InsertMenuItem(headerMenu, currentPosition, TRUE, &mii);

		menuItemMappings.insert({ id, column.type });

		totalInserted++;
	}

	int cmd =
		TrackPopupMenu(headerMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERTICAL | TPM_RETURNCMD,
			cursorPos.x, cursorPos.y, 0, m_hListView, nullptr);

	if (cmd == 0)
	{
		return;
	}

	OnListViewHeaderMenuItemSelected(cmd, menuItemMappings);
}

std::vector<ColumnType> GetColumnHeaderMenuList(const std::wstring &directory)
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

void ShellBrowserImpl::OnListViewHeaderMenuItemSelected(int menuItemId,
	const std::unordered_map<int, ColumnType> &menuItemMappings)
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

void ShellBrowserImpl::OnShowMoreColumnsSelected()
{
	SelectColumnsDialog selectColumnsDialog(m_app->GetResourceLoader(), m_resourceInstance,
		m_hListView, m_app->GetThemeManager(), this, m_app->GetIconResourceLoader());
	selectColumnsDialog.ShowModalDialog();
}

void ShellBrowserImpl::OnColumnMenuItemSelected(int menuItemId,
	const std::unordered_map<int, ColumnType> &menuItemMappings)
{
	auto currentColumns = GetCurrentColumns();

	ColumnType columnType = menuItemMappings.at(menuItemId);
	auto itr = std::find_if(currentColumns.begin(), currentColumns.end(),
		[columnType](const Column_t &column) { return column.type == columnType; });

	if (itr == currentColumns.end())
	{
		return;
	}

	itr->checked = !itr->checked;

	SetCurrentColumns(currentColumns);

	// If it was the first column that was changed, need to refresh all columns.
	if (menuItemId == 1)
	{
		m_navigationController->Refresh();
	}
}

void ShellBrowserImpl::SetFileAttributesForSelection()
{
	std::list<NSetFileAttributesDialogExternal::SetFileAttributesInfo> sfaiList;
	int index = -1;

	while ((index = ListView_GetNextItem(m_hListView, index, LVNI_SELECTED)) != -1)
	{
		NSetFileAttributesDialogExternal::SetFileAttributesInfo sfai;

		const ItemInfo_t &item = GetItemByIndex(index);
		sfai.wfd = item.wfd;
		StringCchCopy(sfai.szFullFileName, std::size(sfai.szFullFileName),
			item.parsingName.c_str());

		sfaiList.push_back(sfai);
	}

	SetFileAttributesDialog setFileAttributesDialog(m_app->GetResourceLoader(), m_resourceInstance,
		m_hListView, m_app->GetThemeManager(), sfaiList);
	setFileAttributesDialog.ShowModalDialog();
}

bool ShellBrowserImpl::TestListViewItemAttributes(int item, SFGAOF attributes) const
{
	SFGAOF commonAttributes = attributes;
	HRESULT hr = GetListViewItemAttributes(item, &commonAttributes);

	if (SUCCEEDED(hr))
	{
		return (commonAttributes & attributes) == attributes;
	}

	return false;
}

HRESULT ShellBrowserImpl::GetListViewSelectionAttributes(SFGAOF *attributes) const
{
	HRESULT hr = E_FAIL;

	/* TODO: This should probably check all selected files. */
	int selectedItem = ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);

	if (selectedItem != -1)
	{
		hr = GetListViewItemAttributes(selectedItem, attributes);
	}

	return hr;
}

HRESULT ShellBrowserImpl::GetListViewItemAttributes(int item, SFGAOF *attributes) const
{
	const auto &itemInfo = GetItemByIndex(item);
	return GetItemAttributes(itemInfo.pidlComplete.Raw(), attributes);
}

std::vector<PidlAbsolute> ShellBrowserImpl::GetSelectedItemPidls() const
{
	std::vector<PidlAbsolute> selectedItemPidls;
	int index = -1;

	while ((index = ListView_GetNextItem(m_hListView, index, LVNI_SELECTED)) != -1)
	{
		const auto &item = GetItemByIndex(index);
		selectedItemPidls.push_back(item.pidlComplete);
	}

	return selectedItemPidls;
}

void ShellBrowserImpl::OnListViewBeginDrag(const NMLISTVIEW *info)
{
	StartDrag(info->iItem, info->ptAction);
}

void ShellBrowserImpl::OnListViewBeginRightClickDrag(const NMLISTVIEW *info)
{
	StartDrag(info->iItem, info->ptAction);
}

HRESULT ShellBrowserImpl::StartDrag(int draggedItem, const POINT &startPoint)
{
	auto pidls = GetSelectedItemPidls();

	if (pidls.empty())
	{
		return E_UNEXPECTED;
	}

	wil::com_ptr_nothrow<IDataObject> dataObject;
	RETURN_IF_FAILED(CreateDataObjectForShellTransfer(pidls, &dataObject));

	m_performingDrag = true;
	m_draggedDataObject = dataObject.get();
	m_draggedItems = pidls;

	POINT ptItem;
	ListView_GetItemPosition(m_hListView, draggedItem, &ptItem);

	POINT ptOrigin;
	ListView_GetOrigin(m_hListView, &ptOrigin);

	m_ptDraggedOffset.x = ptOrigin.x + startPoint.x - ptItem.x;
	m_ptDraggedOffset.y = ptOrigin.y + startPoint.y - ptItem.y;

	DWORD finalEffect;
	HRESULT hr = SHDoDragDrop(m_hListView, dataObject.get(), nullptr,
		DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK, &finalEffect);

	m_draggedItems.clear();
	m_draggedDataObject = nullptr;
	m_performingDrag = false;

	return hr;
}

void ShellBrowserImpl::AutoSizeColumns()
{
	if (m_folderSettings.viewMode != +ViewMode::Details)
	{
		return;
	}

	HWND header = ListView_GetHeader(m_hListView);
	int numColumns = Header_GetItemCount(header);

	if (numColumns == -1)
	{
		return;
	}

	for (int i = 0; i < numColumns; i++)
	{
		ListView_SetColumnWidth(m_hListView, i, LVSCW_AUTOSIZE);
	}
}

BOOL ShellBrowserImpl::OnListViewBeginLabelEdit(const NMLVDISPINFO *dispInfo)
{
	const auto &item = GetItemByIndex(dispInfo->item.iItem);

	SFGAOF attributes = SFGAO_CANRENAME;
	HRESULT hr = GetItemAttributes(item.pidlComplete.Raw(), &attributes);

	if (FAILED(hr) || WI_IsFlagClear(attributes, SFGAO_CANRENAME))
	{
		return TRUE;
	}

	bool useEditingName = true;

	// The editing name may differ from the display name. For example, the display name of the C:\
	// drive item will be something like "Local Disk (C:)", while its editing name will be "Local
	// Disk". Since the editing name is affected by the file name extensions setting in Explorer, it
	// won't be used if:
	//
	// - Extensions are hidden in Explorer, but shown in Explorer++ (since the editing name would
	//   contain no extension)
	// - Extensions are shown in Explorer, but hidden in Explorer++ (since the editing name would
	//   contain an extension). Note that this case is handled when editing is finished - if
	//   extensions are hidden, the extension will be manually re-added when renaming an item.
	if (!WI_IsFlagSet(item.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
	{
		std::wstring displayName = GetItemDisplayName(dispInfo->item.iItem);

		if (m_config->globalFolderSettings.showExtensions
			|| m_config->globalFolderSettings.hideLinkExtension)
		{
			auto *extension = PathFindExtension(displayName.c_str());

			if (*extension != '\0'
				&& lstrcmp((item.editingName + extension).c_str(), displayName.c_str()) == 0)
			{
				useEditingName = false;
			}
		}
		else
		{
			auto *extension = PathFindExtension(item.editingName.c_str());

			if (*extension != '\0'
				&& lstrcmp((displayName + extension).c_str(), item.editingName.c_str()) == 0)
			{
				useEditingName = false;
			}
		}
	}

	HWND editControl = ListView_GetEditControl(m_hListView);

	if (editControl == nullptr)
	{
		return TRUE;
	}

	// Note that the necessary text is set in the edit control, rather than the listview. This is
	// for the following two reasons:
	//
	// 1. Setting the listview item text after the edit control has already been created won't
	// change the text in the control
	// 2. Even if setting the listview item text did change the edit control text, the text would
	// need to be reverted if the user canceled editing. Setting the edit control text means there's
	// nothing that needs to be changed if editing is canceled.
	if (useEditingName)
	{
		SetWindowText(editControl, item.editingName.c_str());
	}

	ItemNameEditControl::CreateNew(editControl, m_acceleratorManager,
		WI_IsFlagClear(item.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY));

	return FALSE;
}

BOOL ShellBrowserImpl::OnListViewEndLabelEdit(const NMLVDISPINFO *dispInfo)
{
	// Did the user cancel editing?
	if (dispInfo->item.pszText == nullptr)
	{
		return FALSE;
	}

	std::wstring newFilename = dispInfo->item.pszText;

	if (newFilename.empty())
	{
		return FALSE;
	}

	const auto &item = GetItemByIndex(dispInfo->item.iItem);

	if (newFilename == item.editingName)
	{
		return FALSE;
	}

	if (!WI_IsFlagSet(item.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
	{
		auto *extension = PathFindExtension(item.wfd.cFileName);

		bool extensionHidden = !m_config->globalFolderSettings.showExtensions
			|| (m_config->globalFolderSettings.hideLinkExtension
				&& lstrcmpi(extension, _T(".lnk")) == 0);

		// If file extensions are turned off, the new filename will be incorrect (i.e. it will be
		// missing the extension). Therefore, append the extension manually if it is turned off.
		if (extensionHidden && *extension != '\0')
		{
			newFilename += extension;
		}
	}

	wil::com_ptr_nothrow<IShellFolder> parent;
	PCITEMID_CHILD child;
	HRESULT hr = SHBindToParent(item.pidlComplete.Raw(), IID_PPV_ARGS(&parent), &child);

	if (FAILED(hr))
	{
		return FALSE;
	}

	SHGDNF flags = SHGDN_INFOLDER;

	// As with GetDisplayNameOf(), the behavior of SetNameOf() is influenced by whether or not file
	// extensions are displayed in Explorer. If extensions are displayed and the SHGDN_INFOLDER name
	// is set, then the name should contain an extension. On the other hand, if extensions aren't
	// displayed and the SHGDN_INFOLDER name is set, then the name shouldn't contain an extension.
	// Given that extensions can be independently hidden and shown in Explorer++, this behavior is
	// undesirable and incompatible.
	// For example, if extensions are hidden in Explorer, but shown in Explorer++, then it wouldn't
	// be possible to change a file's extension. When setting the SHGDN_INFOLDER name, the original
	// extension would always be re-added by the shell.
	// Therefore, if a file is being edited, the parsing name (which will always contain an
	// extension) will be updated.
	if (!m_directoryState.virtualFolder
		&& !WI_IsFlagSet(item.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
	{
		flags |= SHGDN_FORPARSING;
	}

	unique_pidl_child newChild;
	hr =
		parent->SetNameOf(m_hListView, child, newFilename.c_str(), flags, wil::out_param(newChild));

	// S_FALSE can be returned in certain situations when no rename actually took place. For
	// example, when changing a drive label, elevation will be requested. If the user declines the
	// elevation request, the drive label won't be changed and S_FALSE will be returned. In those
	// situations, there's nothing else that needs to be done.
	// The method can also return S_OK, but not set the new child name. That will happen, for
	// example, if the new name is the same as the old name, in which case, no rename took place.
	if (FAILED(hr) || hr == S_FALSE || !newChild)
	{
		return FALSE;
	}

	// When an item is changed in any way, a notification will be sent. However, that notification
	// isn't going to be received immediately. In the case where the user has renamed an item, that
	// creates a period of time where the updated name is shown, but the item still internally
	// refers to the original name. That then means that attempting to opening the item (or interact
	// with it more generally) will fail, since the item no longer exists with the original name.
	// Performing an immediate update here means that the user can continue to interact with the
	// item, without having to wait for the rename notification to be processed.
	// Note that the name may not actually have changed, even though the SetNameOf() call above
	// succeeded. For example, if a series of '.' characters are added to the end of a file name,
	// they will typically be removed. The rename request will succeed, however, and a rename change
	// notification will be generated. In that sort of situation, this call is superfluous.
	// Attempting to detect whether or not the name actually changed isn't easy, as there is no
	// singular name for shell items. The display name of a shell item (e.g. the drive label) can
	// change, even if the parsing name remains the same. Comparing the parsing names will show that
	// they're equivalent. It's easier just to update the item, regardless.
	unique_pidl_absolute pidlNew(ILCombine(m_directoryState.pidlDirectory.Raw(), newChild.get()));
	UpdateItem(item.pidlComplete.Raw(), pidlNew.get());

	// The text will be set by UpdateItem. It's not safe to return true here, since items can sorted
	// by UpdateItem, which can result in the index of this item being changed.
	return FALSE;
}

LRESULT ShellBrowserImpl::OnListViewCustomDraw(NMLVCUSTOMDRAW *listViewCustomDraw)
{
	switch (listViewCustomDraw->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
	{
		const auto &itemInfo =
			GetItemByIndex(static_cast<int>(listViewCustomDraw->nmcd.dwItemSpec));

		for (const auto &colorRule : m_app->GetColorRuleModel()->GetItems())
		{
			bool matchedFileName = false;
			bool matchedAttributes = false;

			if (!colorRule->GetFilterPattern().empty())
			{
				if (CheckWildcardMatch(colorRule->GetFilterPattern().c_str(),
						itemInfo.displayName.c_str(), !colorRule->GetFilterPatternCaseInsensitive())
					== 1)
				{
					matchedFileName = true;
				}
			}
			else
			{
				matchedFileName = true;
			}

			if (colorRule->GetFilterAttributes() != 0)
			{
				if (itemInfo.isFindDataValid
					&& WI_IsAnyFlagSet(itemInfo.wfd.dwFileAttributes,
						colorRule->GetFilterAttributes()))
				{
					matchedAttributes = true;
				}
			}
			else
			{
				matchedAttributes = true;
			}

			if (matchedFileName && matchedAttributes)
			{
				listViewCustomDraw->clrText = colorRule->GetColor();
				return CDRF_NEWFONT;
			}
		}
	}
	break;
	}

	return CDRF_DODEFAULT;
}

void ShellBrowserImpl::OnColorRulesUpdated()
{
	// Any changes to the color rules will require the listview to be redrawn.
	InvalidateRect(m_hListView, nullptr, false);
}

void ShellBrowserImpl::OnFullRowSelectUpdated(BOOL newValue)
{
	ListViewHelper::AddRemoveExtendedStyles(m_hListView, LVS_EX_FULLROWSELECT, newValue);
}

void ShellBrowserImpl::OnCheckBoxSelectionUpdated(BOOL newValue)
{
	ListViewHelper::AddRemoveExtendedStyles(m_hListView, LVS_EX_CHECKBOXES, newValue);
}

void ShellBrowserImpl::OnShowGridlinesUpdated(BOOL newValue)
{
	ListViewHelper::AddRemoveExtendedStyles(m_hListView, LVS_EX_GRIDLINES, newValue);
}

void ShellBrowserImpl::OnOneClickActivateUpdated(BOOL newValue)
{
	ListViewHelper::ActivateOneClickSelect(m_hListView, newValue,
		m_config->globalFolderSettings.oneClickActivateHoverTime.get());
}

void ShellBrowserImpl::OnOneClickActivateHoverTimeUpdated(UINT newValue)
{
	ListViewHelper::ActivateOneClickSelect(m_hListView,
		m_config->globalFolderSettings.oneClickActivate.get(), newValue);
}
