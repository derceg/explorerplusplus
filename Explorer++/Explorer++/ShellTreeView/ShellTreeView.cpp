// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Wraps a treeview control. Specifically handles
 * adding directories to it and selecting directories.
 * Each non-network drive in the system is also
 * monitored for changes.
 *
 * Notes:
 *  - All items are sorted alphabetically, except for:
 *     - Items on the desktop
 *     - Items in My Computer
 */

#include "stdafx.h"
#include "ShellTreeView.h"
#include "Config.h"
#include "CoreInterface.h"
#include "DarkModeHelper.h"
#include "TabContainer.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/Controls.h"
#include "../Helper/DriveInfo.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <wil/common.h>
#include <propkey.h>

int CALLBACK CompareItemsStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
DWORD WINAPI Thread_MonitorAllDrives(LPVOID pParam);

ShellTreeView::ShellTreeView(HWND hParent, IExplorerplusplus *coreInterface,
	IDirectoryMonitor *pDirMon, TabContainer *tabContainer, FileActionHandler *fileActionHandler,
	CachedIcons *cachedIcons) :
	m_hTreeView(CreateTreeView(hParent)),
	m_config(coreInterface->GetConfig()),
	m_pDirMon(pDirMon),
	m_tabContainer(tabContainer),
	m_fileActionHandler(fileActionHandler),
	m_cachedIcons(cachedIcons),
	m_iRefCount(1),
	m_itemIDCounter(0),
	m_bDragDropRegistered(FALSE),
	m_iconThreadPool(
		1, std::bind(CoInitializeEx, nullptr, COINIT_APARTMENTTHREADED), CoUninitialize),
	m_iconResultIDCounter(0),
	m_subfoldersThreadPool(
		1, std::bind(CoInitializeEx, nullptr, COINIT_APARTMENTTHREADED), CoUninitialize),
	m_subfoldersResultIDCounter(0),
	m_cutItem(nullptr)
{
	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (DarkModeHelper::GetInstance().IsDarkModeEnabled())
	{
		darkModeHelper.AllowDarkModeForWindow(m_hTreeView, true);

		TreeView_SetBkColor(m_hTreeView, TREE_VIEW_DARK_MODE_BACKGROUND_COLOR);
		TreeView_SetTextColor(m_hTreeView, DarkModeHelper::TEXT_COLOR);

		InvalidateRect(m_hTreeView, nullptr, TRUE);

		HWND tooltips = TreeView_GetToolTips(m_hTreeView);
		darkModeHelper.AllowDarkModeForWindow(tooltips, true);
		SetWindowTheme(tooltips, L"Explorer", nullptr);
	}

	SetWindowTheme(m_hTreeView, L"Explorer", nullptr);

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(
		m_hTreeView, TreeViewProcStub, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));
	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(
		hParent, ParentWndProcStub, PARENT_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	InitializeCriticalSection(&m_cs);

	m_iFolderIcon = GetDefaultFolderIconIndex();

	m_bDragging = FALSE;
	m_bDragCancelled = FALSE;
	m_bDragAllowed = FALSE;
	m_bShowHidden = TRUE;

	AddRoot();

	InitializeDragDropHelpers();

	m_bQueryRemoveCompleted = FALSE;
	HANDLE hThread = CreateThread(nullptr, 0, Thread_MonitorAllDrives, this, 0, nullptr);
	CloseHandle(hThread);

	AddClipboardFormatListener(m_hTreeView);

	m_connections.push_back(coreInterface->AddApplicationShuttingDownObserver(
		std::bind(&ShellTreeView::OnApplicationShuttingDown, this)));
}

HWND ShellTreeView::CreateTreeView(HWND parent)
{
	return ::CreateTreeView(parent,
		WS_CHILD | WS_VISIBLE | TVS_SHOWSELALWAYS | TVS_HASBUTTONS | TVS_EDITLABELS | TVS_HASLINES
			| TVS_TRACKSELECT);
}

HWND ShellTreeView::GetHWND() const
{
	return m_hTreeView;
}

ShellTreeView::~ShellTreeView()
{
	DeleteCriticalSection(&m_cs);

	m_iconThreadPool.clear_queue();
}

void ShellTreeView::OnApplicationShuttingDown()
{
	if (m_clipboardDataObject && OleIsCurrentClipboard(m_clipboardDataObject.get()) == S_OK)
	{
		OleFlushClipboard();
	}
}

