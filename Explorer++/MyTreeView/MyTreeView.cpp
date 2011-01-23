/******************************************************************
 *
 * Project: MyTreeView
 * File: MyTreeView.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Wraps a treeview control. Specifically handles
 * adding directories to it and selecting directories.
 * Each non-network drive in the system is also
 * monitored for changes.
 *
 * Notes:
 *  - All items are sorted alphabetically, except for:
 *     - Items on the desktop
 *     - Items in My Computer
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "../Helper/Helper.h"
#include "../Helper/Buffer.h"
#include "../Helper/ShellHelper.h"
#include "MyTreeView.h"
#include "MyTreeViewInternal.h"


typedef struct
{
	LPITEMIDLIST	pidlParentNode;
	LPITEMIDLIST	pidlDestination;
} QueuedItem_t;

LRESULT CALLBACK	TreeViewProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
int CALLBACK		CompareItemsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
void CALLBACK		Timer_DirectoryModified(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);
DWORD WINAPI		Thread_SubFoldersStub(LPVOID pVoid);
DWORD WINAPI		Thread_MonitorAllDrives(LPVOID pParam);
void CALLBACK		TVFindIconAPC(ULONG_PTR dwParam);
BOOL				RemoveFromIconFinderQueue(TreeViewInfo_t *pListViewInfo);

DWORD	g_ThreadId;
WNDPROC	OldTreeViewProc;
UINT	DirWatchFlags = FILE_NOTIFY_CHANGE_DIR_NAME;

list<QueuedItem_t> g_ItemList;

CRITICAL_SECTION g_tv_icon_cs;
int g_ntvAPCsRan = 0;
int g_ntvAPCsQueued = 0;
BOOL g_btvIconThreadSleeping = TRUE;

list<TreeViewInfo_t> g_pTreeViewInfoList;

CMyTreeView::CMyTreeView(HWND hTreeView,HWND hParent,IDirectoryMonitor *pDirMon,
HANDLE hIconsThread)
{
	m_hTreeView = hTreeView;
	m_hParent = hParent;
	
	SetWindowSubclass(m_hTreeView,TreeViewProcStub,0,(DWORD_PTR)this);

	InitializeCriticalSection(&m_cs);
	InitializeCriticalSection(&m_csSubFolders);
	InitializeCriticalSection(&g_tv_icon_cs);

	m_hThread = hIconsThread;

	m_pDirMon = pDirMon;


	m_uItemMap = (int *)malloc(DEFAULT_ITEM_ALLOCATION * sizeof(int));
	m_pItemInfo = (ItemInfo_t *)malloc(DEFAULT_ITEM_ALLOCATION * sizeof(ItemInfo_t));

	m_iCurrentItemAllocation = DEFAULT_ITEM_ALLOCATION;

	memset(m_uItemMap, 0, (DEFAULT_ITEM_ALLOCATION * sizeof(int)));

	m_iFolderIcon = GetDefaultFolderIconIndex();

	AddRoot();

	m_bDragging			= FALSE;
	m_bDragCancelled	= FALSE;
	m_bDragAllowed		= FALSE;

	m_bShowHidden		= TRUE;
	m_bHideRecycleBin	= FALSE;
	m_bHideSysVolInfo	= FALSE;

	InitializeDragDropHelpers();

	m_bQueryRemoveCompleted = FALSE;
	CreateThread(NULL,0,Thread_MonitorAllDrives,this,0,NULL);

	m_iProcessing = 0;
}

CMyTreeView::~CMyTreeView()
{

}

LRESULT CALLBACK TreeViewProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,
UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	CMyTreeView *pMyTreeView = (CMyTreeView *)dwRefData;

	return pMyTreeView->TreeViewProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CMyTreeView::TreeViewProc(HWND hwnd,
UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_CREATE:
			break;

		case WM_SETFOCUS:
			SendMessage(m_hParent,WM_USER_TREEVIEW_GAINEDFOCUS,0,0);
			break;

		case WM_TIMER:
			DirectoryAltered();
			break;

		case WM_DEVICECHANGE:
			return OnDeviceChange(wParam,lParam);
			break;

		case WM_SETCURSOR:
			return OnSetCursor();
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

		case WM_NOTIFY:
			return OnNotify(hwnd,msg,wParam,lParam);
			break;
	}

	return DefSubclassProc(hwnd,msg,wParam,lParam);
}

LRESULT CALLBACK CMyTreeView::OnNotify(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	NMHDR *nmhdr;
	nmhdr = (NMHDR *)lParam;

	switch(nmhdr->code)
	{
	case TVN_BEGINDRAG:
		OnBeginDrag((int)((NMTREEVIEW *)lParam)->itemNew.lParam,DRAG_TYPE_LEFTCLICK);
		break;

	case TVN_GETDISPINFO:
		OnGetDisplayInfo(lParam);
		break;
	}

	return 0;
}

LRESULT CMyTreeView::OnSetCursor(void)
{
	/* If the "app starting" cursor needs
	to be shown, return TRUE to prevent the
	OS from automatically resetting the
	cursor; else return FALSE to allow the
	OS to set the default cursor. */
	if(m_iProcessing > 0)
	{
		SetCursor(LoadCursor(NULL,IDC_APPSTARTING));
		return TRUE;
	}

	/* Set the cursor back to the default. */
	SetCursor(LoadCursor(NULL,IDC_ARROW));
	return FALSE;
}

