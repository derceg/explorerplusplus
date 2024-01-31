// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DropHandler.h"
#include "DataExchangeHelper.h"
#include "GdiplusHelper.h"
#include "Helper.h"
#include "Macros.h"
#include <glog/logging.h>
#include <wil/resource.h>
#include <chrono>

/* Drop formats supported. */
FORMATETC DropHandler::m_ftcUnicodeText = { CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1,
	TYMED_HGLOBAL };
FORMATETC DropHandler::m_ftcPng = { static_cast<CLIPFORMAT>(GetPngClipboardFormat()), nullptr,
	DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
FORMATETC DropHandler::m_ftcDIB = { CF_DIB, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

std::wstring GenerateTimestampForFilename();

DropHandler *DropHandler::CreateNew()
{
	return new DropHandler();
}

HRESULT DropHandler::GetDropFormats(std::list<FORMATETC> &ftcList)
{
	ftcList.push_back(m_ftcUnicodeText);
	ftcList.push_back(m_ftcDIB);

	return S_OK;
}

void DropHandler::CopyClipboardData(IDataObject *pDataObject, HWND hwndDrop,
	const TCHAR *szDestDirectory, IDropFilesCallback *pDropFilesCallback)
{
	m_pDataObject = pDataObject;
	m_dwEffect = DROPEFFECT_COPY;
	m_hwndDrop = hwndDrop;
	m_destDirectory = szDestDirectory;
	m_pDropFilesCallback = pDropFilesCallback;

	POINT pt = { 0, 0 };

	HandleLeftClickDrop(m_pDataObject, &pt);
}

void DropHandler::HandleLeftClickDrop(IDataObject *pDataObject, POINT *pt)
{
	FORMATETC ftc;
	STGMEDIUM stg;
	DWORD *pdwEffect = nullptr;
	DWORD dwEffect = DROPEFFECT_NONE;
	BOOL bPrefferedEffect = FALSE;

	SetFORMATETC(&ftc, (CLIPFORMAT) RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT), nullptr,
		DVASPECT_CONTENT, -1, TYMED_HGLOBAL);

	/* Check if the data has a preferred drop effect
	(i.e. copy or move). */
	HRESULT hr = pDataObject->GetData(&ftc, &stg);

	if (hr == S_OK)
	{
		pdwEffect = (DWORD *) GlobalLock(stg.hGlobal);

		if (pdwEffect != nullptr)
		{
			if (*pdwEffect != DROPEFFECT_NONE)
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

	if (CheckDropFormatSupported(pDataObject, &m_ftcUnicodeText))
	{
		LOG(INFO) << "Helper - Copying CF_UNICODETEXT data";
		hrCopy = CopyUnicodeTextData(pDataObject, pastedFileList);
	}
	else if (CheckDropFormatSupported(pDataObject, &m_ftcPng))
	{
		LOG(INFO) << "Helper - Copying PNG data";

		bool res = CopyPngData(pDataObject, pastedFileList);
		hrCopy = res ? S_OK : E_FAIL;
	}
	else if (CheckDropFormatSupported(pDataObject, &m_ftcDIB))
	{
		LOG(INFO) << "Helper - Copying CF_DIB data";

		bool res = CopyDIBData(pDataObject, pastedFileList);
		hrCopy = res ? S_OK : E_FAIL;
	}

	if (hrCopy == S_OK && !pastedFileList.empty())
	{
		/* The data was copied successfully, so notify
		the caller via the specified callback interface. */
		if (m_pDropFilesCallback != nullptr)
		{
			m_pDropFilesCallback->OnDropFile(pastedFileList, pt);
		}
	}
}

/* Some applications (e.g. Thunderbird) may indicate via
QueryGetData() that they support a particular drop format,
even though a corresponding call to GetData() fails.
Therefore, we'll actually attempt to query the data using
GetData(). */
BOOL DropHandler::CheckDropFormatSupported(IDataObject *pDataObject, FORMATETC *pftc)
{
	HRESULT hr;

	/* When using QueryGetData(), the line index
	must be -1. This may not be the case when calling
	GetData(). */
	LONG lindex = pftc->lindex;
	pftc->lindex = -1;

	hr = pDataObject->QueryGetData(pftc);

	pftc->lindex = lindex;

	if (hr != S_OK)
	{
		return FALSE;
	}

	STGMEDIUM stg;
	hr = pDataObject->GetData(pftc, &stg);

	if (hr != S_OK)
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

	hr = pDataObject->GetData(&m_ftcUnicodeText, &stg);

	if (hr == S_OK)
	{
		auto *pText = static_cast<WCHAR *>(GlobalLock(stg.hGlobal));

		if (pText != nullptr)
		{
			TCHAR szFullFileName[MAX_PATH];

			hr = CopyTextToFile(m_destDirectory.c_str(), pText, szFullFileName,
				SIZEOF_ARRAY(szFullFileName));

			if (hr == S_OK)
			{
				PastedFileList.emplace_back(szFullFileName);
			}

			GlobalUnlock(stg.hGlobal);
		}

		ReleaseStgMedium(&stg);
	}

	return hr;
}

bool DropHandler::CopyPngData(IDataObject *dataObject, std::list<std::wstring> &pastedFileList)
{
	wil::unique_stg_medium stg;
	HRESULT hr = dataObject->GetData(&m_ftcPng, &stg);

	if (FAILED(hr))
	{
		return false;
	}

	wil::unique_hglobal_locked mem(stg.hGlobal);

	if (!mem)
	{
		return false;
	}

	auto dataSize = GlobalSize(mem.get());

	// dataSize is cast to a DWORD in the WriteFile() call below. It's not expected that an image
	// would ever be larger than the size that can be represented in a DWORD (4GB), but if it is,
	// the method should fail here, rather than blindly writing some of the data out.
	if (dataSize == 0 || dataSize > (std::numeric_limits<DWORD>::max)())
	{
		return false;
	}

	/* TODO: Move text into string table. */
	WCHAR fullPath[MAX_PATH];
	auto filename = std::format(L"Clipboard Image ({}).png", GenerateTimestampForFilename());
	auto combineResult = PathCombine(fullPath, m_destDirectory.c_str(), filename.c_str());

	if (!combineResult)
	{
		return false;
	}

	wil::unique_hfile file(CreateFile(fullPath, GENERIC_WRITE, 0, nullptr, CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL, nullptr));

	if (!file)
	{
		return false;
	}

	DWORD bytesWritten = 0;
	WriteFile(file.get(), mem.get(), static_cast<DWORD>(dataSize), &bytesWritten, nullptr);

	pastedFileList.emplace_back(fullPath);

	return true;
}

bool DropHandler::CopyDIBData(IDataObject *dataObject, std::list<std::wstring> &pastedFileList)
{
	wil::unique_stg_medium stg;
	HRESULT hr = dataObject->GetData(&m_ftcDIB, &stg);

	if (FAILED(hr))
	{
		return false;
	}

	wil::unique_hglobal_locked mem(stg.hGlobal);

	if (!mem)
	{
		return false;
	}

	auto *bitmapInfo = reinterpret_cast<BITMAPINFO *>(mem.get());

	int colorTableLength = 0;

	// See
	// https://source.chromium.org/chromium/chromium/src/+/main:ui/base/clipboard/clipboard_win.cc;l=833;drc=177693cc196465bf71d8d0c0fab8c4f1bb9d95b4.
	switch (bitmapInfo->bmiHeader.biBitCount)
	{
	case 1:
	case 4:
	case 8:
		colorTableLength = bitmapInfo->bmiHeader.biClrUsed ? bitmapInfo->bmiHeader.biClrUsed
														   : 1 << bitmapInfo->bmiHeader.biBitCount;
		break;

	case 16:
	case 32:
		if (bitmapInfo->bmiHeader.biCompression == BI_BITFIELDS)
		{
			colorTableLength = 3;
		}
		break;
	}

	void *data = reinterpret_cast<std::byte *>(bitmapInfo) + bitmapInfo->bmiHeader.biSize
		+ (colorTableLength * sizeof(RGBQUAD));

	// Although it's not stated in the documentation for this Gdiplus::Bitmap constructor, it
	// appears that the object references the data passed in, rather than copying it. Therefore, the
	// STGMEDIUM shouldn't be released until this object is destroyed.
	auto bitmap = std::make_unique<Gdiplus::Bitmap>(bitmapInfo, data);

	if (bitmap->GetLastStatus() != Gdiplus::Ok)
	{
		return false;
	}

	/* TODO: Move text into string table. */
	WCHAR fullPath[MAX_PATH];
	auto filename = std::format(L"Clipboard Image ({}).png", GenerateTimestampForFilename());
	auto combineResult = PathCombine(fullPath, m_destDirectory.c_str(), filename.c_str());

	if (!combineResult)
	{
		return false;
	}

	auto pngClsid = GdiplusHelper::GetEncoderClsid(L"image/png");

	if (!pngClsid)
	{
		return false;
	}

	auto status = bitmap->Save(fullPath, &pngClsid.value(), nullptr);

	if (status != Gdiplus::Ok)
	{
		return false;
	}

	pastedFileList.emplace_back(fullPath);

	return true;
}

std::wstring GenerateTimestampForFilename()
{
	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::current_zone()->to_local(now);

	auto timeText = std::format(L"{:L%c}", time);
	std::replace(timeText.begin(), timeText.end(), '/', '-');
	std::replace(timeText.begin(), timeText.end(), ':', '.');

	return timeText;
}

HRESULT DropHandler::CopyTextToFile(const TCHAR *pszDestDirectory, const WCHAR *pszText,
	TCHAR *pszFullFileNameOut, size_t outLen)
{
	HRESULT hr = E_FAIL;

	/* TODO: Move text into string table. */
	TCHAR szFullFileName[MAX_PATH];
	auto filename = std::format(L"Clipboard Text ({}).txt", GenerateTimestampForFilename());
	PathCombine(szFullFileName, pszDestDirectory, filename.c_str());

	HANDLE hFile = CreateFile(szFullFileName, GENERIC_WRITE, 0, nullptr, CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL, nullptr);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD nBytesWritten;

		/* UTF-16 LE BOM. */
		WriteFile(hFile, reinterpret_cast<LPCVOID>("\xFF\xFE"), 2, &nBytesWritten, nullptr);

		WriteFile(hFile, (LPCVOID) pszText, lstrlen(pszText) * sizeof(WCHAR), &nBytesWritten,
			nullptr);

		CloseHandle(hFile);

		StringCchCopy(pszFullFileNameOut, outLen, szFullFileName);

		hr = S_OK;
	}

	return hr;
}
