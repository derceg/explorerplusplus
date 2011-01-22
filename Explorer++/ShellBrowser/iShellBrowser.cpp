/******************************************************************
 *
 * Project: ShellBrowser
 * File: iShellBrowser.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Main file for folder browsing support.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "IShellView.h"
#include "iShellBrowser_internal.h"
#include "../Helper/Controls.h"
#include "../Helper/Helper.h"
#include "../Helper/FileOperations.h"
#include "../Helper/FolderSize.h"
#include "../Helper/ShellHelper.h"


using namespace std;

void CALLBACK	TimerProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);
void CALLBACK	FindIconAPC(ULONG_PTR dwParam);
BOOL			RemoveFromIconFinderQueue(ListViewInfo_t *pListViewInfo,HANDLE hStopEvent);
int				GetIconThreadStatus(void);
void			SetIconThreadStatus(void);

list<ListViewInfo_t>	g_pListViewInfoList;
int					g_nItemsInInfoList;
int					g_nInfoListAllocation;
CRITICAL_SECTION	g_icon_cs;
BOOL				g_bIconThreadSleeping;
int					g_nAPCsRan;
int					g_nAPCsQueued;

WIN32_FIND_DATA *CFolderView::QueryItemInfo(int iItem)
{
	LVITEM			lvItem;
	WIN32_FIND_DATA	*pwfd = NULL;
	BOOL			bRes;

	if(iItem != -1)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= iItem;
		lvItem.iSubItem	= 0;
		bRes = ListView_GetItem(m_hListView,&lvItem);

		if(bRes)
			pwfd = &m_pwfdFiles[lvItem.lParam];
	}

	return pwfd;
}

HRESULT CFolderView::GetAttributesOf(TCHAR *Object,UINT *Attributes)
{
	int i = 0;

	while(StrCmp(Object,m_pwfdFiles[i].cFileName) != 0)
	{
		i++;
	}

	if((m_pwfdFiles[i].dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN)
	{
		*Attributes &= SFGAO_GHOSTED;
	}

	return S_OK;
}

HRESULT CFolderView::SetEnumFlags(DWORD pgrfFlags)
{
	m_EnumFlags	= pgrfFlags;

	return S_OK;
}

void CFolderView::UpdateFileSelectionInfo(int iCacheIndex,BOOL Selected)
{
	ULARGE_INTEGER	ulFileSize;
	BOOL			IsFolder;

	IsFolder = (m_pwfdFiles[iCacheIndex].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	== FILE_ATTRIBUTE_DIRECTORY;

	ulFileSize.LowPart = m_pwfdFiles[iCacheIndex].nFileSizeLow;
	ulFileSize.HighPart = m_pwfdFiles[iCacheIndex].nFileSizeHigh;

	if(Selected)
	{
		if(IsFolder)
			m_NumFoldersSelected++;
		else
			m_NumFilesSelected++;

		m_ulFileSelectionSize.QuadPart += ulFileSize.QuadPart;
	}
	else
	{
		if(IsFolder)
			m_NumFoldersSelected--;
		else
			m_NumFilesSelected--;

		m_ulFileSelectionSize.QuadPart -= ulFileSize.QuadPart;
	}
}

BOOL CFolderView::IsFilenameFiltered(TCHAR *FileName)
{
	if(CheckWildcardMatch(m_szFilter,FileName,m_bFilterCaseSensitive))
		return FALSE;

	return TRUE;
}

HRESULT CFolderView::GetStoredName(int index,TCHAR *Buffer,unsigned int BufferSize)
{
	LVITEM			Item;
	unsigned int	iNeededBufferSize;
	BOOL			bRes;

	Item.mask		= LVIF_PARAM;
	Item.iItem		= index;
	Item.iSubItem	= 0;
	bRes = ListView_GetItem(m_hListView,&Item);

	if(!bRes)
	{
		Buffer = NULL;

		return E_FAIL;
	}

	iNeededBufferSize = lstrlen(m_pwfdFiles[(int)Item.lParam].cFileName) + 1;

	if(BufferSize < iNeededBufferSize)
	{
		Buffer = NULL;

		return E_FAIL;
	}

	StringCchCopy(Buffer,BufferSize,m_pwfdFiles[(int)Item.lParam].cFileName);

	return S_OK;
}

int CFolderView::QueryDisplayName(int iItem,UINT BufferSize,TCHAR *Buffer)
{
	LVITEM lvItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	ListView_GetItem(m_hListView,&lvItem);

	StringCchCopy(Buffer,BufferSize,m_pwfdFiles[(int)lvItem.lParam].cFileName);

	return lstrlen(Buffer);
}

void CFolderView::QueryName(int iIndex,TCHAR *FileName)
{
	LVITEM lvItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iIndex;
	lvItem.iSubItem	= 0;
	ListView_GetItem(m_hListView,&lvItem);

	StringCchCopy(FileName,MAX_PATH,m_pwfdFiles[(int)lvItem.lParam].cFileName);
}

HRESULT CFolderView::QueryFullItemName(int iIndex,TCHAR *FullItemPath)
{
	LVITEM			lvItem;
	BOOL			bRes;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iIndex;
	lvItem.iSubItem	= 0;
	bRes = ListView_GetItem(m_hListView,&lvItem);

	if(bRes)
	{
		QueryFullItemNameInternal((int)lvItem.lParam,FullItemPath);

		return S_OK;
	}

	return E_FAIL;
}

void CFolderView::QueryFullItemNameInternal(int iItemInternal,TCHAR *szFullFileName)
{
	LPITEMIDLIST	pidlComplete = NULL;

	pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[iItemInternal].pridl);

	GetDisplayName(pidlComplete,szFullFileName,SHGDN_FORPARSING);

	CoTaskMemFree(pidlComplete);
}

UINT CFolderView::QueryCurrentDirectory(int BufferSize,TCHAR *Buffer)
{
	if(BufferSize < (lstrlen(m_CurDir) + 1))
	{
		Buffer = NULL;
		return lstrlen(m_CurDir);
	}

	StringCchCopy(Buffer,BufferSize,m_CurDir);

	return lstrlen(m_CurDir);
}

LPITEMIDLIST CFolderView::QueryCurrentDirectoryIdl(void)
{
	return ILClone(m_pidlDirectory);
}

HRESULT CFolderView::CreateHistoryPopup(IN HWND hParent,OUT LPITEMIDLIST *pidl,
IN POINT *pt,IN BOOL bBackOrForward)
{
	int iSel;

	iSel = m_pPathManager->CreateHistoryPopupMenu(hParent,pt,bBackOrForward);

	if(iSel == 0)
		return E_FAIL;

	if(bBackOrForward)
		iSel = -iSel;

	*pidl = m_pPathManager->RetrievePath(iSel);

	return S_OK;
}

BOOL CFolderView::IsBackHistory(void)
{
	int nBackPathsStored;

	nBackPathsStored = m_pPathManager->GetNumBackPathsStored();

	if(nBackPathsStored == 0)
		return FALSE;

	return TRUE;
}

BOOL CFolderView::IsForwardHistory(void)
{
	int nForwardPathsStored;

	nForwardPathsStored = m_pPathManager->GetNumForwardPathsStored();

	if(nForwardPathsStored == 0)
		return FALSE;

	return TRUE;
}

void CFolderView::GetBackHistory(list<LPITEMIDLIST> *lHistory)
{
	return m_pPathManager->GetBackHistory(lHistory);
}

void CFolderView::GetForwardHistory(list<LPITEMIDLIST> *lHistory)
{
	return m_pPathManager->GetForwardHistory(lHistory);
}

LPITEMIDLIST CFolderView::RetrieveHistoryItemWithoutUpdate(int iItem)
{
	return m_pPathManager->RetrievePathWithoutUpdate(iItem);
}

LPITEMIDLIST CFolderView::RetrieveHistoryItem(int iItem)
{
	return m_pPathManager->RetrievePath(iItem);
}

BOOL CFolderView::CanBrowseUp(void)
{
	return !IsNamespaceRoot(m_pidlDirectory);
}

HRESULT CFolderView::CreateViewObject(HWND hOwner,REFIID iid,void **ppv)
{
	return S_OK;
}

BOOL CFolderView::IsSortAscending(void)
{
	return m_bSortAscending;
}

/* TODO: Convert to using pidl's here, rather than
file names. */
int CFolderView::SelectFiles(TCHAR *FileNamePattern)
{
	int iItem;
	
	iItem = LocateFileItemIndex(FileNamePattern);

	if(iItem != -1)
	{
		ListView_FocusItem(m_hListView,iItem,TRUE);
		ListView_SelectItem(m_hListView,iItem,TRUE);
		ListView_EnsureVisible(m_hListView,iItem,FALSE);
		return 1;
	}

	return 0;
}