HTREEITEM CMyTreeView::AddRoot(void)
{
	IShellFolder	*pDesktopFolder = NULL;
	LPITEMIDLIST	pidl = NULL;
	TCHAR			szDesktopParsingPath[MAX_PATH];
	TCHAR			szDesktopDisplayName[MAX_PATH];
	SHFILEINFO		shfi;
	HRESULT			hr;
	TVINSERTSTRUCT	tvis;
	TVITEMEX		tvItem;
	HTREEITEM		hDesktop = NULL;
	int				iItemId;

	TreeView_DeleteAllItems(m_hTreeView);

	hr = SHGetFolderLocation(NULL,CSIDL_DESKTOP,NULL,0,&pidl);

	if(SUCCEEDED(hr))
	{
		GetVirtualFolderParsingPath(CSIDL_DESKTOP,szDesktopParsingPath);
		GetDisplayName(szDesktopParsingPath,szDesktopDisplayName,SHGDN_INFOLDER);

		SHGetFileInfo((LPTSTR)pidl,NULL,&shfi,NULL,SHGFI_PIDL|SHGFI_SYSICONINDEX);

		iItemId = GenerateUniqueItemId();
		m_pItemInfo[iItemId].pidl = ILClone(pidl);
		m_uItemMap[iItemId] = 1;

		tvItem.mask				= TVIF_TEXT|TVIF_IMAGE|TVIF_CHILDREN|TVIF_SELECTEDIMAGE|TVIF_PARAM;
		tvItem.pszText			= szDesktopDisplayName;
		tvItem.cchTextMax		= lstrlen(szDesktopDisplayName);
		tvItem.iImage			= shfi.iIcon;
		tvItem.iSelectedImage	= shfi.iIcon;
		tvItem.cChildren		= 1;
		tvItem.lParam			= (LPARAM)iItemId;

		tvis.hParent			= NULL;
		tvis.hInsertAfter		= TVI_LAST;
		tvis.itemex				= tvItem;

		hDesktop = TreeView_InsertItem(m_hTreeView,&tvis);

		if(hDesktop != NULL)
		{
			hr = SHGetDesktopFolder(&pDesktopFolder);

			if(SUCCEEDED(hr))
			{
				AddDirectoryInternal(pDesktopFolder,pidl,hDesktop);

				SendMessage(m_hTreeView,TVM_EXPAND,(WPARAM)TVE_EXPAND,
					(LPARAM)hDesktop);

				pDesktopFolder->Release();
			}
		}

		CoTaskMemFree(pidl);
	}

	return hDesktop;
}

/*
* Adds all the folder objects within a directory to the
* specified treeview control.
*/
HRESULT CMyTreeView::AddDirectory(HTREEITEM hParent,TCHAR *szParsingPath)
{
	LPITEMIDLIST	pidlDirectory = NULL;
	HRESULT			hr;

	hr = GetIdlFromParsingName(szParsingPath,&pidlDirectory);

	if(SUCCEEDED(hr))
	{
		hr = AddDirectory(hParent,pidlDirectory);

		CoTaskMemFree(pidlDirectory);
	}

	return hr;
}

HRESULT CMyTreeView::AddDirectory(HTREEITEM hParent,LPITEMIDLIST pidlDirectory)
{
	IShellFolder	*pDesktopFolder = NULL;
	IShellFolder	*pShellFolder = NULL;
	HRESULT			hr;

	hr = SHGetDesktopFolder(&pDesktopFolder);

	if(SUCCEEDED(hr))
	{
		if(IsNamespaceRoot(pidlDirectory))
		{
			hr = SHGetDesktopFolder(&pShellFolder);
		}
		else
		{
			hr = pDesktopFolder->BindToObject(pidlDirectory,NULL,
			IID_IShellFolder,(LPVOID *)&pShellFolder);
		}

		if(SUCCEEDED(hr))
		{
			AddDirectoryInternal(pShellFolder,pidlDirectory,hParent);

			pShellFolder->Release();
		}

		pDesktopFolder->Release();
	}

	return hr;
}

void CMyTreeView::OnGetDisplayInfo(LPARAM lParam)
{
	NMTVDISPINFO	*pnmv = NULL;
	TVITEM			*ptvItem = NULL;
	NMHDR			*nmhdr = NULL;

	pnmv	= (NMTVDISPINFO *)lParam;
	ptvItem	= &pnmv->item;
	nmhdr	= &pnmv->hdr;

	if((ptvItem->mask & LVIF_IMAGE) == LVIF_IMAGE)
	{
		ptvItem->iImage	= m_iFolderIcon;
		ptvItem->iSelectedImage	= m_iFolderIcon;

		AddToIconFinderQueue(ptvItem);
	}

	ptvItem->mask |= TVIF_DI_SETITEM;
}

void CMyTreeView::AddToIconFinderQueue(TVITEM *plvItem)
{
	EnterCriticalSection(&g_tv_icon_cs);

	TreeViewInfo_t tvi;

	tvi.hTreeView	= m_hTreeView;
	tvi.hItem		= plvItem->hItem;
	tvi.pidlFull	= BuildPath(plvItem->hItem);

	g_pTreeViewInfoList.push_back(tvi);

	if(g_ntvAPCsRan == g_ntvAPCsQueued)
	{
		g_ntvAPCsQueued++;

		QueueUserAPC(TVFindIconAPC,m_hThread,(ULONG_PTR)this);
	}

	LeaveCriticalSection(&g_tv_icon_cs);
}

