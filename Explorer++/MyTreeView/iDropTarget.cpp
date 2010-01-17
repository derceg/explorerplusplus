/******************************************************************
 *
 * Project: MyTreeView
 * File: iDropTarget.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides support for acting as a drop target.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "../Helper/Helper.h"
#include "MyTreeView.h"
#include "MyTreeViewInternal.h"


#define MIN_X_POS	10
#define MIN_Y_POS	10

void CALLBACK DragExpandTimerProc(HWND hwnd,UINT uMsg,
UINT_PTR idEvent,DWORD dwTime);
void CALLBACK DragScrollTimerProc(HWND hwnd,UINT uMsg,
UINT_PTR idEvent,DWORD dwTime);

HTREEITEM	g_hExpand = NULL;
BOOL		g_bAllowScroll = FALSE;

HRESULT _stdcall CMyTreeView::DragEnter(IDataObject *pDataObject,
DWORD grfKeyState,POINTL pt,DWORD *pdwEffect)
{
	FORMATETC	ftc = {CF_HDROP,0,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	HRESULT		hr;

	m_pDataObject = pDataObject;

	m_bDragging = TRUE;

	/* Check whether the drop source has the type of data (CF_HDROP)
	that is needed for this drag operation. */
	hr = pDataObject->QueryGetData(&ftc);

	/* DON'T use SUCCEEDED (QueryGetData() will return S_FALSE on
	failure). */
	if(hr == S_OK)
	{
		/* The clipboard contains data that we can copy/move. */
		m_bDataAccept	= TRUE;

		GetCurrentDragEffect(grfKeyState,*pdwEffect,&pt);
	}
	else
	{
		/* The clipboard contains data that we cannot copy/move. */
		m_bDataAccept	= FALSE;
		*pdwEffect		= DROPEFFECT_NONE;
	}

	g_hExpand = NULL;

	SetTimer(m_hTreeView,DRAGSCROLL_TIMER_ID,DRAGSCROLL_TIMER_ELAPSE,
		DragScrollTimerProc);

	/* Notify the drop target helper that an object has been dragged into
	the window. */
	m_pDropTargetHelper->DragEnter(m_hTreeView,pDataObject,(POINT *)&pt,*pdwEffect);

	return S_OK;
}

void CALLBACK DragScrollTimerProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
{
	g_bAllowScroll = TRUE;

	KillTimer(hwnd,DRAGSCROLL_TIMER_ID);
}

DWORD CMyTreeView::GetCurrentDragEffect(DWORD grfKeyState,DWORD dwCurrentEffect,POINTL *ptl)
{
	TVHITTESTINFO	tvhi;
	HTREEITEM		hItem;
	DWORD			dwEffect;
	BOOL			bOnSameDrive;

	tvhi.pt.x = ptl->x;
	tvhi.pt.y = ptl->y;
	ScreenToClient(m_hTreeView,&tvhi.pt);

	hItem = (HTREEITEM)SendMessage(m_hTreeView,TVM_HITTEST,0,(LPARAM)&tvhi);

	if(hItem != NULL)
	{
		bOnSameDrive = CheckItemLocations(m_pDataObject,hItem,0);

		dwEffect = DetermineCurrentDragEffect(grfKeyState,
			dwCurrentEffect,m_bDataAccept,bOnSameDrive);
	}
	else
	{
		dwEffect = DROPEFFECT_NONE;
	}

	return dwEffect;
}

void CALLBACK DragExpandTimerProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
{
	TreeView_Expand(hwnd,g_hExpand,TVE_EXPAND);

	KillTimer(hwnd,DRAGEXPAND_TIMER_ID);
}