int CFolderView::LocateFileItemIndex(const TCHAR *szFileName)
{
	LV_FINDINFO	lvFind;
	int			iItem;
	int			iInternalIndex;

	iInternalIndex = LocateFileItemInternalIndex(szFileName);

	if(iInternalIndex != -1)
	{
		lvFind.flags	= LVFI_PARAM;
		lvFind.lParam	= iInternalIndex;
		iItem			= ListView_FindItem(m_hListView,-1,&lvFind);

		return iItem;
	}

	return -1;
}

int CFolderView::LocateFileItemInternalIndex(const TCHAR *szFileName)
{
	LVITEM	lvItem;
	int		i = 0;

	for(i = 0;i < m_nTotalItems;i++)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		ListView_GetItem(m_hListView,&lvItem);

		if((lstrcmp(m_pwfdFiles[(int)lvItem.lParam].cFileName,szFileName) == 0) ||
			(lstrcmp(m_pwfdFiles[(int)lvItem.lParam].cAlternateFileName,szFileName) == 0))
		{
			return (int)lvItem.lParam;
			break;
		}
	}

	return -1;
}

DWORD CFolderView::QueryFileAttributes(int iItem)
{
	LVITEM lvItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	ListView_GetItem(m_hListView,&lvItem);

	return m_pwfdFiles[(int)lvItem.lParam].dwFileAttributes;
}

WIN32_FIND_DATA *CFolderView::QueryFileFindData(int iItem)
{
	LVITEM lvItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	ListView_GetItem(m_hListView,&lvItem);

	return &m_pwfdFiles[(int)lvItem.lParam];
}

void CFolderView::DragStarted(int iFirstItem,POINT *ptCursor)
{
	DraggedFile_t	df;
	int				iSelected = -1;

	if(iFirstItem != -1)
	{
		POINT	ptOrigin;
		POINT	ptItem;

		ListView_GetItemPosition(m_hListView,iFirstItem,&ptItem);

		ListView_GetOrigin(m_hListView,&ptOrigin);

		m_ptDraggedOffset.x = ptOrigin.x + ptCursor->x - ptItem.x;
		m_ptDraggedOffset.y = ptOrigin.y + ptCursor->y - ptItem.y;
	}

	while((iSelected = ListView_GetNextItem(m_hListView,iSelected,LVNI_SELECTED)) != -1)
	{
		QueryDisplayName(iSelected,MAX_PATH,df.szFileName);

		m_DraggedFilesList.push_back(df);
	}

	m_bDragging = TRUE;
}

void CFolderView::DragStopped(void)
{
	m_DraggedFilesList.clear();

	m_bDragging = FALSE;
}

void CFolderView::OnListViewGetDisplayInfo(LPARAM lParam)
{
	NMLVDISPINFO	*pnmv = NULL;
	LVITEM			*plvItem = NULL;
	NMHDR			*nmhdr = NULL;

	pnmv	= (NMLVDISPINFO *)lParam;
	plvItem	= &pnmv->item;
	nmhdr	= &pnmv->hdr;

	/* Construct an image here using the items
	acual icon. This image will be shown initially.
	If the item also has a thumbnail image, this
	will be found later, and will overwrite any
	image settings made here.
	Note that the initial icon image MUST be drawn
	first, or else it may be possible for the
	thumbnail to be drawn before the initial
	image. */
	if(m_ViewMode == VM_THUMBNAILS)
	{
		plvItem->iImage = GetIconThumbnail((int)plvItem->lParam);
		plvItem->mask |= LVIF_DI_SETITEM;

		/* Finally, add this item to the thumbnail queue... */
		if(!m_bNotifiedOfTermination)
			AddToThumbnailFinderQueue(plvItem->lParam);

		return;
	}

	if((plvItem->mask & LVIF_IMAGE) == LVIF_IMAGE)
	{
		if((m_pwfdFiles[plvItem->lParam].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
		{
			/* File. */
			plvItem->iImage	= m_iFileIcon;
		}
		else
		{
			/* Folder. */
			plvItem->iImage	= m_iFolderIcon;
		}

		if(!m_bNotifiedOfTermination)
			AddToIconFinderQueue(plvItem);
	}

	plvItem->mask |= LVIF_DI_SETITEM;
}

void CFolderView::AddToIconFinderQueue(LVITEM *plvItem)
{
	EnterCriticalSection(&g_icon_cs);

	ListViewInfo_t lvil;

	lvil.hListView	= m_hListView;
	lvil.iItem		= plvItem->iItem;
	lvil.pidlFull	= ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)plvItem->lParam].pridl);
	lvil.hEvent		= m_hIconEvent;

	g_pListViewInfoList.push_back(lvil);

	/* This code CANNOT run at the same time as the
	queue remover code. Therefore, if this runs before
	the remover code is called, the number of APCs
	queued/ran will mismatch, and no APC will be queued.
	If it is run after the remover code is called, the
	number of APCs queued/ran will be the same. At this
	point, there is no way to stop the currently running
	APC from exiting. A new APC will thus be queued (and
	MUST be queued).
	The awake/asleep status of the thread therefore does
	NOT matter. */
	if(g_nAPCsRan == g_nAPCsQueued)
	{
		g_nAPCsQueued++;

		QueueUserAPC(FindIconAPC,m_hThread,(ULONG_PTR)this);
	}

	LeaveCriticalSection(&g_icon_cs);
}

void CFolderView::EmptyIconFinderQueue(void)
{
	EnterCriticalSection(&g_icon_cs);

	list<ListViewInfo_t>::iterator last;
	list<ListViewInfo_t>::iterator first;

	last = g_pListViewInfoList.end();

	for(first = g_pListViewInfoList.begin();first != g_pListViewInfoList.end();)
	{
		if(first->hListView == m_hListView)
		{
			CoTaskMemFree(first->pidlFull);
			first = g_pListViewInfoList.erase(first);
		}
		else
		{
			++first;
		}
	}

	LeaveCriticalSection(&g_icon_cs);
}