void CMyTreeView::EmptyIconFinderQueue(void)
{
	EnterCriticalSection(&g_tv_icon_cs);

	list<TreeViewInfo_t>::iterator last;
	list<TreeViewInfo_t>::iterator first;

	last = g_pTreeViewInfoList.end();

	for(first = g_pTreeViewInfoList.begin();first != g_pTreeViewInfoList.end();)
	{
		CoTaskMemFree(first->pidlFull);
		first = g_pTreeViewInfoList.erase(first);
	}

	LeaveCriticalSection(&g_tv_icon_cs);
}

BOOL RemoveFromIconFinderQueue(TreeViewInfo_t *pTreeViewInfo)
{
	BOOL bQueueNotEmpty;

	EnterCriticalSection(&g_tv_icon_cs);

	if(g_pTreeViewInfoList.empty() == TRUE)
	{
		g_btvIconThreadSleeping = TRUE;
		bQueueNotEmpty = FALSE;

		g_ntvAPCsRan++;
	}
	else
	{
		list<TreeViewInfo_t>::iterator itr;

		itr = g_pTreeViewInfoList.end();

		itr--;

		*pTreeViewInfo = *itr;

		g_pTreeViewInfoList.erase(itr);

		bQueueNotEmpty = TRUE;
	}

	LeaveCriticalSection(&g_tv_icon_cs);

	return bQueueNotEmpty;
}

void CALLBACK TVFindIconAPC(ULONG_PTR dwParam)
{
	TreeViewInfo_t	pTreeViewInfo;
	TVITEM			tvItem;
	SHFILEINFO		shfi;
	DWORD_PTR		res = FALSE;
	BOOL			bQueueNotEmpty = TRUE;
	int				iOverlay;

	bQueueNotEmpty = RemoveFromIconFinderQueue(&pTreeViewInfo);

	while(bQueueNotEmpty)
	{
		res = SHGetFileInfo((LPTSTR)pTreeViewInfo.pidlFull,0,&shfi,
			sizeof(SHFILEINFO),SHGFI_PIDL|SHGFI_ICON|SHGFI_OVERLAYINDEX);

		if(res != 0)
		{
			tvItem.mask				= TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
			tvItem.hItem			= pTreeViewInfo.hItem;
			tvItem.iImage			= shfi.iIcon;
			tvItem.iSelectedImage	= shfi.iIcon;

			iOverlay = (shfi.iIcon >> 24);

			if(iOverlay)
			{
				tvItem.mask			|= TVIF_STATE;
				tvItem.state		= INDEXTOOVERLAYMASK(iOverlay);
				tvItem.stateMask	= TVIS_OVERLAYMASK;
			}

			TreeView_SetItem(pTreeViewInfo.hTreeView,&tvItem);

			DestroyIcon(shfi.hIcon);
			CoTaskMemFree(pTreeViewInfo.pidlFull);
		}

		bQueueNotEmpty = RemoveFromIconFinderQueue(&pTreeViewInfo);
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

	GetDisplayName(m_pItemInfo[iItemId1].pidl,szDisplayName1,SHGDN_FORPARSING);
	GetDisplayName(m_pItemInfo[iItemId2].pidl,szDisplayName2,SHGDN_FORPARSING);

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
		if(!SHGetPathFromIDList(m_pItemInfo[iItemId1].pidl,szTemp) &&
			SHGetPathFromIDList(m_pItemInfo[iItemId2].pidl,szTemp))
		{
			return -1;
		}
		else if(SHGetPathFromIDList(m_pItemInfo[iItemId1].pidl,szTemp) &&
			!SHGetPathFromIDList(m_pItemInfo[iItemId2].pidl,szTemp))
		{
			return 1;
		}
		else
		{
			GetDisplayName(m_pItemInfo[iItemId1].pidl,szDisplayName1,SHGDN_INFOLDER);
			GetDisplayName(m_pItemInfo[iItemId2].pidl,szDisplayName2,SHGDN_INFOLDER);

			return StrCmpLogicalW(szDisplayName1,szDisplayName2);
		}
	}
}

