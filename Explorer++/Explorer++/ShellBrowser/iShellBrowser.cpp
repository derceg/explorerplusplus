/******************************************************************
 *
 * Project: ShellBrowser
 * File: iShellBrowser.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Main file for folder browsing support.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "iShellView.h"
#include "iShellBrowser_internal.h"
#include "SortModes.h"
#include "ViewModes.h"
#include "../Helper/Controls.h"
#include "../Helper/DriveInfo.h"
#include "../Helper/FileOperations.h"
#include "../Helper/FolderSize.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <boost/scope_exit.hpp>
#include <list>

#pragma warning(disable:4459) // declaration of 'boost_scope_exit_aux_args' hides global declaration

void CALLBACK	TimerProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);

void CShellBrowser::UpdateFileSelectionInfo(int iCacheIndex,BOOL Selected)
{
	ULARGE_INTEGER	ulFileSize;
	BOOL			IsFolder;

	IsFolder = (m_fileInfoMap.at(iCacheIndex).dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	== FILE_ATTRIBUTE_DIRECTORY;

	ulFileSize.LowPart = m_fileInfoMap.at(iCacheIndex).nFileSizeLow;
	ulFileSize.HighPart = m_fileInfoMap.at(iCacheIndex).nFileSizeHigh;

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

BOOL CShellBrowser::IsFilenameFiltered(const TCHAR *FileName) const
{
	if(CheckWildcardMatch(m_szFilter,FileName,m_bFilterCaseSensitive))
		return FALSE;

	return TRUE;
}

int CShellBrowser::QueryDisplayName(int iItem,UINT BufferSize,TCHAR *Buffer) const
{
	LVITEM lvItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	ListView_GetItem(m_hListView,&lvItem);

	StringCchCopy(Buffer,BufferSize,m_fileInfoMap.at((int)lvItem.lParam).cFileName);

	return lstrlen(Buffer);
}

HRESULT CShellBrowser::QueryFullItemName(int iIndex,TCHAR *FullItemPath,UINT cchMax) const
{
	LVITEM			lvItem;
	BOOL			bRes;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iIndex;
	lvItem.iSubItem	= 0;
	bRes = ListView_GetItem(m_hListView,&lvItem);

	if(bRes)
	{
		QueryFullItemNameInternal((int)lvItem.lParam,FullItemPath,cchMax);

		return S_OK;
	}

	return E_FAIL;
}

void CShellBrowser::QueryFullItemNameInternal(int iItemInternal,TCHAR *szFullFileName,UINT cchMax) const
{
	LPITEMIDLIST	pidlComplete = NULL;

	pidlComplete = ILCombine(m_pidlDirectory,m_extraItemInfoMap.at(iItemInternal).pridl.get());

	GetDisplayName(pidlComplete,szFullFileName,cchMax,SHGDN_FORPARSING);

	CoTaskMemFree(pidlComplete);
}

UINT CShellBrowser::QueryCurrentDirectory(int BufferSize,TCHAR *Buffer) const
{
	if(BufferSize < (lstrlen(m_CurDir) + 1))
	{
		Buffer = NULL;
		return lstrlen(m_CurDir);
	}

	StringCchCopy(Buffer,BufferSize,m_CurDir);

	return lstrlen(m_CurDir);
}

LPITEMIDLIST CShellBrowser::QueryCurrentDirectoryIdl(void) const
{
	return ILClone(m_pidlDirectory);
}

HRESULT CShellBrowser::CreateHistoryPopup(IN HWND hParent,OUT LPITEMIDLIST *pidl,
IN POINT *pt,IN BOOL bBackOrForward)
{
	int iSel;

	iSel = m_pathManager.CreateHistoryPopupMenu(hParent,pt,bBackOrForward);

	if(iSel == 0)
		return E_FAIL;

	if(bBackOrForward)
		iSel = -iSel;

	*pidl = m_pathManager.RetrievePath(iSel);

	return S_OK;
}

BOOL CShellBrowser::IsBackHistory(void) const
{
	int nBackPathsStored;

	nBackPathsStored = m_pathManager.GetNumBackPathsStored();

	if(nBackPathsStored == 0)
		return FALSE;

	return TRUE;
}

BOOL CShellBrowser::IsForwardHistory(void) const
{
	int nForwardPathsStored;

	nForwardPathsStored = m_pathManager.GetNumForwardPathsStored();

	if(nForwardPathsStored == 0)
		return FALSE;

	return TRUE;
}

std::list<LPITEMIDLIST> CShellBrowser::GetBackHistory() const
{
	return m_pathManager.GetBackHistory();
}

std::list<LPITEMIDLIST> CShellBrowser::GetForwardHistory() const
{
	return m_pathManager.GetForwardHistory();
}

LPITEMIDLIST CShellBrowser::RetrieveHistoryItemWithoutUpdate(int iItem)
{
	return m_pathManager.RetrievePathWithoutUpdate(iItem);
}

LPITEMIDLIST CShellBrowser::RetrieveHistoryItem(int iItem)
{
	return m_pathManager.RetrievePath(iItem);
}

BOOL CShellBrowser::CanBrowseUp(void) const
{
	return !IsNamespaceRoot(m_pidlDirectory);
}

/* TODO: Convert to using pidl's here, rather than
file names. */
int CShellBrowser::SelectFiles(const TCHAR *FileNamePattern)
{
	int iItem;
	
	iItem = LocateFileItemIndex(FileNamePattern);

	if(iItem != -1)
	{
		NListView::ListView_FocusItem(m_hListView,iItem,TRUE);
		NListView::ListView_SelectItem(m_hListView,iItem,TRUE);
		ListView_EnsureVisible(m_hListView,iItem,FALSE);
		return 1;
	}

	return 0;
}