HRESULT _stdcall CMyTreeView::DragOver(DWORD grfKeyState,POINTL pt,DWORD *pdwEffect)
{
	TVHITTESTINFO tvht;
	RECT rc;

	*pdwEffect = GetCurrentDragEffect(grfKeyState,*pdwEffect,&pt);

	/* Notify the drop helper of the current operation. */
	m_pDropTargetHelper->DragOver((LPPOINT)&pt,*pdwEffect);

	ScreenToClient(m_hTreeView,(LPPOINT)&pt);

	if(g_bAllowScroll)
	{
		GetClientRect(m_hTreeView,&rc);

		if(pt.x < MIN_X_POS)
			SendMessage(m_hTreeView,WM_HSCROLL,SB_LINELEFT,NULL);
		else if(pt.x > (rc.right - MIN_X_POS))
			SendMessage(m_hTreeView,WM_HSCROLL,SB_LINERIGHT,NULL);

		if(pt.y < MIN_Y_POS)
			SendMessage(m_hTreeView,WM_VSCROLL,SB_LINEUP,NULL);
		else if(pt.y > (rc.bottom - MIN_Y_POS))
			SendMessage(m_hTreeView,WM_VSCROLL,SB_LINEDOWN,NULL);
	}

	tvht.pt.x	= pt.x;
	tvht.pt.y	= pt.y;

	TreeView_HitTest(m_hTreeView,&tvht);

	/* Is the mouse actually over an item? */
	if(!(tvht.flags & LVHT_NOWHERE) && (tvht.hItem != NULL))
	{
		/* The mouse is over an item, so select that item. */
		TreeView_Select(m_hTreeView,tvht.hItem,TVGN_DROPHILITE);

		if(g_hExpand != tvht.hItem)
		{
			g_hExpand = tvht.hItem;
			SetTimer(m_hTreeView,DRAGEXPAND_TIMER_ID,DRAGEXPAND_TIMER_ELAPSE,
				DragExpandTimerProc);
		}
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
BOOL CMyTreeView::CheckItemLocations(IDataObject *pDataObject,HTREEITEM hItem,
int iDroppedItem)
{
	FORMATETC	ftc;
	STGMEDIUM	stg;
	DROPFILES	*pdf = NULL;
	LPITEMIDLIST	pidlDest = NULL;
	TCHAR		szDestDirectory[MAX_PATH];
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
				pidlDest = BuildPath(hItem);

				if(pidlDest != NULL)
				{
					/* Determine the name of the first dropped file. */
					DragQueryFile((HDROP)pdf,iDroppedItem,szFullFileName,
						SIZEOF_ARRAY(szFullFileName));

					GetDisplayName(pidlDest,szDestDirectory,SHGDN_FORPARSING);

					bOnSameDrive = PathIsSameRoot(szDestDirectory,szFullFileName);

					CoTaskMemFree(pidlDest);
				}
			}

			GlobalUnlock(stg.hGlobal);
		}
	}

	return bOnSameDrive;
}

HRESULT _stdcall CMyTreeView::DragLeave(void)
{
	RestoreState();

	KillTimer(m_hTreeView,DRAGEXPAND_TIMER_ID);

	m_pDropTargetHelper->DragLeave();

	return S_OK;
}

HRESULT _stdcall CMyTreeView::Drop(IDataObject *pDataObject,DWORD grfKeyState,
POINTL pt,DWORD *pdwEffect)
{
	FORMATETC		ftc;
	STGMEDIUM		stg;
	TVHITTESTINFO	tvht;
	LPITEMIDLIST	pidlDirectory = NULL;
	DROPFILES		*pdf = NULL;
	TCHAR			szDestDirectory[MAX_PATH + 1];
	HRESULT			hr;

	KillTimer(m_hTreeView,DRAGEXPAND_TIMER_ID);

	ScreenToClient(m_hTreeView,(LPPOINT)&pt);

	tvht.pt.x	= pt.x;
	tvht.pt.y	= pt.y;

	TreeView_HitTest(m_hTreeView,&tvht);

	/* Is the mouse actually over an item? */
	if(!(tvht.flags & LVHT_NOWHERE) && (tvht.hItem != NULL))
	{
		/* The mouse is over an item, so drop the file
		into the selected folder (if possible). */
		ftc.cfFormat	= CF_HDROP;
		ftc.ptd			= NULL;
		ftc.dwAspect	= DVASPECT_CONTENT;
		ftc.lindex		= -1;
		ftc.tymed		= TYMED_HGLOBAL;

		/* Does the dropped object contain the type of
		data we need? */
		hr = pDataObject->GetData(&ftc,&stg);

		if(hr == S_OK)
		{
			if(m_DragType == DRAG_TYPE_RIGHTCLICK)
			{
				IShellFolder *pDesktop = NULL;
				IShellFolder *pShellFolder = NULL;
				IDropTarget *pDrop = NULL;
				DWORD dwe;

				pidlDirectory = BuildPath(tvht.hItem);

				hr = SHGetDesktopFolder(&pDesktop);

				if(SUCCEEDED(hr))
				{
					hr = pDesktop->BindToObject(pidlDirectory,0,IID_IShellFolder,(void **)&pShellFolder);

					if(SUCCEEDED(hr))
					{
						dwe = *pdwEffect;

						hr = pShellFolder->CreateViewObject(m_hTreeView,IID_IDropTarget,(void **)&pDrop);

						if(SUCCEEDED(hr))
						{
							pDrop->DragEnter(pDataObject,MK_RBUTTON,pt,&dwe);

							dwe = *pdwEffect;
							pDrop->Drop(pDataObject,grfKeyState,pt,&dwe);

							pDrop->DragLeave();

							pDrop->Release();
						}

						pShellFolder->Release();
					}

					pDesktop->Release();

					CoTaskMemFree(pidlDirectory);
				}
			}
			else
			{
				pdf = (DROPFILES *)GlobalLock(stg.hGlobal);

				if(pdf != NULL)
				{
					pidlDirectory = BuildPath(tvht.hItem);

					GetDisplayName(pidlDirectory,szDestDirectory,SHGDN_FORPARSING);

					CoTaskMemFree(pidlDirectory);

					szDestDirectory[lstrlen(szDestDirectory) + 1] = '\0';

					CopyDroppedFiles(pdf,(LPPOINT)&pt,
						szDestDirectory,grfKeyState,pdwEffect);

					GlobalUnlock(stg.hGlobal);
				}
			}
		}
	}

	RestoreState();

	m_pDropTargetHelper->Drop(pDataObject,(POINT *)&pt,*pdwEffect);

	return S_OK;
}