BOOL RemoveFromIconFinderQueue(ListViewInfo_t *pListViewInfo,HANDLE hStopEvent)
{
	BOOL bQueueNotEmpty;

	EnterCriticalSection(&g_icon_cs);

	if(hStopEvent != NULL)
	{
		/* Set the event into the signaled
		state. */
		SetEvent(hStopEvent);
	}

	if(g_pListViewInfoList.empty() == TRUE)
	{
		g_bIconThreadSleeping = TRUE;
		bQueueNotEmpty = FALSE;

		g_nAPCsRan++;
	}
	else
	{
		list<ListViewInfo_t>::iterator itr;

		itr = g_pListViewInfoList.end();

		itr--;

		*pListViewInfo = *itr;

		/* Set the event to the non-signaled
		state. */
		ResetEvent(pListViewInfo->hEvent);

		g_pListViewInfoList.erase(itr);

		bQueueNotEmpty = TRUE;
	}

	LeaveCriticalSection(&g_icon_cs);

	return bQueueNotEmpty;
}

int GetIconThreadStatus(void)
{
	return g_bIconThreadSleeping;
}

void SetIconThreadStatus(void)
{
	EnterCriticalSection(&g_icon_cs);

	g_bIconThreadSleeping = FALSE;

	LeaveCriticalSection(&g_icon_cs);
}

/* What happens when a FolderView is destroyed in
the middle of this procedure:
As this code only relies on the iItem, pidlFull
and hListView members of the ListViewInfo_t
structure, there is no inherent danger in deleting
a FolderView in the middle of this code.
Since the ListViewInfo_t structure in use CANNOT be
freed or destroyed outside of this code, there is
no danger in the pidlFull member becoming corrupted
whilst this code is running.
At worst, the iItem, pidlFull and hListView members
will become invalid when the FolderView object is
destroyed, however the Win32 API will reject the
call to ListView_SetItem if the ListView argument
is invalid, and so this is not expected to be a
problem. */
void CALLBACK FindIconAPC(ULONG_PTR dwParam)
{
	ListViewInfo_t	pListViewInfo;
	LVITEM			lvItem;
	SHFILEINFO		shfi;
	DWORD_PTR		res = FALSE;
	BOOL			bQueueNotEmpty = TRUE;

	SetIconThreadStatus();

	bQueueNotEmpty = RemoveFromIconFinderQueue(&pListViewInfo,NULL);

	while(bQueueNotEmpty)
	{
		/* This call may cause this thread to sleep. If another APC has been
		queued, it will be allowed through once this call has been made.
		Running another APC within the middle of this one will crash the program.
		Note: MUST use SHGFI_ICON here, rather than SHGFO_SYSICONINDEX, or else
		icon overlays won't be applied. */
		res = SHGetFileInfo((LPTSTR)pListViewInfo.pidlFull,0,&shfi,
			sizeof(SHFILEINFO),SHGFI_PIDL|SHGFI_ICON|SHGFI_OVERLAYINDEX);

		if(res != 0)
		{
			lvItem.mask			= LVIF_IMAGE|LVIF_STATE;
			lvItem.iItem		= pListViewInfo.iItem;
			lvItem.iSubItem		= 0;
			lvItem.iImage		= shfi.iIcon;
			lvItem.stateMask	= LVIS_OVERLAYMASK;

			/* Any icon overlay will be contained in the upper eight
			bits of shfi.iIcon. */
			lvItem.state		= INDEXTOOVERLAYMASK(shfi.iIcon >> 24);

			ListView_SetItem(pListViewInfo.hListView,&lvItem);

			DestroyIcon(shfi.hIcon);
			CoTaskMemFree(pListViewInfo.pidlFull);
		}

		bQueueNotEmpty = RemoveFromIconFinderQueue(&pListViewInfo,pListViewInfo.hEvent);
	}
}

LPITEMIDLIST CFolderView::QueryItemRelativeIdl(int iItem)
{
	LVITEM lvItem;
	BOOL bRet;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	bRet = ListView_GetItem(m_hListView,&lvItem);

	if(bRet)
		return ILClone((ITEMIDLIST *)m_pExtraItemInfo[(int)lvItem.lParam].pridl);

	return NULL;
}

BOOL CFolderView::InVirtualFolder(void)
{
	return m_bVirtualFolder;
}

BOOL CFolderView::CanDeleteItem(int iItem)
{
	IShellFolder	*pDesktopFolder = NULL;
	LPITEMIDLIST	pidlFull = NULL;
	LVITEM			lvItem;
	ULONG			uAttributes = SFGAO_CANDELETE;
	HRESULT			hr;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	ListView_GetItem(m_hListView,&lvItem);

	hr = SHGetDesktopFolder(&pDesktopFolder);

	if(SUCCEEDED(hr))
	{
		pidlFull = ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)lvItem.lParam].pridl);

		pDesktopFolder->GetAttributesOf(1,(LPCITEMIDLIST *)&pidlFull,&uAttributes);

		pDesktopFolder->Release();
		CoTaskMemFree(pidlFull);
	}

	return ((uAttributes & SFGAO_CANDELETE) == SFGAO_CANDELETE);
}

/* We can create files in this folder if it is
part of the filesystem, or if it is the root of
the namespace (i.e. the desktop). */
BOOL CFolderView::CanCreate(void)
{
	LPITEMIDLIST	pidl = NULL;
	HRESULT			hr;
	BOOL			bCanCreate = FALSE;

	hr = SHGetFolderLocation(NULL,CSIDL_DESKTOP,NULL,0,&pidl);

	if(SUCCEEDED(hr))
	{
		bCanCreate = !InVirtualFolder() || CompareIdls(m_pidlDirectory,pidl);

		CoTaskMemFree(pidl);
	}

	return bCanCreate;
}

void CFolderView::SetDirMonitorId(int iDirMonitorId)
{
	m_iDirMonitorId = iDirMonitorId;
}

void CFolderView::SetParentDirMointorId(int iParentDirMonitorId)
{
	m_iParentDirMonitorId = iParentDirMonitorId;
}

int CFolderView::GetDirMonitorId(void)
{
	return m_iDirMonitorId;
}

int CFolderView::GetParentDirMointorId(void)
{
	return m_iParentDirMonitorId;
}

HRESULT CFolderView::RetrieveItemInfoTip(int iItem,TCHAR *szInfoTip,size_t cchMax)
{
	LVITEM	lvItem;
	BOOL	bItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	bItem = ListView_GetItem(m_hListView,&lvItem);

	if(bItem)
	{
		return GetFileInfoTip(m_hOwner,m_pidlDirectory,
			const_cast<LPCITEMIDLIST *>(&m_pExtraItemInfo[(int)lvItem.lParam].pridl),
			szInfoTip,(UINT)cchMax);
	}
	else
	{
		return E_FAIL;
	}
}

BOOL CFolderView::CompareVirtualFolders(UINT uFolderCSIDL)
{
	TCHAR	szParsingPath[MAX_PATH];

	GetVirtualFolderParsingPath(uFolderCSIDL,szParsingPath);

	if(StrCmp(m_CurDir,szParsingPath) == 0)
		return TRUE;

	return FALSE;
}

int CFolderView::GenerateUniqueItemId(void)
{
	BOOL	bFound = FALSE;
	int		i = 0;

	/* Cache the last position and start searching
	from there next time. If no free spot is found,
	search from 0 up to the cached spot. */

	for(i = m_iCachedPosition;i < m_iCurrentAllocation;i++)
	{
		if(m_pItemMap[i] == 0)
		{
			m_pItemMap[i] = 1;
			bFound = TRUE;
			break;
		}
	}

	if(!bFound)
	{
		for(i = 0;i < m_iCachedPosition;i++)
		{
			if(m_pItemMap[i] == 0)
			{
				m_pItemMap[i] = 1;
				bFound = TRUE;
				break;
			}
		}
	}

	if(bFound)
	{
		m_iCachedPosition = i;
		return i;
	}
	else
	{
		m_iCachedPosition = 0;
		return -1;
	}
}

