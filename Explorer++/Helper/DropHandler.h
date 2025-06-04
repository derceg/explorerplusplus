// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ReferenceCount.h"
#include <list>

class IDropFilesCallback
{
public:
	virtual ~IDropFilesCallback() = default;

	virtual void OnDropFile(const std::list<std::wstring> &PastedFileList, const POINT *ppt) = 0;
};

class DropHandler : public ReferenceCount
{
public:
	/* As this class is reference counted, the constructor
	and destructor are both private. Use this method to
	get a new instance of this class. */
	static DropHandler *CreateNew();

	static std::vector<CLIPFORMAT> GetDropFormats();

	void CopyClipboardData(IDataObject *pDataObject, HWND hwndDrop, const TCHAR *szDestDirectory,
		IDropFilesCallback *pDropFilesCallback);

private:
	DropHandler() = default;
	~DropHandler() = default;

	void HandleLeftClickDrop(IDataObject *pDataObject, POINT *pt);

	BOOL CheckDropFormatSupported(IDataObject *pDataObject, FORMATETC *pftc);

	HRESULT CopyUnicodeTextData(IDataObject *pDataObject, std::list<std::wstring> &PastedFileList);
	bool CopyPngData(IDataObject *dataObject, std::list<std::wstring> &pastedFileList);
	bool CopyDIBData(IDataObject *dataObject, std::list<std::wstring> &pastedFileList);

	HRESULT CopyTextToFile(const TCHAR *pszDestDirectory, const WCHAR *pszText,
		TCHAR *pszFullFileNameOut, size_t outLen);

	/* Holds the drop formats supported. */
	static FORMATETC m_ftcUnicodeText;
	static FORMATETC m_ftcPng;
	static FORMATETC m_ftcDIB;

	IDataObject *m_pDataObject;
	IDropFilesCallback *m_pDropFilesCallback;
	DWORD m_dwEffect;
	HWND m_hwndDrop;
	std::wstring m_destDirectory;
};