LRESULT CALLBACK ShellTreeView::TreeViewProcStub(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *shellTreeView = reinterpret_cast<ShellTreeView *>(dwRefData);

	return shellTreeView->TreeViewProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK ShellTreeView::TreeViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_TIMER:
		DirectoryAltered();
		break;

	case WM_DEVICECHANGE:
		return OnDeviceChange(wParam, lParam);

	case WM_RBUTTONDOWN:
		if ((wParam & MK_RBUTTON) && !(wParam & MK_LBUTTON) && !(wParam & MK_MBUTTON))
		{
			TVHITTESTINFO tvhti;

			tvhti.pt.x = LOWORD(lParam);
			tvhti.pt.y = HIWORD(lParam);

			/* Test to see if the mouse click was
			on an item or not. */
			TreeView_HitTest(m_hTreeView, &tvhti);

			if (!(tvhti.flags & LVHT_NOWHERE))
			{
				m_bDragAllowed = TRUE;
			}
		}
		break;

	case WM_RBUTTONUP:
		m_bDragCancelled = FALSE;
		m_bDragAllowed = FALSE;
		break;

	case WM_MBUTTONDOWN:
	{
		POINT pt;
		POINTSTOPOINT(pt, MAKEPOINTS(lParam));
		OnMiddleButtonDown(&pt);
	}
	break;

	case WM_MBUTTONUP:
	{
		POINT pt;
		POINTSTOPOINT(pt, MAKEPOINTS(lParam));
		OnMiddleButtonUp(&pt, static_cast<UINT>(wParam));
	}
	break;

	case WM_MOUSEMOVE:
	{
		if (!m_bDragging && !m_bDragCancelled && m_bDragAllowed)
		{
			if ((wParam & MK_RBUTTON) && !(wParam & MK_LBUTTON) && !(wParam & MK_MBUTTON))
			{
				TVHITTESTINFO tvhti;
				TVITEM tvItem;
				POINT pt;
				DWORD dwPos;
				HRESULT hr;
				BOOL bRet;

				dwPos = GetMessagePos();
				pt.x = GET_X_LPARAM(dwPos);
				pt.y = GET_Y_LPARAM(dwPos);
				MapWindowPoints(HWND_DESKTOP, m_hTreeView, &pt, 1);

				tvhti.pt = pt;

				/* Test to see if the mouse click was
				on an item or not. */
				TreeView_HitTest(m_hTreeView, &tvhti);

				if (!(tvhti.flags & LVHT_NOWHERE))
				{
					tvItem.mask = TVIF_PARAM | TVIF_HANDLE;
					tvItem.hItem = tvhti.hItem;
					bRet = TreeView_GetItem(m_hTreeView, &tvItem);

					if (bRet)
					{
						hr = OnBeginDrag((int) tvItem.lParam, DragType::RightClick);

						if (hr == DRAGDROP_S_CANCEL)
						{
							m_bDragCancelled = TRUE;
						}
					}
				}
			}
		}
	}
	break;

	case WM_CLIPBOARDUPDATE:
		OnClipboardUpdate();
		return 0;

	case WM_APP_ICON_RESULT_READY:
		ProcessIconResult(static_cast<int>(wParam));
		break;

	case WM_APP_SUBFOLDERS_RESULT_READY:
		ProcessSubfoldersResult(static_cast<int>(wParam));
		break;

	case WM_DESTROY:
		if (m_bDragDropRegistered)
		{
			RevokeDragDrop(m_hTreeView);
			m_bDragDropRegistered = FALSE;
		}

		RemoveClipboardFormatListener(m_hTreeView);
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK ShellTreeView::ParentWndProcStub(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *treeView = reinterpret_cast<ShellTreeView *>(dwRefData);
	return treeView->ParentWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK ShellTreeView::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hTreeView)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case TVN_BEGINDRAG:
			{
				auto *pnmTreeView = reinterpret_cast<NMTREEVIEW *>(lParam);
				OnBeginDrag(static_cast<int>(pnmTreeView->itemNew.lParam), DragType::LeftClick);
			}
			break;

			case TVN_GETDISPINFO:
				OnGetDisplayInfo(reinterpret_cast<NMTVDISPINFO *>(lParam));
				break;

			case TVN_ITEMEXPANDING:
				OnItemExpanding(reinterpret_cast<NMTREEVIEW *>(lParam));
				break;

			case TVN_KEYDOWN:
				return OnKeyDown(reinterpret_cast<NMTVKEYDOWN *>(lParam));

			case TVN_ENDLABELEDIT:
				/* TODO: Should return the value from this function. Can't do it
				at the moment, since the treeview looks items up by their label
				when a directory modification event is received (meaning that if
				the label changes, the lookup for the old file name will fail). */
				OnEndLabelEdit(reinterpret_cast<NMTVDISPINFO *>(lParam));
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

HTREEITEM ShellTreeView::AddRoot()
{
	TreeView_DeleteAllItems(m_hTreeView);

	unique_pidl_absolute pidl;
	HRESULT hr = SHGetFolderLocation(nullptr, CSIDL_DESKTOP, nullptr, 0, wil::out_param(pidl));

	if (FAILED(hr))
	{
		return nullptr;
	}

	std::wstring desktopDisplayName;
	GetDisplayName(pidl.get(), SHGDN_INFOLDER, desktopDisplayName);

	int itemId = GenerateUniqueItemId();
	m_itemInfoMap[itemId].pidl.reset(ILCloneFull(pidl.get()));

	TVITEMEX tvItem;
	tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN;
	tvItem.pszText = desktopDisplayName.data();
	tvItem.iImage = I_IMAGECALLBACK;
	tvItem.iSelectedImage = I_IMAGECALLBACK;
	tvItem.cChildren = 1;
	tvItem.lParam = itemId;

	TVINSERTSTRUCT tvis;
	tvis.hParent = nullptr;
	tvis.hInsertAfter = TVI_LAST;
	tvis.itemex = tvItem;

	auto hDesktop = TreeView_InsertItem(m_hTreeView, &tvis);

	if (hDesktop != nullptr)
	{
		SendMessage(m_hTreeView, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(hDesktop));
	}

	return hDesktop;
}

void ShellTreeView::OnGetDisplayInfo(NMTVDISPINFO *pnmtvdi)
{
	TVITEM *ptvItem = &pnmtvdi->item;

	if (WI_IsFlagSet(ptvItem->mask, TVIF_IMAGE))
	{
		const ItemInfo_t &itemInfo = m_itemInfoMap.at(static_cast<int>(ptvItem->lParam));
		auto cachedIconIndex = GetCachedIconIndex(itemInfo);

		if (cachedIconIndex)
		{
			ptvItem->iImage = (*cachedIconIndex & 0x0FFF);
			ptvItem->iSelectedImage = (*cachedIconIndex & 0x0FFF);
		}
		else
		{
			ptvItem->iImage = m_iFolderIcon;
			ptvItem->iSelectedImage = m_iFolderIcon;
		}

		QueueIconTask(ptvItem->hItem, static_cast<int>(ptvItem->lParam));
	}

	if (WI_IsFlagSet(ptvItem->mask, TVIF_CHILDREN))
	{
		ptvItem->cChildren = 1;

		QueueSubfoldersTask(ptvItem->hItem);
	}

	ptvItem->mask |= TVIF_DI_SETITEM;
}

std::optional<int> ShellTreeView::GetCachedIconIndex(const ItemInfo_t &itemInfo)
{
	std::wstring filePath;
	HRESULT hr = GetDisplayName(itemInfo.pidl.get(), SHGDN_FORPARSING, filePath);

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

void ShellTreeView::QueueIconTask(HTREEITEM item, int internalIndex)
{
	const ItemInfo_t &itemInfo = m_itemInfoMap.at(internalIndex);

	BasicItemInfo basicItemInfo;
	basicItemInfo.pidl.reset(ILCloneFull(itemInfo.pidl.get()));

	int iconResultID = m_iconResultIDCounter++;

	auto result =
		m_iconThreadPool.push([this, iconResultID, item, internalIndex, basicItemInfo](int id) {
			UNREFERENCED_PARAMETER(id);

			return FindIconAsync(
				m_hTreeView, iconResultID, item, internalIndex, basicItemInfo.pidl.get());
		});

	m_iconResults.insert({ iconResultID, std::move(result) });
}

std::optional<ShellTreeView::IconResult> ShellTreeView::FindIconAsync(
	HWND treeView, int iconResultId, HTREEITEM item, int internalIndex, PCIDLIST_ABSOLUTE pidl)
{
	SHFILEINFO shfi;
	DWORD_PTR res = SHGetFileInfo(reinterpret_cast<LPCTSTR>(pidl), 0, &shfi, sizeof(SHFILEINFO),
		SHGFI_PIDL | SHGFI_ICON | SHGFI_OVERLAYINDEX);

	if (res == 0)
	{
		return std::nullopt;
	}

	DestroyIcon(shfi.hIcon);

	PostMessage(treeView, WM_APP_ICON_RESULT_READY, iconResultId, 0);

	IconResult result;
	result.item = item;
	result.internalIndex = internalIndex;
	result.iconIndex = shfi.iIcon;

	return result;
}

void ShellTreeView::ProcessIconResult(int iconResultId)
{
	auto itr = m_iconResults.find(iconResultId);

	if (itr == m_iconResults.end())
	{
		return;
	}

	auto cleanup = wil::scope_exit([this, itr]() {
		m_iconResults.erase(itr);
	});

	auto result = itr->second.get();

	if (!result)
	{
		return;
	}

	const ItemInfo_t &itemInfo = m_itemInfoMap.at(result->internalIndex);

	std::wstring filePath;
	HRESULT hr = GetDisplayName(itemInfo.pidl.get(), SHGDN_FORPARSING, filePath);

	if (SUCCEEDED(hr))
	{
		m_cachedIcons->addOrUpdateFileIcon(filePath, result->iconIndex);
	}

	TVITEM tvItem;
	tvItem.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
	tvItem.hItem = result->item;
	tvItem.iImage = result->iconIndex;
	tvItem.iSelectedImage = result->iconIndex;
	tvItem.stateMask = TVIS_OVERLAYMASK;
	tvItem.state = INDEXTOOVERLAYMASK(result->iconIndex >> 24);
	TreeView_SetItem(m_hTreeView, &tvItem);
}

void ShellTreeView::QueueSubfoldersTask(HTREEITEM item)
{
	BasicItemInfo basicItemInfo;
	basicItemInfo.pidl = GetItemPidl(item);

	int subfoldersResultID = m_subfoldersResultIDCounter++;

	auto result =
		m_subfoldersThreadPool.push([this, subfoldersResultID, item, basicItemInfo](int id) {
			UNREFERENCED_PARAMETER(id);

			return CheckSubfoldersAsync(
				m_hTreeView, subfoldersResultID, item, basicItemInfo.pidl.get());
		});

	m_subfoldersResults.insert({ subfoldersResultID, std::move(result) });
}

std::optional<ShellTreeView::SubfoldersResult> ShellTreeView::CheckSubfoldersAsync(
	HWND treeView, int subfoldersResultId, HTREEITEM item, PCIDLIST_ABSOLUTE pidl)
{
	wil::com_ptr_nothrow<IShellFolder> pShellFolder;
	PCITEMID_CHILD pidlRelative;
	HRESULT hr = SHBindToParent(pidl, IID_PPV_ARGS(&pShellFolder), &pidlRelative);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	ULONG attributes = SFGAO_HASSUBFOLDER;
	hr = pShellFolder->GetAttributesOf(1, &pidlRelative, &attributes);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	PostMessage(treeView, WM_APP_SUBFOLDERS_RESULT_READY, subfoldersResultId, 0);

	SubfoldersResult result;
	result.item = item;
	result.hasSubfolder = WI_IsFlagSet(attributes, SFGAO_HASSUBFOLDER);

	return result;
}

void ShellTreeView::ProcessSubfoldersResult(int subfoldersResultId)
{
	auto itr = m_subfoldersResults.find(subfoldersResultId);

	if (itr == m_subfoldersResults.end())
	{
		return;
	}

	auto cleanup = wil::scope_exit([this, itr]() {
		m_subfoldersResults.erase(itr);
	});

	auto result = itr->second.get();

	if (!result)
	{
		return;
	}

	if (result->hasSubfolder)
	{
		// By default it's assumed that an item has subfolders, so if it does
		// actually have subfolders, there's nothing else that needs to be done.
		return;
	}

	TVITEM tvItem;
	tvItem.mask = TVIF_HANDLE | TVIF_CHILDREN;
	tvItem.hItem = result->item;
	tvItem.cChildren = 0;
	TreeView_SetItem(m_hTreeView, &tvItem);
}

void ShellTreeView::OnItemExpanding(const NMTREEVIEW *nmtv)
{
	HTREEITEM parentItem = nmtv->itemNew.hItem;

	if (nmtv->action == TVE_EXPAND)
	{
		ExpandDirectory(parentItem);
	}
	else
	{
		auto hSelection = TreeView_GetSelection(m_hTreeView);

		if (hSelection != nullptr)
		{
			HTREEITEM hItem = hSelection;

			do
			{
				hItem = TreeView_GetParent(m_hTreeView, hItem);
			} while (hItem != parentItem && hItem != nullptr);

			// If the currently selected item is below the item being
			// collapsed, the selection should be adjusted to the parent item.
			if (hItem == parentItem)
			{
				TreeView_SelectItem(m_hTreeView, parentItem);
			}
		}

		EraseItems(parentItem);

		SendMessage(m_hTreeView, TVM_EXPAND, TVE_COLLAPSE | TVE_COLLAPSERESET,
			reinterpret_cast<LPARAM>(parentItem));
	}
}

LRESULT ShellTreeView::OnKeyDown(const NMTVKEYDOWN *keyDown)
{
	switch (keyDown->wVKey)
	{
	case 'C':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			CopySelectedItemToClipboard(true);
		}
		break;

	case 'X':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			CopySelectedItemToClipboard(false);
		}
		break;

	case 'V':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			PasteClipboardData();
		}
		break;

	case VK_DELETE:
		if (IsKeyDown(VK_SHIFT))
		{
			DeleteSelectedItem(true);
		}
		else
		{
			DeleteSelectedItem(false);
		}
		break;
	}

	// If the ctrl key is down, this key sequence is likely a modifier. Stop any other pressed key
	// from been used in an incremental search.
	if (IsKeyDown(VK_CONTROL))
	{
		return 1;
	}

	return 0;
}