int CShellBrowser::LocateFileItemIndex(const TCHAR *szFileName) const
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

int CShellBrowser::LocateFileItemInternalIndex(const TCHAR *szFileName) const
{
	LVITEM	lvItem;
	int		i = 0;

	for(i = 0;i < m_nTotalItems;i++)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		ListView_GetItem(m_hListView,&lvItem);

		if((lstrcmp(m_fileInfoMap.at((int)lvItem.lParam).cFileName,szFileName) == 0) ||
			(lstrcmp(m_fileInfoMap.at((int)lvItem.lParam).cAlternateFileName,szFileName) == 0))
		{
			return (int)lvItem.lParam;
			break;
		}
	}

	return -1;
}

boost::optional<int> CShellBrowser::LocateItemByInternalIndex(int internalIndex) const
{
	LVFINDINFO lvfi;
	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = internalIndex;
	int item = ListView_FindItem(m_hListView, -1, &lvfi);

	if (item == -1)
	{
		return boost::none;
	}

	return item;
}

DWORD CShellBrowser::QueryFileAttributes(int iItem) const
{
	LVITEM lvItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	ListView_GetItem(m_hListView,&lvItem);

	return m_fileInfoMap.at((int)lvItem.lParam).dwFileAttributes;
}

WIN32_FIND_DATA CShellBrowser::QueryFileFindData(int iItem) const
{
	LVITEM lvItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	ListView_GetItem(m_hListView,&lvItem);

	return m_fileInfoMap.at((int)lvItem.lParam);
}

void CShellBrowser::DragStarted(int iFirstItem,POINT *ptCursor)
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
		QueryDisplayName(iSelected, SIZEOF_ARRAY(df.szFileName), df.szFileName);

		m_DraggedFilesList.push_back(df);
	}

	m_bDragging = TRUE;
}

void CShellBrowser::DragStopped(void)
{
	m_DraggedFilesList.clear();

	m_bDragging = FALSE;
}

