/******************************************************************
 *
 * Project: Explorer++
 * File: OrganizeBookmarksDialog.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles drag and drop for the bookmarks toolbar.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"

Explorerplusplus::CBookmarkToolbarDrop::CBookmarkToolbarDrop(Explorerplusplus *pContainer)
{
	m_pContainer = pContainer;

	InitializeDragDropHelpers();
}

Explorerplusplus::CBookmarkToolbarDrop::~CBookmarkToolbarDrop()
{

}

HRESULT __stdcall Explorerplusplus::CBookmarkToolbarDrop::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	return E_NOINTERFACE;
}

ULONG __stdcall Explorerplusplus::CBookmarkToolbarDrop::AddRef(void)
{
	return ++m_iRefCount;
}

ULONG __stdcall Explorerplusplus::CBookmarkToolbarDrop::Release(void)
{
	m_iRefCount--;
	
	if(m_iRefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_iRefCount;
}

HRESULT _stdcall Explorerplusplus::CBookmarkToolbarDrop::DragEnter(IDataObject *pDataObject,
DWORD grfKeyStat,POINTL pt,DWORD *pdwEffect)
{
	FORMATETC	ftc = {CF_HDROP,0,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	STGMEDIUM	stg;
	DROPFILES	*pdf = NULL;
	TCHAR		szFullFileName[MAX_PATH];
	HRESULT		hr;
	BOOL		bAllFolders = FALSE;
	int			nDroppedFiles;
	int			i = 0;

	/* Check whether the drop source has the type of data (CF_HDROP)
	that is needed for this drag operation. */
	hr = pDataObject->GetData(&ftc,&stg);

	if(hr == S_OK)
	{
		bAllFolders = TRUE;

		pdf = (DROPFILES *)GlobalLock(stg.hGlobal);

		if(pdf != NULL)
		{
			/* Request a count of the number of files that have been dropped. */
			nDroppedFiles = DragQueryFile((HDROP)pdf,0xFFFFFFFF,NULL,NULL);

			/* Only accept dropped folders, NOT files. */
			for(i = 0;i < nDroppedFiles;i++)
			{
				/* Determine the name of the dropped file. */
				DragQueryFile((HDROP)pdf,i,szFullFileName,
					SIZEOF_ARRAY(szFullFileName));

				if(!PathIsDirectory(szFullFileName))
				{
					bAllFolders = FALSE;
					break;
				}
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	if(bAllFolders)
	{
		m_bAcceptData = TRUE;
		*pdwEffect		= DROPEFFECT_COPY;
	}
	else
	{
		m_bAcceptData = FALSE;
		*pdwEffect		= DROPEFFECT_NONE;
	}

	/* Notify the drop target helper that an object has been dragged into
	the window. */
	m_pDropTargetHelper->DragEnter(m_pContainer->m_hBookmarksToolbar,
		pDataObject,(POINT *)&pt,*pdwEffect);

	return hr;
}

HRESULT _stdcall Explorerplusplus::CBookmarkToolbarDrop::DragOver(DWORD grfKeyState,
POINTL pt,DWORD *pdwEffect)
{
	TBINSERTMARK tbim;
	RECT rcItem;
	POINT point;
	int nButtons;
	int iButton;

	if(m_bAcceptData)
		*pdwEffect = DROPEFFECT_COPY;
	else
		*pdwEffect = DROPEFFECT_NONE;

	point.x = pt.x;
	point.y = pt.y;
	ScreenToClient(m_pContainer->m_hBookmarksToolbar,&point);
	iButton = (int)SendMessage(m_pContainer->m_hBookmarksToolbar,
		TB_HITTEST,0,(LPARAM)&point);

	if(iButton < 0)
	{
		nButtons = (int)SendMessage(m_pContainer->m_hBookmarksToolbar,
			TB_BUTTONCOUNT,0,0);

		tbim.iButton	= nButtons - 1;
		tbim.dwFlags	= TBIMHT_AFTER;
		SendMessage(m_pContainer->m_hBookmarksToolbar,
			TB_SETINSERTMARK,0,(LPARAM)&tbim);
	}
	else
	{
		SendMessage(m_pContainer->m_hBookmarksToolbar,
			TB_GETITEMRECT,iButton,(LPARAM)&rcItem);

		if(point.x > (rcItem.left + (rcItem.right - rcItem.left) / 2))
			tbim.dwFlags = TBIMHT_AFTER;
		else
			tbim.dwFlags = 0;

		tbim.iButton = iButton;
		SendMessage(m_pContainer->m_hBookmarksToolbar,
			TB_SETINSERTMARK,0,(LPARAM)&tbim);
	}

	/* Notify the drop helper of the current operation. */
	m_pDropTargetHelper->DragOver((LPPOINT)&pt,*pdwEffect);

	return S_OK;
}

HRESULT _stdcall Explorerplusplus::CBookmarkToolbarDrop::DragLeave(void)
{
	TBINSERTMARK tbim;

	/* Stop showing the insertion mark. */
	tbim.iButton	= -1;
	SendMessage(m_pContainer->m_hBookmarksToolbar,
		TB_SETINSERTMARK,0,(LPARAM)&tbim);

	m_pDropTargetHelper->DragLeave();

	return S_OK;
}

HRESULT _stdcall Explorerplusplus::CBookmarkToolbarDrop::Drop(IDataObject *pDataObject,
DWORD grfKeyState,POINTL pt,DWORD *pdwEffect)
{
	FORMATETC		ftc;
	STGMEDIUM		stg;
	DROPFILES		*pdf = NULL;
	TBINSERTMARK	tbim;
	Bookmark_t		RootBookmark;
	Bookmark_t		NewBookmark;
	TCHAR			szBookmarkName[MAX_PATH];
	HRESULT			hr;
	int				nDroppedFiles;

	tbim.iButton	= -1;
	SendMessage(m_pContainer->m_hBookmarksToolbar,
		TB_SETINSERTMARK,0,(LPARAM)&tbim);

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
		m_pDropTargetHelper->Drop(pDataObject,(POINT *)&pt,*pdwEffect);

		pdf = (DROPFILES *)GlobalLock(stg.hGlobal);

		if(pdf != NULL)
		{
			/* Request a count of the number of files that have been dropped. */
			nDroppedFiles = DragQueryFile((HDROP)pdf,0xFFFFFFFF,NULL,NULL);

			TCHAR			szFullFileName[MAX_PATH];
			int				i = 0;

			m_pContainer->m_Bookmark.GetRoot(&RootBookmark);

			/* For each folder item, create a new bookmark. */
			for(i = 0;i < nDroppedFiles;i++)
			{
				/* Determine the name of the dropped file. */
				DragQueryFile((HDROP)pdf,i,szFullFileName,
					SIZEOF_ARRAY(szFullFileName));

				if(PathIsDirectory(szFullFileName))
				{
					/* The name of the bookmark is the stripped folder name. */
					StringCchCopy(szBookmarkName,SIZEOF_ARRAY(szBookmarkName),szFullFileName);
					PathStripPath(szBookmarkName);
					StringCchCopy(NewBookmark.szItemName,SIZEOF_ARRAY(NewBookmark.szItemName),szBookmarkName);
					StringCchCopy(NewBookmark.szItemDescription,SIZEOF_ARRAY(NewBookmark.szItemDescription),EMPTY_STRING);
					StringCchCopy(NewBookmark.szLocation,SIZEOF_ARRAY(NewBookmark.szLocation),szFullFileName);

					NewBookmark.Type = BOOKMARK_TYPE_BOOKMARK;
					NewBookmark.bShowOnToolbar = TRUE;

					/* Create the bookmark. */
					m_pContainer->m_Bookmark.CreateNewBookmark((void *)RootBookmark.pHandle,&NewBookmark);

					/* ...and add it to the toolbar. */
					m_pContainer->InsertBookmarkIntoToolbar(&NewBookmark,
						m_pContainer->GenerateUniqueBookmarkToolbarId());
				}
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	/* Rebuild the bookmarks menu. */
	m_pContainer->InsertBookmarksIntoMenu();

	return S_OK;
}

HRESULT Explorerplusplus::CBookmarkToolbarDrop::InitializeDragDropHelpers(void)
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