/* Sorts items in the following order:
 - Drives
 - Virtual Items
 - Real Items

Each set is ordered alphabetically. */
int CALLBACK CompareItemsStub(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	ShellTreeView *shellTreeView = nullptr;

	shellTreeView = (ShellTreeView *) lParamSort;

	return shellTreeView->CompareItems(lParam1, lParam2);
}

int CALLBACK ShellTreeView::CompareItems(LPARAM lParam1, LPARAM lParam2)
{
	TCHAR szTemp[MAX_PATH];
	int iItemId1 = (int) lParam1;
	int iItemId2 = (int) lParam2;

	const ItemInfo_t &itemInfo1 = m_itemInfoMap.at(iItemId1);
	const ItemInfo_t &itemInfo2 = m_itemInfoMap.at(iItemId2);

	std::wstring displayName1;
	GetDisplayName(itemInfo1.pidl.get(), SHGDN_FORPARSING, displayName1);

	std::wstring displayName2;
	GetDisplayName(itemInfo2.pidl.get(), SHGDN_FORPARSING, displayName2);

	if (PathIsRoot(displayName1.c_str()) && !PathIsRoot(displayName2.c_str()))
	{
		return -1;
	}
	else if (!PathIsRoot(displayName1.c_str()) && PathIsRoot(displayName2.c_str()))
	{
		return 1;
	}
	else if (PathIsRoot(displayName1.c_str()) && PathIsRoot(displayName2.c_str()))
	{
		return lstrcmpi(displayName1.c_str(), displayName2.c_str());
	}
	else
	{
		if (!SHGetPathFromIDList(itemInfo1.pidl.get(), szTemp)
			&& SHGetPathFromIDList(itemInfo2.pidl.get(), szTemp))
		{
			return -1;
		}
		else if (SHGetPathFromIDList(itemInfo1.pidl.get(), szTemp)
			&& !SHGetPathFromIDList(itemInfo2.pidl.get(), szTemp))
		{
			return 1;
		}
		else
		{
			GetDisplayName(itemInfo1.pidl.get(), SHGDN_INFOLDER, displayName1);
			GetDisplayName(itemInfo2.pidl.get(), SHGDN_INFOLDER, displayName2);

			if (m_config->globalFolderSettings.useNaturalSortOrder)
			{
				return StrCmpLogicalW(displayName1.c_str(), displayName2.c_str());
			}
			else
			{
				return StrCmpIW(displayName1.c_str(), displayName2.c_str());
			}
		}
	}
}

HRESULT ShellTreeView::ExpandDirectory(HTREEITEM hParent)
{
	auto pidlDirectory = GetItemPidl(hParent);

	wil::com_ptr_nothrow<IShellFolder2> shellFolder2;
	HRESULT hr = BindToIdl(pidlDirectory.get(), IID_PPV_ARGS(&shellFolder2));

	if (FAILED(hr))
	{
		return hr;
	}

	SHCONTF enumFlags = SHCONTF_FOLDERS;

	if (m_bShowHidden)
	{
		enumFlags |= SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN;
	}

	wil::com_ptr_nothrow<IEnumIDList> pEnumIDList;
	hr = shellFolder2->EnumObjects(nullptr, enumFlags, &pEnumIDList);

	if (FAILED(hr) || !pEnumIDList)
	{
		return hr;
	}

	SendMessage(m_hTreeView, WM_SETREDRAW, FALSE, 0);

	std::vector<EnumeratedItem> items;

	unique_pidl_child pidlItem;
	ULONG uFetched = 1;

	while (pEnumIDList->Next(1, wil::out_param(pidlItem), &uFetched) == S_OK && (uFetched == 1))
	{
		if (m_config->checkPinnedToNamespaceTreeProperty)
		{
			BOOL showItem = GetBooleanVariant(
				shellFolder2.get(), pidlItem.get(), &PKEY_IsPinnedToNameSpaceTree, TRUE);

			if (!showItem)
			{
				continue;
			}
		}

		STRRET str;
		hr = shellFolder2->GetDisplayNameOf(pidlItem.get(), SHGDN_NORMAL, &str);

		if (SUCCEEDED(hr))
		{
			TCHAR itemName[MAX_PATH];
			hr = StrRetToBuf(&str, pidlItem.get(), itemName, SIZEOF_ARRAY(itemName));

			if (SUCCEEDED(hr))
			{
				int itemId = GenerateUniqueItemId();
				m_itemInfoMap[itemId].pidl.reset(ILCombine(pidlDirectory.get(), pidlItem.get()));
				m_itemInfoMap[itemId].pridl.reset(ILCloneChild(pidlItem.get()));

				EnumeratedItem item;
				item.internalIndex = itemId;
				item.name = itemName;
				items.push_back(item);
			}
		}
	}

	for (auto &item : items)
	{
		TVITEMEX tvItem;
		tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN;
		tvItem.pszText = item.name.data();
		tvItem.iImage = I_IMAGECALLBACK;
		tvItem.iSelectedImage = I_IMAGECALLBACK;
		tvItem.lParam = item.internalIndex;
		tvItem.cChildren = I_CHILDRENCALLBACK;

		TVINSERTSTRUCT tvis;
		tvis.hInsertAfter = TVI_LAST;
		tvis.hParent = hParent;
		tvis.itemex = tvItem;

		TreeView_InsertItem(m_hTreeView, &tvis);
	}

	TVSORTCB tvscb;
	tvscb.hParent = hParent;
	tvscb.lpfnCompare = CompareItemsStub;
	tvscb.lParam = reinterpret_cast<LPARAM>(this);
	TreeView_SortChildrenCB(m_hTreeView, &tvscb, 0);

	SendMessage(m_hTreeView, WM_SETREDRAW, TRUE, 0);

	return hr;
}

