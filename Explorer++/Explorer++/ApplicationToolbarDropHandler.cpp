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
#include "ApplicationToolbarDropHandler.h"
#include "../Helper/Macros.h"


CApplicationToolbarDropHandler::CApplicationToolbarDropHandler(HWND hToolbar, CApplicationToolbar *toolbar) :
m_RefCount(1),
m_hToolbar(hToolbar),
m_toolbar(toolbar)
{
	HRESULT hr = CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER,
		IID_IDragSourceHelper, (LPVOID *) &m_pDragSourceHelper);

	if(SUCCEEDED(hr))
	{
		hr = m_pDragSourceHelper->QueryInterface(IID_IDropTargetHelper, (LPVOID *) &m_pDropTargetHelper);
	}
}

CApplicationToolbarDropHandler::~CApplicationToolbarDropHandler()
{

}

HRESULT __stdcall CApplicationToolbarDropHandler::QueryInterface(REFIID iid, void **ppvObject)
{
	*ppvObject = NULL;

	return E_NOINTERFACE;
}

ULONG __stdcall CApplicationToolbarDropHandler::AddRef(void)
{
	return ++m_RefCount;
}

ULONG __stdcall CApplicationToolbarDropHandler::Release(void)
{
	m_RefCount--;

	if(m_RefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_RefCount;
}

HRESULT _stdcall CApplicationToolbarDropHandler::DragEnter(IDataObject *pDataObject,
	DWORD grfKeyStat, POINTL pt, DWORD *pdwEffect)
{
	FORMATETC ftc = GetSupportedDropFormat();
	HRESULT hr = pDataObject->QueryGetData(&ftc);

	/* DON'T use SUCCEEDED (QueryGetData() will return S_FALSE on
	failure). */
	if(hr == S_OK)
	{
		*pdwEffect = DROPEFFECT_COPY;
	}
	else
	{
		*pdwEffect = DROPEFFECT_NONE;
	}

	m_pDropTargetHelper->DragEnter(m_hToolbar, pDataObject, (POINT *) &pt, *pdwEffect);

	return hr;
}

HRESULT _stdcall CApplicationToolbarDropHandler::DragOver(DWORD grfKeyState,
	POINTL pt, DWORD *pdwEffect)
{
	*pdwEffect = DROPEFFECT_COPY;

	m_pDropTargetHelper->DragOver((LPPOINT) &pt, *pdwEffect);

	return S_OK;
}

HRESULT _stdcall CApplicationToolbarDropHandler::DragLeave(void)
{
	m_pDropTargetHelper->DragLeave();

	return S_OK;
}

FORMATETC CApplicationToolbarDropHandler::GetSupportedDropFormat()
{
	FORMATETC ftc = {CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	return ftc;
}

HRESULT _stdcall CApplicationToolbarDropHandler::Drop(IDataObject *pDataObject,
	DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
	FORMATETC ftc = GetSupportedDropFormat();
	STGMEDIUM stg;
	HRESULT hr = pDataObject->GetData(&ftc, &stg);

	if(hr == S_OK)
	{
		m_pDropTargetHelper->Drop(pDataObject, (POINT *) &ptl, *pdwEffect);

		DROPFILES *df = reinterpret_cast<DROPFILES *>(GlobalLock(stg.hGlobal));

		if(df != NULL)
		{
			POINT pt;
			pt.x = ptl.x;
			pt.y = ptl.y;
			ScreenToClient(m_hToolbar, &pt);

			/* Check whether the files were dropped over
			another toolbar button. If they were, pass
			the list of dropped files to the application
			represented by the button. */
			int buttonIndex = static_cast<int>(SendMessage(m_hToolbar, TB_HITTEST, 0, (LPARAM) &pt));

			if(buttonIndex >= 0)
			{
				OpenExistingButton(df, buttonIndex);
			}
			else
			{
				AddNewButton(df);
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return S_OK;
}

void CApplicationToolbarDropHandler::OpenExistingButton(DROPFILES *df, int buttonIndex)
{
	int numFiles = DragQueryFile(reinterpret_cast<HDROP>(df), 0xFFFFFFFF, NULL, NULL);

	std::wstring parameters;

	for(int i = 0; i < numFiles; i++)
	{
		TCHAR path[MAX_PATH];
		DragQueryFile((HDROP) df, i, path, SIZEOF_ARRAY(path));

		if(i != 0)
		{
			parameters.append(_T(" "));
		}

		TCHAR parameter[MAX_PATH];
		StringCchPrintf(parameter, SIZEOF_ARRAY(parameter), _T("\"%s\""), path);
		parameters.append(parameter);
	}

	m_toolbar->OpenItem(buttonIndex, &parameters);
}

void CApplicationToolbarDropHandler::AddNewButton(DROPFILES *df)
{
	int numFiles = DragQueryFile(reinterpret_cast<HDROP>(df), 0xFFFFFFFF, NULL, NULL);

	for(int i = 0; i < numFiles; i++)
	{
		TCHAR path[MAX_PATH];
		DragQueryFile(reinterpret_cast<HDROP>(df), i, path, SIZEOF_ARRAY(path));

		DWORD attributes = GetFileAttributes(path);

		/* Ignore folders. */
		if(attributes != INVALID_FILE_ATTRIBUTES &&
			(attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}

		/* The name of the button will be set
		to the filename (without any extension). */
		TCHAR buttonName[MAX_PATH];
		StringCchCopy(buttonName, SIZEOF_ARRAY(buttonName), path);
		PathStripPath(buttonName);

		if(buttonName[0] != '.')
		{
			PathRemoveExtension(buttonName);
		}

		m_toolbar->AddNewItem(buttonName, path, TRUE);
	}
}