void CFolderView::PositionDroppedItems(void)
{
	list<DroppedFile_t>::iterator	itr;
	BOOL							bDropItemSet = FALSE;
	int								iItem;

	/* LVNI_TOLEFT and LVNI_TORIGHT cause exceptions
	in details view. */
	if(m_ViewMode == VM_DETAILS)
	{
		m_DroppedFileNameList.clear();
		return;
	}

	if(!m_DroppedFileNameList.empty())
	{
		/* The auto arrange style must be off for the items
		to be moved. Therefore, if the style is on, turn it
		off, move the items, and the turn it back on. */
		if(m_bAutoArrange)
			ListView_SetAutoArrange(m_hListView,FALSE);

		for(itr = m_DroppedFileNameList.begin();itr != m_DroppedFileNameList.end();)
		{
			iItem = LocateFileItemIndex(itr->szFileName);

			if(iItem != -1)
			{
				if(!bDropItemSet)
					m_iDropped = iItem;

				if(m_bAutoArrange)
				{
					/* TODO: Merge this code with RepositionLocalFiles(). */
					LVFINDINFO lvfi;
					LVHITTESTINFO lvhti;
					RECT rcItem;
					POINT ptOrigin;
					POINT pt;
					POINT ptNext;
					BOOL bRowEnd = FALSE;
					BOOL bRowStart = FALSE;
					int iNext;
					int iHitItem;
					int nItems;

					pt = itr->DropPoint;

					ListView_GetOrigin(m_hListView,&ptOrigin);
					pt.x -= ptOrigin.x;
					pt.y -= ptOrigin.y;

					lvhti.pt = pt;
					iHitItem = ListView_HitTest(m_hListView,&lvhti);

					/* Based on ListView_HandleInsertionMark() code. */
					if(iHitItem != -1 && lvhti.flags & LVHT_ONITEM)
					{
						ListView_GetItemRect(m_hListView,lvhti.iItem,&rcItem,LVIR_BOUNDS);

						if((pt.x - rcItem.left) >
							((rcItem.right - rcItem.left)/2))
						{
							iNext = iHitItem;
						}
						else
						{
							/* Can just insert the item _after_ the item to the
							left, unless this is the start of a row. */
							iNext = ListView_GetNextItem(m_hListView,iHitItem,LVNI_TOLEFT);

							if(iNext == -1)
								iNext = iHitItem;

							bRowStart = (ListView_GetNextItem(m_hListView,iNext,LVNI_TOLEFT) == -1);
						}
					}
					else
					{
						lvfi.flags			= LVFI_NEARESTXY;
						lvfi.pt				= pt;
						lvfi.vkDirection	= VK_UP;
						iNext = ListView_FindItem(m_hListView,-1,&lvfi);

						if(iNext == -1)
						{
							lvfi.flags			= LVFI_NEARESTXY;
							lvfi.pt				= pt;
							lvfi.vkDirection	= VK_LEFT;
							iNext = ListView_FindItem(m_hListView,-1,&lvfi);
						}

						ListView_GetItemRect(m_hListView,iNext,&rcItem,LVIR_BOUNDS);

						if(pt.x > rcItem.left +
							((rcItem.right - rcItem.left)/2))
						{
							if(pt.y > rcItem.bottom)
							{
								int iBelow;

								iBelow = ListView_GetNextItem(m_hListView,iNext,LVNI_BELOW);

								if(iBelow != -1)
									iNext = iBelow;
							}

							bRowEnd = TRUE;
						}

						nItems = ListView_GetItemCount(m_hListView);

						ListView_GetItemRect(m_hListView,nItems - 1,&rcItem,LVIR_BOUNDS);

						if((pt.x > rcItem.left + ((rcItem.right - rcItem.left)/2)) &&
							pt.x < rcItem.right + ((rcItem.right - rcItem.left)/2) + 2 &&
							pt.y > rcItem.top)
						{
							iNext = nItems - 1;

							bRowEnd = TRUE;
						}

						if(!bRowEnd)
						{
							int iLeft;

							iLeft = ListView_GetNextItem(m_hListView,iNext,LVNI_TOLEFT);

							if(iLeft != -1)
								iNext = iLeft;
							else
								bRowStart = TRUE;
						}
					}

					ListView_GetItemPosition(m_hListView,iNext,&ptNext);

					/* Offset by 1 pixel in the x-direction. This ensures that
					the dropped item will always be placed AFTER iNext. */
					if(bRowStart)
					{
						/* If at the start of a row, simply place at x = 0
						so that dropped item will be placed before first
						item... */
						ListView_SetItemPosition32(m_hListView,
							iItem,0,ptNext.y);
					}
					else
					{
						ListView_SetItemPosition32(m_hListView,
							iItem,ptNext.x + 1,ptNext.y);
					}
				}
				else
				{
					ListView_SetItemPosition32(m_hListView,iItem,itr->DropPoint.x,itr->DropPoint.y);
				}

				ListView_SelectItem(m_hListView,iItem,TRUE);
				ListView_FocusItem(m_hListView,iItem,TRUE);

				itr = m_DroppedFileNameList.erase(itr);
			}
			else
			{
				++itr;
			}
		}

		if(m_bAutoArrange)
			ListView_SetAutoArrange(m_hListView,TRUE);
	}
}

int CFolderView::DetermineItemSortedPosition(LPARAM lParam)
{
	LVITEM	lvItem;
	BOOL	bItem;
	int		res = 1;
	int		nItems = 0;
	int		i = 0;

	nItems = ListView_GetItemCount(m_hListView);

	while(res > 0 && i < nItems)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		bItem = ListView_GetItem(m_hListView,&lvItem);

		if(bItem)
			res = SortItemsRelative(lParam,lvItem.lParam);
		else
			res = 0;

		i++;
	}

	/* The item will always be inserted BEFORE
	the item at position i that we specify here.
	For example, specifying 0, will place the item
	at 0 (and push 0 to 1), specifying 1 will place
	the item at 1 (and push 1 to 2).
	Therefore, to place in the last position, need
	to place AFTER last item. */
	if((i - 1) == nItems - 1 && res > 0)
	{
		i++;
	}

	return i - 1;
}

int CFolderView::SortItemsRelative(LPARAM lParam1,LPARAM lParam2)
{
	int iSort;

	iSort = DetermineRelativeItemPositions(lParam1,lParam2);

	if(iSort == 0)
	{
		iSort = StrCmpLogicalW(m_pExtraItemInfo[lParam1].szDisplayName,
			m_pExtraItemInfo[lParam2].szDisplayName);
	}

	return iSort;
}

