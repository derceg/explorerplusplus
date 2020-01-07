// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Handles the dropping of items onto
 * the main listview.
 */

#include "stdafx.h"
#include "ShellBrowser.h"
#include "ViewModes.h"
#include "../Helper/DropHandler.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"


/* Scroll definitions. */
#define MIN_X_POS	20
#define MIN_Y_POS	20
#define X_SCROLL_AMOUNT	10
#define Y_SCROLL_AMOUNT	10

HRESULT _stdcall CShellBrowser::DragEnter(IDataObject *pDataObject,
DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect)
{
	HRESULT hReturn;
	POINT pt;

	m_bPerformingDrag = TRUE;
	m_bDeselectDropFolder = FALSE;
	m_iDropFolder = -1;

	if(m_bVirtualFolder && !m_bDragging)
	{
		m_bDataAccept	= FALSE;
		*pdwEffect		= DROPEFFECT_NONE;

		hReturn = S_OK;
	}
	else
	{
		std::list<FORMATETC> ftcList;
		CDropHandler::GetDropFormats(ftcList);

		BOOL bDataAccept = FALSE;

		/* Check whether the drop source has the type of data
		that is needed for this drag operation. */
		for(auto ftc : ftcList)
		{
			if(pDataObject->QueryGetData(&ftc) == S_OK)
			{
				bDataAccept = TRUE;
				break;
			}
		}

		if(bDataAccept)
		{
			m_bDataAccept = TRUE;

			m_bOnSameDrive = CheckItemLocations(pDataObject,0);
			*pdwEffect = DetermineDragEffect(grfKeyState,*pdwEffect,
				m_bDataAccept,m_bOnSameDrive);
		}
		else
		{
			m_bDataAccept	= FALSE;
			*pdwEffect		= DROPEFFECT_NONE;
		}

		hReturn = S_OK;
	}

	if(grfKeyState & MK_LBUTTON)
		m_DragType = DragType::LeftClick;
	else if(grfKeyState & MK_RBUTTON)
		m_DragType = DragType::RightClick;

	pt.x = ptl.x;
	pt.y = ptl.y;

	/* Notify the drop target helper that an object has been dragged into
	the window. */
	m_pDropTargetHelper->DragEnter(m_hListView,pDataObject,&pt,*pdwEffect);

	return hReturn;
}

HRESULT _stdcall CShellBrowser::DragOver(DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect)
{
	RECT	rc;
	POINT	pt;

	*pdwEffect = DetermineDragEffect(grfKeyState,*pdwEffect,
		m_bDataAccept,m_bOnSameDrive);

	pt.x = ptl.x;
	pt.y = ptl.y;

	/* Notify the drop helper of the current operation. */
	m_pDropTargetHelper->DragOver((LPPOINT)&pt,*pdwEffect);

	ScreenToClient(m_hListView,(LPPOINT)&pt);
	GetClientRect(m_hListView,&rc);

	/* If the cursor is too close to either the top or bottom
	of the listview, scroll the listview in the required direction. */
	ScrollListViewFromCursor(m_hListView,&pt);

	HandleDragSelection((LPPOINT)&pt);

	if(m_bDataAccept)
	{
		if(!m_bOverFolder)
			NListView::ListView_PositionInsertMark(m_hListView,&pt);
		else
			NListView::ListView_PositionInsertMark(m_hListView,NULL);
	}

	return S_OK;
}

