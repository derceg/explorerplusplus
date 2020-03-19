// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationToolbarDropHandler.h"
#include "../Helper/Macros.h"

ApplicationToolbarDropHandler::ApplicationToolbarDropHandler(
	HWND hToolbar, ApplicationToolbar *toolbar) :
	m_RefCount(1),
	m_toolbar(toolbar),
	m_hToolbar(hToolbar)
{
	HRESULT hr = CoCreateInstance(
		CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pDragSourceHelper));

	if (SUCCEEDED(hr))
	{
		hr = m_pDragSourceHelper->QueryInterface(IID_PPV_ARGS(&m_pDropTargetHelper));
	}
}

HRESULT __stdcall ApplicationToolbarDropHandler::QueryInterface(REFIID iid, void **ppvObject)
{
	if (ppvObject == nullptr)
	{
		return E_POINTER;
	}

	*ppvObject = nullptr;

	if (iid == IID_IUnknown)
	{
		*ppvObject = static_cast<IUnknown *>(this);
	}
	else if (iid == IID_IDropTarget)
	{
		*ppvObject = static_cast<IDropTarget *>(this);
	}

	if (*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall ApplicationToolbarDropHandler::AddRef()
{
	return ++m_RefCount;
}

ULONG __stdcall ApplicationToolbarDropHandler::Release()
{
	m_RefCount--;

	if (m_RefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_RefCount;
}

HRESULT _stdcall ApplicationToolbarDropHandler::DragEnter(
	IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	UNREFERENCED_PARAMETER(grfKeyState);

	FORMATETC ftc = GetSupportedDropFormat();
	HRESULT hr = pDataObject->QueryGetData(&ftc);

	/* DON'T use SUCCEEDED (QueryGetData() will return S_FALSE on
	failure). */
	if (hr == S_OK)
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

HRESULT _stdcall ApplicationToolbarDropHandler::DragOver(
	DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	UNREFERENCED_PARAMETER(grfKeyState);

	*pdwEffect = DROPEFFECT_COPY;

	m_pDropTargetHelper->DragOver((LPPOINT) &pt, *pdwEffect);

	return S_OK;
}

HRESULT _stdcall ApplicationToolbarDropHandler::DragLeave()
{
	m_pDropTargetHelper->DragLeave();

	return S_OK;
}

FORMATETC ApplicationToolbarDropHandler::GetSupportedDropFormat()
{
	FORMATETC ftc = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	return ftc;
}

HRESULT _stdcall ApplicationToolbarDropHandler::Drop(
	IDataObject *pDataObject, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
	UNREFERENCED_PARAMETER(grfKeyState);

	FORMATETC ftc = GetSupportedDropFormat();
	STGMEDIUM stg;
	HRESULT hr = pDataObject->GetData(&ftc, &stg);

	if (hr == S_OK)
	{
		m_pDropTargetHelper->Drop(pDataObject, (POINT *) &ptl, *pdwEffect);

		auto *df = reinterpret_cast<DROPFILES *>(GlobalLock(stg.hGlobal));

		if (df != nullptr)
		{
			POINT pt;
			pt.x = ptl.x;
			pt.y = ptl.y;
			ScreenToClient(m_hToolbar, &pt);

			/* Check whether the files were dropped over
			another toolbar button. If they were, pass
			the list of dropped files to the application
			represented by the button. */
			int buttonIndex =
				static_cast<int>(SendMessage(m_hToolbar, TB_HITTEST, 0, (LPARAM) &pt));

			if (buttonIndex >= 0)
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

void ApplicationToolbarDropHandler::OpenExistingButton(DROPFILES *df, int buttonIndex)
{
	int numFiles = DragQueryFile(reinterpret_cast<HDROP>(df), 0xFFFFFFFF, nullptr, 0);

	std::wstring parameters;

	for (int i = 0; i < numFiles; i++)
	{
		TCHAR path[MAX_PATH];
		DragQueryFile((HDROP) df, i, path, SIZEOF_ARRAY(path));

		if (i != 0)
		{
			parameters.append(_T(" "));
		}

		TCHAR parameter[MAX_PATH];
		StringCchPrintf(parameter, SIZEOF_ARRAY(parameter), _T("\"%s\""), path);
		parameters.append(parameter);
	}

	m_toolbar->OpenItem(buttonIndex, &parameters);
}

void ApplicationToolbarDropHandler::AddNewButton(DROPFILES *df)
{
	int numFiles = DragQueryFile(reinterpret_cast<HDROP>(df), 0xFFFFFFFF, nullptr, 0);

	for (int i = 0; i < numFiles; i++)
	{
		TCHAR path[MAX_PATH];
		DragQueryFile(reinterpret_cast<HDROP>(df), i, path, SIZEOF_ARRAY(path));

		DWORD attributes = GetFileAttributes(path);

		/* Ignore folders. */
		if (attributes != INVALID_FILE_ATTRIBUTES
			&& (attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}

		/* The name of the button will be set
		to the filename (without any extension). */
		TCHAR buttonName[MAX_PATH];
		StringCchCopy(buttonName, SIZEOF_ARRAY(buttonName), path);
		PathStripPath(buttonName);

		if (buttonName[0] != '.')
		{
			PathRemoveExtension(buttonName);
		}

		m_toolbar->AddNewItem(buttonName, path, TRUE);
	}
}