void CMyTreeView::AddDirectoryInternal(IShellFolder *pShellFolder,LPITEMIDLIST pidlDirectory,
HTREEITEM hParent)
{
	IEnumIDList		*pEnumIDList = NULL;
	LPITEMIDLIST	pidl = NULL;
	LPITEMIDLIST	rgelt = NULL;
	ThreadInfo_t	*pThreadInfo = NULL;
	SHCONTF			EnumFlags;
	TCHAR			szDirectory[MAX_PATH];
	TCHAR			szDirectory2[MAX_PATH];
	ULONG			uFetched;
	TVINSERTSTRUCT	tvis;
	TVITEMEX		tvItem;
	HRESULT			hr;
	BOOL			bVirtualFolder;
	BOOL			bMyComputer = FALSE;

	bVirtualFolder = !SHGetPathFromIDList(pidlDirectory,szDirectory);

	if(IsNamespaceRoot(pidlDirectory))
	{
		bVirtualFolder = TRUE;
	}

	hr = GetDisplayName(pidlDirectory,szDirectory,SHGDN_FORPARSING);
	hr = GetDisplayName(pidlDirectory,szDirectory2,SHGDN_FORPARSING);

	hr = SHGetFolderLocation(NULL,CSIDL_DRIVES,NULL,0,&pidl);

	if(SUCCEEDED(hr))
	{
		if(CompareIdls(pidlDirectory,pidl))
			bMyComputer = TRUE;

		CoTaskMemFree(pidl);
	}

	SendMessage(m_hTreeView,WM_SETREDRAW,(WPARAM)FALSE,(LPARAM)NULL);

	EnumFlags = SHCONTF_FOLDERS;

	if(m_bShowHidden)
		EnumFlags |= SHCONTF_INCLUDEHIDDEN;

	hr = pShellFolder->EnumObjects(NULL,EnumFlags,&pEnumIDList);

	vector<ItemStore_t> vItems;
	vector<ItemStore_t>::iterator itr;
	ItemStore_t ItemStore;

	if(SUCCEEDED(hr) && pEnumIDList != NULL)
	{
		/* Iterate over the subfolders items, and place them in the tree. */
		uFetched = 1;
		while(pEnumIDList->Next(1,&rgelt,&uFetched) == S_OK && (uFetched == 1))
		{
			ULONG Attributes = SFGAO_FOLDER|SFGAO_FILESYSTEM;

			/* Only retrieve the attributes for this item. */
			hr = pShellFolder->GetAttributesOf(1,(LPCITEMIDLIST *)&rgelt,&Attributes);

			if(SUCCEEDED(hr))
			{
				/* Is the item a folder? (SFGAO_STREAM is set on .zip files, along with
				SFGAO_FOLDER). */
				if((Attributes & SFGAO_FOLDER))
				{
					LPITEMIDLIST	pidlComplete = NULL;
					STRRET			str;
					TCHAR			ItemName[MAX_PATH];
					int				iItemId;

					hr = pShellFolder->GetDisplayNameOf(rgelt,SHGDN_NORMAL,&str);

					if(SUCCEEDED(hr))
					{
						BOOL bSkipItem = FALSE;

						StrRetToBuf(&str,rgelt,ItemName,SIZEOF_ARRAY(ItemName));

						// Hides $RECYCLE.BIN folder
						if(m_bHideRecycleBin)
						{
							if (lstrcmpi(ItemName,_T("$RECYCLE.BIN")) == 0 ||
								lstrcmpi(ItemName,_T("RECYCLER")) == 0 ||
								lstrcmpi(ItemName,_T("RECYCLE BIN")) == 0)
							{
								bSkipItem = TRUE;
							}
						}

						// Hides "System Volume Information" folder
						if(m_bHideSysVolInfo)
						{
							if (lstrcmpi(ItemName,_T("System Volume Information")) == 0)
								bSkipItem = TRUE;
						}

						if (!bSkipItem)
						{ 
							pidlComplete = ILCombine(pidlDirectory,rgelt);

							iItemId = GenerateUniqueItemId();
							m_pItemInfo[iItemId].pidl = ILClone(pidlComplete);
							m_pItemInfo[iItemId].pridl = ILClone(rgelt);

							ItemStore.iItemId = iItemId;
							StringCchCopy(ItemStore.ItemName,SIZEOF_ARRAY(ItemStore.ItemName),ItemName);

							/* If this is a virtual directory, we'll post sort the items,
							otherwise we'll pre-sort. */
							if(bVirtualFolder)
							{
								vItems.push_back(ItemStore);
							}
							else
							{
								itr = vItems.end();

								/* Compare to the last item in the array and work
								backwards. */
								if(vItems.size() > 0)
								{
									itr--;

									while(StrCmpLogicalW(ItemName,itr->ItemName) < 0 && itr != vItems.begin())
									{
										itr--;
									}

									/* itr in this case is the item AFTER
									which the current item should be inserted.
									The only exception to this is when we are
									inserting an item at the start of the list,
									in which case we need to insert BEFORE the
									first item. */
									if(itr != vItems.begin() || StrCmpLogicalW(ItemName,itr->ItemName) > 0)
										itr++;
								}

								vItems.insert(itr,ItemStore);
							}
						}

						CoTaskMemFree(pidlComplete);
					}
				}
			}

			CoTaskMemFree(rgelt);
		}

		pEnumIDList->Release();


		for(itr = vItems.begin();itr != vItems.end();itr++)
		{
			UINT ItemMask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_CHILDREN;

			tvItem.mask				= ItemMask;
			tvItem.pszText			= itr->ItemName;
			tvItem.iImage			= I_IMAGECALLBACK;
			tvItem.iSelectedImage	= I_IMAGECALLBACK;
			tvItem.lParam			= (LPARAM)itr->iItemId;
			tvItem.cChildren		= 1;

			tvis.hInsertAfter		= TVI_LAST;
			tvis.hParent			= hParent;
			tvis.itemex				= tvItem;

			TreeView_InsertItem(m_hTreeView,&tvis);
		}

		if(bVirtualFolder)
		{
			TVSORTCB tvscb;

			tvscb.hParent		= hParent;
			tvscb.lpfnCompare	= CompareItemsStub;
			tvscb.lParam		= (LPARAM)this;

			TreeView_SortChildrenCB(m_hTreeView,&tvscb,0);
		}
	}

	SendMessage(m_hTreeView,WM_SETREDRAW,(WPARAM)TRUE,(LPARAM)NULL);

	pThreadInfo = (ThreadInfo_t *)malloc(sizeof(ThreadInfo_t));

	if(pThreadInfo != NULL)
	{
		pThreadInfo->hTreeView		= m_hTreeView;
		pThreadInfo->pidl			= ILClone(pidlDirectory);
		pThreadInfo->hParent		= hParent;
		pThreadInfo->pMyTreeView	= this;

		CreateThread(NULL,0,Thread_SubFoldersStub,pThreadInfo,0,&g_ThreadId);
	}
}

