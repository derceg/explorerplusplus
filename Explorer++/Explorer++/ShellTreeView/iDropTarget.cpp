// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellTreeView.h"
#include "../Helper/DropHandler.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"

const LONG MIN_X_POS = 10;
const LONG MIN_Y_POS = 10;

const UINT DRAGEXPAND_TIMER_ID = 1;
const UINT DRAGEXPAND_TIMER_ELAPSE = 800;

const UINT DRAGSCROLL_TIMER_ID = 2;
const UINT DRAGSCROLL_TIMER_ELAPSE = 1000;

void CALLBACK DragExpandTimerProc(HWND hwnd,UINT uMsg,
UINT_PTR idEvent,DWORD dwTime);
void CALLBACK DragScrollTimerProc(HWND hwnd,UINT uMsg,
UINT_PTR idEvent,DWORD dwTime);

HTREEITEM	g_hExpand = nullptr;
BOOL		g_bAllowScroll = FALSE;

HRESULT _stdcall ShellTreeView::DragEnter(IDataObject *pDataObject,
DWORD grfKeyState,POINTL pt,DWORD *pdwEffect)
{
	m_pDataObject = pDataObject;

	m_bDragging = TRUE;

	std::list<FORMATETC> ftcList;
	DropHandler::GetDropFormats(ftcList);

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

		GetCurrentDragEffect(grfKeyState,*pdwEffect,&pt);
	}
	else
	{
		/* The clipboard contains data that we cannot copy/move. */
		m_bDataAccept	= FALSE;
		*pdwEffect		= DROPEFFECT_NONE;
	}

	g_hExpand = nullptr;

	SetTimer(m_hTreeView,DRAGSCROLL_TIMER_ID,DRAGSCROLL_TIMER_ELAPSE,
		DragScrollTimerProc);

	if(grfKeyState & MK_LBUTTON)
		m_DragType = DragType::LeftClick;
	else if(grfKeyState & MK_RBUTTON)
		m_DragType = DragType::RightClick;

	/* Notify the drop target helper that an object has been dragged into
	the window. */
	m_pDropTargetHelper->DragEnter(m_hTreeView,pDataObject,(POINT *)&pt,*pdwEffect);

	return S_OK;
}

void CALLBACK DragScrollTimerProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
{
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(idEvent);
	UNREFERENCED_PARAMETER(dwTime);

	g_bAllowScroll = TRUE;

	KillTimer(hwnd,DRAGSCROLL_TIMER_ID);
}

DWORD ShellTreeView::GetCurrentDragEffect(DWORD grfKeyState,DWORD dwCurrentEffect,POINTL *ptl)
{
	TVHITTESTINFO	tvhi;
	HTREEITEM		hItem;
	DWORD			dwEffect;
	BOOL			bOnSameDrive;

	tvhi.pt.x = ptl->x;
	tvhi.pt.y = ptl->y;
	ScreenToClient(m_hTreeView,&tvhi.pt);

	hItem = (HTREEITEM)SendMessage(m_hTreeView,TVM_HITTEST,0,(LPARAM)&tvhi);

	if(hItem != nullptr)
	{
		bOnSameDrive = CheckItemLocations(m_pDataObject,hItem,0);

		dwEffect = DetermineDragEffect(grfKeyState,
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
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(idEvent);
	UNREFERENCED_PARAMETER(dwTime);

	TreeView_Expand(hwnd,g_hExpand,TVE_EXPAND);

	KillTimer(hwnd,DRAGEXPAND_TIMER_ID);
}

HRESULT _stdcall ShellTreeView::DragOver(DWORD grfKeyState,POINTL pt,DWORD *pdwEffect)
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
	if(!(tvht.flags & LVHT_NOWHERE) && (tvht.hItem != nullptr))
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
BOOL ShellTreeView::CheckItemLocations(IDataObject *pDataObject,HTREEITEM hItem,
int iDroppedItem)
{
	FORMATETC	ftc;
	STGMEDIUM	stg;
	DROPFILES	*pdf = nullptr;
	TCHAR		szDestDirectory[MAX_PATH];
	TCHAR		szFullFileName[MAX_PATH];
	HRESULT		hr;
	BOOL		bOnSameDrive = FALSE;
	int			nDroppedFiles;

	ftc.cfFormat	= CF_HDROP;
	ftc.ptd			= nullptr;
	ftc.dwAspect	= DVASPECT_CONTENT;
	ftc.lindex		= -1;
	ftc.tymed		= TYMED_HGLOBAL;

	hr = pDataObject->GetData(&ftc,&stg);

	if(hr == S_OK)
	{
		pdf = (DROPFILES *)GlobalLock(stg.hGlobal);

		if(pdf != nullptr)
		{
			/* Request a count of the number of files that have been dropped. */
			nDroppedFiles = DragQueryFile((HDROP)pdf,0xFFFFFFFF, nullptr,NULL);

			if(iDroppedItem < nDroppedFiles)
			{
				auto pidlDest = GetItemPidl(hItem);

				if(pidlDest)
				{
					/* Determine the name of the first dropped file. */
					DragQueryFile((HDROP)pdf,iDroppedItem,szFullFileName,
						SIZEOF_ARRAY(szFullFileName));

					GetDisplayName(pidlDest.get(),szDestDirectory,SIZEOF_ARRAY(szDestDirectory),SHGDN_FORPARSING);

					bOnSameDrive = PathIsSameRoot(szDestDirectory,szFullFileName);
				}
			}

			GlobalUnlock(stg.hGlobal);
		}
	}

	return bOnSameDrive;
}

HRESULT _stdcall ShellTreeView::DragLeave()
{
	RestoreState();

	KillTimer(m_hTreeView,DRAGEXPAND_TIMER_ID);

	m_pDropTargetHelper->DragLeave();

	return S_OK;
}

HRESULT _stdcall ShellTreeView::Drop(IDataObject *pDataObject,DWORD grfKeyState,
POINTL pt,DWORD *pdwEffect)
{
	KillTimer(m_hTreeView,DRAGEXPAND_TIMER_ID);

	TVHITTESTINFO tvht;
	tvht.pt.x = pt.x;
	tvht.pt.y = pt.y;

	ScreenToClient(m_hTreeView,(LPPOINT)&tvht.pt);
	TreeView_HitTest(m_hTreeView,&tvht);

	/* Is the mouse actually over an item? */
	if(!(tvht.flags & LVHT_NOWHERE) && (tvht.hItem != nullptr) && m_bDataAccept)
	{
		auto pidlDirectory = GetItemPidl(tvht.hItem);

		TCHAR szDestDirectory[MAX_PATH];
		GetDisplayName(pidlDirectory.get(),szDestDirectory,SIZEOF_ARRAY(szDestDirectory),SHGDN_FORPARSING);

		DropHandler *pDropHandler = DropHandler::CreateNew();
		pDropHandler->Drop(pDataObject,
			grfKeyState,pt,pdwEffect,m_hTreeView,
			m_DragType,szDestDirectory, nullptr,FALSE);
		pDropHandler->Release();
	}

	RestoreState();

	m_pDropTargetHelper->Drop(pDataObject,(POINT *)&pt,*pdwEffect);

	return S_OK;
}

void ShellTreeView::RestoreState()
{
	TreeView_Select(m_hTreeView,nullptr,TVGN_DROPHILITE);

	g_bAllowScroll = FALSE;
	m_bDragging = FALSE;
}