void CShellBrowser::OnListViewGetDisplayInfo(LPARAM lParam)
{
	NMLVDISPINFO	*pnmv = NULL;
	LVITEM			*plvItem = NULL;
	NMHDR			*nmhdr = NULL;

	pnmv	= (NMLVDISPINFO *)lParam;
	plvItem	= &pnmv->item;
	nmhdr	= &pnmv->hdr;

	/* Construct an image here using the items
	actual icon. This image will be shown initially.
	If the item also has a thumbnail image, this
	will be found later, and will overwrite any
	image settings made here.
	Note that the initial icon image MUST be drawn
	first, or else it may be possible for the
	thumbnail to be drawn before the initial
	image. */
	if(m_ViewMode == VM_THUMBNAILS && (plvItem->mask & LVIF_IMAGE) == LVIF_IMAGE)
	{
		plvItem->iImage = GetIconThumbnail((int)plvItem->lParam);
		plvItem->mask |= LVIF_DI_SETITEM;

		QueueThumbnailTask(static_cast<int>(plvItem->lParam));

		return;
	}

	if (m_ViewMode == VM_DETAILS && (plvItem->mask & LVIF_TEXT) == LVIF_TEXT)
	{
		QueueColumnTask(static_cast<int>(plvItem->lParam), plvItem->iSubItem);
	}

	if((plvItem->mask & LVIF_IMAGE) == LVIF_IMAGE)
	{
		if((m_fileInfoMap.at(static_cast<int>(plvItem->lParam)).dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
		{
			/* File. */
			plvItem->iImage	= m_iFileIcon;
		}
		else
		{
			/* Folder. */
			plvItem->iImage	= m_iFolderIcon;
		}

		QueueIconTask(static_cast<int>(plvItem->lParam));
	}

	plvItem->mask |= LVIF_DI_SETITEM;
}

void CShellBrowser::QueueIconTask(int internalIndex)
{
	int iconResultID = m_iconResultIDCounter++;

	auto result = m_itemImageThreadPool.push([this, iconResultID, internalIndex](int id) {
		UNREFERENCED_PARAMETER(id);

		return this->FindIconAsync(iconResultID, internalIndex);
	});

	m_iconResults.insert({ iconResultID, std::move(result) });
}

boost::optional<CShellBrowser::ImageResult_t> CShellBrowser::FindIconAsync(int iconResultId, int internalIndex) const
{
	LPITEMIDLIST pidlComplete = ILCombine(m_pidlDirectory, m_extraItemInfoMap.at(internalIndex).pridl.get());

	BOOST_SCOPE_EXIT(pidlComplete) {
		CoTaskMemFree(pidlComplete);
	} BOOST_SCOPE_EXIT_END

	// Must use SHGFI_ICON here, rather than SHGFO_SYSICONINDEX, or else 
	// icon overlays won't be applied.
	SHFILEINFO shfi;
	DWORD_PTR res = SHGetFileInfo(reinterpret_cast<LPCTSTR>(pidlComplete), 0, &shfi,
		sizeof(SHFILEINFO), SHGFI_PIDL | SHGFI_ICON | SHGFI_OVERLAYINDEX);

	if (res == 0)
	{
		return boost::none;
	}

	DestroyIcon(shfi.hIcon);

	PostMessage(m_hListView, WM_APP_ICON_RESULT_READY, iconResultId, 0);

	ImageResult_t result;
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

	LVITEM lvItem;
	lvItem.mask = LVIF_IMAGE | LVIF_STATE;
	lvItem.iItem = *index;
	lvItem.iSubItem = 0;
	lvItem.iImage = result->iconIndex;
	lvItem.stateMask = LVIS_OVERLAYMASK;
	lvItem.state = INDEXTOOVERLAYMASK(result->iconIndex >> 24);
	ListView_SetItem(m_hListView, &lvItem);
}

LPITEMIDLIST CShellBrowser::QueryItemRelativeIdl(int iItem) const
{
	LVITEM lvItem;
	BOOL bRet;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	bRet = ListView_GetItem(m_hListView,&lvItem);

	if(bRet)
		return ILClone((ITEMIDLIST *)m_extraItemInfoMap.at((int)lvItem.lParam).pridl.get());

	return NULL;
}

BOOL CShellBrowser::InVirtualFolder(void) const
{
	return m_bVirtualFolder;
}

/* We can create files in this folder if it is
part of the filesystem, or if it is the root of
the namespace (i.e. the desktop). */
BOOL CShellBrowser::CanCreate(void) const
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

void CShellBrowser::SetDirMonitorId(int iDirMonitorId)
{
	m_iDirMonitorId = iDirMonitorId;
}

int CShellBrowser::GetDirMonitorId(void) const
{
	return m_iDirMonitorId;
}

BOOL CShellBrowser::CompareVirtualFolders(UINT uFolderCSIDL) const
{
	TCHAR	szParsingPath[MAX_PATH];

	GetCsidlDisplayName(uFolderCSIDL,szParsingPath,SIZEOF_ARRAY(szParsingPath),SHGDN_FORPARSING);

	if(StrCmp(m_CurDir,szParsingPath) == 0)
		return TRUE;

	return FALSE;
}

int CShellBrowser::GenerateUniqueItemId(void)
{
	return m_itemIDCounter++;
}

void CShellBrowser::PositionDroppedItems(void)
{
	std::list<DroppedFile_t>::iterator	itr;
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
			NListView::ListView_SetAutoArrange(m_hListView,FALSE);

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

				NListView::ListView_SelectItem(m_hListView,iItem,TRUE);
				NListView::ListView_FocusItem(m_hListView,iItem,TRUE);

				itr = m_DroppedFileNameList.erase(itr);
			}
			else
			{
				++itr;
			}
		}

		if(m_bAutoArrange)
			NListView::ListView_SetAutoArrange(m_hListView,TRUE);
	}
}