/* Determines the drop effect based on the
location of the source and destination
directories.
Note that the first dropped file is taken
as representative of the rest (meaning that
if the files come from different drives,
whether this operation is classed as a copy
or move is only based on the location of the
first file). */
DWORD CShellBrowser::CheckItemLocations(IDataObject *pDataObject,int iDroppedItem)
{
	FORMATETC	ftc;
	STGMEDIUM	stg;
	DROPFILES	*pdf = NULL;
	TCHAR		szFullFileName[MAX_PATH];
	HRESULT		hr;
	BOOL		bOnSameDrive = FALSE;
	int			nDroppedFiles;

	ftc.cfFormat	= CF_HDROP;
	ftc.ptd			= NULL;
	ftc.dwAspect	= DVASPECT_CONTENT;
	ftc.lindex		= -1;
	ftc.tymed		= TYMED_HGLOBAL;

	hr = pDataObject->GetData(&ftc,&stg);

	if(hr == S_OK)
	{
		pdf = (DROPFILES *)GlobalLock(stg.hGlobal);

		if(pdf != NULL)
		{
			/* Request a count of the number of files that have been dropped. */
			nDroppedFiles = DragQueryFile((HDROP)pdf,0xFFFFFFFF,NULL,NULL);

			if(iDroppedItem < nDroppedFiles)
			{
				/* Determine the name of the first dropped file. */
				DragQueryFile((HDROP)pdf,iDroppedItem,szFullFileName,
					SIZEOF_ARRAY(szFullFileName));

				/* TODO: Compare against sub-folders? (i.e. path may
				need to be adjusted if the dragged item is currently
				over a folder). */
				std::wstring destDirectory = GetDirectory();

				if (PathIsSameRoot(destDirectory.c_str(), szFullFileName))
				{
					bOnSameDrive = TRUE;
				}
				else
				{
					bOnSameDrive = FALSE;
				}
			}

			GlobalUnlock(stg.hGlobal);
		}
	}

	return bOnSameDrive;
}