int ShellTreeView::GenerateUniqueItemId()
{
	return m_itemIDCounter++;
}

HTREEITEM ShellTreeView::DetermineItemSortedPosition(HTREEITEM hParent, const TCHAR *szItem)
{
	HTREEITEM htInsertAfter = nullptr;

	if (PathIsRoot(szItem))
	{
		return DetermineDriveSortedPosition(hParent, szItem);
	}
	else
	{
		HTREEITEM htItem;
		HTREEITEM hPreviousItem;
		TVITEMEX item;
		SFGAOF attributes;

		/* Insert the item in its sorted position, skipping
		past any drives or any non-filesystem items (i.e.
		'My Computer', 'Recycle Bin', etc). */
		htItem = TreeView_GetChild(m_hTreeView, hParent);

		/* If the parent has no children, this item will
		be the first that appears. */
		if (htItem == nullptr)
			return TVI_FIRST;

		hPreviousItem = TVI_FIRST;

		while (htInsertAfter == nullptr)
		{
			item.mask = TVIF_PARAM | TVIF_HANDLE;
			item.hItem = htItem;
			TreeView_GetItem(m_hTreeView, &item);

			std::wstring fullItemPath;
			GetDisplayName(m_itemInfoMap.at(static_cast<int>(item.lParam)).pidl.get(),
				SHGDN_FORPARSING, fullItemPath);

			attributes = GetFileAttributes(fullItemPath.c_str());

			/* Only perform the comparison if the current item is a real
			file or folder. */
			if (!PathIsRoot(fullItemPath.c_str())
				&& ((attributes & SFGAO_FILESYSTEM) != SFGAO_FILESYSTEM))
			{
				if (lstrcmpi(szItem, fullItemPath.c_str()) < 0)
				{
					htInsertAfter = hPreviousItem;
				}
			}

			hPreviousItem = htItem;
			htItem = TreeView_GetNextSibling(m_hTreeView, htItem);

			if ((htItem == nullptr) && !htInsertAfter)
			{
				htInsertAfter = TVI_LAST;
			}
		}
	}

	return htInsertAfter;
}

HTREEITEM ShellTreeView::DetermineDriveSortedPosition(HTREEITEM hParent, const TCHAR *szItemName)
{
	HTREEITEM htItem;
	HTREEITEM hPreviousItem;
	TVITEMEX item;

	/* Drives will always be the first children of the specified
	item (usually 'My Computer'). Therefore, keep looping while
	there are more child items and the current item comes
	afterwards, or if there are no child items, place the item
	as the first child. */
	htItem = TreeView_GetChild(m_hTreeView, hParent);

	if (htItem == nullptr)
		return TVI_FIRST;

	hPreviousItem = TVI_FIRST;

	while (htItem != nullptr)
	{
		item.mask = TVIF_PARAM | TVIF_HANDLE;
		item.hItem = htItem;
		TreeView_GetItem(m_hTreeView, &item);

		std::wstring fullItemPath;
		GetDisplayName(m_itemInfoMap.at(static_cast<int>(item.lParam)).pidl.get(), SHGDN_FORPARSING,
			fullItemPath);

		if (PathIsRoot(fullItemPath.c_str()))
		{
			if (lstrcmp(szItemName, fullItemPath.c_str()) < 0)
				return hPreviousItem;
		}
		else
		{
			return hPreviousItem;
		}

		hPreviousItem = htItem;
		htItem = TreeView_GetNextSibling(m_hTreeView, htItem);
	}

	return htItem;
}

unique_pidl_absolute ShellTreeView::GetSelectedItemPidl() const
{
	auto selectedItem = TreeView_GetSelection(m_hTreeView);
	return GetItemPidl(selectedItem);
}

unique_pidl_absolute ShellTreeView::GetItemPidl(HTREEITEM hTreeItem) const
{
	const ItemInfo_t &itemInfo = GetItemByHandle(hTreeItem);
	unique_pidl_absolute pidl(ILCloneFull(itemInfo.pidl.get()));
	return pidl;
}

const ShellTreeView::ItemInfo_t &ShellTreeView::GetItemByHandle(HTREEITEM item) const
{
	int internalIndex = GetItemInternalIndex(item);
	return m_itemInfoMap.at(internalIndex);
}

ShellTreeView::ItemInfo_t &ShellTreeView::GetItemByHandle(HTREEITEM item)
{
	int internalIndex = GetItemInternalIndex(item);
	return m_itemInfoMap.at(internalIndex);
}

int ShellTreeView::GetItemInternalIndex(HTREEITEM item) const
{
	TVITEMEX tvItemEx;
	tvItemEx.mask = TVIF_HANDLE | TVIF_PARAM;
	tvItemEx.hItem = item;
	[[maybe_unused]] bool res = TreeView_GetItem(m_hTreeView, &tvItemEx);
	assert(res);

	return static_cast<int>(tvItemEx.lParam);
}

HTREEITEM ShellTreeView::LocateItem(PCIDLIST_ABSOLUTE pidlDirectory)
{
	return LocateItemInternal(pidlDirectory, FALSE);
}

/* Finds items that have been deleted or renamed
(meaning that their pidl's are no longer valid).

Use two basic strategies to find the item:
1. Find the parent item through its pidl, the child
through it's name.
2. Find the item simply by its name.
The first method would not normally be needed, but
real folders can have a display name that differs
from their parsing name.
For example, the C:\Users\Username\Documents
folder in Windows 7 has a display name of
"My Documents". This is an issue, since a
direct path lookup will fail. */
/* TODO: Store parsing name with each item, and
match against that. */
HTREEITEM ShellTreeView::LocateDeletedItem(const TCHAR *szFullFileName)
{
	HTREEITEM hItem = nullptr;
	PIDLIST_ABSOLUTE pidl = nullptr;
	TCHAR szParent[MAX_PATH];
	BOOL bFound = FALSE;
	HRESULT hr;

	StringCchCopy(szParent, SIZEOF_ARRAY(szParent), szFullFileName);
	PathRemoveFileSpec(szParent);
	hr = SHParseDisplayName(szParent, nullptr, &pidl, 0, nullptr);

	if (SUCCEEDED(hr))
	{
		hItem = LocateExistingItem(pidl);

		if (hItem != nullptr)
		{
			HTREEITEM hChild;
			TVITEM tvItem;
			TCHAR szFileName[MAX_PATH];

			hChild = TreeView_GetChild(m_hTreeView, hItem);

			StringCchCopy(szFileName, SIZEOF_ARRAY(szFileName), szFullFileName);
			PathStripPath(szFileName);

			/* Now try to find the child folder. */
			while (hChild != nullptr)
			{
				TCHAR szItem[MAX_PATH];

				tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
				tvItem.hItem = hChild;
				tvItem.pszText = szItem;
				tvItem.cchTextMax = SIZEOF_ARRAY(szItem);
				TreeView_GetItem(m_hTreeView, &tvItem);

				if (lstrcmp(szFileName, szItem) == 0)
				{
					hItem = hChild;
					bFound = TRUE;
					break;
				}

				hChild = TreeView_GetNextSibling(m_hTreeView, hChild);
			}
		}
	}

	if (!bFound)
	{
		hItem = LocateItemByPath(szFullFileName, FALSE);
	}

	return hItem;
}

HTREEITEM ShellTreeView::LocateExistingItem(const TCHAR *szParsingPath)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(szParsingPath, nullptr, wil::out_param(pidl), 0, nullptr);

	if (SUCCEEDED(hr))
	{
		return LocateExistingItem(pidl.get());
	}

	return nullptr;
}

