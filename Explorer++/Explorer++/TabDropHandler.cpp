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
#include "Misc.h"
#include "Explorer++.h"


void CALLBACK TabDragTimerProc(HWND hwnd,UINT uMsg,
UINT_PTR idEvent,DWORD dwTime);

BOOL	g_bTabDragTimerElapsed;

HRESULT _stdcall CContainer::DragEnter(IDataObject *pDataObject,
DWORD grfKeyState,POINTL pt,DWORD *pdwEffect)
{
	FORMATETC	ftc = {CF_HDROP,0,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	HRESULT		hr;

	m_iTabDragTab = m_iTabSelectedItem;
	g_bTabDragTimerElapsed = FALSE;

	/* Check whether the drop source has the type of data (CF_HDROP)
	that is needed for this drag operation. */
	hr = pDataObject->QueryGetData(&ftc);

	/* DON'T use SUCCEEDED (QueryGetData() will return S_FALSE on
	failure). */
	if(hr == S_OK)
	{
		/* The clipboard contains data that we can copy/move. */
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
		/* The clipboard contains data that we cannot copy/move. */
		m_bDataAccept	= FALSE;
		*pdwEffect		= DROPEFFECT_NONE;
	}

	m_pDropTargetHelper->DragEnter(m_hTabCtrl,pDataObject,(POINT *)&pt,*pdwEffect);

	return S_OK;
}

HRESULT _stdcall CContainer::DragOver(DWORD grfKeyState,POINTL pt,DWORD *pdwEffect)
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

				OnTabChangeInternal();

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
void CContainer::GetSourceFileName(IDataObject *pDataObject)
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
BOOL CContainer::CheckItemLocations(int iTabId)
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

HRESULT _stdcall CContainer::DragLeave(void)
{
	m_pDropTargetHelper->DragLeave();

	return S_OK;
}

HRESULT _stdcall CContainer::Drop(IDataObject *pDataObject,DWORD grfKeyState,
POINTL pt,DWORD *pdwEffect)
{
	DROPFILES *pdf = NULL;
	FORMATETC ftc;
	STGMEDIUM stg;
	TCHITTESTINFO tchi;
	TCITEM tcItem;
	TCHAR szDestDirectory[MAX_PATH];
	DWORD dwEffect;
	HRESULT hr;
	BOOL bOnSameDrive;
	int nDroppedFiles;
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
			pdf = (DROPFILES *)GlobalLock(stg.hGlobal);

			if(pdf != NULL)
			{
				/* Request a count of the number of files that have been dropped. */
				nDroppedFiles = DragQueryFile((HDROP)pdf,0xFFFFFFFF,NULL,NULL);

				bOnSameDrive = CheckItemLocations((int)tcItem.lParam);

				dwEffect = DetermineCurrentDragEffect(grfKeyState,
					*pdwEffect,m_bDataAccept,bOnSameDrive);

				switch(dwEffect)
				{
				case DROPEFFECT_COPY:
				case DROPEFFECT_MOVE:
					{
						IBufferManager	*pbm = NULL;
						TCHAR			szFullFileName[MAX_PATH];
						int				i = 0;

						pbm = new CBufferManager();

						for(i = 0;i < nDroppedFiles;i++)
						{
							/* Determine the name of the dropped file. */
							DragQueryFile((HDROP)pdf,i,szFullFileName,
								SIZEOF_ARRAY(szFullFileName));

							pbm->WriteListEntry(szFullFileName);
						}

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

								shfo.hwnd	= m_hContainer;
								shfo.wFunc	= dwEffect == DROPEFFECT_COPY ? FO_COPY : FO_MOVE;
								shfo.pFrom	= szFileNameList;
								shfo.pTo	= szDestDirectory;
								shfo.fFlags	= FOF_RENAMEONCOLLISION;
								SHFileOperation(&shfo);

								free(szFileNameList);
							}
						}

						pbm->Release();
					}
					break;

				case DROPEFFECT_LINK:
					{
						TCHAR szFullFileName[MAX_PATH];
						TCHAR	szLink[MAX_PATH];
						TCHAR	szFileName[MAX_PATH];
						int i = 0;

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
					break;
				}
			}
		}
	}

	return S_OK;
}