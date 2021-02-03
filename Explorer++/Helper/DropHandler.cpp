// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DropHandler.h"
#include "Helper.h"
#include "Logging.h"
#include "Macros.h"

/* Drop formats supported. */
FORMATETC	DropHandler::m_ftcText = {CF_TEXT,nullptr,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
FORMATETC	DropHandler::m_ftcUnicodeText = {CF_UNICODETEXT,nullptr,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
FORMATETC	DropHandler::m_ftcDIBV5 = {CF_DIBV5,nullptr,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};

DropHandler *DropHandler::CreateNew()
{
	return new DropHandler();
}

HRESULT DropHandler::GetDropFormats(std::list<FORMATETC> &ftcList)
{
	ftcList.push_back(m_ftcText);
	ftcList.push_back(m_ftcUnicodeText);
	ftcList.push_back(m_ftcDIBV5);

	return S_OK;
}

void DropHandler::CopyClipboardData(IDataObject *pDataObject,HWND hwndDrop,
const TCHAR *szDestDirectory,IDropFilesCallback *pDropFilesCallback)
{
	m_pDataObject		= pDataObject;
	m_dwEffect			= DROPEFFECT_COPY;
	m_hwndDrop			= hwndDrop;
	m_destDirectory	= szDestDirectory;
	m_pDropFilesCallback	= pDropFilesCallback;

	POINT pt = {0,0};

	HandleLeftClickDrop(m_pDataObject,&pt);
}

void DropHandler::HandleLeftClickDrop(IDataObject *pDataObject,POINT *pt)
{
	FORMATETC ftc;
	STGMEDIUM stg;
	DWORD *pdwEffect = nullptr;
	DWORD dwEffect = DROPEFFECT_NONE;
	BOOL bPrefferedEffect = FALSE;

	SetFORMATETC(&ftc,(CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT),
		nullptr,DVASPECT_CONTENT,-1,TYMED_HGLOBAL);

	/* Check if the data has a preferred drop effect
	(i.e. copy or move). */
	HRESULT hr = pDataObject->GetData(&ftc,&stg);

	if(hr == S_OK)
	{
		pdwEffect = (DWORD *)GlobalLock(stg.hGlobal);

		if(pdwEffect != nullptr)
		{
			if(*pdwEffect != DROPEFFECT_NONE)
			{
				dwEffect = *pdwEffect;
				bPrefferedEffect = TRUE;
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	HRESULT hrCopy = E_FAIL;
	std::list<std::wstring> pastedFileList;

	if(CheckDropFormatSupported(pDataObject,&m_ftcUnicodeText))
	{
		LOG(debug) << _T("Helper - Copying CF_UNICODETEXT data");
		hrCopy = CopyUnicodeTextData(pDataObject,pastedFileList);
	}
	else if(CheckDropFormatSupported(pDataObject,&m_ftcText))
	{
		LOG(debug) << _T("Helper - Copying CF_TEXT data");
		hrCopy = CopyAnsiTextData(pDataObject,pastedFileList);
	}
	else if(CheckDropFormatSupported(pDataObject,&m_ftcDIBV5))
	{
		LOG(debug) << _T("Helper - Copying CF_DIBV5 data");
		hrCopy = CopyDIBV5Data(pDataObject,pastedFileList);
	}

	if(hrCopy == S_OK && !pastedFileList.empty())
	{
		/* The data was copied successfully, so notify
		the caller via the specified callback interface. */
		if(m_pDropFilesCallback != nullptr)
		{
			m_pDropFilesCallback->OnDropFile(pastedFileList,pt);
		}
	}
}

/* Some applications (e.g. Thunderbird) may indicate via
QueryGetData() that they support a particular drop format,
even though a corresponding call to GetData() fails.
Therefore, we'll actually attempt to query the data using
GetData(). */
BOOL DropHandler::CheckDropFormatSupported(IDataObject *pDataObject,FORMATETC *pftc)
{
	HRESULT hr;

	/* When using QueryGetData(), the line index
	must be -1. This may not be the case when calling
	GetData(). */
	LONG lindex = pftc->lindex;
	pftc->lindex = -1;
	
	hr = pDataObject->QueryGetData(pftc);

	pftc->lindex = lindex;

	if(hr != S_OK)
	{
		return FALSE;
	}

	STGMEDIUM stg;
	hr = pDataObject->GetData(pftc,&stg);

	if(hr != S_OK)
	{
		return FALSE;
	}

	ReleaseStgMedium(&stg);

	return TRUE;
}

HRESULT DropHandler::CopyUnicodeTextData(IDataObject *pDataObject,
	std::list<std::wstring> &PastedFileList)
{
	STGMEDIUM stg;
	HRESULT hr;

	hr = pDataObject->GetData(&m_ftcUnicodeText,&stg);

	if(hr == S_OK)
	{
		auto *pText = static_cast<WCHAR *>(GlobalLock(stg.hGlobal));

		if(pText != nullptr)
		{
			TCHAR szFullFileName[MAX_PATH];

			hr = CopyTextToFile(m_destDirectory.c_str(), pText, szFullFileName, SIZEOF_ARRAY(szFullFileName));

			if(hr == S_OK)
			{
				TCHAR szFileName[MAX_PATH];
				StringCchCopy(szFileName,SIZEOF_ARRAY(szFileName),szFullFileName);
				PathStripPath(szFileName);

				PastedFileList.emplace_back(szFileName);
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return hr;
}

HRESULT DropHandler::CopyAnsiTextData(IDataObject *pDataObject,
	std::list<std::wstring> &PastedFileList)
{
	STGMEDIUM stg;
	HRESULT hr;

	hr = pDataObject->GetData(&m_ftcText,&stg);

	if(hr == S_OK)
	{
		char *pText = static_cast<char *>(GlobalLock(stg.hGlobal));

		if(pText != nullptr)
		{
			auto *pszUnicodeText = new WCHAR[strlen(pText) + 1];

			int iRet = MultiByteToWideChar(CP_ACP,0,pText,-1,pszUnicodeText,
				static_cast<int>(strlen(pText) + 1));

			if(iRet != 0)
			{
				TCHAR szFullFileName[MAX_PATH];

				hr = CopyTextToFile(m_destDirectory.c_str(), pszUnicodeText,
					szFullFileName, SIZEOF_ARRAY(szFullFileName));

				if(hr == S_OK)
				{
					TCHAR szFileName[MAX_PATH];
					StringCchCopy(szFileName, SIZEOF_ARRAY(szFileName), szFullFileName);
					PathStripPath(szFileName);

					PastedFileList.emplace_back(szFileName);
				}
			}

			delete[] pszUnicodeText;

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return hr;
}

HRESULT DropHandler::CopyDIBV5Data(IDataObject *pDataObject,
	std::list<std::wstring> &PastedFileList)
{
	STGMEDIUM stg;
	HRESULT hr;

	hr = pDataObject->GetData(&m_ftcDIBV5,&stg);

	if(hr == S_OK)
	{
		auto *pbmp = static_cast<BITMAPINFO *>(GlobalLock(stg.hGlobal));

		if(pbmp != nullptr)
		{
			SYSTEMTIME st;
			FILETIME ft;
			FILETIME lft;
			TCHAR szTime[512];

			GetLocalTime(&st);
			SystemTimeToFileTime(&st,&ft);
			LocalFileTimeToFileTime(&ft,&lft);
			CreateFileTimeString(&lft,szTime,SIZEOF_ARRAY(szTime),FALSE);

			for(int i = 0;i < lstrlen(szTime);i++)
			{
				if(szTime[i] == '/')
				{
					szTime[i] = '-';
				}
				else if(szTime[i] == ':')
				{
					szTime[i] = '.';
				}
			}

			TCHAR szFullFileName[MAX_PATH];
			TCHAR szFileName[MAX_PATH];

			/* TODO: Move text into string table. */
			StringCchPrintf(szFileName,SIZEOF_ARRAY(szFileName),
				_T("Clipboard Image (%s).bmp"),szTime);

			PathCombine(szFullFileName,m_destDirectory.c_str(),
				szFileName);

			HANDLE hFile = CreateFile(szFullFileName,
				GENERIC_WRITE,0,nullptr,CREATE_NEW,
				FILE_ATTRIBUTE_NORMAL,nullptr);

			if(hFile != INVALID_HANDLE_VALUE)
			{
				auto dwSize = static_cast<DWORD>(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (GlobalSize(stg.hGlobal) - sizeof(BITMAPINFOHEADER)));

				auto pData = new BYTE[dwSize];

				auto *pbfh = (BITMAPFILEHEADER *)pData;

				/* 'BM'. */
				pbfh->bfType		= 0x4D42;

				pbfh->bfSize		= pbmp->bmiHeader.biSize;
				pbfh->bfReserved1	= 0;
				pbfh->bfReserved2	= 0;
				pbfh->bfOffBits		= sizeof(BITMAPFILEHEADER);

				auto *pb5h = (BITMAPINFOHEADER *)(pData + sizeof(BITMAPFILEHEADER));

				memcpy(pb5h,&pbmp->bmiHeader,sizeof(BITMAPINFOHEADER));

				auto *prgb = (RGBQUAD *)(pData + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));

				memcpy(prgb,pbmp->bmiColors,GlobalSize(stg.hGlobal) - sizeof(BITMAPINFOHEADER));

				DWORD nBytesWritten;

				WriteFile(hFile,(LPCVOID)pData,
					dwSize,
					&nBytesWritten,nullptr);

				CloseHandle(hFile);

				delete[] pData;

				PastedFileList.emplace_back(szFileName);
			}

			GlobalUnlock(stg.hGlobal);
		}

		/* Must release the storage medium. */
		ReleaseStgMedium(&stg);
	}

	return hr;
}

HRESULT DropHandler::CopyTextToFile(const TCHAR *pszDestDirectory,
	const WCHAR *pszText,TCHAR *pszFullFileNameOut, size_t outLen)
{
	SYSTEMTIME st;
	FILETIME ft;
	FILETIME lft;
	HRESULT hr = E_FAIL;
	TCHAR szTime[512];

	GetLocalTime(&st);
	SystemTimeToFileTime(&st,&ft);
	LocalFileTimeToFileTime(&ft,&lft);
	CreateFileTimeString(&lft,szTime,SIZEOF_ARRAY(szTime),FALSE);

	for(int i = 0;i < lstrlen(szTime);i++)
	{
		if(szTime[i] == '/')
		{
			szTime[i] = '-';
		}
		else if(szTime[i] == ':')
		{
			szTime[i] = '.';
		}
	}

	TCHAR szFullFileName[MAX_PATH];
	TCHAR szFileName[MAX_PATH];

	/* TODO: Move text into string table. */
	StringCchPrintf(szFileName,SIZEOF_ARRAY(szFileName),
		_T("Clipboard Text (%s).txt"),szTime);

	PathCombine(szFullFileName,pszDestDirectory,
		szFileName);

	HANDLE hFile = CreateFile(szFullFileName,
		GENERIC_WRITE,0,nullptr,CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL,nullptr);

	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD nBytesWritten;

		/* UTF-16 LE BOM. */
		WriteFile(hFile,reinterpret_cast<LPCVOID>("\xFF\xFE"),2,
			&nBytesWritten,nullptr);

		WriteFile(hFile,(LPCVOID)pszText,
			lstrlen(pszText) * sizeof(WCHAR),
			&nBytesWritten,nullptr);

		CloseHandle(hFile);

		StringCchCopy(pszFullFileNameOut,outLen,szFullFileName);

		hr = S_OK;
	}

	return hr;
}