HTREEITEM ShellTreeView::LocateExistingItem(PCIDLIST_ABSOLUTE pidlDirectory)
{
	return LocateItemInternal(pidlDirectory, TRUE);
}

HTREEITEM ShellTreeView::LocateItemInternal(
	PCIDLIST_ABSOLUTE pidlDirectory, BOOL bOnlyLocateExistingItem)
{
	HTREEITEM hRoot;
	HTREEITEM hItem;
	TVITEMEX item;
	BOOL bFound = FALSE;

	/* Get the root of the tree (root of namespace). */
	hRoot = TreeView_GetRoot(m_hTreeView);
	hItem = hRoot;

	item.mask = TVIF_PARAM | TVIF_HANDLE;
	item.hItem = hItem;
	TreeView_GetItem(m_hTreeView, &item);

	/* Keep searching until the specified item
	is found or it is found the item does not
	exist in the treeview.
	Look through each item, once an ancestor is
	found, look through it's children, expanding
	the parent node if necessary. */
	while (!bFound && hItem != nullptr)
	{
		if (ArePidlsEquivalent(
				m_itemInfoMap.at(static_cast<int>(item.lParam)).pidl.get(), pidlDirectory))
		{
			bFound = TRUE;

			break;
		}

		if (ILIsParent(
				m_itemInfoMap.at(static_cast<int>(item.lParam)).pidl.get(), pidlDirectory, FALSE))
		{
			if ((TreeView_GetChild(m_hTreeView, hItem)) == nullptr)
			{
				if (bOnlyLocateExistingItem)
				{
					return nullptr;
				}
				else
				{
					SendMessage(m_hTreeView, TVM_EXPAND, TVE_EXPAND, (LPARAM) hItem);
				}
			}

			hItem = TreeView_GetChild(m_hTreeView, hItem);
		}
		else
		{
			hItem = TreeView_GetNextSibling(m_hTreeView, hItem);
		}

		item.mask = TVIF_PARAM | TVIF_HANDLE;
		item.hItem = hItem;
		TreeView_GetItem(m_hTreeView, &item);
	}

	return hItem;
}

HTREEITEM ShellTreeView::LocateItemByPath(const TCHAR *szItemPath, BOOL bExpand)
{
	HTREEITEM hMyComputer;
	HTREEITEM hItem;
	HTREEITEM hNextItem;
	TVITEMEX item;
	TCHAR *ptr = nullptr;
	TCHAR itemText[MAX_PATH];
	TCHAR fullItemPathCopy[MAX_PATH];
	TCHAR *nextToken = nullptr;

	StringCchCopy(fullItemPathCopy, SIZEOF_ARRAY(fullItemPathCopy), szItemPath);

	PathRemoveBackslash(fullItemPathCopy);

	unique_pidl_absolute pidlMyComputer;
	SHGetFolderLocation(nullptr, CSIDL_DRIVES, nullptr, 0, wil::out_param(pidlMyComputer));

	hMyComputer = LocateItem(pidlMyComputer.get());

	/* First of drives in system. */
	hItem = TreeView_GetChild(m_hTreeView, hMyComputer);

	/* My Computer node may not be expanded. */
	if (hItem == nullptr)
		return nullptr;

	ptr = wcstok_s(fullItemPathCopy, _T("\\"), &nextToken);

	StringCchCopy(itemText, SIZEOF_ARRAY(itemText), ptr);
	StringCchCat(itemText, SIZEOF_ARRAY(itemText), _T("\\"));
	ptr = itemText;

	item.mask = TVIF_HANDLE | TVIF_PARAM;
	item.hItem = hItem;
	TreeView_GetItem(m_hTreeView, &item);

	std::wstring itemName;
	GetDisplayName(
		m_itemInfoMap.at(static_cast<int>(item.lParam)).pidl.get(), SHGDN_FORPARSING, itemName);

	while (StrCmpI(ptr, itemName.c_str()) != 0)
	{
		hItem = TreeView_GetNextSibling(m_hTreeView, hItem);

		if (hItem == nullptr)
			return nullptr;

		item.mask = TVIF_PARAM;
		item.hItem = hItem;
		TreeView_GetItem(m_hTreeView, &item);

		GetDisplayName(
			m_itemInfoMap.at(static_cast<int>(item.lParam)).pidl.get(), SHGDN_FORPARSING, itemName);
	}

	item.mask = TVIF_TEXT;

	while ((ptr = wcstok_s(nullptr, _T("\\"), &nextToken)) != nullptr)
	{
		if (TreeView_GetChild(m_hTreeView, hItem) == nullptr)
		{
			if (bExpand)
				SendMessage(m_hTreeView, TVM_EXPAND, TVE_EXPAND, (LPARAM) hItem);
			else
				return nullptr;
		}

		hNextItem = TreeView_GetChild(m_hTreeView, hItem);
		hItem = hNextItem;

		item.pszText = itemText;
		item.cchTextMax = SIZEOF_ARRAY(itemText);
		item.hItem = hItem;
		TreeView_GetItem(m_hTreeView, &item);

		while (StrCmpI(ptr, itemText) != 0)
		{
			hItem = TreeView_GetNextSibling(m_hTreeView, hItem);

			if (hItem == nullptr)
				return nullptr;

			item.pszText = itemText;
			item.cchTextMax = SIZEOF_ARRAY(itemText);
			item.hItem = hItem;
			TreeView_GetItem(m_hTreeView, &item);
		}
	}

	return hItem;
}

/* Locate an item which is a Desktop (sub)child, if visible.
   Does not expand any item */
HTREEITEM ShellTreeView::LocateItemOnDesktopTree(const TCHAR *szFullFileName)
{
	HTREEITEM hItem;
	TVITEMEX tvItem;
	TCHAR szFileName[MAX_PATH];
	TCHAR szDesktop[MAX_PATH];
	TCHAR szCurrentItem[MAX_PATH];
	TCHAR *pItemName = nullptr;
	TCHAR *nextToken = nullptr;
	BOOL bDesktop;
	BOOL bFound;

	bDesktop = IsDesktopSubChild(szFullFileName);

	if (!bDesktop)
	{
		return nullptr;
	}

	StringCchCopy(szFileName, SIZEOF_ARRAY(szFileName), szFullFileName);

	SHGetFolderPath(nullptr, CSIDL_DESKTOP, nullptr, SHGFP_TYPE_CURRENT, szDesktop);

	pItemName = &szFileName[lstrlen(szDesktop)];

	if (lstrlen(szFullFileName) > lstrlen(szDesktop))
	{
		pItemName++; // Skip the "\\" after the desktop folder name
	}

	nextToken = nullptr;
	pItemName = wcstok_s(pItemName, _T("\\"), &nextToken);

	hItem = TreeView_GetRoot(m_hTreeView);

	while (pItemName != nullptr)
	{
		hItem = TreeView_GetChild(m_hTreeView, hItem);
		bFound = FALSE;

		while (hItem != nullptr && !bFound)
		{
			tvItem.mask = TVIF_TEXT;
			tvItem.hItem = hItem;
			tvItem.pszText = szCurrentItem;
			tvItem.cchTextMax = SIZEOF_ARRAY(szCurrentItem);
			TreeView_GetItem(m_hTreeView, &tvItem);

			if (lstrcmp(szCurrentItem, pItemName) == 0)
			{
				bFound = TRUE;
			}
			else
			{
				hItem = TreeView_GetNextSibling(m_hTreeView, hItem);
			}
		}

		if (!bFound)
		{
			return nullptr;
		}

		// Item found, pass to sub-level
		pItemName = wcstok_s(nextToken, _T("\\"), &nextToken);
	}

	return hItem;
}

