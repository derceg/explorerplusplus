/******************************************************************
 *
 * Project: Explorer++
 * File: TabDropHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Manages drag and drop for the tabs.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "Explorer++.h"


void CALLBACK TabDragTimerProc(HWND hwnd,UINT uMsg,
UINT_PTR idEvent,DWORD dwTime);

BOOL	g_bTabDragTimerElapsed;

HRESULT _stdcall Explorerplusplus::DragEnter(IDataObject *pDataObject,
DWORD grfKeyState,POINTL pt,DWORD *pdwEffect)
{
	m_iTabDragTab = m_iTabSelectedItem;
	g_bTabDragTimerElapsed = FALSE;

	std::list<FORMATETC> ftcList;
	CDropHandler::GetDropFormats(&ftcList);

	BOOL bDataAccept = FALSE;

	/* Check whether the drop source has the type of data
	that is needed for this drag operation. */
	for each(auto ftc in ftcList)
	{
		if(pDataObject->QueryGetData(&ftc) == S_OK)
		{
			bDataAccept = TRUE;
			break;
		}
	}

	if(bDataAccept)
	{
		m_bDataAccept	= TRUE;

		GetSourceFileName(pDataObject);

		TCHITTESTINFO	tchi;
		TCITEM			tcItem;
		BOOL			bOnSameDrive;
		int				iTab;

		tchi.pt.x = pt.x;
		tchi.pt.y = pt.y;
		ScreenToClient(m_hTabCtrl,&tchi.pt);
		iTab = TabCtrl_HitTest(m_hTabCtrl,&tchi);

		if(iTab != -1)
		{
			tcItem.mask = TCIF_PARAM;
			TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

			bOnSameDrive = CheckItemLocations((int)tcItem.lParam);

			*pdwEffect = DetermineCurrentDragEffect(grfKeyState,
				*pdwEffect,m_bDataAccept,bOnSameDrive);
		}
		else
		{
			*pdwEffect = DROPEFFECT_NONE;
		}
	}
	else
	{
		m_bDataAccept	= FALSE;
		*pdwEffect		= DROPEFFECT_NONE;
	}

	if(grfKeyState & MK_LBUTTON)
		m_DragType = DRAG_TYPE_LEFTCLICK;
	else if(grfKeyState & MK_RBUTTON)
		m_DragType = DRAG_TYPE_RIGHTCLICK;

	m_pDropTargetHelper->DragEnter(m_hTabCtrl,pDataObject,(POINT *)&pt,*pdwEffect);

	return S_OK;
}

HRESULT _stdcall Explorerplusplus::DragOver(DWORD grfKeyState,POINTL pt,DWORD *pdwEffect)
{
	TCHITTESTINFO HitTestInfo;
	BOOL bOnSameDrive;
	int iTab;

	HitTestInfo.pt.x	= pt.x;
	HitTestInfo.pt.y	= pt.y;

	ScreenToClient(m_hTabCtrl,&HitTestInfo.pt);

	iTab = TabCtrl_HitTest(m_hTabCtrl,&HitTestInfo);

	if(iTab == -1)
	{
		*pdwEffect = DROPEFFECT_NONE;
	}
	else
	{
		TCITEM tcItem;

		tcItem.mask	= TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

		if(iTab != m_iTabSelectedItem)
		{
			/* Set a timer. If the mouse does not move
			outside this tab before the timer expires,
			and the item is still been dragged, switch
			focus to this tab.
			The timer will be set if the tab the item
			is currently above does not match the
			tab it was above initially (when the timer
			was previously set). */
			if(iTab != m_iTabDragTab)
			{
				SetTimer(m_hTabCtrl,TABDRAG_TIMER_ID,TABDRAG_TIMER_ELAPSED,TabDragTimerProc);

				m_iTabDragTab = iTab;
			}
			else if(g_bTabDragTimerElapsed)
			{
				m_iTabSelectedItem = iTab;

				TabCtrl_SetCurSel(m_hTabCtrl,m_iTabSelectedItem);

				OnTabChangeInternal(TRUE);

				g_bTabDragTimerElapsed = FALSE;
			}
		}
		else
		{
			KillTimer(m_hTabCtrl,TABDRAG_TIMER_ID);
			m_iTabDragTab = iTab;
		}

		/* If the tab contains a virtual folder,
		then files cannot be dropped. */
		if(m_pShellBrowser[(int)tcItem.lParam]->InVirtualFolder())
		{
			*pdwEffect = DROPEFFECT_NONE;
		}
		else
		{
			bOnSameDrive = CheckItemLocations((int)tcItem.lParam);

			*pdwEffect = DetermineCurrentDragEffect(grfKeyState,
				*pdwEffect,m_bDataAccept,bOnSameDrive);
		}
	}

	m_pDropTargetHelper->DragOver((LPPOINT)&pt,*pdwEffect);

	return S_OK;
}

/* Retrieves the filename of the first file been
dropped. */
void Explorerplusplus::GetSourceFileName(IDataObject *pDataObject)
{
	FORMATETC	ftc;
	STGMEDIUM	stg;
	DROPFILES	*pdf = NULL;
	HRESULT		hr;
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

			if(nDroppedFiles >= 1)
			{
				/* Determine the name of the first dropped file. */
				DragQueryFile((HDROP)pdf,0,m_pszSource,
					SIZEOF_ARRAY(m_pszSource));
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}
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
BOOL Explorerplusplus::CheckItemLocations(int iTabId)
{
	TCHAR			szDestDirectory[MAX_PATH];
	BOOL			bOnSameDrive = FALSE;

	m_pShellBrowser[iTabId]->QueryCurrentDirectory(SIZEOF_ARRAY(szDestDirectory),
		szDestDirectory);

	bOnSameDrive = PathIsSameRoot(szDestDirectory,m_pszSource);

	return bOnSameDrive;
}

void CALLBACK TabDragTimerProc(HWND hwnd,UINT uMsg,
UINT_PTR idEvent,DWORD dwTime)
{
	g_bTabDragTimerElapsed = TRUE;
}

HRESULT _stdcall Explorerplusplus::DragLeave(void)
{
	m_pDropTargetHelper->DragLeave();

	return S_OK;
}

HRESULT _stdcall Explorerplusplus::Drop(IDataObject *pDataObject,DWORD grfKeyState,
POINTL pt,DWORD *pdwEffect)
{
	TCHITTESTINFO tchi;
	TCITEM tcItem;
	TCHAR szDestDirectory[MAX_PATH];
	int iTab;

	m_pDropTargetHelper->Drop(pDataObject,(POINT *)&pt,*pdwEffect);

	tchi.pt.x = pt.x;
	tchi.pt.y = pt.y;
	ScreenToClient(m_hTabCtrl,&tchi.pt);
	iTab = TabCtrl_HitTest(m_hTabCtrl,&tchi);

	if(iTab != -1)
	{
		tcItem.mask = TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

		m_pShellBrowser[(int)tcItem.lParam]->QueryCurrentDirectory(SIZEOF_ARRAY(szDestDirectory),
			szDestDirectory);
	}
	else
	{
		return S_OK;
	}
	
	if(m_bDataAccept)
	{
		IDropHandler *pDropHandler = NULL;

		pDropHandler = new CDropHandler();

		pDropHandler->Drop(pDataObject,
			grfKeyState,pt,pdwEffect,m_hTabCtrl,
			m_DragType,szDestDirectory,
			NULL,FALSE);

		pDropHandler->Release();
	}

	return S_OK;
}