/*
Cases:
- The dragged item is NOT over an item in the listview.
Any previously selected item should revert to its
previous state (i.e. selected if it was previously
selected; not selected if it wasn't).
- The dragged item IS over an item in the listview.
Three sub-cases:
(a) The item its over is a file (or somewhere where
the dragged file can't be dropped). Do not alter
the selection of the file in the listview.
(b) The item is over a folder. In this case, save
the items previous selection state, and select
it.
(c) The item is over itself. Do not alter selection
in any way.

Deselction occurs when:
1. The dragged file is dropped.
2. The dragged file moves over the background of the
listview, or over a file it cannot be dragged into.
3. The dragged file moves over an item it can be
dragged into (and this item is different from the
previous item that was selected).
4. The drag and drop operation is canceled.

Therefore, the ONLY time selection is maintained is when
the dragged item remains over the same selected item.

These are handled by:
1. Drop()
2. DragOver()
3. DragOver()
4. DragLeave()
*/
void CShellBrowser::HandleDragSelection(const POINT *ppt)
{
	LVHITTESTINFO	info;
	BOOL			bClash = FALSE;
	BOOL			bOverFolder = FALSE;
	BOOL			bOverItem = FALSE;
	int				iItem;
	int				iInternalIndex = -1;

	info.pt = *ppt;
	ListView_HitTest(m_hListView,&info);

	if(!(info.flags & LVHT_NOWHERE) && info.iItem != -1)
	{
		LVITEM lvItem;

		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= info.iItem;
		lvItem.iSubItem	= 0;
		ListView_GetItem(m_hListView,&lvItem);

		iInternalIndex = (int)lvItem.lParam;

		if(iInternalIndex != -1)
		{
			bOverItem = TRUE;

			if(m_bOverFolder && info.iItem == m_iDropFolder)
			{
				bOverFolder = TRUE;
			}
		}
	}

	if(!bOverFolder)
	{
		/* The dragged item is not over the previously
		selected item. Revert the selection state on
		the old item. */
		if(m_bOverFolder)
		{
			/* Only deselect the previous item if it
			was originally deselcted. */
			if(m_bDeselectDropFolder)
			{
				ListView_SetItemState(m_hListView,
					m_iDropFolder,0,LVIS_SELECTED);
			}
		}

		m_bOverFolder = FALSE;

		/* Now, if the dragged item is over an item in
		the listview, test it to see whether or not it
		is a folder. */
		if(bOverItem)
		{
			/* Check for a clash (only if over a folder). */
			if((m_itemInfoMap.at(iInternalIndex).wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				== FILE_ATTRIBUTE_DIRECTORY)
			{
				if(m_bDragging)
				{
					std::list<DraggedFile_t>::iterator itr;

					for(itr = m_DraggedFilesList.begin();itr != m_DraggedFilesList.end();itr++)
					{
						iItem = LocateFileItemIndex(itr->szFileName);

						if(info.iItem == iItem)
							bClash = TRUE;
					}
				}

				/* The dragged item is over a valid folder
				(that isn't itself). */
				if(!bClash)
				{
					UINT mask;

					/* Get the original selection state of the item. */
					mask = ListView_GetItemState(m_hListView,info.iItem,LVIS_SELECTED);

					if((mask & LVIS_SELECTED) == LVIS_SELECTED)
						m_bDeselectDropFolder = FALSE;
					else
						m_bDeselectDropFolder = TRUE;

					/* Select the item. */
					ListView_SetItemState(m_hListView,info.iItem,
						LVIS_SELECTED,LVIS_SELECTED);

					m_iDropFolder = info.iItem;
					m_bOverFolder = TRUE;
				}
			}
		}
	}
}

HRESULT _stdcall CShellBrowser::DragLeave(void)
{
	m_pDropTargetHelper->DragLeave();

	NListView::ListView_PositionInsertMark(m_hListView,NULL);

	if(m_bDeselectDropFolder)
	{
		/* Deselect any folder that may have been selected during dragging. */
		ListView_SetItemState(m_hListView,m_iDropFolder,0,LVIS_SELECTED);
	}

	m_bPerformingDrag = FALSE;

	return S_OK;
}

void CShellBrowser::OnDropFile(const std::list<std::wstring> &PastedFileList, const POINT *ppt)
{
	DroppedFile_t DroppedFile;
	POINT ptOrigin;
	POINT LocalDropPoint;

	/* Don't reposition the file if it was dropped
	in a subfolder. */
	if(!m_bOverFolder)
	{
		ListView_GetOrigin(m_hListView,&ptOrigin);

		LocalDropPoint = *ppt;

		ScreenToClient(m_hListView,(LPPOINT)&LocalDropPoint);

		/* The location of each of the dropped items will be the same. */
		DroppedFile.DropPoint.x = ptOrigin.x + LocalDropPoint.x;
		DroppedFile.DropPoint.y = ptOrigin.y + LocalDropPoint.y;

		for(const auto &strFilename : PastedFileList)
		{
			StringCchCopy(DroppedFile.szFileName,
				SIZEOF_ARRAY(DroppedFile.szFileName),strFilename.c_str());
			PathStripPath(DroppedFile.szFileName);

			m_DroppedFileNameList.push_back(DroppedFile);
		}
	}
}

/*
Causes any dragged files to be dropped in the current
directory.

- If the files are been dragged locally, they will simply
be moved.
- If the files are been dragged from another directory,
they will be copied/moved to the current directory.

Modifier keys (from http://blogs.msdn.com/oldnewthing/archive/2004/11/12/256472.aspx):
If Ctrl+Shift (or Alt) are held down, then the operation creates a shortcut. 
If Shift is held down, then the operation is a move. 
If Ctrl is held down, then the operation is a copy. 
If no modifiers are held down and the source and destination are on the same drive, then the operation is a move. 
If no modifiers are held down and the source and destination are on different drives, then the operation is a copy.
*/
HRESULT _stdcall CShellBrowser::Drop(IDataObject *pDataObject,
DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect)
{
	FORMATETC		ftcHDrop = {CF_HDROP,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	STGMEDIUM		stg;
	DROPFILES		*pdf = NULL;
	POINT			pt;
	HRESULT			hr;
	DWORD			dwEffect;
	int				nDroppedFiles;

	/* Need to remove the drag image before any files are copied/moved.
	This is because a copy/replace dialog may need to shown (if there
	is a collision), and the drag image no longer needs to be there.
	The insertion mark may stay until the end. */
	m_pDropTargetHelper->Drop(pDataObject,(POINT *)&pt,*pdwEffect);

	if(m_bDeselectDropFolder)
	{
		ListView_SetItemState(m_hListView,
			m_iDropFolder,0,LVIS_SELECTED);
	}

	m_bPerformingDrag = FALSE;

	pt.x = ptl.x;
	pt.y = ptl.y;

	std::wstring destDirectory = GetDirectory();

	TCHAR finalDestDirectory[MAX_PATH];
	StringCchCopy(finalDestDirectory, std::size(finalDestDirectory), destDirectory.c_str());

	/* If the item(s) have been dropped over a folder in the
	listview, append the folders name onto the destination path. */
	if(m_bOverFolder)
	{
		LVITEM lvItem;

		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= m_iDropFolder;
		lvItem.iSubItem	= 0;
		ListView_GetItem(m_hListView,&lvItem);

		PathAppend(finalDestDirectory, m_itemInfoMap.at((int)lvItem.lParam).wfd.cFileName);
	}

	if(m_bDataAccept)
	{
		BOOL bHandled = FALSE;

		if(m_DragType == DragType::LeftClick && m_bDragging && !m_bOverFolder)
		{
			hr = pDataObject->GetData(&ftcHDrop,&stg);

			if(hr == S_OK)
			{
				pdf = (DROPFILES *)GlobalLock(stg.hGlobal);

				if(pdf != NULL)
				{
					nDroppedFiles = DragQueryFile((HDROP)pdf,0xFFFFFFFF,NULL,NULL);

					/* The drop effect will be the same for all files
					that are been dragged locally. */
					dwEffect = DetermineDragEffect(grfKeyState,*pdwEffect,
						m_bDataAccept,m_bOnSameDrive);

					if(dwEffect == DROPEFFECT_MOVE)
					{
						POINT point;

						point.x = pt.x;
						point.y = pt.y;
						RepositionLocalFiles(&point);

						bHandled = TRUE;
					}
				}
			}
		}

		if(!bHandled)
		{
			CDropHandler *pDropHandler = CDropHandler::CreateNew();

			/* The drop handler will call Release(), so we
			need to AddRef() here. In the future, this should
			be switched to an independent class. */
			AddRef();

			pDropHandler->Drop(pDataObject,
				grfKeyState,ptl,pdwEffect,m_hListView,
				m_DragType,finalDestDirectory,this,FALSE);

			/* When dragging and dropping, any dropped items
			will be selected, while any previously selected
			items will be deselected. */
			/* TODO: May need to modify this in case there
			was an error with the drop (i.e. don't deselect current
			files if nothing was actually copied/moved). */
			if(!m_bOverFolder)
			{
				NListView::ListView_SelectAllItems(m_hListView,FALSE);
			}

			pDropHandler->Release();
		}
	}

	/*if(m_bDeselectDropFolder)
	{
		ListView_SetItemState(m_hListView,
			m_iDropFolder,0,LVIS_SELECTED);
	}*/

	/* Remove the insertion mark from the listview. */
	NListView::ListView_PositionInsertMark(m_hListView,NULL);

	//m_bPerformingDrag = FALSE;

	return S_OK;
}

/* TODO: This isn't declared. */
int CALLBACK SortTemporaryStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CShellBrowser *pShellBrowser = reinterpret_cast<CShellBrowser *>(lParamSort);
	return pShellBrowser->SortTemporary(lParam1,lParam2);
}

int CALLBACK CShellBrowser::SortTemporary(LPARAM lParam1,LPARAM lParam2)
{
	return m_itemInfoMap.at(static_cast<int>(lParam1)).iRelativeSort -
		m_itemInfoMap.at(static_cast<int>(lParam2)).iRelativeSort;
}

void CShellBrowser::RepositionLocalFiles(const POINT *ppt)
{
	std::list<DraggedFile_t>::iterator	itr;
	POINT							pt;
	POINT							ptOrigin;
	int								iItem;

	pt = *ppt;
	ScreenToClient(m_hListView,&pt);

	/* The auto arrange style must be off for the items
	to be moved. Therefore, if the style is on, turn it
	off, move the items, and the turn it back on. */
	if(m_folderSettings.autoArrange)
		NListView::ListView_SetAutoArrange(m_hListView,FALSE);

	for(itr = m_DraggedFilesList.begin();
		itr != m_DraggedFilesList.end();itr++)
	{
		iItem = LocateFileItemIndex(itr->szFileName);

		if(iItem != -1)
		{
			if(m_folderSettings.viewMode == +ViewMode::Details)
			{
				LVITEM lvItem;
				POINT ptItem;
				BOOL bBelowPreviousItem = TRUE;
				BOOL bRes;
				int iInsert = 0;
				int iSort = 0;
				int nItems;
				int i = 0;

				nItems = ListView_GetItemCount(m_hListView);

				/* Find the closest item to the dropped item. */
				for(i = 0;i < nItems;i++)
				{
					ListView_GetItemPosition(m_hListView,i,&ptItem);

					if(bBelowPreviousItem && (pt.y - ptItem.y) < 0)
					{
						iInsert = i - 1;
						break;
					}

					/* If the dropped item is below the last item,
					it will be moved to the last position. */
					if(i == (nItems - 1))
					{
						iInsert = nItems;
					}

					bBelowPreviousItem = (pt.y - ptItem.y) > 0;
				}

				for(i = 0;i < nItems;i++)
				{
					lvItem.mask		= LVIF_PARAM;
					lvItem.iItem	= i;
					lvItem.iSubItem	= 0;
					bRes = ListView_GetItem(m_hListView,&lvItem);

					if(bRes)
					{
						if(i == iItem)
						{
							m_itemInfoMap.at((int)lvItem.lParam).iRelativeSort = iInsert;
						}
						else
						{
							if(iSort == iInsert)
								iSort++;

							m_itemInfoMap.at((int)lvItem.lParam).iRelativeSort = iSort;
						}
					}

					iSort++;
				}

				ListView_SortItems(m_hListView,SortTemporaryStub,(LPARAM)this);
			}
			else
			{
				/* If auto arrange is on, the dropped item(s) will
				have to be 'snapped' to the nearest item position.
				Otherwise, they may simply be placed where they are
				dropped. */
				if(m_folderSettings.autoArrange)
				{
					LVFINDINFO lvfi;
					LVHITTESTINFO lvhti;
					RECT rcItem;
					POINT ptNext;
					BOOL bRowEnd = FALSE;
					BOOL bRowStart = FALSE;
					int iNext;
					int iHitItem;
					int nItems;

					lvhti.pt = pt;
					iHitItem = ListView_HitTest(m_hListView,&lvhti);

					/* Based on ListView_PositionInsertMark() code. */
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
					ListView_GetOrigin(m_hListView,&ptOrigin);

					/* ListView may be scrolled horizontally or vertically. */
					ListView_SetItemPosition32(m_hListView,
						iItem,ptOrigin.x + pt.x - m_ptDraggedOffset.x,
						ptOrigin.y + pt.y - m_ptDraggedOffset.y);
				}
			}
		}
	}

	if(m_folderSettings.autoArrange)
		NListView::ListView_SetAutoArrange(m_hListView,TRUE);

	m_bDragging = FALSE;

	m_bPerformingDrag = FALSE;
}

void CShellBrowser::ScrollListViewFromCursor(HWND hListView, const POINT *CursorPos)
{
	RECT		rc;
	LONG_PTR	fStyle;

	fStyle = GetWindowLongPtr(hListView,GWL_STYLE);

	GetClientRect(hListView,&rc);

	/* The listview can be scrolled only if there
	is a scrollbar present. */
	if((fStyle & WS_HSCROLL) == WS_HSCROLL)
	{
		if(CursorPos->x < MIN_X_POS)
			ListView_Scroll(hListView,-X_SCROLL_AMOUNT,0);
		else if(CursorPos->x > (rc.right - MIN_X_POS))
			ListView_Scroll(hListView,X_SCROLL_AMOUNT,0);
	}
	
	/* The listview can be scrolled only if there
	is a scrollbar present. */
	if((fStyle & WS_VSCROLL) == WS_VSCROLL)
	{
		if(CursorPos->y < MIN_Y_POS)
			ListView_Scroll(hListView,0,-Y_SCROLL_AMOUNT);
		else if(CursorPos->y > (rc.bottom - MIN_Y_POS))
			ListView_Scroll(hListView,0,Y_SCROLL_AMOUNT);
	}
}