void ShellTreeView::EraseItems(HTREEITEM hParent)
{
	auto hItem = TreeView_GetChild(m_hTreeView, hParent);

	while (hItem != nullptr)
	{
		TVITEMEX tvItemEx;
		tvItemEx.mask = TVIF_PARAM | TVIF_HANDLE | TVIF_CHILDREN;
		tvItemEx.hItem = hItem;
		TreeView_GetItem(m_hTreeView, &tvItemEx);

		if (tvItemEx.cChildren != 0)
		{
			EraseItems(hItem);
		}

		m_itemInfoMap.erase(static_cast<int>(tvItemEx.lParam));

		hItem = TreeView_GetNextSibling(m_hTreeView, hItem);
	}
}

LRESULT CALLBACK ShellTreeView::OnDeviceChange(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	/* Device has being added/inserted into the system. Update the
	treeview as neccessary. */
	case DBT_DEVICEARRIVAL:
	{
		DEV_BROADCAST_HDR *dbh;

		dbh = (DEV_BROADCAST_HDR *) lParam;

		if (dbh->dbch_devicetype == DBT_DEVTYP_VOLUME)
		{
			DEV_BROADCAST_VOLUME *pdbv = nullptr;
			SHFILEINFO shfi;
			HTREEITEM hItem;
			TVITEM tvItem;
			TCHAR driveLetter;
			TCHAR driveName[4];

			pdbv = (DEV_BROADCAST_VOLUME *) dbh;

			/* Build a string that will form the drive name. */
			driveLetter = GetDriveLetterFromMask(pdbv->dbcv_unitmask);
			StringCchPrintf(driveName, SIZEOF_ARRAY(driveName), _T("%c:\\"), driveLetter);

			if (pdbv->dbcv_flags & DBTF_MEDIA)
			{
				hItem = LocateItemByPath(driveName, FALSE);

				if (hItem != nullptr)
				{
					SHGetFileInfo(driveName, 0, &shfi, sizeof(shfi), SHGFI_SYSICONINDEX);

					std::wstring displayName;
					GetDisplayName(driveName, SHGDN_INFOLDER, displayName);

					/* Update the drives icon and display name. */
					tvItem.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
					tvItem.hItem = hItem;
					tvItem.iImage = shfi.iIcon;
					tvItem.iSelectedImage = shfi.iIcon;
					tvItem.pszText = displayName.data();
					TreeView_SetItem(m_hTreeView, &tvItem);
				}
			}
			else
			{
				/* Add the drive to the treeview. */
				AddItem(driveName);

				MonitorDrive(driveName);
			}
		}
	}
	break;

	case DBT_DEVICEQUERYREMOVE:
	{
		/* The system is looking for permission to remove
		a drive. Stop monitoring the drive. */
		DEV_BROADCAST_HDR *dbh = nullptr;
		DEV_BROADCAST_HANDLE *pdbHandle = nullptr;
		std::list<DriveEvent_t>::iterator itr;

		dbh = (DEV_BROADCAST_HDR *) lParam;

		switch (dbh->dbch_devicetype)
		{
		case DBT_DEVTYP_HANDLE:
		{
			pdbHandle = (DEV_BROADCAST_HANDLE *) dbh;

			/* Loop through each of the registered drives to
			find the one that requested removal. Once it is
			found, stop monitoring it, close its handle,
			and allow the operating system to release the drive.
			Don't remove the drive from the treeview (until it
			has actually been removed). */
			for (itr = m_pDriveList.begin(); itr != m_pDriveList.end(); itr++)
			{
				if (itr->hDrive == pdbHandle->dbch_handle)
				{
					m_pDirMon->StopDirectoryMonitor(itr->iMonitorId);

					/* Log the removal. If a device removal failure message
					is later received, the last entry logged here will be
					restored. */
					m_bQueryRemoveCompleted = TRUE;
					StringCchCopy(m_szQueryRemove, SIZEOF_ARRAY(m_szQueryRemove), itr->szDrive);
					break;
				}
			}
		}
		break;
		}

		return TRUE;
	}

	case DBT_DEVICEQUERYREMOVEFAILED:
	{
		/* The device was not removed from the system. */
		DEV_BROADCAST_HDR *dbh = nullptr;
		DEV_BROADCAST_HANDLE *pdbHandle = nullptr;

		dbh = (DEV_BROADCAST_HDR *) lParam;

		switch (dbh->dbch_devicetype)
		{
		case DBT_DEVTYP_HANDLE:
			pdbHandle = (DEV_BROADCAST_HANDLE *) dbh;

			if (m_bQueryRemoveCompleted)
			{
			}
			break;
		}
	}
	break;

	case DBT_DEVICEREMOVECOMPLETE:
	{
		DEV_BROADCAST_HDR *dbh = nullptr;
		DEV_BROADCAST_HANDLE *pdbHandle = nullptr;
		std::list<DriveEvent_t>::iterator itr;

		dbh = (DEV_BROADCAST_HDR *) lParam;

		switch (dbh->dbch_devicetype)
		{
		case DBT_DEVTYP_HANDLE:
		{
			pdbHandle = (DEV_BROADCAST_HANDLE *) dbh;

			/* The device was removed from the system.
			Unregister its notification handle. */
			UnregisterDeviceNotification(pdbHandle->dbch_hdevnotify);
		}
		break;

		case DBT_DEVTYP_VOLUME:
		{
			DEV_BROADCAST_VOLUME *pdbv = nullptr;
			SHFILEINFO shfi;
			HTREEITEM hItem;
			TVITEM tvItem;
			TCHAR driveLetter;
			TCHAR driveName[4];

			pdbv = (DEV_BROADCAST_VOLUME *) dbh;

			/* Build a string that will form the drive name. */
			driveLetter = GetDriveLetterFromMask(pdbv->dbcv_unitmask);
			StringCchPrintf(driveName, SIZEOF_ARRAY(driveName), _T("%c:\\"), driveLetter);

			if (pdbv->dbcv_flags & DBTF_MEDIA)
			{
				hItem = LocateItemByPath(driveName, FALSE);

				if (hItem != nullptr)
				{
					SHGetFileInfo(driveName, 0, &shfi, sizeof(shfi), SHGFI_SYSICONINDEX);

					std::wstring displayName;
					GetDisplayName(driveName, SHGDN_INFOLDER, displayName);

					/* Update the drives icon and display name. */
					tvItem.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
					tvItem.hItem = hItem;
					tvItem.iImage = shfi.iIcon;
					tvItem.iSelectedImage = shfi.iIcon;
					tvItem.pszText = displayName.data();
					TreeView_SetItem(m_hTreeView, &tvItem);
				}
			}
			else
			{
				/* Remove the drive from the treeview. */
				RemoveItem(driveName);
			}
		}
		break;
		}

		return TRUE;
	}
	}

	return FALSE;
}

DWORD WINAPI Thread_MonitorAllDrives(LPVOID pParam)
{
	ShellTreeView *shellTreeView = nullptr;
	TCHAR *pszDriveStrings = nullptr;
	TCHAR *ptrDrive = nullptr;
	DWORD dwSize;

	shellTreeView = (ShellTreeView *) pParam;

	dwSize = GetLogicalDriveStrings(0, nullptr);

	pszDriveStrings = (TCHAR *) malloc((dwSize + 1) * sizeof(TCHAR));

	if (pszDriveStrings == nullptr)
		return 0;

	dwSize = GetLogicalDriveStrings(dwSize, pszDriveStrings);

	if (dwSize != 0)
	{
		ptrDrive = pszDriveStrings;

		while (*ptrDrive != '\0')
		{
			shellTreeView->MonitorDrivePublic(ptrDrive);

			ptrDrive += lstrlen(ptrDrive) + 1;
		}
	}

	free(pszDriveStrings);

	return 1;
}

void ShellTreeView::MonitorDrivePublic(const TCHAR *szDrive)
{
	MonitorDrive(szDrive);
}