int CFolderView::DetermineRelativeItemPositions(LPARAM lParam1,LPARAM lParam2)
{
	switch(m_SortMode)
	{
	case FSM_NAME:
		return SortByNameStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_TYPE:
		return SortByTypeStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_SIZE:
		return SortBySizeStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_DATEMODIFIED:
		return SortByDateModifiedStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_TOTALSIZE:
		return SortByTotalSizeStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_FREESPACE:
		return SortByFreeSpaceStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_DATEDELETED:
		return SortByDateDeletedStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_ORIGINALLOCATION:
		return SortByOriginalLocationStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_ATTRIBUTES:
		return SortByAttributesStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_REALSIZE:
		return SortByRealSizeStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_SHORTNAME:
		return SortByShortNameStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_OWNER:
		return SortByOwnerStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_PRODUCTNAME:
		return SortByProductNameStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_COMPANY:
		return SortByCompanyStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_DESCRIPTION:
		return SortByDescriptionStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_FILEVERSION:
		return SortByFileVersionStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_PRODUCTVERSION:
		return SortByProductVersionStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_SHORTCUTTO:
		return SortByShortcutToStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_HARDLINKS:
		return SortByHardlinksStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_EXTENSION:
		return SortByExtensionStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_CREATED:
		return SortByDateCreatedStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_ACCESSED:
		return SortByDateAccessedStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_TITLE:
		return SortByTitleStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_SUBJECT:
		return SortBySubjectStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_AUTHOR:
		return SortByAuthorStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_KEYWORDS:
		return SortByKeywordsStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_COMMENTS:
		return SortByCommentsStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_CAMERAMODEL:
		return SortByCameraModelStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_DATETAKEN:
		return SortByDateTakenStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_WIDTH:
		return SortByWidthStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_HEIGHT:
		return SortByHeightStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_VIRTUALCOMMENTS:
		return SortByVirtualCommentsStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_FILESYSTEM:
		return SortByFileSystemStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_VIRTUALTYPE:
		return SortByVirtualTypeStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_NUMPRINTERDOCUMENTS:
		return SortByNumPrinterDocumentsStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_PRINTERSTATUS:
		return SortByPrinterStatusStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_PRINTERCOMMENTS:
		return SortByPrinterCommentsStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_PRINTERLOCATION:
		return SortByPrinterLocationStub(lParam1,lParam2,(LPARAM)this);
		break;

	case FSM_NETWORKADAPTER_STATUS:
		return SortByNetworkAdapterStatusStub(lParam1,lParam2,(LPARAM)this);
		break;
	}

	return 0;
}

BOOL CFolderView::IsFileReal(int iItem)
{
	LVITEM	lvItem;
	BOOL	bRes;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	bRes = ListView_GetItem(m_hListView,&lvItem);

	if(bRes)
		return m_pExtraItemInfo[(int)lvItem.lParam].bReal;

	return FALSE;
}

BOOL CFolderView::GhostItem(int iItem)
{
	return GhostItemInternal(iItem,TRUE);
}

BOOL CFolderView::DeghostItem(int iItem)
{
	return GhostItemInternal(iItem,FALSE);
}

