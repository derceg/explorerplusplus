/******************************************************************
 *
 * Project: Explorer++
 * File: AddressBarHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the address bar.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/Macros.h"


LRESULT CALLBACK EditSubclassStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

void Explorerplusplus::CreateAddressBar(void)
{
	m_hAddressBar = CreateComboBox(m_hMainRebar,WS_CHILD|WS_VISIBLE|WS_TABSTOP|
		CBS_DROPDOWN|CBS_AUTOHSCROLL|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);

	HIMAGELIST SmallIcons;
	Shell_GetImageLists(NULL,&SmallIcons);
	SendMessage(m_hAddressBar,CBEM_SETIMAGELIST,0,reinterpret_cast<LPARAM>(SmallIcons));

	HWND hEdit = reinterpret_cast<HWND>(SendMessage(m_hAddressBar,CBEM_GETEDITCONTROL,0,0));
	SetWindowSubclass(hEdit,EditSubclassStub,0,reinterpret_cast<DWORD_PTR>(this));

	/* Turn on auto complete for the edit control within the combobox.
	This will let the os complete paths as they are typed. */
	SHAutoComplete(hEdit,SHACF_FILESYSTEM|SHACF_AUTOSUGGEST_FORCE_ON);
}

LRESULT CALLBACK EditSubclassStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	Explorerplusplus *pContainer = (Explorerplusplus *)dwRefData;

	return pContainer->EditSubclass(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::EditSubclass(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_KEYDOWN:
			switch(wParam)
			{
				case VK_RETURN:
					SendMessage(m_hContainer,CBN_KEYDOWN,VK_RETURN,0);
					return 0;
					break;
			}
			break;

		case WM_SETFOCUS:
			HandleToolbarItemStates();
			break;

		case WM_MOUSEWHEEL:
			if(OnMouseWheel(MOUSEWHEEL_SOURCE_OTHER,wParam,lParam))
			{
				return 0;
			}
			break;
	}

	return DefSubclassProc(hwnd,msg,wParam,lParam);
}

/* Called when the user presses 'Enter' while
the address bar has focus, or when the 'Go'
toolbar button to the right of the address
bar is pressed.

The path entered may be relative to the current
directory, or absolute.
Basic procedure:
1. Path is expanded (if possible)
2. Any special character sequences ("..", ".") are removed
3. If the path is a URL, pass it straight out, else
4. If the path is relative, add it onto onto the current directory
*/
void Explorerplusplus::OnAddressBarGo(void)
{
	TCHAR szPath[MAX_PATH];
	TCHAR szFullFilePath[MAX_PATH];
	TCHAR szCurrentDirectory[MAX_PATH];

	/* Retrieve the combobox text, and determine if it is a
	valid path. */
	SendMessage(m_hAddressBar,WM_GETTEXT,SIZEOF_ARRAY(szPath),(LPARAM)szPath);

	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(szCurrentDirectory),szCurrentDirectory);
	DecodePath(szPath,szCurrentDirectory,szFullFilePath,SIZEOF_ARRAY(szFullFilePath));

	OpenItem(szFullFilePath,FALSE,FALSE);
}

void Explorerplusplus::OnAddressBarBeginDrag(void)
{
	IDragSourceHelper *pDragSourceHelper = NULL;
	IDropSource *pDropSource = NULL;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_ALL,
		IID_IDragSourceHelper,(LPVOID *)&pDragSourceHelper);

	if(SUCCEEDED(hr))
	{
		hr = CreateDropSource(&pDropSource,DRAG_TYPE_LEFTCLICK);

		if(SUCCEEDED(hr))
		{
			LPITEMIDLIST pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

			FORMATETC ftc[2];
			STGMEDIUM stg[2];

			SetFORMATETC(&ftc[0],(CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR),
				NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL);

			HGLOBAL hglb = GlobalAlloc(GMEM_MOVEABLE,1000);

			FILEGROUPDESCRIPTOR *pfgd = static_cast<FILEGROUPDESCRIPTOR *>(GlobalLock(hglb));

			pfgd->cItems = 1;

			FILEDESCRIPTOR *pfd = (FILEDESCRIPTOR *)((LPBYTE)pfgd + sizeof(UINT));

			/* File information (name, size, date created, etc). */
			pfd[0].dwFlags			= FD_ATTRIBUTES|FD_FILESIZE;
			pfd[0].dwFileAttributes	= FILE_ATTRIBUTE_NORMAL;
			pfd[0].nFileSizeLow		= 16384;
			pfd[0].nFileSizeHigh	= 0;

			/* The name of the file will be the folder name, followed by .lnk. */
			TCHAR szDisplayName[MAX_PATH];
			GetDisplayName(pidlDirectory,szDisplayName,SHGDN_INFOLDER);
			StringCchCat(szDisplayName,SIZEOF_ARRAY(szDisplayName),_T(".lnk"));
			StringCchCopy(pfd[0].cFileName,SIZEOF_ARRAY(pfd[0].cFileName),szDisplayName);

			GlobalUnlock(hglb);

			stg[0].pUnkForRelease	= 0;
			stg[0].hGlobal			= hglb;
			stg[0].tymed			= TYMED_HGLOBAL;

			/* File contents. */
			SetFORMATETC(&ftc[1],(CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS),
				NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL);

			hglb = GlobalAlloc(GMEM_MOVEABLE,16384);

			IShellLink *pShellLink = NULL;
			IPersistStream *pPersistStream = NULL;
			HRESULT hr;

			hr = CoCreateInstance(CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,
				IID_IShellLink,(LPVOID*)&pShellLink);

			if(SUCCEEDED(hr))
			{
				TCHAR szPath[MAX_PATH];

				GetDisplayName(pidlDirectory,szPath,SHGDN_FORPARSING);

				pShellLink->SetPath(szPath);

				hr = pShellLink->QueryInterface(IID_IPersistStream,(LPVOID*)&pPersistStream);

				if(SUCCEEDED(hr))
				{
					IStream *pStream = NULL;

					CreateStreamOnHGlobal(hglb,FALSE,&pStream);

					hr = pPersistStream->Save(pStream,TRUE);
				}
			}

			GlobalUnlock(hglb);

			stg[1].pUnkForRelease	= 0;
			stg[1].hGlobal			= hglb;
			stg[1].tymed			= TYMED_HGLOBAL;

			IDataObject *pDataObject = NULL;
			POINT pt = {0,0};

			hr = CreateDataObject(ftc,stg,&pDataObject,2);

			pDragSourceHelper->InitializeFromWindow(m_hAddressBar,&pt,pDataObject);

			DWORD dwEffect;

			DoDragDrop(pDataObject,pDropSource,DROPEFFECT_LINK,&dwEffect);

			CoTaskMemFree(pidlDirectory);

			pDataObject->Release();
			pDropSource->Release();
		}

		pDragSourceHelper->Release();
	}
}