void ShellTreeView::MonitorDrive(const TCHAR *szDrive)
{
	DirectoryAltered_t *pDirectoryAltered = nullptr;
	DEV_BROADCAST_HANDLE dbv;
	HANDLE hDrive;
	HDEVNOTIFY hDevNotify;
	DriveEvent_t de;
	int iMonitorId;

	/* Remote (i.e. network) drives will NOT be monitored. */
	if (GetDriveType(szDrive) != DRIVE_REMOTE)
	{
		hDrive = CreateFile(szDrive, FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

		if (hDrive != INVALID_HANDLE_VALUE)
		{
			pDirectoryAltered = (DirectoryAltered_t *) malloc(sizeof(DirectoryAltered_t));

			StringCchCopy(
				pDirectoryAltered->szPath, SIZEOF_ARRAY(pDirectoryAltered->szPath), szDrive);
			pDirectoryAltered->shellTreeView = this;

			iMonitorId = m_pDirMon->WatchDirectory(hDrive, szDrive, FILE_NOTIFY_CHANGE_DIR_NAME,
				ShellTreeView::DirectoryAlteredCallback, TRUE, (void *) pDirectoryAltered);

			dbv.dbch_size = sizeof(dbv);
			dbv.dbch_devicetype = DBT_DEVTYP_HANDLE;
			dbv.dbch_handle = hDrive;

			/* Register to receive hardware events (i.e. insertion,
			removal, etc) for the specified drive. */
			hDevNotify = RegisterDeviceNotification(m_hTreeView, &dbv, DEVICE_NOTIFY_WINDOW_HANDLE);

			/* If the handle was successfully registered, log the
			drive path, handle and monitoring id. */
			if (hDevNotify != nullptr)
			{
				StringCchCopy(de.szDrive, SIZEOF_ARRAY(de.szDrive), szDrive);
				de.hDrive = hDrive;
				de.iMonitorId = iMonitorId;

				m_pDriveList.push_back(de);
			}
		}
	}
}

void ShellTreeView::OnMiddleButtonDown(const POINT *pt)
{
	TVHITTESTINFO hitTestInfo;
	hitTestInfo.pt = *pt;

	TreeView_HitTest(m_hTreeView, &hitTestInfo);

	if (hitTestInfo.flags != LVHT_NOWHERE)
	{
		m_middleButtonItem = hitTestInfo.hItem;
	}
	else
	{
		m_middleButtonItem = nullptr;
	}
}

void ShellTreeView::OnMiddleButtonUp(const POINT *pt, UINT keysDown)
{
	TVHITTESTINFO hitTestInfo;
	hitTestInfo.pt = *pt;

	TreeView_HitTest(m_hTreeView, &hitTestInfo);

	if (hitTestInfo.flags == LVHT_NOWHERE)
	{
		return;
	}

	// Only open an item if it was the one on which the middle mouse button was initially clicked
	// on.
	if (hitTestInfo.hItem != m_middleButtonItem)
	{
		return;
	}

	bool switchToNewTab = m_config->openTabsInForeground;

	if (WI_IsFlagSet(keysDown, MK_SHIFT))
	{
		switchToNewTab = !switchToNewTab;
	}

	auto pidl = GetItemPidl(hitTestInfo.hItem);
	m_tabContainer->CreateNewTab(pidl.get(), TabSettings(_selected = switchToNewTab));
}

HRESULT ShellTreeView::InitializeDragDropHelpers()
{
	HRESULT hr;

	/* Initialize the drag source helper, and use it to initialize the drop target helper. */
	hr = CoCreateInstance(
		CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pDragSourceHelper));

	if (SUCCEEDED(hr))
	{
		hr = m_pDragSourceHelper->QueryInterface(IID_PPV_ARGS(&m_pDropTargetHelper));

		if (SUCCEEDED(hr))
		{
			hr = RegisterDragDrop(m_hTreeView, this);

			if (SUCCEEDED(hr))
			{
				m_bDragDropRegistered = TRUE;
			}
		}
	}

	return hr;
}

