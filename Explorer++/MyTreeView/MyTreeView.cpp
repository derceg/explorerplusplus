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
#include "MyTreeView.h"
#include "../Helper/CachedIcons.h"
#include "../Helper/DriveInfo.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <wil/com.h>
#include <wil/common.h>

int CALLBACK		CompareItemsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
DWORD WINAPI		Thread_MonitorAllDrives(LPVOID pParam);

CMyTreeView::CMyTreeView(HWND hTreeView, HWND hParent, IDirectoryMonitor *pDirMon, CachedIcons *cachedIcons) :
	m_hTreeView(hTreeView),
	m_pDirMon(pDirMon),
	m_cachedIcons(cachedIcons),
	m_iRefCount(1),
	m_itemIDCounter(0),
	m_bDragDropRegistered(FALSE),
	m_iconThreadPool(1),
	m_iconResultIDCounter(0),
	m_subfoldersThreadPool(1),
	m_subfoldersResultIDCounter(0)
{
	m_windowSubclasses.push_back(WindowSubclassWrapper(m_hTreeView, TreeViewProcStub,
		SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));
	m_windowSubclasses.push_back(WindowSubclassWrapper(hParent, ParentWndProcStub,
		PARENT_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	InitializeCriticalSection(&m_cs);

	m_iFolderIcon = GetDefaultFolderIconIndex();

	m_bDragging			= FALSE;
	m_bDragCancelled	= FALSE;
	m_bDragAllowed		= FALSE;
	m_bShowHidden		= TRUE;

	auto initializeComTask = [] (int id) {
		UNREFERENCED_PARAMETER(id);

		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	};

	m_iconThreadPool.push(initializeComTask);
	m_subfoldersThreadPool.push(initializeComTask);

	AddRoot();

	InitializeDragDropHelpers();

	m_bQueryRemoveCompleted = FALSE;
	HANDLE hThread = CreateThread(NULL,0,Thread_MonitorAllDrives,this,0,NULL);
	CloseHandle(hThread);
}

CMyTreeView::~CMyTreeView()
{
	DeleteCriticalSection(&m_cs);

	m_iconThreadPool.clear_queue();

	auto uninitializeComTask = [] (int id) {
		UNREFERENCED_PARAMETER(id);

		CoUninitialize();
	};

	m_iconThreadPool.push(uninitializeComTask);
	m_subfoldersThreadPool.push(uninitializeComTask);
}

LRESULT CALLBACK CMyTreeView::TreeViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CMyTreeView *pMyTreeView = reinterpret_cast<CMyTreeView *>(dwRefData);

	return pMyTreeView->TreeViewProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CMyTreeView::TreeViewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_TIMER:
			DirectoryAltered();
			break;

		case WM_DEVICECHANGE:
			return OnDeviceChange(wParam,lParam);
			break;

		case WM_RBUTTONDOWN:
			if((wParam & MK_RBUTTON) && !(wParam & MK_LBUTTON)
				&& !(wParam & MK_MBUTTON))
			{
				TVHITTESTINFO tvhti;

				tvhti.pt.x	= LOWORD(lParam);
				tvhti.pt.y	= HIWORD(lParam);

				/* Test to see if the mouse click was
				on an item or not. */
				TreeView_HitTest(m_hTreeView,&tvhti);

				if(!(tvhti.flags & LVHT_NOWHERE))
				{
					m_bDragAllowed = TRUE;
				}
			}
			break;

		case WM_RBUTTONUP:
			m_bDragCancelled = FALSE;
			m_bDragAllowed = FALSE;
			break;

		case WM_MOUSEMOVE:
			{
				if(!m_bDragging && !m_bDragCancelled && m_bDragAllowed)
				{
					if((wParam & MK_RBUTTON) && !(wParam & MK_LBUTTON)
						&& !(wParam & MK_MBUTTON))
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
						MapWindowPoints(HWND_DESKTOP,m_hTreeView,&pt,1);

						tvhti.pt = pt;

						/* Test to see if the mouse click was
						on an item or not. */
						TreeView_HitTest(m_hTreeView,&tvhti);

						if(!(tvhti.flags & LVHT_NOWHERE))
						{
							tvItem.mask		= TVIF_PARAM|TVIF_HANDLE;
							tvItem.hItem	= tvhti.hItem;
							bRet = TreeView_GetItem(m_hTreeView,&tvItem);

							if(bRet)
							{
								hr = OnBeginDrag((int)tvItem.lParam,DRAG_TYPE_RIGHTCLICK);

								if(hr == DRAGDROP_S_CANCEL)
								{
									m_bDragCancelled = TRUE;
								}
							}
						}
					}
				}
			}
			break;

		case WM_APP_ICON_RESULT_READY:
			ProcessIconResult(static_cast<int>(wParam));
			break;

		case WM_APP_SUBFOLDERS_RESULT_READY:
			ProcessSubfoldersResult(static_cast<int>(wParam));
			break;

		case WM_DESTROY:
			if(m_bDragDropRegistered)
			{
				RevokeDragDrop(m_hTreeView);
				m_bDragDropRegistered = FALSE;
			}
			break;
	}

	return DefSubclassProc(hwnd,msg,wParam,lParam);
}

LRESULT CALLBACK CMyTreeView::ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	CMyTreeView *treeView = reinterpret_cast<CMyTreeView *>(dwRefData);
	return treeView->ParentWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CMyTreeView::ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
				NMTREEVIEW *pnmTreeView = reinterpret_cast<NMTREEVIEW *>(lParam);
				OnBeginDrag(static_cast<int>(pnmTreeView->itemNew.lParam), DRAG_TYPE_LEFTCLICK);
			}
			break;

			case TVN_GETDISPINFO:
				OnGetDisplayInfo(reinterpret_cast<NMTVDISPINFO *>(lParam));
				break;

			case TVN_ITEMEXPANDING:
				OnItemExpanding(reinterpret_cast<NMTREEVIEW *>(lParam));
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