BOOL CFolderView::GhostItemInternal(int iItem,BOOL bGhost)
{
	LVITEM	lvItem;
	BOOL	bRet;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	bRet = ListView_GetItem(m_hListView,&lvItem);

	if(bRet)
	{
		/* If the file is hidden, prevent changes to its visibility state (i.e.
		hidden items will ALWAYS be ghosted). */
		if(m_pwfdFiles[(int)lvItem.lParam].dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			return FALSE;

		if(bGhost)
		{
			ListView_SetItemState(m_hListView,iItem,LVIS_CUT,LVIS_CUT);
		}
		else
		{
			ListView_SetItemState(m_hListView,iItem,0,LVIS_CUT);
		}
	}

	return TRUE;
}

void CFolderView::RemoveFilteredItems(void)
{
	LVITEM			lvItem;
	int				nItems;
	int				i = 0;

	if(!m_bApplyFilter)
		return;

	nItems = ListView_GetItemCount(m_hListView);

	for(i = nItems - 1;i >= 0;i--)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		ListView_GetItem(m_hListView,&lvItem);

		if(!((m_pwfdFiles[(int)lvItem.lParam].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
		{
			if(IsFilenameFiltered(m_pExtraItemInfo[(int)lvItem.lParam].szDisplayName))
			{
				RemoveFilteredItem(i,(int)lvItem.lParam);
			}
		}
	}

	SendMessage(m_hOwner,WM_USER_UPDATEWINDOWS,0,0);
}

void CFolderView::RemoveFilteredItem(int iItem,int iItemInternal)
{
	ULARGE_INTEGER	ulFileSize;

	if(ListView_GetItemState(m_hListView,iItem,LVIS_SELECTED)
		== LVIS_SELECTED)
	{
		ulFileSize.LowPart = m_pwfdFiles[iItemInternal].nFileSizeLow;
		ulFileSize.HighPart = m_pwfdFiles[iItemInternal].nFileSizeHigh;

		m_ulFileSelectionSize.QuadPart -= ulFileSize.QuadPart;
	}

	/* Take the file size of the removed file away from the total
	directory size. */
	ulFileSize.LowPart = m_pwfdFiles[iItemInternal].nFileSizeLow;
	ulFileSize.HighPart = m_pwfdFiles[iItemInternal].nFileSizeHigh;

	m_ulTotalDirSize.QuadPart -= ulFileSize.QuadPart;

	/* Remove the item from the m_hListView. */
	ListView_DeleteItem(m_hListView,iItem);

	m_nTotalItems--;

	m_FilteredItemsList.push_back(iItemInternal);
}

int CFolderView::QueryNumItems(void)
{
	return m_nTotalItems;
}

int CFolderView::QueryNumSelectedFiles(void)
{
	return m_NumFilesSelected;
}

int CFolderView::QueryNumSelectedFolders(void)
{
	return m_NumFoldersSelected;
}

int CFolderView::QueryNumSelected(void)
{
	return m_NumFilesSelected + m_NumFoldersSelected;
}

void CFolderView::QueryFolderInfo(FolderInfo_t *pFolderInfo)
{
	pFolderInfo->TotalFolderSize.QuadPart		= m_ulTotalDirSize.QuadPart;
	pFolderInfo->TotalSelectionSize.QuadPart	= m_ulFileSelectionSize.QuadPart;
}

void CFolderView::DetermineFolderVirtual(LPITEMIDLIST pidlDirectory)
{
	TCHAR szParsingPath[MAX_PATH];

	m_bVirtualFolder = !SHGetPathFromIDList(pidlDirectory,szParsingPath);

	if(m_bVirtualFolder)
	{
		/* Mark the recycle bin and desktop as
		real folders. Shouldn't be able to create
		folders in Recycle Bin. */
		if(CompareVirtualFolders(CSIDL_BITBUCKET))
			m_bVirtualFolder = TRUE;
		else if(CompareVirtualFolders(CSIDL_DESKTOP))
			m_bVirtualFolder = FALSE;
	}
}

void CFolderView::SetFilter(TCHAR *szFilter)
{
	StringCchCopy(m_szFilter,SIZEOF_ARRAY(m_szFilter),szFilter);

	if(m_bApplyFilter)
	{
		UnfilterAllItems();
		UpdateFiltering();
	}
}

void CFolderView::GetFilter(TCHAR *szFilter,int cchMax)
{
	StringCchCopy(szFilter,cchMax,m_szFilter);
}

void CFolderView::SetFilterStatus(BOOL bFilter)
{
	m_bApplyFilter = bFilter;

	UpdateFiltering();
}

BOOL CFolderView::GetFilterStatus(void)
{
	return m_bApplyFilter;
}

void CFolderView::SetFilterCaseSensitive(BOOL bFilterCaseSensitive)
{
	m_bFilterCaseSensitive = bFilterCaseSensitive;
}

BOOL CFolderView::GetFilterCaseSensitive(void)
{
	return m_bFilterCaseSensitive;
}

void CFolderView::UpdateFiltering(void)
{
	if(m_bApplyFilter)
	{
		RemoveFilteredItems();

		SendMessage(m_hOwner,WM_USER_FILTERINGAPPLIED,m_ID,TRUE);
	}
	else
	{
		UnfilterAllItems();

		if(m_nTotalItems == 0)
			SendMessage(m_hOwner,WM_USER_FOLDEREMPTY,m_ID,TRUE);
		else
			SendMessage(m_hOwner,WM_USER_FILTERINGAPPLIED,m_ID,FALSE);
	}
}

void CFolderView::UnfilterAllItems(void)
{
	list<int>::iterator	itr;
	AwaitingAdd_t		AwaitingAdd;

	for(itr = m_FilteredItemsList.begin();itr != m_FilteredItemsList.end();itr++)
	{
		int iSorted = DetermineItemSortedPosition(*itr);

		AwaitingAdd.iItem			= iSorted;
		AwaitingAdd.bPosition		= TRUE;
		AwaitingAdd.iAfter			= iSorted - 1;
		AwaitingAdd.iItemInternal	= *itr;

		m_AwaitingAddList.push_back(AwaitingAdd);
	}

	m_FilteredItemsList.clear();

	InsertAwaitingItems(m_bShowInGroups);

	SendMessage(m_hOwner,WM_USER_UPDATEWINDOWS,0,0);
}

void CFolderView::VerifySortMode(void)
{
	BOOL	bValid = FALSE;
	int		i = 0;

	if(CompareVirtualFolders(CSIDL_CONTROLS))
	{
		/* Control panel. */

		for(i = 0;i < sizeof(ControlPanelSortModes) / sizeof(int);i++)
		{
			if(m_SortMode == ControlPanelSortModes[i])
			{
				bValid = TRUE;
				break;
			}
		}

		if(!bValid)
			m_SortMode = FSM_NAME;
	}
	else if(CompareVirtualFolders(CSIDL_DRIVES))
	{
		/* My Computer. */

		for(i = 0;i < sizeof(MyComputerSortModes) / sizeof(int);i++)
		{
			if(m_SortMode == MyComputerSortModes[i])
			{
				bValid = TRUE;
				break;
			}
		}

		if(!bValid)
			m_SortMode = FSM_NAME;
	}
	else if(CompareVirtualFolders(CSIDL_BITBUCKET))
	{
		/* Recycle Bin. */

		for(i = 0;i < sizeof(RecycleBinSortModes) / sizeof(int);i++)
		{
			if(m_SortMode == RecycleBinSortModes[i])
			{
				bValid = TRUE;
				break;
			}
		}

		if(!bValid)
			m_SortMode = FSM_NAME;
	}
	else if(CompareVirtualFolders(CSIDL_PRINTERS))
	{
		/* Printers virtual folder. */

		for(i = 0;i < sizeof(PrintersSortModes) / sizeof(int);i++)
		{
			if(m_SortMode == PrintersSortModes[i])
			{
				bValid = TRUE;
				break;
			}
		}

		if(!bValid)
			m_SortMode = FSM_NAME;
	}
	else if(CompareVirtualFolders(CSIDL_CONNECTIONS))
	{
		/* Network connections virtual folder. */

		for(i = 0;i < sizeof(NetworkConnectionsSortModes) / sizeof(int);i++)
		{
			if(m_SortMode == NetworkConnectionsSortModes[i])
			{
				bValid = TRUE;
				break;
			}
		}

		if(!bValid)
			m_SortMode = FSM_NAME;
	}
	else
	{
		/* Real folder. */

		for(i = 0;i < sizeof(RealFolderSortModes) / sizeof(int);i++)
		{
			if(m_SortMode == RealFolderSortModes[i])
			{
				bValid = TRUE;
				break;
			}
		}

		if(!bValid)
			m_SortMode = FSM_NAME;
	}
}

BOOL CFolderView::ToggleSortAscending(void)
{
	m_bSortAscending = !m_bSortAscending;

	return m_bSortAscending;
}

BOOL CFolderView::SetSortAscending(BOOL bAscending)
{
	m_bSortAscending = bAscending;

	return m_bSortAscending;
}

BOOL CFolderView::GetSortAscending(void)
{
	return m_bSortAscending;
}

BOOL CFolderView::ToggleAutoArrange(void)
{
	m_bAutoArrange = !m_bAutoArrange;

	ListView_SetAutoArrange(m_hListView,m_bAutoArrange);

	return m_bAutoArrange;
}

BOOL CFolderView::QuerySortAscending(void)
{
	return m_bSortAscending;
}

BOOL CFolderView::QueryAutoArrange(void)
{
	return m_bAutoArrange;
}

BOOL CFolderView::QueryShowHidden(void)
{
	return m_bShowHidden;
}

BOOL CFolderView::SetShowHidden(BOOL bShowHidden)
{
	m_bShowHidden = bShowHidden;

	return m_bShowHidden;
}

BOOL CFolderView::ToggleShowHidden(void)
{
	m_bShowHidden = !m_bShowHidden;

	return m_bShowHidden;
}

void CFolderView::CheckFolderLockState(TCHAR *szParsingPath)
{
	HANDLE hFile;

	hFile = CreateFile(szParsingPath,FILE_ALL_ACCESS,
	FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,
	FILE_FLAG_BACKUP_SEMANTICS,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
	{
		if(GetLastError() == ERROR_ACCESS_DENIED)
		{
			SECURITY_DESCRIPTOR Des;
			ACL Acl;

			InitializeSecurityDescriptor(&Des,SECURITY_DESCRIPTOR_REVISION);
			InitializeAcl(&Acl,sizeof(Acl),ACL_REVISION);
			SetSecurityDescriptorDacl(&Des,FALSE,&Acl,FALSE);

			if(m_bUnlockFolders)
			{
				/*The folder is locked. Attempt to unlock it (won't
				work on folders that have been locked by other users).*/
				SetFileSecurity(szParsingPath,DACL_SECURITY_INFORMATION,
				&Des);
			}
			else
			{
				//ListView_SetBackgroundImage(m_hListView,IDB_FOLDERLOCKED);
			}
		}
	}
	else
	{
		CloseHandle(hFile);
	}
}

void CFolderView::ResetFolderMemoryAllocations(void)
{
	HIMAGELIST himl;
	HIMAGELIST himlOld;
	int nItems;
	int i = 0;

	for(i = 0;i < m_iCurrentAllocation;i++)
	{
		if(m_pItemMap[i] == 1)
		{
			CoTaskMemFree(m_pExtraItemInfo[i].pridl);
		}
	}

	/* If we're in thumbnails view, destroy the current
	imagelist, and create a new one. */
	if(m_ViewMode == VM_THUMBNAILS)
	{
		EnterCriticalSection(&g_csThumbnails);

		himlOld = ListView_GetImageList(m_hListView,LVSIL_NORMAL);

		nItems = ListView_GetItemCount(m_hListView);

		/* Create and set the new imagelist. */
		himl = ImageList_Create(THUMBNAIL_ITEM_WIDTH,THUMBNAIL_ITEM_HEIGHT,
			ILC_COLOR32,nItems,nItems + 100);
		ListView_SetImageList(m_hListView,himl,LVSIL_NORMAL);

		ImageList_Destroy(himlOld);

		LeaveCriticalSection(&g_csThumbnails);
	}

	EnterCriticalSection(&m_csDirectoryAltered);
	m_AlteredList.clear();
	LeaveCriticalSection(&m_csDirectoryAltered);

	m_iCurrentAllocation = DEFAULT_MEM_ALLOC;

	m_pwfdFiles = (WIN32_FIND_DATA *)realloc(m_pwfdFiles,
		m_iCurrentAllocation * (sizeof(WIN32_FIND_DATA)));

	m_pExtraItemInfo = (CItemObject *)realloc(m_pExtraItemInfo,
		m_iCurrentAllocation * sizeof(CItemObject));

	m_pItemMap = (int *)realloc(m_pItemMap,m_iCurrentAllocation * sizeof(int));

	InitializeItemMap(0,m_iCurrentAllocation);

	CoTaskMemFree(m_pidlDirectory);

	m_FilteredItemsList.clear();
	m_AwaitingAddList.clear();

	m_nItemsInInfoList = 0;
}

BOOL CFolderView::QueryDragging(void)
{
	return m_bPerformingDrag;
}

BOOL CFolderView::GetTerminationStatus(void)
{
	return m_bNotifiedOfTermination;
}

void CFolderView::SetTerminationStatus(void)
{
	EmptyIconFinderQueue();
	EmptyThumbnailsQueue();
	EmptyColumnQueue();
	EmptyFolderQueue();
	m_bNotifiedOfTermination = TRUE;

	/*if(g_nAPCsRan == g_nAPCsQueued)
		SendMessage(m_hOwner,WM_USER_RELEASEBROWSER,(WPARAM)m_ID,NULL);*/
}

void CFolderView::QueryCurrentSortModes(list<int> *pSortModes)
{
	list<Column_t>::iterator itr;
	int ColumnSortMode;

	for(itr = m_pActiveColumnList->begin();itr != m_pActiveColumnList->end();itr++)
	{
		if(itr->bChecked)
		{
			ColumnSortMode = DetermineColumnSortMode(itr->id);

			pSortModes->push_back(ColumnSortMode);
		}
	}
}

HICON CFolderView::GetItemIcon(int iItem)
{
	LVITEM lvItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	ListView_GetItem(m_hListView,&lvItem);

	if(!m_pExtraItemInfo[(int)lvItem.lParam].bIconRetrieved)
		return GetItemIconInternal((int)lvItem.lParam);
	else
		return ImageList_GetIcon(m_hListViewImageList,
		m_pExtraItemInfo[(int)lvItem.lParam].iIcon,ILD_NORMAL);
}

HICON CFolderView::GetItemIconInternal(int iItemInternal)
{
	SHFILEINFO	shfi;
	TCHAR		szItemPath[MAX_PATH];

	QueryFullItemNameInternal(iItemInternal,szItemPath);

	if(!m_pExtraItemInfo[iItemInternal].bIconRetrieved)
	{
		SHGetFileInfo(szItemPath,0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);

		m_pExtraItemInfo[iItemInternal].bIconRetrieved	= TRUE;
		m_pExtraItemInfo[iItemInternal].iIcon			= shfi.iIcon;
	}

	return ImageList_GetIcon(m_hListViewImageList,
	m_pExtraItemInfo[iItemInternal].iIcon,ILD_NORMAL);
}

void CFolderView::ExportCurrentColumns(list<Column_t> *pColumns)
{
	list<Column_t>::iterator itr;
	Column_t Column;
	int iColumn = 0;

	for(itr = m_pActiveColumnList->begin();itr != m_pActiveColumnList->end();itr++)
	{
		if(m_ViewMode == VM_DETAILS && itr->bChecked)
		{
			itr->iWidth = ListView_GetColumnWidth(m_hListView,iColumn);

			iColumn++;
		}

		Column.id		= itr->id;
		Column.bChecked	= itr->bChecked;
		Column.iWidth	= itr->iWidth;

		pColumns->push_back(Column);
	}
}

void CFolderView::ImportColumns(list<Column_t> *pColumns,BOOL bColumnsSwapped)
{
	list<Column_t>::iterator itr;
	list<Column_t>::iterator itr2;
	list<Column_t>::iterator itr3;
	Column_t ci;
	BOOL bResortFolder = FALSE;
	int iColumn = 0;
	int i = 0;

	for(itr = pColumns->begin();itr != pColumns->end();itr++)
	{
		/* Check if this column represents the current sorting mode.
		If it does, and it is been removed, set the sort mode back
		to the first checked column. */
		if(!itr->bChecked && DetermineColumnSortMode(itr->id) == m_SortMode)
		{
			/* Find the first checked column. */
			for(itr2 = pColumns->begin();itr2 != pColumns->end();itr2++)
			{
				if(itr2->bChecked)
				{
					m_SortMode = DetermineColumnSortMode(itr2->id);

					bResortFolder = TRUE;
					break;
				}
			}
		}

		GetColumnInternal(itr->id,&ci);

		if(itr->bChecked)
		{
			if(m_ViewMode == VM_DETAILS)
			{
				for(itr2 = m_pActiveColumnList->begin();itr2 != m_pActiveColumnList->end();itr2++)
				{
					if(itr2->id == itr->id)
						break;
				}

				if(!itr2->bChecked)
				{
					InsertColumn(itr->id,iColumn,itr->iWidth);

					for(i = 0;i < m_nTotalItems;i++)
					{
						SetColumnData(itr->id,i,iColumn);
					}
				}
			}

			iColumn++;
		}
		else
		{
			for(itr2 = m_pActiveColumnList->begin();itr2 != m_pActiveColumnList->end();itr2++)
			{
				if(itr2->id == itr->id)
					break;
			}

			if(itr2->bChecked)
				ListView_DeleteColumn(m_hListView,iColumn);
		}
	}

	/* Copy the new columns. */
	*m_pActiveColumnList = *pColumns;

	/* The folder will need to be resorted if the
	sorting column was removed. */
	if(bResortFolder)
	{
		SortFolder(m_SortMode);
	}

	m_bColumnsPlaced = FALSE;
}

void CFolderView::GetColumnInternal(unsigned int id,Column_t *pci)
{
	list<Column_t>::iterator itr;

	for(itr = m_pActiveColumnList->begin();itr != m_pActiveColumnList->end();itr++)
	{
		if(itr->id == id)
		{
			*pci = *itr;
			return;
		}
	}
}

/* Queues an in-place rename for an item.
This method is used when a file is created
using the shell new menu, and the item
may or may not have been inserted into
the listview yet. */
void CFolderView::QueueRename(LPITEMIDLIST pidlItem)
{
	/* Items are inserted within the context
	of this thread. Therefore, either pending
	items have already been inserted, or they
	have yet to be inserted.
	First, look for the file using it's display
	name. If the file is not found, set a flag
	indicating that when items are inserted,
	they should be checked against this item.
	Once a match is found, rename the item
	in-place within the listview. */

	TCHAR szItem[MAX_PATH];
	LPITEMIDLIST pidlComplete = NULL;
	LVITEM lvItem;
	BOOL bItemFound = FALSE;
	int nItems;
	int i = 0;

	GetDisplayName(pidlItem,szItem,SHGDN_INFOLDER);

	nItems = ListView_GetItemCount(m_hListView);

	for(i = 0;i < nItems;i++)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		ListView_GetItem(m_hListView,&lvItem);

		pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)lvItem.lParam].pridl);

		if(CompareIdls(pidlItem,pidlComplete))
		{
			CoTaskMemFree(pidlComplete);

			bItemFound = TRUE;

			ListView_EditLabel(m_hListView,i);
			break;
		}

		CoTaskMemFree(pidlComplete);
	}

	if(!bItemFound)
	{
		m_bNewItemCreated = TRUE;
		m_pidlNewItem = ILClone(pidlItem);
	}
}