int CMyTreeView::GenerateUniqueItemId(void)
{
	BOOL	bFound = FALSE;
	int		i = 0;

	for(i = 0;i < m_iCurrentItemAllocation;i++)
	{
		if(m_uItemMap[i] == 0)
		{
			m_uItemMap[i] = 1;
			bFound = TRUE;
			break;
		}
	}

	if(bFound)
		return i;
	else
	{
		int iCurrent;

		m_uItemMap = (int *)realloc(m_uItemMap,(m_iCurrentItemAllocation +
			DEFAULT_ITEM_ALLOCATION) * sizeof(int));
		m_pItemInfo = (ItemInfo_t *)realloc(m_pItemInfo,(m_iCurrentItemAllocation +
			DEFAULT_ITEM_ALLOCATION) * sizeof(ItemInfo_t));

		if(m_uItemMap != NULL && m_pItemInfo != NULL)
		{
			iCurrent = m_iCurrentItemAllocation;

			m_iCurrentItemAllocation += DEFAULT_ITEM_ALLOCATION;

			for(i = iCurrent;i < m_iCurrentItemAllocation;i++)
			{
				m_uItemMap[i] = 0;
			}

			/* Return the first of the new items. */
			m_uItemMap[iCurrent] = 1;
			return iCurrent;
		}
		else
		{
			return -1;
		}
	}
}

DWORD WINAPI Thread_SubFoldersStub(LPVOID pParam)
{
	CMyTreeView		*pMyTreeView = NULL;
	ThreadInfo_t	*pThreadInfo = NULL;

	pThreadInfo = (ThreadInfo_t *)pParam;

	pMyTreeView = pThreadInfo->pMyTreeView;

	CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);

	pMyTreeView->Thread_SubFolders(pParam);

	CoUninitialize();

	return 0;
}

/* Check nearest ancestor that IS in treeview. If this ancestor is been expanded,
queue a callback operation; else expand it.
Once the ancestor has been expanded, the callback will be run. This will then
check whether the ancestor is the direct parent of the item, or if it is
farther up the tree. If it is the direct parent, the item will simply be
selected. If it isn't the direct parent, this procedure will repeat, except that
the nearest ancestor is now the ancestor that was just expanded. */
DWORD WINAPI CMyTreeView::Thread_AddDirectoryInternal(IShellFolder *pShellFolder,
LPITEMIDLIST pidlDirectory,HTREEITEM hParent)
{
	IEnumIDList		*pEnumIDList = NULL;
	LPITEMIDLIST	rgelt = NULL;
	ItemInfo_t		*pItemInfo = NULL;
	SHCONTF			EnumFlags;
	TCHAR			szDirectory[MAX_PATH];
	ULONG			uFetched;
	HTREEITEM		hItem;
	TVINSERTSTRUCT	tvis;
	TVITEMEX		tvItem;
	HRESULT			hr;
	int				iMonitorId = -1;

	hr = GetDisplayName(pidlDirectory,szDirectory,SHGDN_FORPARSING);

	EnumFlags = SHCONTF_FOLDERS;

	if(m_bShowHidden)
		EnumFlags |= SHCONTF_INCLUDEHIDDEN;

	hr = pShellFolder->EnumObjects(NULL,EnumFlags,&pEnumIDList);

	if(SUCCEEDED(hr))
	{
		/* Iterate over the subfolders items, and place them in the tree. */
		uFetched = 1;
		while(pEnumIDList->Next(1,&rgelt,&uFetched) == S_OK && (uFetched == 1))
		{
			SHFILEINFO shfi;
			ULONG Attributes = SFGAO_FOLDER|SFGAO_STREAM|SFGAO_FILESYSTEM;

			/* Only retrieve the attributes for this item. */
			hr = pShellFolder->GetAttributesOf(1,(LPCITEMIDLIST *)&rgelt,&Attributes);

			if(SUCCEEDED(hr))
			{
				/* Is the item a folder? (SFGAO_STREAM is set on .zip files, along with
				SFGAO_FOLDER). */
				if((Attributes & SFGAO_FOLDER) && !(Attributes & SFGAO_STREAM))
				{
					LPITEMIDLIST	pidlComplete = NULL;
					STRRET			str;
					TCHAR			ItemName[MAX_PATH];
					UINT			ItemMask;

					hr = pShellFolder->GetDisplayNameOf(rgelt,SHGDN_NORMAL,&str);

					if(SUCCEEDED(hr))
					{
						StrRetToBuf(&str,rgelt,ItemName,SIZEOF_ARRAY(ItemName));

						pidlComplete = ILCombine(pidlDirectory,rgelt);

						SHGetFileInfo((LPTSTR)pidlComplete,NULL,
						&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_SYSICONINDEX|SHGFI_ATTRIBUTES);

						iMonitorId = -1;

						hr = GetDisplayName(pidlComplete,szDirectory,SHGDN_FORPARSING);

						pItemInfo = (ItemInfo_t *)malloc(sizeof(ItemInfo_t));

						pItemInfo->pidl			= ILCombine(pidlDirectory,rgelt);

						ItemMask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_CHILDREN;

						tvItem.mask				= ItemMask;
						tvItem.pszText			= ItemName;
						tvItem.cchTextMax		= lstrlen(ItemName);
						tvItem.iImage			= shfi.iIcon;
						tvItem.iSelectedImage	= shfi.iIcon;
						tvItem.lParam			= (LPARAM)pItemInfo;
						tvItem.cChildren		= ((shfi.dwAttributes & SFGAO_HASSUBFOLDER) == SFGAO_HASSUBFOLDER) ? 1:0;

						tvis.hInsertAfter		= DetermineItemSortedPosition(hParent,szDirectory);
						tvis.hParent			= hParent;
						tvis.itemex				= tvItem;

						hItem = TreeView_InsertItem(m_hTreeView,&tvis);

						CoTaskMemFree(pidlComplete);
					}
				}
			}

			CoTaskMemFree(rgelt);
		}

		pEnumIDList->Release();
	}

	tvItem.mask		= TVIF_HANDLE|TVIF_PARAM;
	tvItem.hItem	= hParent;
	TreeView_GetItem(m_hTreeView,&tvItem);

	pItemInfo = (ItemInfo_t *)tvItem.lParam;

	TreeView_Expand(m_hTreeView,hParent,TVE_EXPAND);

	return 0;
}