HTREEITEM CMyTreeView::AddRoot()
{
	TreeView_DeleteAllItems(m_hTreeView);

	unique_pidl_absolute pidl;
	HRESULT hr = SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 0, wil::out_param(pidl));

	if (FAILED(hr))
	{
		return nullptr;
	}

	TCHAR szDesktopDisplayName[MAX_PATH];
	GetDisplayName(pidl.get(), szDesktopDisplayName, SIZEOF_ARRAY(szDesktopDisplayName), SHGDN_INFOLDER);

	int itemId = GenerateUniqueItemId();
	m_itemInfoMap[itemId].pidl.reset(ILCloneFull(pidl.get()));

	TVITEMEX tvItem;
	tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN;
	tvItem.pszText = szDesktopDisplayName;
	tvItem.iImage = I_IMAGECALLBACK;
	tvItem.iSelectedImage = I_IMAGECALLBACK;
	tvItem.cChildren = 1;
	tvItem.lParam = itemId;

	TVINSERTSTRUCT tvis;
	tvis.hParent = nullptr;
	tvis.hInsertAfter = TVI_LAST;
	tvis.itemex = tvItem;

	HTREEITEM hDesktop = TreeView_InsertItem(m_hTreeView, &tvis);

	if (hDesktop != nullptr)
	{
		SendMessage(m_hTreeView, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(hDesktop));
	}

	return hDesktop;
}

HRESULT CMyTreeView::ExpandDirectory(HTREEITEM hParent)
{
	auto pidlDirectory = GetItemPidl(hParent);

	wil::com_ptr<IShellFolder> pShellFolder;
	HRESULT hr = BindToIdl(pidlDirectory.get(), IID_PPV_ARGS(&pShellFolder));

	if (SUCCEEDED(hr))
	{
		AddDirectoryInternal(pShellFolder.get(), pidlDirectory.get(), hParent);
	}

	return hr;
}

void CMyTreeView::OnGetDisplayInfo(NMTVDISPINFO *pnmtvdi)
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