void CFolderView::SelectItems(list<PastedFile_t> *pFileList)
{
	list<PastedFile_t>::iterator itr;
	int iIndex;

	m_pFileSelectionList.clear();

	for(itr = pFileList->begin();itr != pFileList->end();)
	{
		iIndex = LocateFileItemIndex(itr->szFileName);

		if(iIndex != -1)
		{
			ListView_SelectItem(m_hListView,iIndex,TRUE);

			if(itr == pFileList->begin())
			{
				/* Focus on the first item, and ensure it is visible. */
				ListView_FocusItem(m_hListView,iIndex,TRUE);
				ListView_EnsureVisible(m_hListView,iIndex,FALSE);
			}

			itr = pFileList->erase(itr);
		}
		else
		{
			++itr;
		}
	}

	m_pFileSelectionList = *pFileList;
}

/* Refreshes all icons in the current
window. This may be needed when a file
association changes, for example. */
void CFolderView::RefreshAllIcons(void)
{
	LVITEM lvItem;
	int i = 0;

	for(i = 0;i < m_nTotalItems;i++)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		ListView_GetItem(m_hListView,&lvItem);

		/* BUG: If any items' index changes
		while the icons are been updated, it
		may not be updated. Need to loop through
		all active items (in internal structure,
		find in listview, and update). */
		AddToIconFinderQueue(&lvItem);
	}
}