DWORD WINAPI CMyTreeView::Thread_SubFolders(LPVOID pParam)
{
	IShellFolder	*pShellFolder = NULL;
	LPITEMIDLIST	pidl = NULL;
	LPITEMIDLIST	pidlRelative = NULL;
	HTREEITEM		hItem;
	TVITEM			tvItem;
	ThreadInfo_t	*pThreadInfo = NULL;
	ULONG			Attributes;
	HRESULT			hr;
	BOOL			res;

	pThreadInfo = (ThreadInfo_t *)pParam;

	hItem = TreeView_GetChild(pThreadInfo->hTreeView,pThreadInfo->hParent);

	while(hItem != NULL)
	{
		Attributes = SFGAO_HASSUBFOLDER;

		tvItem.mask	= TVIF_PARAM|TVIF_HANDLE;
		tvItem.hItem	= hItem;

		res = TreeView_GetItem(pThreadInfo->hTreeView,&tvItem);

		if(res != FALSE)
		{
			int iItemID;
			BOOL bValid = FALSE;

			iItemID = (int)tvItem.lParam;

			EnterCriticalSection(&m_csSubFolders);

			/* If the item is valid at this point, we'll
			clone it's pidl, and use it to check whether
			the item has any subfolders. */
			if(iItemID < m_iCurrentItemAllocation)
			{
				if(m_uItemMap[iItemID] == 1)
				{
					pidl = ILClone(m_pItemInfo[(int)tvItem.lParam].pidl);

					bValid = TRUE;
				}
			}

			LeaveCriticalSection(&m_csSubFolders);

			if(bValid)
			{
				hr = SHBindToParent(pidl,IID_IShellFolder,(void **)&pShellFolder,(LPCITEMIDLIST *)&pidlRelative);

				if(SUCCEEDED(hr))
				{
					/* Only retrieve the attributes for this item. */
					hr = pShellFolder->GetAttributesOf(1,(LPCITEMIDLIST *)&pidlRelative,&Attributes);

					if(SUCCEEDED(hr))
					{
						if((Attributes & SFGAO_HASSUBFOLDER) != SFGAO_HASSUBFOLDER)
						{
							tvItem.mask			= TVIF_CHILDREN;
							tvItem.cChildren	= 0;
							TreeView_SetItem(pThreadInfo->hTreeView,&tvItem);
						}

						pShellFolder->Release();
					}
				}

				CoTaskMemFree((LPVOID)pidl);
			}
		}

		hItem = TreeView_GetNextSibling(pThreadInfo->hTreeView,hItem);
	}

	CoTaskMemFree(pThreadInfo->pidl);

	free(pThreadInfo);

	return 0;
}

HTREEITEM CMyTreeView::DetermineItemSortedPosition(HTREEITEM hParent,TCHAR *szItem)
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
		ItemInfo_t	*pItemInfo = NULL;
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

			pItemInfo = &m_pItemInfo[(int)Item.lParam];

			GetDisplayName(pItemInfo->pidl,szFullItemPath,SHGDN_FORPARSING);

			Attributes = SFGAO_FILESYSTEM;
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

