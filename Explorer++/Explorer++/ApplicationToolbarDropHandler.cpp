/******************************************************************
 *
 * Project: Explorer++
 * File: ApplicationToolbarDropHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles drag and drop for the application toolbar.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"


#define GENERAL_ALLOCATION_UNIT	1024

Explorerplusplus::CApplicationToolbarDrop::CApplicationToolbarDrop(Explorerplusplus *pContainer)
{
	m_pContainer = pContainer;

	InitializeDragDropHelpers();
}

Explorerplusplus::CApplicationToolbarDrop::~CApplicationToolbarDrop()
{

}

HRESULT __stdcall Explorerplusplus::CApplicationToolbarDrop::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	return E_NOINTERFACE;
}

ULONG __stdcall Explorerplusplus::CApplicationToolbarDrop::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall Explorerplusplus::CApplicationToolbarDrop::Release(void)
{
	m_iRefCount--;
	
	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

HRESULT _stdcall Explorerplusplus::CApplicationToolbarDrop::DragEnter(IDataObject *pDataObject,
DWORD grfKeyStat,POINTL pt,DWORD *pdwEffect)
{
	FORMATETC	ftc = {CF_HDROP,0,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	HRESULT		hr;

	/* Check whether the drop source has the type of data (CF_HDROP)
	that is needed for this drag operation. */
	hr = pDataObject->QueryGetData(&ftc);

	/* DON'T use SUCCEEDED (QueryGetData() will return S_FALSE on
	failure). */
	if(hr == S_OK)
	{
		*pdwEffect		= DROPEFFECT_COPY;
	}
	else
	{
		*pdwEffect		= DROPEFFECT_NONE;
	}

	/* Notify the drop target helper that an object has been dragged into
	the window. */
	m_pDropTargetHelper->DragEnter(m_pContainer->m_hApplicationToolbar,
		pDataObject,(POINT *)&pt,*pdwEffect);

	return hr;
}

HRESULT _stdcall Explorerplusplus::CApplicationToolbarDrop::DragOver(DWORD grfKeyState,
POINTL pt,DWORD *pdwEffect)
{
	*pdwEffect = DROPEFFECT_COPY;

	/* Notify the drop helper of the current operation. */
	m_pDropTargetHelper->DragOver((LPPOINT)&pt,*pdwEffect);

	return S_OK;
}

HRESULT _stdcall Explorerplusplus::CApplicationToolbarDrop::DragLeave(void)
{
	m_pDropTargetHelper->DragLeave();

	return S_OK;
}

HRESULT _stdcall Explorerplusplus::CApplicationToolbarDrop::Drop(IDataObject *pDataObject,
DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect)
{
	FORMATETC		ftc;
	STGMEDIUM		stg;
	DROPFILES		*pdf = NULL;
	ApplicationButton_t	*pab = NULL;
	TCHAR			szName[512];
	HRESULT			hr;
	int				nDroppedFiles;

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
		/* Need to remove the drag image before any files are copied/moved.
		This is because a copy/replace dialog may need to shown (if there
		is a collision), and the drag image no longer needs to be there.
		The insertion mark may stay until the end. */
		m_pDropTargetHelper->Drop(pDataObject,(POINT *)&ptl,*pdwEffect);

		pdf = (DROPFILES *)GlobalLock(stg.hGlobal);

		if(pdf != NULL)
		{
			TCHAR	szFullFileName[MAX_PATH];
			POINT	pt;
			int		iButton;
			int		i = 0;

			/* Request a count of the number of files that have been dropped. */
			nDroppedFiles = DragQueryFile((HDROP)pdf,0xFFFFFFFF,NULL,NULL);

			pt.x = ptl.x;
			pt.y = ptl.y;

			ScreenToClient(m_pContainer->m_hApplicationToolbar,&pt);

			/* Check whether the files were dropped over another toolbar button. If
			they were, open the dropped file in the application represented by the
			button. */
			iButton = (int)SendMessage(m_pContainer->m_hApplicationToolbar,TB_HITTEST,0,(LPARAM)&pt);

			/* Pass the dropped files to the application... */
			if(iButton >= 0)
			{
				TCHAR *pszParameters = NULL;
				TCHAR szParameter[512];
				int iAllocated = 0;

				iAllocated = GENERAL_ALLOCATION_UNIT;
				pszParameters = (TCHAR *)malloc(iAllocated * sizeof(TCHAR));
				memset(pszParameters,0,iAllocated * sizeof(TCHAR));

				if(pszParameters != NULL)
				{
					/* Build the command line... */
					for(i = 0;i < nDroppedFiles;i++)
					{
						DragQueryFile((HDROP)pdf,i,szFullFileName,
							SIZEOF_ARRAY(szFullFileName));

						if((lstrlen(szFullFileName) + lstrlen(pszParameters)) > iAllocated)
						{
							iAllocated += GENERAL_ALLOCATION_UNIT;
							pszParameters = (TCHAR *)realloc(pszParameters,iAllocated * sizeof(TCHAR));
						}

						StringCchPrintf(szParameter,SIZEOF_ARRAY(szParameter),_T(" \"%s\""),szFullFileName);
						StrCat(pszParameters,szParameter);
					}

					m_pContainer->ApplicationToolbarOpenItem(iButton,pszParameters);

					free(pszParameters);
				}
			}
			else
			{
				for(i = 0;i < nDroppedFiles;i++)
				{
					/* Determine the name of the dropped file. */
					DragQueryFile((HDROP)pdf,i,szFullFileName,
						SIZEOF_ARRAY(szFullFileName));

					/* Make sure this item is a file (and not
					a folder). */

					/* Remove the path and any extension. */
					StringCchCopy(szName,SIZEOF_ARRAY(szName),szFullFileName);
					PathStripPath(szName);

					if(szName[0] != '.')
						PathRemoveExtension(szName);

					pab = m_pContainer->ApplicationToolbarAddItem(szName,szFullFileName,TRUE);

					/* Add the application button to the toolbar. */
					m_pContainer->ApplicationToolbarAddButtonToToolbar(pab);
				}
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return S_OK;
}

HRESULT Explorerplusplus::CApplicationToolbarDrop::InitializeDragDropHelpers(void)
{
	HRESULT hr;

	/* Initialize the drag source helper, and use it to initialize the drop target helper. */
	hr = CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_INPROC_SERVER,
	IID_IDragSourceHelper,(LPVOID *)&m_pDragSourceHelper);

	if(SUCCEEDED(hr))
	{
		hr = m_pDragSourceHelper->QueryInterface(IID_IDropTargetHelper,(LPVOID *)&m_pDropTargetHelper);
	}

	return hr;
}