void CMyTreeView::CopyDroppedFiles(DROPFILES *pdf,POINT *ppt,
TCHAR *szDestDirectory,DWORD grfKeyState,DWORD *pdwEffect)
{
	IBufferManager	*pbmCopy = NULL;
	IBufferManager	*pbmMove = NULL;
	TCHAR			szFullFileName[MAX_PATH];
	DWORD			dwEffect;
	int				nDroppedFiles;
	int				i = 0;

	pbmCopy = new CBufferManager();
	pbmMove = new CBufferManager();

	nDroppedFiles = DragQueryFile((HDROP)pdf,0xFFFFFFFF,NULL,NULL);

	for(i = 0;i < nDroppedFiles;i++)
	{
		/* Determine the name of the dropped file. */
		DragQueryFile((HDROP)pdf,i,szFullFileName,
			SIZEOF_ARRAY(szFullFileName));

		POINTL ptl;

		ptl.x = ppt->x;
		ptl.y = ppt->y;

		dwEffect = GetCurrentDragEffect(grfKeyState,*pdwEffect,&ptl);

		if(dwEffect == DROPEFFECT_MOVE)
			pbmMove->WriteListEntry(szFullFileName);
		else if(dwEffect == DROPEFFECT_COPY)
			pbmCopy->WriteListEntry(szFullFileName);
		else if(dwEffect == DROPEFFECT_LINK)
			CreateShortcutToDroppedFile(szDestDirectory,szFullFileName);
	}

	CopyDroppedFilesInternal(pbmCopy,szDestDirectory,TRUE,FALSE);
	CopyDroppedFilesInternal(pbmMove,szDestDirectory,FALSE,FALSE);

	pbmCopy->Release();
	pbmMove->Release();
}

void CMyTreeView::CopyDroppedFilesInternal(IBufferManager *pbm,
TCHAR *szDestDirectory,BOOL bCopy,BOOL bRenameOnCollision)
{
	SHFILEOPSTRUCT	shfo;
	TCHAR			*szFileNameList = NULL;
	DWORD			dwBufferSize;

	pbm->QueryBufferSize(&dwBufferSize);

	if(dwBufferSize > 1)
	{
		szFileNameList = (TCHAR *)malloc(dwBufferSize * sizeof(TCHAR));

		if(szFileNameList != NULL)
		{
			pbm->QueryBuffer(szFileNameList,dwBufferSize);

			shfo.hwnd	= m_hParent;
			shfo.wFunc	= bCopy == TRUE ? FO_COPY : FO_MOVE;
			shfo.pFrom	= szFileNameList;
			shfo.pTo	= szDestDirectory;
			shfo.fFlags	= bRenameOnCollision == TRUE ? FOF_RENAMEONCOLLISION : 0;
			SHFileOperation(&shfo);

			free(szFileNameList);
		}
	}
}

void CMyTreeView::CreateShortcutsToDroppedFiles(DROPFILES *pdf,
TCHAR *szDestDirectory,int nDroppedFiles)
{
	TCHAR	szFullFileName[MAX_PATH];
	TCHAR	szLink[MAX_PATH];
	TCHAR	szFileName[MAX_PATH];
	int		i = 0;

	for(i = 0;i < nDroppedFiles;i++)
	{
		/* Determine the name of the dropped file. */
		DragQueryFile((HDROP)pdf,i,szFullFileName,
			SIZEOF_ARRAY(szFullFileName));

		StringCchCopy(szFileName,SIZEOF_ARRAY(szFileName),szFullFileName);
		PathStripPath(szFileName);
		PathRenameExtension(szFileName,_T(".lnk"));
		StringCchCopy(szLink,SIZEOF_ARRAY(szLink),szDestDirectory);
		PathAppend(szLink,szFileName);

		CreateLinkToFile(szFullFileName,szLink,EMPTY_STRING);
	}
}

void CMyTreeView::CreateShortcutToDroppedFile(TCHAR *szDestDirectory,
TCHAR *szFullFileName)
{
	TCHAR	szLink[MAX_PATH];
	TCHAR	szFileName[MAX_PATH];

	StringCchCopy(szFileName,SIZEOF_ARRAY(szFileName),szFullFileName);
	PathStripPath(szFileName);
	PathRenameExtension(szFileName,_T(".lnk"));
	StringCchCopy(szLink,SIZEOF_ARRAY(szLink),szDestDirectory);
	PathAppend(szLink,szFileName);

	CreateLinkToFile(szFullFileName,szLink,EMPTY_STRING);
}

void CMyTreeView::RestoreState(void)
{
	TreeView_Select(m_hTreeView,NULL,TVGN_DROPHILITE);

	g_bAllowScroll = FALSE;
	m_bDragging = FALSE;
}