HTREEITEM CMyTreeView::DetermineDriveSortedPosition(HTREEITEM hParent,TCHAR *szItemName)
{
	HTREEITEM	htItem;
	HTREEITEM	hPreviousItem;
	TVITEMEX	Item;
	ItemInfo_t	*pItemInfo = NULL;
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

		pItemInfo = &m_pItemInfo[(int)Item.lParam];

		GetDisplayName(pItemInfo->pidl,szFullItemPath,SHGDN_FORPARSING);

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

LPITEMIDLIST CMyTreeView::BuildPath(HTREEITEM hTreeItem)
{
	TVITEMEX	Item;
	ItemInfo_t	*pItemInfo = NULL;

	Item.mask			= TVIF_HANDLE|TVIF_PARAM;
	Item.hItem			= hTreeItem;
	TreeView_GetItem(m_hTreeView,&Item);

	pItemInfo = &m_pItemInfo[(int)Item.lParam];

	return ILClone(pItemInfo->pidl);
}

HTREEITEM CMyTreeView::LocateItem(TCHAR *szParsingPath)
{
	LPITEMIDLIST	pidl = NULL;
	HTREEITEM		hItem = NULL;
	HRESULT			hr;

	hr = GetIdlFromParsingName(szParsingPath,&pidl);

	if(SUCCEEDED(hr))
	{
		hItem = LocateItem(pidl);

		CoTaskMemFree(pidl);
	}

	return hItem;
}

HTREEITEM CMyTreeView::LocateItem(LPITEMIDLIST pidlDirectory)
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
HTREEITEM CMyTreeView::LocateDeletedItem(IN TCHAR *szFullFileName)
{
	HTREEITEM hItem = NULL;
	LPITEMIDLIST pidl = NULL;
	TCHAR szParent[MAX_PATH];
	BOOL bFound = FALSE;
	HRESULT hr;

	StringCchCopy(szParent,SIZEOF_ARRAY(szParent),szFullFileName);
	PathRemoveFileSpec(szParent);
	hr = GetIdlFromParsingName(szParent,&pidl);

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

HTREEITEM CMyTreeView::LocateExistingItem(TCHAR *szParsingPath)
{
	LPITEMIDLIST	pidl = NULL;
	HTREEITEM		hItem;
	HRESULT			hr;

	hr = GetIdlFromParsingName(szParsingPath,&pidl);

	if(SUCCEEDED(hr))
	{
		hItem = LocateExistingItem(pidl);

		CoTaskMemFree(pidl);

		return hItem;
	}

	return NULL;
}

HTREEITEM CMyTreeView::LocateExistingItem(LPITEMIDLIST pidlDirectory)
{
	return LocateItemInternal(pidlDirectory,TRUE);
}

HTREEITEM CMyTreeView::LocateItemInternal(LPITEMIDLIST pidlDirectory,BOOL bOnlyLocateExistingItem)
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
		ItemInfo_t *pItemInfo = NULL;

		pItemInfo = &m_pItemInfo[(int)Item.lParam];

		if(CompareIdls((LPCITEMIDLIST)pItemInfo->pidl,pidlDirectory))
		{
			bFound = TRUE;

			break;
		}

		if(ILIsParent((LPCITEMIDLIST)pItemInfo->pidl,pidlDirectory,FALSE))
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

HTREEITEM CMyTreeView::LocateItemByPath(TCHAR *szItemPath,BOOL bExpand)
{
	LPITEMIDLIST	pidlMyComputer	= NULL;
	HTREEITEM		hMyComputer;
	HTREEITEM		hItem;
	HTREEITEM		hNextItem;
	TVITEMEX		Item;
	ItemInfo_t		*pItemInfo = NULL;
	TCHAR			*ptr = NULL;
	TCHAR			ItemText[MAX_PATH];
	TCHAR			FullItemPathCopy[MAX_PATH];
	TCHAR			szItemName[MAX_PATH];
	TCHAR			*next_token = NULL;

	StringCchCopy(FullItemPathCopy,SIZEOF_ARRAY(FullItemPathCopy),
	szItemPath);

	PathRemoveBackslash(FullItemPathCopy);

	SHGetFolderLocation(NULL,CSIDL_DRIVES,NULL,NULL,&pidlMyComputer);

	hMyComputer = LocateItem(pidlMyComputer);

	CoTaskMemFree(pidlMyComputer);

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

	pItemInfo = &m_pItemInfo[(int)Item.lParam];

	GetDisplayName(pItemInfo->pidl,szItemName,SHGDN_FORPARSING);

	while(StrCmpI(ptr,szItemName) != 0)
	{
		hItem = TreeView_GetNextSibling(m_hTreeView,hItem);

		if(hItem == NULL)
			return NULL;

		Item.mask		= TVIF_PARAM;
		Item.hItem		= hItem;
		TreeView_GetItem(m_hTreeView,&Item);

		pItemInfo = &m_pItemInfo[(int)Item.lParam];

		GetDisplayName(pItemInfo->pidl,szItemName,SHGDN_FORPARSING);
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
		Item.cchTextMax	= MAX_PATH;
		Item.hItem		= hItem;
		TreeView_GetItem(m_hTreeView,&Item);

		while(StrCmpI(ptr,ItemText) != 0)
		{
			hItem = TreeView_GetNextSibling(m_hTreeView,hItem);

			if(hItem == NULL)
				return NULL;

			Item.pszText	= ItemText;
			Item.cchTextMax	= MAX_PATH;
			Item.hItem		= hItem;
			TreeView_GetItem(m_hTreeView,&Item);
		}
	}

	return hItem;
}

/* Locate an item which is a Desktop (sub)child, if visible.
   Does not expand any item */
HTREEITEM CMyTreeView::LocateItemOnDesktopTree(TCHAR *szFullFileName)
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
			tvItem.cchTextMax	= MAX_PATH;
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
	TVITEMEX	Item;
	HTREEITEM	hItem;
	ItemInfo_t	*pItemInfo = NULL;

	hItem = TreeView_GetChild(m_hTreeView,hParent);

	while(hItem != NULL)
	{
		Item.mask		= TVIF_PARAM|TVIF_HANDLE|TVIF_CHILDREN;
		Item.hItem		= hItem;
		TreeView_GetItem(m_hTreeView,&Item);

		if(Item.cChildren != 0)
			EraseItems(hItem);

		EnterCriticalSection(&m_csSubFolders);

		pItemInfo = &m_pItemInfo[(int)Item.lParam];

		CoTaskMemFree((LPVOID)pItemInfo->pidl);
		CoTaskMemFree((LPVOID)pItemInfo->pridl);

		/* Free up this items id. */
		m_uItemMap[(int)Item.lParam] = 0;

		LeaveCriticalSection(&m_csSubFolders);

		hItem = TreeView_GetNextSibling(m_hTreeView,hItem);
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
					DriveLetter = GetDriveNameFromMask(pdbv->dbcv_unitmask);
					StringCchPrintf(DriveName,SIZEOF_ARRAY(DriveName),_T("%c:\\"),DriveLetter);

					if(pdbv->dbcv_flags & DBTF_MEDIA)
					{
						hItem = LocateItemByPath(DriveName,FALSE);

						if(hItem != NULL)
						{
							SHGetFileInfo(DriveName,0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);
							GetDisplayName(DriveName,szDisplayName,SHGDN_INFOLDER);

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
				list<DriveEvent_t>::iterator	itr;

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
				list<DriveEvent_t>::iterator	itr;

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
							DriveLetter = GetDriveNameFromMask(pdbv->dbcv_unitmask);
							StringCchPrintf(DriveName,SIZEOF_ARRAY(DriveName),_T("%c:\\"),DriveLetter);

							if(pdbv->dbcv_flags & DBTF_MEDIA)
							{
								hItem = LocateItemByPath(DriveName,FALSE);

								if(hItem != NULL)
								{
									SHGetFileInfo(DriveName,0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);
									GetDisplayName(DriveName,szDisplayName,SHGDN_INFOLDER);

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

void CMyTreeView::MonitorDrivePublic(TCHAR *szDrive)
{
	MonitorDrive(szDrive);
}

void CMyTreeView::MonitorDrive(TCHAR *szDrive)
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

			StringCchCopy(pDirectoryAltered->szPath,MAX_PATH,szDrive);
			pDirectoryAltered->pMyTreeView	= this;

			iMonitorId = m_pDirMon->WatchDirectory(hDrive,szDrive,DirWatchFlags,
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
	IID_IDragSourceHelper,(LPVOID *)&m_pDragSourceHelper);

	if(SUCCEEDED(hr))
	{
		hr = m_pDragSourceHelper->QueryInterface(IID_IDropTargetHelper,(LPVOID *)&m_pDropTargetHelper);

		RegisterDragDrop(m_hTreeView,this);
	}

	return hr;
}

/* IUnknown interface members. */
HRESULT __stdcall CMyTreeView::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

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

void CMyTreeView::SetHideRecycleBin(BOOL bHideRecycleBin)
{
	m_bHideRecycleBin = bHideRecycleBin;
}

void CMyTreeView::SetHideSysVolInfo(BOOL bHideSysVolInfo)
{
	m_bHideSysVolInfo = bHideSysVolInfo;
}

void CMyTreeView::RefreshAllIcons(void)
{
	HTREEITEM	hRoot;
	TVITEMEX	tvItem;
	ItemInfo_t	*pItemInfo = NULL;
	SHFILEINFO	shfi;

	/* Get the root of the tree (root of namespace). */
	hRoot = TreeView_GetRoot(m_hTreeView);

	tvItem.mask				= TVIF_HANDLE|TVIF_PARAM;
	tvItem.hItem			= hRoot;
	TreeView_GetItem(m_hTreeView,&tvItem);

	pItemInfo = &m_pItemInfo[(int)tvItem.lParam];

	SHGetFileInfo((LPCTSTR)pItemInfo->pidl,0,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_SYSICONINDEX);

	tvItem.mask				= TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
	tvItem.hItem			= hRoot;
	tvItem.iImage			= shfi.iIcon;
	tvItem.iSelectedImage	= shfi.iIcon;
	TreeView_SetItem(m_hTreeView,&tvItem);

	RefreshAllIconsInternal(TreeView_GetChild(m_hTreeView,hRoot));
}

void CMyTreeView::RefreshAllIconsInternal(HTREEITEM hFirstSibling)
{
	HTREEITEM	hNextSibling;
	HTREEITEM	hChild;
	TVITEM		tvItem;
	ItemInfo_t	*pItemInfo = NULL;
	SHFILEINFO	shfi;

	hNextSibling = TreeView_GetNextSibling(m_hTreeView,hFirstSibling);

	tvItem.mask				= TVIF_HANDLE|TVIF_PARAM;
	tvItem.hItem			= hFirstSibling;
	TreeView_GetItem(m_hTreeView,&tvItem);

	pItemInfo = &m_pItemInfo[(int)tvItem.lParam];

	SHGetFileInfo((LPCTSTR)pItemInfo->pidl,0,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_SYSICONINDEX);

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

		pItemInfo = &m_pItemInfo[(int)tvItem.lParam];

		SHGetFileInfo((LPCTSTR)pItemInfo->pidl,0,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_SYSICONINDEX);

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
	LPITEMIDLIST		ridl = NULL;
	ItemInfo_t			*pItemInfo = NULL;
	DWORD				Effect;
	POINT				pt = {0,0};
	HRESULT				hr;	

	hr = CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_ALL,
		IID_IDragSourceHelper,(LPVOID *)&pDragSourceHelper);

	if(SUCCEEDED(hr))
	{
		pItemInfo = &m_pItemInfo[iItemId];

		hr = SHBindToParent(pItemInfo->pidl,IID_IShellFolder,
			(LPVOID *)&pShellFolder,(LPCITEMIDLIST *)&ridl);

		if(SUCCEEDED(hr))
		{
			/* Needs to be done from the parent folder for the drag/dop to work correctly.
			If done from the desktop folder, only links to files are created. They are
			not copied/moved. */
			pShellFolder->GetUIObjectOf(m_hTreeView,1,(LPCITEMIDLIST *)&ridl,
				IID_IDataObject,NULL,(LPVOID *)&pDataObject);

			hr = pDragSourceHelper->InitializeFromWindow(m_hTreeView,&pt,pDataObject);

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

BOOL CMyTreeView::IsDesktop(TCHAR *szPath)
{
	TCHAR szDesktop[MAX_PATH];

	SHGetFolderPath(NULL,CSIDL_DESKTOP,NULL,SHGFP_TYPE_CURRENT,szDesktop);

	return (lstrcmp(szPath,szDesktop) == 0);
}

BOOL CMyTreeView::IsDesktopSubChild(TCHAR *szFullFileName)
{
	TCHAR szDesktop[MAX_PATH];

	SHGetFolderPath(NULL,CSIDL_DESKTOP,NULL,SHGFP_TYPE_CURRENT,szDesktop);

	return (wcsncmp(szFullFileName,szDesktop,lstrlen(szDesktop)) == 0);
}