void CFolderView::OnDeviceChange(WPARAM wParam,LPARAM lParam)
{
	/* Note changes made here may have no effect. Since
	the icon for the cd/dvd/etc. may not have been
	updated by the time this function is called, it's
	possible this may not change anything. */

	/* If we are currently not in my computer, this
	message can be safely ignored (drives are only
	shown in my computer). */
	if(CompareVirtualFolders(CSIDL_DRIVES))
	{
		switch(wParam)
		{
			/* Device has being added/inserted into the system. Update the
			drives toolbar as necessary. */
		case DBT_DEVICEARRIVAL:
			{
				DEV_BROADCAST_HDR *dbh = NULL;

				dbh = (DEV_BROADCAST_HDR *)lParam;

				if(dbh->dbch_devicetype == DBT_DEVTYP_VOLUME)
				{
					DEV_BROADCAST_VOLUME	*pdbv = NULL;
					TCHAR					chDrive;
					TCHAR					szDrive[4];

					pdbv = (DEV_BROADCAST_VOLUME *)dbh;

					/* Build a string that will form the drive name. */
					chDrive = GetDriveNameFromMask(pdbv->dbcv_unitmask);
					StringCchPrintf(szDrive,SIZEOF_ARRAY(szDrive),
						_T("%c:\\"),chDrive);

					if(pdbv->dbcv_flags & DBTF_MEDIA)
					{
						UpdateDriveIcon(szDrive);
					}
					else
					{
						OnFileActionAdded(szDrive);
					}
				}
			}
			break;

		case DBT_DEVICEREMOVECOMPLETE:
			{
				DEV_BROADCAST_HDR				*dbh = NULL;

				dbh = (DEV_BROADCAST_HDR *)lParam;

				if(dbh->dbch_devicetype == DBT_DEVTYP_VOLUME)
				{
					DEV_BROADCAST_VOLUME	*pdbv = NULL;
					TCHAR					chDrive;
					TCHAR					szDrive[4];

					pdbv = (DEV_BROADCAST_VOLUME *)dbh;

					/* Build a string that will form the drive name. */
					chDrive = GetDriveNameFromMask(pdbv->dbcv_unitmask);
					StringCchPrintf(szDrive,SIZEOF_ARRAY(szDrive),_T("%c:\\"),chDrive);

					/* The device was removed from the system.
					Remove it from the listview (only if the drive
					was actually removed - the drive may not have
					been removed, for example, if a cd/dvd was
					changed). */
					if(pdbv->dbcv_flags & DBTF_MEDIA)
					{
						UpdateDriveIcon(szDrive);
					}
					else
					{
						/* At this point, the drive has been completely removed
						from the system. Therefore, its display name cannot be
						queried. Need to search for the drive using ONLY its
						drive letter/name. Once its index in the listview has
						been determined, it can simply be removed. */
						RemoveDrive(szDrive);
					}
				}
			}
			break;
		}
	}
}

void CFolderView::UpdateDriveIcon(TCHAR *szDrive)
{
	LPITEMIDLIST			pidlDrive = NULL;
	LPITEMIDLIST			pidlItem = NULL;
	LVITEM					lvItem;
	SHFILEINFO				shfi;
	TCHAR					szDisplayName[MAX_PATH];
	HRESULT					hr;
	int						iItem = -1;
	int						iItemInternal = -1;
	int						i = 0;

	/* Look for the item using its display name, NOT
	its drive letter/name. */
	GetDisplayName(szDrive,szDisplayName,SHGDN_INFOLDER);

	hr = GetIdlFromParsingName(szDrive,&pidlDrive);

	if(SUCCEEDED(hr))
	{
		for(i = 0;i < m_nTotalItems;i++)
		{
			lvItem.mask		= LVIF_PARAM;
			lvItem.iItem	= i;
			lvItem.iSubItem	= 0;
			ListView_GetItem(m_hListView,&lvItem);

			pidlItem = ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)lvItem.lParam].pridl);

			if(CompareIdls(pidlDrive,pidlItem))
			{
				iItem = i;
				iItemInternal = (int)lvItem.lParam;

				CoTaskMemFree(pidlItem);

				break;
			}

			CoTaskMemFree(pidlItem);
		}

		CoTaskMemFree(pidlDrive);
	}

	if(iItem != -1)
	{
		SHGetFileInfo(szDrive,0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);

		StringCchCopy(m_pExtraItemInfo[iItemInternal].szDisplayName,
			SIZEOF_ARRAY(m_pExtraItemInfo[iItemInternal].szDisplayName),
			szDisplayName);

		/* Update the drives icon and display name. */
		lvItem.mask		= LVIF_TEXT|LVIF_IMAGE;
		lvItem.iImage	= shfi.iIcon;
		lvItem.iItem	= iItem;
		lvItem.iSubItem	= 0;
		lvItem.pszText	= m_pExtraItemInfo[iItemInternal].szDisplayName;
		ListView_SetItem(m_hListView,&lvItem);
	}
}

void CFolderView::RemoveDrive(TCHAR *szDrive)
{
	LVITEM lvItem;
	int iItemInternal = -1;
	int i = 0;

	for(i = 0;i < m_nTotalItems;i++)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		ListView_GetItem(m_hListView,&lvItem);

		if(m_pExtraItemInfo[(int)lvItem.lParam].bDrive)
		{
			if(lstrcmp(szDrive,m_pExtraItemInfo[(int)lvItem.lParam].szDrive) == 0)
			{
				iItemInternal = (int)lvItem.lParam;
				break;
			}
		}
	}

	if(iItemInternal != -1)
		RemoveItem(iItemInternal);
}

int CFolderView::GetFolderIndex(void)
{
	return m_iUniqueFolderIndex;
}