std::optional<int> CMyTreeView::GetCachedIconIndex(const ItemInfo_t &itemInfo)
{
	TCHAR filePath[MAX_PATH];
	HRESULT hr = GetDisplayName(itemInfo.pidl.get(), filePath, SIZEOF_ARRAY(filePath), SHGDN_FORPARSING);

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

void CMyTreeView::QueueIconTask(HTREEITEM item, int internalIndex)
{
	const ItemInfo_t &itemInfo = m_itemInfoMap.at(internalIndex);

	BasicItemInfo basicItemInfo;
	basicItemInfo.pidl.reset(ILCloneFull(itemInfo.pidl.get()));

	int iconResultID = m_iconResultIDCounter++;

	auto result = m_iconThreadPool.push([this, iconResultID, item, internalIndex, basicItemInfo](int id) {
		UNREFERENCED_PARAMETER(id);

		return FindIconAsync(m_hTreeView, iconResultID, item, internalIndex, basicItemInfo.pidl.get());
	});

	m_iconResults.insert({ iconResultID, std::move(result) });
}

std::optional<CMyTreeView::IconResult> CMyTreeView::FindIconAsync(HWND treeView,
	int iconResultId, HTREEITEM item, int internalIndex, PCIDLIST_ABSOLUTE pidl)
{
	SHFILEINFO shfi;
	DWORD_PTR res = SHGetFileInfo(reinterpret_cast<LPCTSTR>(pidl), 0, &shfi,
		sizeof(SHFILEINFO), SHGFI_PIDL | SHGFI_ICON | SHGFI_OVERLAYINDEX);

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

void CMyTreeView::ProcessIconResult(int iconResultId)
{
	auto itr = m_iconResults.find(iconResultId);

	if (itr == m_iconResults.end())
	{
		return;
	}

	auto cleanup = wil::scope_exit([this, itr] () {
		m_iconResults.erase(itr);
	});

	auto result = itr->second.get();

	if (!result)
	{
		return;
	}

	const ItemInfo_t &itemInfo = m_itemInfoMap.at(result->internalIndex);

	TCHAR filePath[MAX_PATH];
	HRESULT hr = GetDisplayName(itemInfo.pidl.get(), filePath, static_cast<UINT>(std::size(filePath)), SHGDN_FORPARSING);

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

void CMyTreeView::QueueSubfoldersTask(HTREEITEM item)
{
	BasicItemInfo basicItemInfo;
	basicItemInfo.pidl = GetItemPidl(item);

	int subfoldersResultID = m_subfoldersResultIDCounter++;

	auto result = m_subfoldersThreadPool.push([this, subfoldersResultID, item, basicItemInfo] (int id) {
		UNREFERENCED_PARAMETER(id);

		return CheckSubfoldersAsync(m_hTreeView, subfoldersResultID, item, basicItemInfo.pidl.get());
	});

	m_subfoldersResults.insert({ subfoldersResultID, std::move(result) });
}

std::optional<CMyTreeView::SubfoldersResult> CMyTreeView::CheckSubfoldersAsync(HWND treeView,
	int subfoldersResultId, HTREEITEM item, PCIDLIST_ABSOLUTE pidl)
{
	wil::com_ptr<IShellFolder> pShellFolder;
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

void CMyTreeView::ProcessSubfoldersResult(int subfoldersResultId)
{
	auto itr = m_subfoldersResults.find(subfoldersResultId);

	if (itr == m_subfoldersResults.end())
	{
		return;
	}

	auto cleanup = wil::scope_exit([this, itr] () {
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

void CMyTreeView::OnItemExpanding(const NMTREEVIEW *nmtv)
{
	HTREEITEM parentItem = nmtv->itemNew.hItem;

	if (nmtv->action == TVE_EXPAND)
	{
		ExpandDirectory(parentItem);
	}
	else
	{
		HTREEITEM hSelection = TreeView_GetSelection(m_hTreeView);

		if (hSelection != NULL)
		{
			HTREEITEM hItem = hSelection;

			do
			{
				hItem = TreeView_GetParent(m_hTreeView, hItem);
			} while (hItem != parentItem && hItem != NULL);

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

/* Sorts items in the following order:
 - Drives
 - Virtual Items
 - Real Items

Each set is ordered alphabetically. */
int CALLBACK CompareItemsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CMyTreeView *pMyTreeView = NULL;

	pMyTreeView = (CMyTreeView *)lParamSort;

	return pMyTreeView->CompareItems(lParam1,lParam2);
}

int CALLBACK CMyTreeView::CompareItems(LPARAM lParam1,LPARAM lParam2)
{
	TCHAR szDisplayName1[MAX_PATH];
	TCHAR szDisplayName2[MAX_PATH];
	TCHAR szTemp[MAX_PATH];
	int iItemId1 = (int)lParam1;
	int iItemId2 = (int)lParam2;

	const ItemInfo_t &itemInfo1 = m_itemInfoMap.at(iItemId1);
	const ItemInfo_t &itemInfo2 = m_itemInfoMap.at(iItemId2);

	GetDisplayName(itemInfo1.pidl.get(),szDisplayName1,SIZEOF_ARRAY(szDisplayName1),SHGDN_FORPARSING);
	GetDisplayName(itemInfo2.pidl.get(),szDisplayName2,SIZEOF_ARRAY(szDisplayName2),SHGDN_FORPARSING);

	if(PathIsRoot(szDisplayName1) && !PathIsRoot(szDisplayName2))
	{
		return -1;
	}
	else if(!PathIsRoot(szDisplayName1) && PathIsRoot(szDisplayName2))
	{
		return 1;
	}
	else if(PathIsRoot(szDisplayName1) && PathIsRoot(szDisplayName2))
	{
		return lstrcmpi(szDisplayName1,szDisplayName2);
	}
	else
	{
		if(!SHGetPathFromIDList(itemInfo1.pidl.get(),szTemp) &&
			SHGetPathFromIDList(itemInfo2.pidl.get(),szTemp))
		{
			return -1;
		}
		else if(SHGetPathFromIDList(itemInfo1.pidl.get(),szTemp) &&
			!SHGetPathFromIDList(itemInfo2.pidl.get(),szTemp))
		{
			return 1;
		}
		else
		{
			GetDisplayName(itemInfo1.pidl.get(),szDisplayName1,SIZEOF_ARRAY(szDisplayName1),SHGDN_INFOLDER);
			GetDisplayName(itemInfo2.pidl.get(),szDisplayName2,SIZEOF_ARRAY(szDisplayName2),SHGDN_INFOLDER);

			return StrCmpLogicalW(szDisplayName1,szDisplayName2);
		}
	}
}

void CMyTreeView::AddDirectoryInternal(IShellFolder *pShellFolder, PCIDLIST_ABSOLUTE pidlDirectory,
	HTREEITEM hParent)
{
	SHCONTF EnumFlags = SHCONTF_FOLDERS;

	if (m_bShowHidden)
	{
		EnumFlags |= SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN;
	}

	wil::com_ptr<IEnumIDList> pEnumIDList;
	HRESULT hr = pShellFolder->EnumObjects(NULL, EnumFlags, &pEnumIDList);

	if (FAILED(hr) || !pEnumIDList)
	{
		return;
	}

	SendMessage(m_hTreeView, WM_SETREDRAW, FALSE, 0);

	std::vector<EnumeratedItem> items;

	unique_pidl_child rgelt;
	ULONG uFetched = 1;

	while (pEnumIDList->Next(1, wil::out_param(rgelt), &uFetched) == S_OK && (uFetched == 1))
	{
		STRRET str;
		hr = pShellFolder->GetDisplayNameOf(rgelt.get(), SHGDN_NORMAL, &str);

		if (SUCCEEDED(hr))
		{
			TCHAR itemName[MAX_PATH];
			hr = StrRetToBuf(&str, rgelt.get(), itemName, SIZEOF_ARRAY(itemName));

			if (SUCCEEDED(hr))
			{
				int itemId = GenerateUniqueItemId();
				m_itemInfoMap[itemId].pidl.reset(ILCombine(pidlDirectory, rgelt.get()));
				m_itemInfoMap[itemId].pridl.reset(ILCloneChild(rgelt.get()));

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
}

int CMyTreeView::GenerateUniqueItemId()
{
	return m_itemIDCounter++;
}

HTREEITEM CMyTreeView::DetermineItemSortedPosition(HTREEITEM hParent, const TCHAR *szItem)
{
	HTREEITEM	htInsertAfter = NULL;

	if(PathIsRoot(szItem))
	{
		return DetermineDriveSortedPosition(hParent,szItem);
	}
	else
	{
		HTREEITEM	htItem;
		HTREEITEM	hPreviousItem;
		TVITEMEX	Item;
		SFGAOF		Attributes;
		TCHAR		szFullItemPath[MAX_PATH];

		/* Insert the item in its sorted position, skipping
		past any drives or any non-filesystem items (i.e.
		'My Computer', 'Recycle Bin', etc). */
		htItem = TreeView_GetChild(m_hTreeView,hParent);

		/* If the parent has no children, this item will
		be the first that appears. */
		if(htItem == NULL)
			return TVI_FIRST;

		hPreviousItem = TVI_FIRST;

		while(htInsertAfter == NULL)
		{
			Item.mask		= TVIF_PARAM|TVIF_HANDLE;
			Item.hItem		= htItem;
			TreeView_GetItem(m_hTreeView,&Item);

			GetDisplayName(m_itemInfoMap.at(static_cast<int>(Item.lParam)).pidl.get(),
				szFullItemPath, SIZEOF_ARRAY(szFullItemPath), SHGDN_FORPARSING);

			Attributes = GetFileAttributes(szFullItemPath);

			/* Only perform the comparison if the current item is a real
			file or folder. */
			if(!PathIsRoot(szFullItemPath) && ((Attributes & SFGAO_FILESYSTEM) != SFGAO_FILESYSTEM))
			{
				if(lstrcmpi(szItem,szFullItemPath) < 0)
				{
					htInsertAfter = hPreviousItem;
				}
			}

			hPreviousItem = htItem;
			htItem = TreeView_GetNextSibling(m_hTreeView,htItem);

			if((htItem == NULL) && !htInsertAfter)
			{
				htInsertAfter = TVI_LAST;
			}
		}
	}

	return htInsertAfter;
}

HTREEITEM CMyTreeView::DetermineDriveSortedPosition(HTREEITEM hParent, const TCHAR *szItemName)
{
	HTREEITEM	htItem;
	HTREEITEM	hPreviousItem;
	TVITEMEX	Item;
	TCHAR		szFullItemPath[MAX_PATH];

	/* Drives will always be the first children of the specified
	item (usually 'My Computer'). Therefore, keep looping while
	there are more child items and the current item comes
	afterwards, or if there are no child items, place the item
	as the first child. */
	htItem = TreeView_GetChild(m_hTreeView,hParent);

	if(htItem == NULL)
		return TVI_FIRST;

	hPreviousItem = TVI_FIRST;

	while(htItem != NULL)
	{
		Item.mask		= TVIF_PARAM | TVIF_HANDLE;
		Item.hItem		= htItem;
		TreeView_GetItem(m_hTreeView,&Item);

		GetDisplayName(m_itemInfoMap.at(static_cast<int>(Item.lParam)).pidl.get(),
			szFullItemPath, SIZEOF_ARRAY(szFullItemPath), SHGDN_FORPARSING);

		if(PathIsRoot(szFullItemPath))
		{
			if(lstrcmp(szItemName,szFullItemPath) < 0)
				return hPreviousItem;
		}
		else
		{
			return hPreviousItem;
		}

		hPreviousItem = htItem;
		htItem = TreeView_GetNextSibling(m_hTreeView,htItem);
	}

	return htItem;
}

unique_pidl_absolute CMyTreeView::GetItemPidl(HTREEITEM hTreeItem)
{
	TVITEMEX tvItemEx;
	tvItemEx.mask = TVIF_HANDLE | TVIF_PARAM;
	tvItemEx.hItem = hTreeItem;
	TreeView_GetItem(m_hTreeView, &tvItemEx);

	const ItemInfo_t &itemInfo = m_itemInfoMap.at(static_cast<int>(tvItemEx.lParam));
	unique_pidl_absolute pidl(ILCloneFull(itemInfo.pidl.get()));

	return pidl;
}

HTREEITEM CMyTreeView::LocateItem(PCIDLIST_ABSOLUTE pidlDirectory)
{
	return LocateItemInternal(pidlDirectory,FALSE);
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
HTREEITEM CMyTreeView::LocateDeletedItem(const TCHAR *szFullFileName)
{
	HTREEITEM hItem = NULL;
	PIDLIST_ABSOLUTE pidl = NULL;
	TCHAR szParent[MAX_PATH];
	BOOL bFound = FALSE;
	HRESULT hr;

	StringCchCopy(szParent,SIZEOF_ARRAY(szParent),szFullFileName);
	PathRemoveFileSpec(szParent);
	hr = SHParseDisplayName(szParent, nullptr, &pidl, 0, nullptr);

	if(SUCCEEDED(hr))
	{
		hItem = LocateExistingItem(pidl);

		if(hItem != NULL)
		{
			HTREEITEM hChild;
			TVITEM tvItem;
			TCHAR szFileName[MAX_PATH];

			hChild = TreeView_GetChild(m_hTreeView,hItem);

			StringCchCopy(szFileName,SIZEOF_ARRAY(szFileName),szFullFileName);
			PathStripPath(szFileName);

			/* Now try to find the child folder. */
			while(hChild != NULL)
			{
				TCHAR szItem[MAX_PATH];

				tvItem.mask			= TVIF_TEXT|TVIF_HANDLE;
				tvItem.hItem		= hChild;
				tvItem.pszText		= szItem;
				tvItem.cchTextMax	= SIZEOF_ARRAY(szItem);
				TreeView_GetItem(m_hTreeView,&tvItem);

				if(lstrcmp(szFileName,szItem) == 0)
				{
					hItem = hChild;
					bFound = TRUE;
					break;
				}

				hChild = TreeView_GetNextSibling(m_hTreeView,hChild);
			}
		}
	}

	if(!bFound)
	{
		hItem = LocateItemByPath(szFullFileName,FALSE);
	}

	return hItem;
}

HTREEITEM CMyTreeView::LocateExistingItem(const TCHAR *szParsingPath)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(szParsingPath, nullptr, wil::out_param(pidl), 0, nullptr);

	if(SUCCEEDED(hr))
	{
		return LocateExistingItem(pidl.get());
	}

	return NULL;
}

HTREEITEM CMyTreeView::LocateExistingItem(PCIDLIST_ABSOLUTE pidlDirectory)
{
	return LocateItemInternal(pidlDirectory,TRUE);
}

HTREEITEM CMyTreeView::LocateItemInternal(PCIDLIST_ABSOLUTE pidlDirectory, BOOL bOnlyLocateExistingItem)
{
	HTREEITEM	hRoot;
	HTREEITEM	hItem;
	TVITEMEX	Item;
	BOOL		bFound = FALSE;

	/* Get the root of the tree (root of namespace). */
	hRoot = TreeView_GetRoot(m_hTreeView);
	hItem = hRoot;

	Item.mask		= TVIF_PARAM|TVIF_HANDLE;
	Item.hItem		= hItem;
	TreeView_GetItem(m_hTreeView,&Item);

	/* Keep searching until the specified item
	is found or it is found the item does not
	exist in the treeview.
	Look through each item, once an ancestor is
	found, look through it's children, expanding
	the parent node if necessary. */
	while(!bFound && hItem != NULL)
	{
		if(CompareIdls(m_itemInfoMap.at(static_cast<int>(Item.lParam)).pidl.get(),pidlDirectory))
		{
			bFound = TRUE;

			break;
		}

		if(ILIsParent(m_itemInfoMap.at(static_cast<int>(Item.lParam)).pidl.get(),pidlDirectory,FALSE))
		{
			if((TreeView_GetChild(m_hTreeView,hItem)) == NULL)
			{
				if(bOnlyLocateExistingItem)
				{
					return NULL;
				}
				else
				{
					SendMessage(m_hTreeView,TVM_EXPAND,(WPARAM)TVE_EXPAND,
					(LPARAM)hItem);
				}
			}

			hItem = TreeView_GetChild(m_hTreeView,hItem);
		}
		else
		{
			hItem = TreeView_GetNextSibling(m_hTreeView,hItem);
		}

		Item.mask		= TVIF_PARAM|TVIF_HANDLE;
		Item.hItem		= hItem;
		TreeView_GetItem(m_hTreeView,&Item);
	}

	return hItem;
}

HTREEITEM CMyTreeView::LocateItemByPath(const TCHAR *szItemPath, BOOL bExpand)
{
	HTREEITEM		hMyComputer;
	HTREEITEM		hItem;
	HTREEITEM		hNextItem;
	TVITEMEX		Item;
	TCHAR			*ptr = NULL;
	TCHAR			ItemText[MAX_PATH];
	TCHAR			FullItemPathCopy[MAX_PATH];
	TCHAR			szItemName[MAX_PATH];
	TCHAR			*next_token = NULL;

	StringCchCopy(FullItemPathCopy,SIZEOF_ARRAY(FullItemPathCopy),
	szItemPath);

	PathRemoveBackslash(FullItemPathCopy);

	unique_pidl_absolute pidlMyComputer;
	SHGetFolderLocation(NULL,CSIDL_DRIVES,NULL,NULL,wil::out_param(pidlMyComputer));

	hMyComputer = LocateItem(pidlMyComputer.get());

	/* First of drives in system. */
	hItem = TreeView_GetChild(m_hTreeView,hMyComputer);

	/* My Computer node may not be expanded. */
	if(hItem == NULL)
		return NULL;

	ptr = cstrtok_s(FullItemPathCopy,_T("\\"),&next_token);

	StringCchCopy(ItemText,SIZEOF_ARRAY(ItemText),ptr);
	StringCchCat(ItemText,SIZEOF_ARRAY(ItemText),_T("\\"));
	ptr = ItemText;

	Item.mask		= TVIF_HANDLE|TVIF_PARAM;
	Item.hItem		= hItem;
	TreeView_GetItem(m_hTreeView,&Item);

	GetDisplayName(m_itemInfoMap.at(static_cast<int>(Item.lParam)).pidl.get(),
		szItemName, SIZEOF_ARRAY(szItemName), SHGDN_FORPARSING);

	while(StrCmpI(ptr,szItemName) != 0)
	{
		hItem = TreeView_GetNextSibling(m_hTreeView,hItem);

		if(hItem == NULL)
			return NULL;

		Item.mask		= TVIF_PARAM;
		Item.hItem		= hItem;
		TreeView_GetItem(m_hTreeView,&Item);

		GetDisplayName(m_itemInfoMap.at(static_cast<int>(Item.lParam)).pidl.get(),
			szItemName, SIZEOF_ARRAY(szItemName), SHGDN_FORPARSING);
	}

	Item.mask = TVIF_TEXT;

	while((ptr = cstrtok_s(NULL,_T("\\"),&next_token)) != NULL)
	{
		if(TreeView_GetChild(m_hTreeView,hItem) == NULL)
		{
			if(bExpand)
				SendMessage(m_hTreeView,TVM_EXPAND,(WPARAM)TVE_EXPAND,(LPARAM)hItem);
			else
				return NULL;
		}

		hNextItem = TreeView_GetChild(m_hTreeView,hItem);
		hItem = hNextItem;

		Item.pszText	= ItemText;
		Item.cchTextMax	= SIZEOF_ARRAY(ItemText);
		Item.hItem		= hItem;
		TreeView_GetItem(m_hTreeView,&Item);

		while(StrCmpI(ptr,ItemText) != 0)
		{
			hItem = TreeView_GetNextSibling(m_hTreeView,hItem);

			if(hItem == NULL)
				return NULL;

			Item.pszText	= ItemText;
			Item.cchTextMax	= SIZEOF_ARRAY(ItemText);
			Item.hItem		= hItem;
			TreeView_GetItem(m_hTreeView,&Item);
		}
	}

	return hItem;
}

/* Locate an item which is a Desktop (sub)child, if visible.
   Does not expand any item */
HTREEITEM CMyTreeView::LocateItemOnDesktopTree(const TCHAR *szFullFileName)
{
	HTREEITEM	hItem;
	TVITEMEX	tvItem;
	TCHAR		szFileName[MAX_PATH];
	TCHAR		szDesktop[MAX_PATH];
	TCHAR		szCurrentItem[MAX_PATH];
	TCHAR		*pItemName = NULL;
	TCHAR		*next_token = NULL;
	BOOL		bDesktop;
	BOOL		bFound;

	bDesktop = IsDesktopSubChild(szFullFileName);

	if (!bDesktop)
	{
		return NULL;
	}

	StringCchCopy(szFileName,SIZEOF_ARRAY(szFileName), szFullFileName);
	
	SHGetFolderPath(NULL,CSIDL_DESKTOP,NULL,SHGFP_TYPE_CURRENT,szDesktop);

	pItemName = &szFileName[lstrlen(szDesktop)];

	if(lstrlen(szFullFileName) > lstrlen(szDesktop))
	{
		pItemName++;  // Skip the "\\" after the desktop folder name
	}
	
	next_token = NULL;
	pItemName = cstrtok_s(pItemName,_T("\\"),&next_token);

	hItem = TreeView_GetRoot(m_hTreeView);

	while(pItemName != NULL)
	{
		hItem = TreeView_GetChild(m_hTreeView,hItem);
		bFound = FALSE;

		while(hItem != NULL && !bFound)
		{
			tvItem.mask			= TVIF_TEXT;
			tvItem.hItem		= hItem;
			tvItem.pszText		= szCurrentItem;
			tvItem.cchTextMax	= SIZEOF_ARRAY(szCurrentItem);
			TreeView_GetItem(m_hTreeView,&tvItem);

			if(lstrcmp(szCurrentItem,pItemName) == 0)
			{
				bFound = TRUE;
			}
			else
			{
				hItem = TreeView_GetNextSibling(m_hTreeView,hItem);
			}
		}

		if(!bFound)
		{
			return NULL;
		}

		// Item found, pass to sub-level
		pItemName = cstrtok_s(next_token,_T("\\"),&next_token);
	}

	return hItem;
}

void CMyTreeView::EraseItems(HTREEITEM hParent)
{
	HTREEITEM hItem = TreeView_GetChild(m_hTreeView, hParent);

	while (hItem != NULL)
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

LRESULT CALLBACK CMyTreeView::OnDeviceChange(WPARAM wParam,LPARAM lParam)
{
	switch(wParam)
	{
		/* Device has being added/inserted into the system. Update the
		treeview as neccessary. */
		case DBT_DEVICEARRIVAL:
			{
				DEV_BROADCAST_HDR *dbh;

				dbh = (DEV_BROADCAST_HDR *)lParam;

				if(dbh->dbch_devicetype == DBT_DEVTYP_VOLUME)
				{
					DEV_BROADCAST_VOLUME	*pdbv = NULL;
					SHFILEINFO				shfi;
					HTREEITEM				hItem;
					TVITEM					tvItem;
					TCHAR					DriveLetter;
					TCHAR					DriveName[4];
					TCHAR					szDisplayName[MAX_PATH];

					pdbv = (DEV_BROADCAST_VOLUME *)dbh;

					/* Build a string that will form the drive name. */
					DriveLetter = GetDriveLetterFromMask(pdbv->dbcv_unitmask);
					StringCchPrintf(DriveName,SIZEOF_ARRAY(DriveName),_T("%c:\\"),DriveLetter);

					if(pdbv->dbcv_flags & DBTF_MEDIA)
					{
						hItem = LocateItemByPath(DriveName,FALSE);

						if(hItem != NULL)
						{
							SHGetFileInfo(DriveName,0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);
							GetDisplayName(DriveName,szDisplayName,SIZEOF_ARRAY(szDisplayName),SHGDN_INFOLDER);

							/* Update the drives icon and display name. */
							tvItem.mask				= TVIF_HANDLE|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
							tvItem.hItem			= hItem;
							tvItem.iImage			= shfi.iIcon;
							tvItem.iSelectedImage	= shfi.iIcon;
							tvItem.pszText			= szDisplayName;
							TreeView_SetItem(m_hTreeView,&tvItem);
						}
					}
					else
					{
						/* Add the drive to the treeview. */
						AddItem(DriveName);

						MonitorDrive(DriveName);
					}
				}
			}
			break;

		case DBT_DEVICEQUERYREMOVE:
			{
				/* The system is looking for permission to remove
				a drive. Stop monitoring the drive. */
				DEV_BROADCAST_HDR				*dbh = NULL;
				DEV_BROADCAST_HANDLE			*pdbHandle = NULL;
				std::list<DriveEvent_t>::iterator	itr;

				dbh = (DEV_BROADCAST_HDR *)lParam;

				switch(dbh->dbch_devicetype)
				{
					case DBT_DEVTYP_HANDLE:
						{
							pdbHandle = (DEV_BROADCAST_HANDLE *)dbh;

							/* Loop through each of the registered drives to
							find the one that requested removal. Once it is
							found, stop monitoring it, close its handle,
							and allow the operating system to release the drive.
							Don't remove the drive from the treeview (until it
							has actually been removed). */
							for(itr = m_pDriveList.begin();itr != m_pDriveList.end();itr++)
							{
								if(itr->hDrive == pdbHandle->dbch_handle)
								{
									m_pDirMon->StopDirectoryMonitor(itr->iMonitorId);

									/* Log the removal. If a device removal failure message
									is later received, the last entry logged here will be
									restored. */
									m_bQueryRemoveCompleted = TRUE;
									StringCchCopy(m_szQueryRemove,SIZEOF_ARRAY(m_szQueryRemove),itr->szDrive);
									break;
								}
							}
						}
						break;
				}

				return TRUE;
			}
			break;

		case DBT_DEVICEQUERYREMOVEFAILED:
			{
				/* The device was not removed from the system. */
				DEV_BROADCAST_HDR				*dbh = NULL;
				DEV_BROADCAST_HANDLE			*pdbHandle = NULL;

				dbh = (DEV_BROADCAST_HDR *)lParam;

				switch(dbh->dbch_devicetype)
				{
					case DBT_DEVTYP_HANDLE:
						pdbHandle = (DEV_BROADCAST_HANDLE *)dbh;

						if(m_bQueryRemoveCompleted)
						{
						}
						break;
				}
			}
			break;

		case DBT_DEVICEREMOVECOMPLETE:
			{
				DEV_BROADCAST_HDR				*dbh = NULL;
				DEV_BROADCAST_HANDLE			*pdbHandle = NULL;
				std::list<DriveEvent_t>::iterator	itr;

				dbh = (DEV_BROADCAST_HDR *)lParam;

				switch(dbh->dbch_devicetype)
				{
					case DBT_DEVTYP_HANDLE:
						{
							pdbHandle = (DEV_BROADCAST_HANDLE *)dbh;

							/* The device was removed from the system.
							Unregister its notification handle. */
							UnregisterDeviceNotification(pdbHandle->dbch_hdevnotify);
						}
						break;

					case DBT_DEVTYP_VOLUME:
						{
							DEV_BROADCAST_VOLUME	*pdbv = NULL;
							SHFILEINFO				shfi;
							HTREEITEM				hItem;
							TVITEM					tvItem;
							TCHAR					DriveLetter;
							TCHAR					DriveName[4];
							TCHAR					szDisplayName[MAX_PATH];

							pdbv = (DEV_BROADCAST_VOLUME *)dbh;

							/* Build a string that will form the drive name. */
							DriveLetter = GetDriveLetterFromMask(pdbv->dbcv_unitmask);
							StringCchPrintf(DriveName,SIZEOF_ARRAY(DriveName),_T("%c:\\"),DriveLetter);

							if(pdbv->dbcv_flags & DBTF_MEDIA)
							{
								hItem = LocateItemByPath(DriveName,FALSE);

								if(hItem != NULL)
								{
									SHGetFileInfo(DriveName,0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);
									GetDisplayName(DriveName,szDisplayName,SIZEOF_ARRAY(szDisplayName),SHGDN_INFOLDER);

									/* Update the drives icon and display name. */
									tvItem.mask				= TVIF_HANDLE|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
									tvItem.hItem			= hItem;
									tvItem.iImage			= shfi.iIcon;
									tvItem.iSelectedImage	= shfi.iIcon;
									tvItem.pszText			= szDisplayName;
									TreeView_SetItem(m_hTreeView,&tvItem);
								}
							}
							else
							{
								/* Remove the drive from the treeview. */
								RemoveItem(DriveName);
							}
						}
						break;
				}

				return TRUE;
			}
			break;
	}

	return FALSE;
}

DWORD WINAPI Thread_MonitorAllDrives(LPVOID pParam)
{
	CMyTreeView *pMyTreeView = NULL;
	TCHAR	*pszDriveStrings = NULL;
	TCHAR	*ptrDrive = NULL;
	DWORD	dwSize;

	pMyTreeView = (CMyTreeView *)pParam;

	dwSize = GetLogicalDriveStrings(0,NULL);

	pszDriveStrings = (TCHAR *)malloc((dwSize + 1) * sizeof(TCHAR));

	if(pszDriveStrings == NULL)
		return 0;

	dwSize = GetLogicalDriveStrings(dwSize,pszDriveStrings);

	if(dwSize != 0)
	{
		ptrDrive = pszDriveStrings;

		while(*ptrDrive != '\0')
		{
			pMyTreeView->MonitorDrivePublic(ptrDrive);

			ptrDrive += lstrlen(ptrDrive) + 1;
		}
	}

	free(pszDriveStrings);

	return 1;
}

void CMyTreeView::MonitorDrivePublic(const TCHAR *szDrive)
{
	MonitorDrive(szDrive);
}

void CMyTreeView::MonitorDrive(const TCHAR *szDrive)
{
	DirectoryAltered_t		*pDirectoryAltered = NULL;
	DEV_BROADCAST_HANDLE	dbv;
	HANDLE					hDrive;
	HDEVNOTIFY				hDevNotify;
	DriveEvent_t			de;
	int						iMonitorId;

	/* Remote (i.e. network) drives will NOT be monitored. */
	if(GetDriveType(szDrive) != DRIVE_REMOTE)
	{
		hDrive = CreateFile(szDrive,
			FILE_LIST_DIRECTORY,FILE_SHARE_READ|
			FILE_SHARE_DELETE|FILE_SHARE_WRITE,
			NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|
			FILE_FLAG_OVERLAPPED,NULL);

		if(hDrive != INVALID_HANDLE_VALUE)
		{
			pDirectoryAltered = (DirectoryAltered_t *)malloc(sizeof(DirectoryAltered_t));

			StringCchCopy(pDirectoryAltered->szPath, SIZEOF_ARRAY(pDirectoryAltered->szPath), szDrive);
			pDirectoryAltered->pMyTreeView	= this;

			iMonitorId = m_pDirMon->WatchDirectory(hDrive,szDrive,FILE_NOTIFY_CHANGE_DIR_NAME,
				CMyTreeView::DirectoryAlteredCallback,TRUE,(void *)pDirectoryAltered);

			dbv.dbch_size		= sizeof(dbv);
			dbv.dbch_devicetype	= DBT_DEVTYP_HANDLE;
			dbv.dbch_handle		= hDrive;

			/* Register to receive hardware events (i.e. insertion,
			removal, etc) for the specified drive. */
			hDevNotify = RegisterDeviceNotification(m_hTreeView,
				&dbv,DEVICE_NOTIFY_WINDOW_HANDLE);

			/* If the handle was successfully registered, log the
			drive path, handle and monitoring id. */
			if(hDevNotify != NULL)
			{
				StringCchCopy(de.szDrive,SIZEOF_ARRAY(de.szDrive),szDrive);
				de.hDrive = hDrive;
				de.iMonitorId = iMonitorId;

				m_pDriveList.push_back(de);
			}
		}
	}
}

HRESULT CMyTreeView::InitializeDragDropHelpers(void)
{
	HRESULT hr;

	/* Initialize the drag source helper, and use it to initialize the drop target helper. */
	hr = CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&m_pDragSourceHelper));

	if(SUCCEEDED(hr))
	{
		hr = m_pDragSourceHelper->QueryInterface(IID_PPV_ARGS(&m_pDropTargetHelper));

		if(SUCCEEDED(hr))
		{
			hr = RegisterDragDrop(m_hTreeView, this);

			if(SUCCEEDED(hr))
			{
				m_bDragDropRegistered = TRUE;
			}
		}
	}

	return hr;
}

/* IUnknown interface members. */
HRESULT __stdcall CMyTreeView::QueryInterface(REFIID iid, void **ppvObject)
{
	if(ppvObject == NULL)
	{
		return E_POINTER;
	}

	*ppvObject = NULL;

	if(iid == IID_IUnknown)
	{
		/* IDropTarget and IDropSource
		both derive from IUnknown, so
		need to explicitly indicate
		which is required (in this
		case, both are equally good). */
		*ppvObject = static_cast<IUnknown *>(static_cast<IDropTarget *>(this));
	}
	else if(iid == IID_IDropTarget)
	{
		*ppvObject = static_cast<IDropTarget *>(this);
	}
	else if(iid == IID_IDropSource)
	{
		*ppvObject = static_cast<IDropSource *>(this);
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall CMyTreeView::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall CMyTreeView::Release(void)
{
	m_iRefCount--;
	
	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

BOOL CMyTreeView::QueryDragging(void)
{
	return m_bDragging;
}

void CMyTreeView::SetShowHidden(BOOL bShowHidden)
{
	m_bShowHidden = bShowHidden;
}

void CMyTreeView::RefreshAllIcons(void)
{
	HTREEITEM hRoot = TreeView_GetRoot(m_hTreeView);

	TVITEMEX tvItemEx;
	tvItemEx.mask			= TVIF_HANDLE|TVIF_PARAM;
	tvItemEx.hItem			= hRoot;
	TreeView_GetItem(m_hTreeView,&tvItemEx);

	const ItemInfo_t &itemInfo = m_itemInfoMap.at(static_cast<int>(tvItemEx.lParam));

	SHFILEINFO shfi;
	SHGetFileInfo(reinterpret_cast<LPCTSTR>(itemInfo.pidl.get()), 0, &shfi,
		sizeof(shfi), SHGFI_PIDL | SHGFI_SYSICONINDEX);

	tvItemEx.mask			= TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
	tvItemEx.hItem			= hRoot;
	tvItemEx.iImage			= shfi.iIcon;
	tvItemEx.iSelectedImage	= shfi.iIcon;
	TreeView_SetItem(m_hTreeView,&tvItemEx);

	RefreshAllIconsInternal(TreeView_GetChild(m_hTreeView,hRoot));
}

void CMyTreeView::RefreshAllIconsInternal(HTREEITEM hFirstSibling)
{
	HTREEITEM	hNextSibling;
	HTREEITEM	hChild;
	TVITEM		tvItem;
	SHFILEINFO	shfi;

	hNextSibling = TreeView_GetNextSibling(m_hTreeView,hFirstSibling);

	tvItem.mask				= TVIF_HANDLE|TVIF_PARAM;
	tvItem.hItem			= hFirstSibling;
	TreeView_GetItem(m_hTreeView,&tvItem);

	const ItemInfo_t &itemInfo = m_itemInfoMap[static_cast<int>(tvItem.lParam)];
	SHGetFileInfo(reinterpret_cast<LPCTSTR>(itemInfo.pidl.get()), 0, &shfi,
		sizeof(shfi), SHGFI_PIDL | SHGFI_SYSICONINDEX);

	tvItem.mask				= TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
	tvItem.hItem			= hFirstSibling;
	tvItem.iImage			= shfi.iIcon;
	tvItem.iSelectedImage	= shfi.iIcon;
	TreeView_SetItem(m_hTreeView,&tvItem);

	hChild = TreeView_GetChild(m_hTreeView,hFirstSibling);

	if(hChild != NULL)
		RefreshAllIconsInternal(hChild);

	while(hNextSibling != NULL)
	{
		tvItem.mask				= TVIF_HANDLE|TVIF_PARAM;
		tvItem.hItem			= hNextSibling;
		TreeView_GetItem(m_hTreeView,&tvItem);

		const ItemInfo_t &itemInfoNext = m_itemInfoMap[static_cast<int>(tvItem.lParam)];
		SHGetFileInfo(reinterpret_cast<LPCTSTR>(itemInfoNext.pidl.get()), 0, &shfi,
			sizeof(shfi), SHGFI_PIDL | SHGFI_SYSICONINDEX);

		tvItem.mask				= TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		tvItem.hItem			= hNextSibling;
		tvItem.iImage			= shfi.iIcon;
		tvItem.iSelectedImage	= shfi.iIcon;
		TreeView_SetItem(m_hTreeView,&tvItem);

		hChild = TreeView_GetChild(m_hTreeView,hNextSibling);

		if(hChild != NULL)
			RefreshAllIconsInternal(hChild);

		hNextSibling = TreeView_GetNextSibling(m_hTreeView,hNextSibling);
	}
}

HRESULT CMyTreeView::OnBeginDrag(int iItemId,DragTypes_t DragType)
{
	IDataObject			*pDataObject = NULL;
	IDragSourceHelper	*pDragSourceHelper = NULL;
	IShellFolder		*pShellFolder = NULL;
	PCITEMID_CHILD		ridl = NULL;
	DWORD				Effect;
	POINT				pt = {0,0};
	HRESULT				hr;	

	hr = CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_ALL,
		IID_PPV_ARGS(&pDragSourceHelper));

	if(SUCCEEDED(hr))
	{
		hr = SHBindToParent(m_itemInfoMap.at(iItemId).pidl.get(), IID_PPV_ARGS(&pShellFolder), &ridl);

		if(SUCCEEDED(hr))
		{
			/* Needs to be done from the parent folder for the drag/dop to work correctly.
			If done from the desktop folder, only links to files are created. They are
			not copied/moved. */
			GetUIObjectOf(pShellFolder, m_hTreeView, 1, &ridl, IID_PPV_ARGS(&pDataObject));

			pDragSourceHelper->InitializeFromWindow(m_hTreeView,&pt,pDataObject);

			m_DragType = DragType;

			hr = DoDragDrop(pDataObject,this,DROPEFFECT_COPY|DROPEFFECT_MOVE|
				DROPEFFECT_LINK,&Effect);

			m_bDragging = FALSE;

			pDataObject->Release();
			pShellFolder->Release();
		}

		pDragSourceHelper->Release();
	}

	return hr;
}

BOOL CMyTreeView::IsDesktop(const TCHAR *szPath)
{
	TCHAR szDesktop[MAX_PATH];

	SHGetFolderPath(NULL,CSIDL_DESKTOP,NULL,SHGFP_TYPE_CURRENT,szDesktop);

	return (lstrcmp(szPath,szDesktop) == 0);
}

BOOL CMyTreeView::IsDesktopSubChild(const TCHAR *szFullFileName)
{
	TCHAR szDesktop[MAX_PATH];

	SHGetFolderPath(NULL,CSIDL_DESKTOP,NULL,SHGFP_TYPE_CURRENT,szDesktop);

	return (wcsncmp(szFullFileName,szDesktop,lstrlen(szDesktop)) == 0);
}