int CShellBrowser::DetermineItemSortedPosition(LPARAM lParam) const
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
			res = Sort(static_cast<int>(lParam),static_cast<int>(lvItem.lParam));
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

BOOL CShellBrowser::IsFileReal(int iItem) const
{
	LVITEM	lvItem;
	BOOL	bRes;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	bRes = ListView_GetItem(m_hListView,&lvItem);

	if(bRes)
		return m_extraItemInfoMap.at((int)lvItem.lParam).bReal;

	return FALSE;
}

BOOL CShellBrowser::GhostItem(int iItem)
{
	return GhostItemInternal(iItem,TRUE);
}

BOOL CShellBrowser::DeghostItem(int iItem)
{
	return GhostItemInternal(iItem,FALSE);
}

BOOL CShellBrowser::GhostItemInternal(int iItem,BOOL bGhost)
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
		if(m_fileInfoMap.at((int)lvItem.lParam).dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
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

void CShellBrowser::RemoveFilteredItems(void)
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

		if(!((m_fileInfoMap.at((int)lvItem.lParam).dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
		{
			if(IsFilenameFiltered(m_extraItemInfoMap.at((int)lvItem.lParam).szDisplayName))
			{
				RemoveFilteredItem(i,(int)lvItem.lParam);
			}
		}
	}

	SendMessage(m_hOwner,WM_USER_UPDATEWINDOWS,0,0);
}

void CShellBrowser::RemoveFilteredItem(int iItem,int iItemInternal)
{
	ULARGE_INTEGER	ulFileSize;

	if(ListView_GetItemState(m_hListView,iItem,LVIS_SELECTED)
		== LVIS_SELECTED)
	{
		ulFileSize.LowPart = m_fileInfoMap.at(iItemInternal).nFileSizeLow;
		ulFileSize.HighPart = m_fileInfoMap.at(iItemInternal).nFileSizeHigh;

		m_ulFileSelectionSize.QuadPart -= ulFileSize.QuadPart;
	}

	/* Take the file size of the removed file away from the total
	directory size. */
	ulFileSize.LowPart = m_fileInfoMap.at(iItemInternal).nFileSizeLow;
	ulFileSize.HighPart = m_fileInfoMap.at(iItemInternal).nFileSizeHigh;

	m_ulTotalDirSize.QuadPart -= ulFileSize.QuadPart;

	/* Remove the item from the m_hListView. */
	ListView_DeleteItem(m_hListView,iItem);

	m_nTotalItems--;

	m_FilteredItemsList.push_back(iItemInternal);
}

int CShellBrowser::QueryNumItems(void) const
{
	return m_nTotalItems;
}

int CShellBrowser::QueryNumSelectedFiles(void) const
{
	return m_NumFilesSelected;
}

int CShellBrowser::QueryNumSelectedFolders(void) const
{
	return m_NumFoldersSelected;
}

int CShellBrowser::QueryNumSelected(void) const
{
	return m_NumFilesSelected + m_NumFoldersSelected;
}

void CShellBrowser::QueryFolderInfo(FolderInfo_t *pFolderInfo)
{
	pFolderInfo->TotalFolderSize.QuadPart		= m_ulTotalDirSize.QuadPart;
	pFolderInfo->TotalSelectionSize.QuadPart	= m_ulFileSelectionSize.QuadPart;
}

void CShellBrowser::DetermineFolderVirtual(LPITEMIDLIST pidlDirectory)
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

void CShellBrowser::SetFilter(const TCHAR *szFilter)
{
	StringCchCopy(m_szFilter,SIZEOF_ARRAY(m_szFilter),szFilter);

	if(m_bApplyFilter)
	{
		UnfilterAllItems();
		UpdateFiltering();
	}
}

void CShellBrowser::GetFilter(TCHAR *szFilter,int cchMax) const
{
	StringCchCopy(szFilter,cchMax,m_szFilter);
}

void CShellBrowser::SetFilterStatus(BOOL bFilter)
{
	m_bApplyFilter = bFilter;

	UpdateFiltering();
}

BOOL CShellBrowser::GetFilterStatus(void) const
{
	return m_bApplyFilter;
}

void CShellBrowser::SetFilterCaseSensitive(BOOL bFilterCaseSensitive)
{
	m_bFilterCaseSensitive = bFilterCaseSensitive;
}

BOOL CShellBrowser::GetFilterCaseSensitive(void) const
{
	return m_bFilterCaseSensitive;
}

void CShellBrowser::UpdateFiltering(void)
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

void CShellBrowser::UnfilterAllItems(void)
{
	std::list<int>::iterator	itr;
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

void CShellBrowser::VerifySortMode(void)
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

BOOL CShellBrowser::GetSortAscending(void) const
{
	return m_bSortAscending;
}

BOOL CShellBrowser::SetSortAscending(BOOL bAscending)
{
	m_bSortAscending = bAscending;

	return m_bSortAscending;
}

BOOL CShellBrowser::ToggleSortAscending(void)
{
	m_bSortAscending = !m_bSortAscending;

	return m_bSortAscending;
}

BOOL CShellBrowser::GetShowHidden(void) const
{
	return m_bShowHidden;
}

BOOL CShellBrowser::SetShowHidden(BOOL bShowHidden)
{
	m_bShowHidden = bShowHidden;

	return m_bShowHidden;
}

BOOL CShellBrowser::ToggleShowHidden(void)
{
	m_bShowHidden = !m_bShowHidden;

	return m_bShowHidden;
}

void CShellBrowser::ResetFolderMemoryAllocations(void)
{
	HIMAGELIST himl;
	HIMAGELIST himlOld;
	int nItems;

	/* If we're in thumbnails view, destroy the current
	imagelist, and create a new one. */
	if(m_ViewMode == VM_THUMBNAILS)
	{
		himlOld = ListView_GetImageList(m_hListView,LVSIL_NORMAL);

		nItems = ListView_GetItemCount(m_hListView);

		/* Create and set the new imagelist. */
		himl = ImageList_Create(THUMBNAIL_ITEM_WIDTH,THUMBNAIL_ITEM_HEIGHT,
			ILC_COLOR32,nItems,nItems + 100);
		ListView_SetImageList(m_hListView,himl,LVSIL_NORMAL);

		ImageList_Destroy(himlOld);
	}

	EnterCriticalSection(&m_csDirectoryAltered);
	m_AlteredList.clear();
	LeaveCriticalSection(&m_csDirectoryAltered);

	m_itemIDCounter = 0;

	m_fileInfoMap.clear();
	m_extraItemInfoMap.clear();

	m_cachedFolderSizes.clear();

	CoTaskMemFree(m_pidlDirectory);

	m_FilteredItemsList.clear();
	m_AwaitingAddList.clear();
}

BOOL CShellBrowser::QueryDragging(void) const
{
	return m_bPerformingDrag;
}

std::list<int> CShellBrowser::QueryCurrentSortModes() const
{
	std::list<int> sortModes;

	for(auto itr = m_pActiveColumnList->begin();itr != m_pActiveColumnList->end();itr++)
	{
		if(itr->bChecked)
		{
			int ColumnSortMode = DetermineColumnSortMode(itr->id);
			sortModes.push_back(ColumnSortMode);
		}
	}

	return sortModes;
}

void CShellBrowser::ExportCurrentColumns(std::list<Column_t> *pColumns)
{
	std::list<Column_t>::iterator itr;
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

void CShellBrowser::ImportColumns(std::list<Column_t> *pColumns)
{
	std::list<Column_t>::iterator itr;
	std::list<Column_t>::iterator itr2;
	std::list<Column_t>::iterator itr3;
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
						SetColumnText(itr->id,i,iColumn);
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

void CShellBrowser::GetColumnInternal(unsigned int id,Column_t *pci) const
{
	std::list<Column_t>::iterator itr;

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
void CShellBrowser::QueueRename(LPCITEMIDLIST pidlItem)
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

	GetDisplayName(pidlItem,szItem,SIZEOF_ARRAY(szItem),SHGDN_INFOLDER);

	nItems = ListView_GetItemCount(m_hListView);

	for(i = 0;i < nItems;i++)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= i;
		lvItem.iSubItem	= 0;
		ListView_GetItem(m_hListView,&lvItem);

		pidlComplete = ILCombine(m_pidlDirectory,m_extraItemInfoMap.at((int)lvItem.lParam).pridl.get());

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

void CShellBrowser::SelectItems(const std::list<std::wstring> &PastedFileList)
{
	int i = 0;

	m_FileSelectionList.clear();

	for(const auto &PastedFile : PastedFileList)
	{
		int iIndex = LocateFileItemIndex(PastedFile.c_str());

		if(iIndex != -1)
		{
			NListView::ListView_SelectItem(m_hListView,iIndex,TRUE);

			if(i == 0)
			{
				/* Focus on the first item, and ensure it is visible. */
				NListView::ListView_FocusItem(m_hListView,iIndex,TRUE);
				ListView_EnsureVisible(m_hListView,iIndex,FALSE);

				i++;
			}
		}
		else
		{
			m_FileSelectionList.push_back(PastedFile);
		}
	}
}

void CShellBrowser::OnDeviceChange(WPARAM wParam,LPARAM lParam)
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
					chDrive = GetDriveLetterFromMask(pdbv->dbcv_unitmask);
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
					chDrive = GetDriveLetterFromMask(pdbv->dbcv_unitmask);
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

void CShellBrowser::UpdateDriveIcon(const TCHAR *szDrive)
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
	GetDisplayName(szDrive,szDisplayName,SIZEOF_ARRAY(szDisplayName),SHGDN_INFOLDER);

	hr = GetIdlFromParsingName(szDrive,&pidlDrive);

	if(SUCCEEDED(hr))
	{
		for(i = 0;i < m_nTotalItems;i++)
		{
			lvItem.mask		= LVIF_PARAM;
			lvItem.iItem	= i;
			lvItem.iSubItem	= 0;
			ListView_GetItem(m_hListView,&lvItem);

			pidlItem = ILCombine(m_pidlDirectory,m_extraItemInfoMap.at((int)lvItem.lParam).pridl.get());

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

		StringCchCopy(m_extraItemInfoMap.at(iItemInternal).szDisplayName,
			SIZEOF_ARRAY(m_extraItemInfoMap.at(iItemInternal).szDisplayName),
			szDisplayName);

		/* Update the drives icon and display name. */
		lvItem.mask		= LVIF_TEXT|LVIF_IMAGE;
		lvItem.iImage	= shfi.iIcon;
		lvItem.iItem	= iItem;
		lvItem.iSubItem	= 0;
		lvItem.pszText	= szDisplayName;
		ListView_SetItem(m_hListView,&lvItem);
	}
}

void CShellBrowser::RemoveDrive(const TCHAR *szDrive)
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

		if(m_extraItemInfoMap.at((int)lvItem.lParam).bDrive)
		{
			if(lstrcmp(szDrive,m_extraItemInfoMap.at((int)lvItem.lParam).szDrive) == 0)
			{
				iItemInternal = (int)lvItem.lParam;
				break;
			}
		}
	}

	if(iItemInternal != -1)
		RemoveItem(iItemInternal);
}

int CShellBrowser::GetFolderIndex(void) const
{
	return m_iUniqueFolderIndex;
}