/* IUnknown interface members. */
HRESULT __stdcall ShellTreeView::QueryInterface(REFIID iid, void **ppvObject)
{
	if (ppvObject == nullptr)
	{
		return E_POINTER;
	}

	*ppvObject = nullptr;

	if (iid == IID_IUnknown)
	{
		/* IDropTarget and IDropSource
		both derive from IUnknown, so
		need to explicitly indicate
		which is required (in this
		case, both are equally good). */
		*ppvObject = static_cast<IUnknown *>(static_cast<IDropTarget *>(this));
	}
	else if (iid == IID_IDropTarget)
	{
		*ppvObject = static_cast<IDropTarget *>(this);
	}
	else if (iid == IID_IDropSource)
	{
		*ppvObject = static_cast<IDropSource *>(this);
	}

	if (*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall ShellTreeView::AddRef()
{
	return ++m_iRefCount;
}

ULONG __stdcall ShellTreeView::Release()
{
	m_iRefCount--;

	if (m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

BOOL ShellTreeView::QueryDragging()
{
	return m_bDragging;
}

void ShellTreeView::SetShowHidden(BOOL bShowHidden)
{
	m_bShowHidden = bShowHidden;
}

void ShellTreeView::RefreshAllIcons()
{
	auto hRoot = TreeView_GetRoot(m_hTreeView);

	TVITEMEX tvItemEx;
	tvItemEx.mask = TVIF_HANDLE | TVIF_PARAM;
	tvItemEx.hItem = hRoot;
	TreeView_GetItem(m_hTreeView, &tvItemEx);

	const ItemInfo_t &itemInfo = m_itemInfoMap.at(static_cast<int>(tvItemEx.lParam));

	SHFILEINFO shfi;
	SHGetFileInfo(reinterpret_cast<LPCTSTR>(itemInfo.pidl.get()), 0, &shfi, sizeof(shfi),
		SHGFI_PIDL | SHGFI_SYSICONINDEX);

	tvItemEx.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvItemEx.hItem = hRoot;
	tvItemEx.iImage = shfi.iIcon;
	tvItemEx.iSelectedImage = shfi.iIcon;
	TreeView_SetItem(m_hTreeView, &tvItemEx);

	RefreshAllIconsInternal(TreeView_GetChild(m_hTreeView, hRoot));
}

void ShellTreeView::RefreshAllIconsInternal(HTREEITEM hFirstSibling)
{
	HTREEITEM hNextSibling;
	HTREEITEM hChild;
	TVITEM tvItem;
	SHFILEINFO shfi;

	hNextSibling = TreeView_GetNextSibling(m_hTreeView, hFirstSibling);

	tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
	tvItem.hItem = hFirstSibling;
	TreeView_GetItem(m_hTreeView, &tvItem);

	const ItemInfo_t &itemInfo = m_itemInfoMap[static_cast<int>(tvItem.lParam)];
	SHGetFileInfo(reinterpret_cast<LPCTSTR>(itemInfo.pidl.get()), 0, &shfi, sizeof(shfi),
		SHGFI_PIDL | SHGFI_SYSICONINDEX);

	tvItem.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvItem.hItem = hFirstSibling;
	tvItem.iImage = shfi.iIcon;
	tvItem.iSelectedImage = shfi.iIcon;
	TreeView_SetItem(m_hTreeView, &tvItem);

	hChild = TreeView_GetChild(m_hTreeView, hFirstSibling);

	if (hChild != nullptr)
		RefreshAllIconsInternal(hChild);

	while (hNextSibling != nullptr)
	{
		tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
		tvItem.hItem = hNextSibling;
		TreeView_GetItem(m_hTreeView, &tvItem);

		const ItemInfo_t &itemInfoNext = m_itemInfoMap[static_cast<int>(tvItem.lParam)];
		SHGetFileInfo(reinterpret_cast<LPCTSTR>(itemInfoNext.pidl.get()), 0, &shfi, sizeof(shfi),
			SHGFI_PIDL | SHGFI_SYSICONINDEX);

		tvItem.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvItem.hItem = hNextSibling;
		tvItem.iImage = shfi.iIcon;
		tvItem.iSelectedImage = shfi.iIcon;
		TreeView_SetItem(m_hTreeView, &tvItem);

		hChild = TreeView_GetChild(m_hTreeView, hNextSibling);

		if (hChild != nullptr)
			RefreshAllIconsInternal(hChild);

		hNextSibling = TreeView_GetNextSibling(m_hTreeView, hNextSibling);
	}
}

HRESULT ShellTreeView::OnBeginDrag(int iItemId, DragType dragType)
{
	IDataObject *pDataObject = nullptr;
	IDragSourceHelper *pDragSourceHelper = nullptr;
	IShellFolder *pShellFolder = nullptr;
	PCITEMID_CHILD ridl = nullptr;
	DWORD effect;
	POINT pt = { 0, 0 };
	HRESULT hr;

	hr = CoCreateInstance(
		CLSID_DragDropHelper, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pDragSourceHelper));

	if (SUCCEEDED(hr))
	{
		hr = SHBindToParent(
			m_itemInfoMap.at(iItemId).pidl.get(), IID_PPV_ARGS(&pShellFolder), &ridl);

		if (SUCCEEDED(hr))
		{
			/* Needs to be done from the parent folder for the drag/dop to work correctly.
			If done from the desktop folder, only links to files are created. They are
			not copied/moved. */
			GetUIObjectOf(pShellFolder, m_hTreeView, 1, &ridl, IID_PPV_ARGS(&pDataObject));

			pDragSourceHelper->InitializeFromWindow(m_hTreeView, &pt, pDataObject);

			m_DragType = dragType;

			hr = DoDragDrop(
				pDataObject, this, DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK, &effect);

			m_bDragging = FALSE;

			pDataObject->Release();
			pShellFolder->Release();
		}

		pDragSourceHelper->Release();
	}

	return hr;
}

BOOL ShellTreeView::IsDesktop(const TCHAR *szPath)
{
	TCHAR szDesktop[MAX_PATH];

	SHGetFolderPath(nullptr, CSIDL_DESKTOP, nullptr, SHGFP_TYPE_CURRENT, szDesktop);

	return (lstrcmp(szPath, szDesktop) == 0);
}

BOOL ShellTreeView::IsDesktopSubChild(const TCHAR *szFullFileName)
{
	TCHAR szDesktop[MAX_PATH];

	SHGetFolderPath(nullptr, CSIDL_DESKTOP, nullptr, SHGFP_TYPE_CURRENT, szDesktop);

	return (wcsncmp(szFullFileName, szDesktop, lstrlen(szDesktop)) == 0);
}

void ShellTreeView::StartRenamingSelectedItem()
{
	auto selectedItem = TreeView_GetSelection(m_hTreeView);
	TreeView_EditLabel(m_hTreeView, selectedItem);
}

void ShellTreeView::ShowPropertiesOfSelectedItem() const
{
	auto pidlDirectory = GetSelectedItemPidl();
	ShowMultipleFileProperties(pidlDirectory.get(), nullptr, m_hTreeView, 0);
}

void ShellTreeView::DeleteSelectedItem(bool permanent)
{
	HTREEITEM item = TreeView_GetSelection(m_hTreeView);
	HTREEITEM parentItem = TreeView_GetParent(m_hTreeView, item);

	// Select the parent item to release the lock and allow deletion.
	TreeView_Select(m_hTreeView, parentItem, TVGN_CARET);

	auto pidl = GetItemPidl(item);

	DWORD mask = 0;

	if (permanent)
	{
		mask = CMIC_MASK_SHIFT_DOWN;
	}

	ExecuteActionFromContextMenu(pidl.get(), nullptr, m_hTreeView, 0, _T("delete"), mask);
}

bool ShellTreeView::OnEndLabelEdit(const NMTVDISPINFO *dispInfo)
{
	// If label editing was canceled or no text was entered, simply notify the control to revert to
	// the previous text.
	if (!dispInfo->item.pszText || lstrlen(dispInfo->item.pszText) == 0)
	{
		return false;
	}

	const auto &itemInfo = GetItemByHandle(dispInfo->item.hItem);

	std::wstring oldFileName;
	HRESULT hr = GetDisplayName(itemInfo.pidl.get(), SHGDN_FORPARSING, oldFileName);

	if (FAILED(hr))
	{
		return false;
	}

	TCHAR newFileName[MAX_PATH];
	StringCchCopy(newFileName, SIZEOF_ARRAY(newFileName), oldFileName.c_str());
	PathRemoveFileSpec(newFileName);
	bool res = PathAppend(newFileName, dispInfo->item.pszText);

	if (!res)
	{
		return false;
	}

	FileActionHandler::RenamedItem_t renamedItem;
	renamedItem.strOldFilename = oldFileName;
	renamedItem.strNewFilename = newFileName;

	TrimStringRight(renamedItem.strNewFilename, _T(" "));

	std::list<FileActionHandler::RenamedItem_t> renamedItemList;
	renamedItemList.push_back(renamedItem);
	m_fileActionHandler->RenameFiles(renamedItemList);

	return true;
}

void ShellTreeView::CopySelectedItemToClipboard(bool copy)
{
	HTREEITEM item = TreeView_GetSelection(m_hTreeView);
	auto &itemInfo = GetItemByHandle(item);

	std::wstring fullFileName;
	HRESULT hr = GetDisplayName(itemInfo.pidl.get(), SHGDN_FORPARSING, fullFileName);

	if (FAILED(hr))
	{
		return;
	}

	std::vector<std::wstring> fileNameList = { fullFileName };
	wil::com_ptr_nothrow<IDataObject> clipboardDataObject;

	if (copy)
	{
		hr = CopyFiles(fileNameList, &clipboardDataObject);

		if (SUCCEEDED(hr))
		{
			UpdateCurrentClipboardObject(clipboardDataObject);
		}
	}
	else
	{
		hr = CutFiles(fileNameList, &clipboardDataObject);

		if (SUCCEEDED(hr))
		{
			UpdateCurrentClipboardObject(clipboardDataObject);

			m_cutItem = item;
			UpdateItemState(item, TVIS_CUT, TVIS_CUT);
		}
	}
}

void ShellTreeView::PasteClipboardData()
{
	wil::com_ptr_nothrow<IDataObject> clipboardObject;
	HRESULT hr = OleGetClipboard(&clipboardObject);

	if (FAILED(hr))
	{
		return;
	}

	auto &selectedItem = GetItemByHandle(TreeView_GetSelection(m_hTreeView));

	std::wstring destinationPath;
	hr = GetDisplayName(selectedItem.pidl.get(), SHGDN_FORPARSING, destinationPath);

	if (FAILED(hr))
	{
		return;
	}

	DropHandler *dropHandler = DropHandler::CreateNew();
	dropHandler->CopyClipboardData(clipboardObject.get(), m_hTreeView, destinationPath.c_str(),
		nullptr, !m_config->overwriteExistingFilesConfirmation);
	dropHandler->Release();
}

void ShellTreeView::UpdateCurrentClipboardObject(
	wil::com_ptr_nothrow<IDataObject> clipboardDataObject)
{
	// When copying an item, the WM_CLIPBOARDUPDATE message will be processed after the copy
	// operation has been fully completed. Therefore, any previously cut item will need to have its
	// state restored first. Relying on the WM_CLIPBOARDUPDATE handler wouldn't work, as by the time
	// it runs, m_cutItem would refer to the newly cut item.
	if (m_cutItem)
	{
		UpdateItemState(m_cutItem, TVIS_CUT, 0);
	}

	m_clipboardDataObject = clipboardDataObject;
}

void ShellTreeView::OnClipboardUpdate()
{
	if (m_clipboardDataObject && OleIsCurrentClipboard(m_clipboardDataObject.get()) == S_FALSE)
	{
		if (m_cutItem)
		{
			UpdateItemState(m_cutItem, TVIS_CUT, 0);

			m_cutItem = nullptr;
		}

		m_clipboardDataObject.reset();
	}
}

void ShellTreeView::UpdateItemState(HTREEITEM item, UINT stateMask, UINT state)
{
	TVITEM tvItem;
	tvItem.mask = TVIF_HANDLE | TVIF_STATE;
	tvItem.hItem = item;
	tvItem.stateMask = stateMask;
	tvItem.state = state;
	[[maybe_unused]] bool res = TreeView_SetItem(m_hTreeView, &tvItem);
	assert(res);
}