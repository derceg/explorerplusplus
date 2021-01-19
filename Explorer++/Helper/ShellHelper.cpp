// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellHelper.h"
#include "FileOperations.h"
#include "Helper.h"
#include "Macros.h"
#include "ProcessHelper.h"
#include "RegistrySettings.h"
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <wil/com.h>
#include <propkey.h>

bool AddJumpListTasksInternal(
	IObjectCollection *objectCollection, const std::list<JumpListTaskInformation> &taskList);
HRESULT AddJumpListTaskInternal(IObjectCollection *objectCollection, const TCHAR *name,
	const TCHAR *path, const TCHAR *arguments, const TCHAR *iconPath, int iconIndex);

HRESULT GetDisplayName(const std::wstring &parsingPath, DWORD flags, std::wstring &output)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(parsingPath.c_str(), nullptr, wil::out_param(pidl), 0, nullptr);

	if (SUCCEEDED(hr))
	{
		hr = GetDisplayName(pidl.get(), flags, output);
	}

	return hr;
}

HRESULT GetDisplayName(PCIDLIST_ABSOLUTE pidl, DWORD flags, std::wstring &output)
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	PCITEMID_CHILD pidlChild = nullptr;
	HRESULT hr = SHBindToParent(pidl, IID_PPV_ARGS(&shellFolder), &pidlChild);

	if (FAILED(hr))
	{
		return hr;
	}

	return GetDisplayName(shellFolder.get(), pidlChild, flags, output);
}

HRESULT GetDisplayName(
	IShellFolder *shellFolder, PCITEMID_CHILD pidlChild, DWORD flags, std::wstring &output)
{
	STRRET str;
	HRESULT hr = shellFolder->GetDisplayNameOf(pidlChild, flags, &str);

	if (FAILED(hr))
	{
		return hr;
	}

	wil::unique_cotaskmem_string name;
	hr = StrRetToStr(&str, pidlChild, &name);

	if (FAILED(hr))
	{
		return hr;
	}

	output = name.get();

	return hr;
}

HRESULT GetCsidlDisplayName(int csidl, DWORD flags, std::wstring &output)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHGetFolderLocation(nullptr, csidl, nullptr, 0, wil::out_param(pidl));

	if (SUCCEEDED(hr))
	{
		hr = GetDisplayName(pidl.get(), flags, output);
	}

	return hr;
}

HRESULT GetItemAttributes(const TCHAR *szItemParsingPath, SFGAOF *pItemAttributes)
{
	if (szItemParsingPath == nullptr || pItemAttributes == nullptr)
	{
		return E_FAIL;
	}

	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(szItemParsingPath, nullptr, wil::out_param(pidl), 0, nullptr);

	if (SUCCEEDED(hr))
	{
		hr = GetItemAttributes(pidl.get(), pItemAttributes);
	}

	return hr;
}

HRESULT GetItemAttributes(PCIDLIST_ABSOLUTE pidl, SFGAOF *pItemAttributes)
{
	if (pidl == nullptr || pItemAttributes == nullptr)
	{
		return E_FAIL;
	}

	IShellFolder *pShellFolder = nullptr;
	PCUITEMID_CHILD pidlRelative = nullptr;
	HRESULT hr = SHBindToParent(pidl, IID_PPV_ARGS(&pShellFolder), &pidlRelative);

	if (SUCCEEDED(hr))
	{
		hr = pShellFolder->GetAttributesOf(1, &pidlRelative, pItemAttributes);

		pShellFolder->Release();
	}

	return hr;
}

BOOL ExecuteFileAction(HWND hwnd, const TCHAR *szVerb, const TCHAR *szParameters,
	const TCHAR *szStartDirectory, LPCITEMIDLIST pidl)
{
	SHELLEXECUTEINFO sei;

	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_INVOKEIDLIST;
	sei.lpVerb = szVerb;
	sei.lpIDList = (LPVOID) pidl;
	sei.hwnd = hwnd;
	sei.nShow = SW_SHOW;
	sei.lpParameters = szParameters;
	sei.lpDirectory = szStartDirectory;
	sei.lpFile = nullptr;
	sei.hInstApp = nullptr;

	return ShellExecuteEx(&sei);
}

BOOL ExecuteAndShowCurrentProcess(HWND hwnd, const TCHAR *szParameters)
{
	TCHAR szCurrentProcess[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), szCurrentProcess, SIZEOF_ARRAY(szCurrentProcess));

	return ExecuteAndShowProcess(hwnd, szCurrentProcess, szParameters);
}

BOOL ExecuteAndShowProcess(HWND hwnd, const TCHAR *szProcess, const TCHAR *szParameters)
{
	SHELLEXECUTEINFO sei;

	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_DEFAULT;
	sei.lpVerb = _T("open");
	sei.lpFile = szProcess;
	sei.lpParameters = szParameters;
	sei.lpDirectory = nullptr;
	sei.hwnd = hwnd;
	sei.nShow = SW_SHOW;

	return ShellExecuteEx(&sei);
}

HRESULT GetVirtualParentPath(PCIDLIST_ABSOLUTE pidlDirectory, PIDLIST_ABSOLUTE *pidlParent)
{
	if (IsNamespaceRoot(pidlDirectory))
	{
		*pidlParent = nullptr;
		return E_FAIL;
	}
	else
	{
		*pidlParent = ILCloneFull(pidlDirectory);
		ILRemoveLastID(*pidlParent);
		return S_OK;
	}
}

BOOL IsNamespaceRoot(PCIDLIST_ABSOLUTE pidl)
{
	BOOL bNamespaceRoot = FALSE;
	unique_pidl_absolute pidlDesktop;
	HRESULT hr =
		SHGetFolderLocation(nullptr, CSIDL_DESKTOP, nullptr, 0, wil::out_param(pidlDesktop));

	if (SUCCEEDED(hr))
	{
		bNamespaceRoot = ArePidlsEquivalent(pidl, pidlDesktop.get());
	}

	return bNamespaceRoot;
}

HRESULT DecodeFriendlyPath(const std::wstring &friendlyPath, std::wstring &parsingPath)
{
	PIDLIST_ABSOLUTE pidl = nullptr;
	std::wstring name;

	SHGetFolderLocation(nullptr, CSIDL_CONTROLS, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_CONTROLS, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_BITBUCKET, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_BITBUCKET, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_DRIVES, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_DRIVES, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_NETWORK, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_NETWORK, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_CONNECTIONS, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_CONNECTIONS, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_PRINTERS, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_PRINTERS, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_FAVORITES, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_FAVORITES, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_MYPICTURES, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_MYPICTURES, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_MYMUSIC, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_MYMUSIC, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	SHGetFolderLocation(nullptr, CSIDL_MYVIDEO, nullptr, 0, &pidl);
	GetDisplayName(pidl, SHGDN_INFOLDER, name);
	CoTaskMemFree(pidl);

	if (name == friendlyPath)
	{
		GetCsidlDisplayName(CSIDL_MYVIDEO, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	if (CompareString(
			LOCALE_INVARIANT, NORM_IGNORECASE, FRIENDLY_NAME_DESKTOP, -1, friendlyPath.c_str(), -1)
		== CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_DESKTOP, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	if (CompareString(
			LOCALE_INVARIANT, NORM_IGNORECASE, FRIENDLY_NAME_PICTURES, -1, friendlyPath.c_str(), -1)
		== CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_MYPICTURES, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	if (CompareString(
			LOCALE_INVARIANT, NORM_IGNORECASE, FRIENDLY_NAME_MUSIC, -1, friendlyPath.c_str(), -1)
		== CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_MYMUSIC, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	if (CompareString(
			LOCALE_INVARIANT, NORM_IGNORECASE, FRIENDLY_NAME_VIDEOS, -1, friendlyPath.c_str(), -1)
		== CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_MYVIDEO, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, FRIENDLY_NAME_DOCUMENTS, -1,
			friendlyPath.c_str(), -1)
		== CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_MYDOCUMENTS, SHGDN_FORPARSING, parsingPath);
		return S_OK;
	}

	return E_FAIL;
}

int GetDefaultFolderIconIndex()
{
	return GetDefaultIcon(DefaultIconType::Folder);
}

int GetDefaultFileIconIndex()
{
	return GetDefaultIcon(DefaultIconType::File);
}

int GetDefaultIcon(DefaultIconType defaultIconType)
{
	SHFILEINFO shfi;
	DWORD dwFileAttributes;

	switch (defaultIconType)
	{
	case DefaultIconType::Folder:
		dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_NORMAL;
		break;

	case DefaultIconType::File:
	default:
		dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		break;
	}

	/* Under unicode, the filename argument cannot be NULL,
	as it is not a valid unicode character. */
	SHGetFileInfo(_T("dummy"), dwFileAttributes, &shfi, sizeof(SHFILEINFO),
		SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);

	return shfi.iIcon;
}

BOOL MyExpandEnvironmentStrings(const TCHAR *szSrc, TCHAR *szExpandedPath, DWORD nSize)
{
	HANDLE hProcess;
	HANDLE hToken;
	BOOL bRet = FALSE;

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());

	if (hProcess != nullptr)
	{
		bRet = OpenProcessToken(hProcess, TOKEN_IMPERSONATE | TOKEN_QUERY, &hToken);

		if (bRet)
		{
			bRet = ExpandEnvironmentStringsForUser(hToken, szSrc, szExpandedPath, nSize);

			CloseHandle(hToken);
		}

		CloseHandle(hProcess);
	}

	return bRet;
}

DWORD DetermineDragEffect(
	DWORD grfKeyState, DWORD dwCurrentEffect, BOOL bDataAccept, BOOL bOnSameDrive)
{
	DWORD dwEffect = DROPEFFECT_NONE;

	if (!bDataAccept)
	{
		dwEffect = DROPEFFECT_NONE;
	}
	else
	{
		/* Test the state of modifier keys. */
		if ((((grfKeyState & MK_CONTROL) == MK_CONTROL && (grfKeyState & MK_SHIFT) == MK_SHIFT)
				|| (grfKeyState & MK_ALT) == MK_ALT)
			&& dwCurrentEffect & DROPEFFECT_LINK)
		{
			/* Shortcut. */
			dwEffect = DROPEFFECT_LINK;
		}
		else if ((grfKeyState & MK_SHIFT) == MK_SHIFT && dwCurrentEffect & DROPEFFECT_MOVE)
		{
			/* Move. */
			dwEffect = DROPEFFECT_MOVE;
		}
		else if ((grfKeyState & MK_CONTROL) == MK_CONTROL && dwCurrentEffect & DROPEFFECT_COPY)
		{
			/* Copy. */
			dwEffect = DROPEFFECT_COPY;
		}
		else
		{
			/* No modifier. Determine the drag effect from
			the location of the source and destination
			directories. */
			if (bOnSameDrive && (dwCurrentEffect & DROPEFFECT_MOVE))
			{
				dwEffect = DROPEFFECT_MOVE;
			}
			else if (dwCurrentEffect & DROPEFFECT_COPY)
			{
				dwEffect = DROPEFFECT_COPY;
			}

			if (dwEffect == DROPEFFECT_NONE)
			{
				/* No suitable drop type found above. Use whichever
				method is available. */
				if (dwCurrentEffect & DROPEFFECT_MOVE)
				{
					dwEffect = DROPEFFECT_MOVE;
				}
				else if (dwCurrentEffect & DROPEFFECT_COPY)
				{
					dwEffect = DROPEFFECT_COPY;
				}
				else if (dwCurrentEffect & DROPEFFECT_LINK)
				{
					dwEffect = DROPEFFECT_LINK;
				}
				else
				{
					dwEffect = DROPEFFECT_NONE;
				}
			}
		}
	}

	return dwEffect;
}

HRESULT BuildHDropList(
	FORMATETC *pftc, STGMEDIUM *pstg, const std::vector<std::wstring> &filenameList)
{
	SetFORMATETC(pftc, CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL);

	UINT uSize = 0;

	uSize = sizeof(DROPFILES);

	for (const auto &filename : filenameList)
	{
		uSize += static_cast<UINT>((filename.length() + 1) * sizeof(TCHAR));
	}

	/* The last string is double-null terminated. */
	uSize += (1 * sizeof(TCHAR));

	HGLOBAL hglbHDrop = GlobalAlloc(GMEM_MOVEABLE, uSize);

	if (hglbHDrop == nullptr)
	{
		return E_FAIL;
	}

	auto pcidaData = static_cast<LPVOID>(GlobalLock(hglbHDrop));

	auto *pdf = static_cast<DROPFILES *>(pcidaData);

	pdf->pFiles = sizeof(DROPFILES);
	pdf->fNC = FALSE;
	pdf->pt.x = 0;
	pdf->pt.y = 0;
	pdf->fWide = TRUE;

	LPBYTE pData;
	UINT uOffset = 0;

	TCHAR chNull = '\0';

	for (const auto &filename : filenameList)
	{
		pData = static_cast<LPBYTE>(pcidaData) + sizeof(DROPFILES) + uOffset;

		memcpy(pData, filename.c_str(), (filename.length() + 1) * sizeof(TCHAR));
		uOffset += static_cast<UINT>((filename.length() + 1) * sizeof(TCHAR));
	}

	/* Copy the last null byte. */
	pData = static_cast<LPBYTE>(pcidaData) + sizeof(DROPFILES) + uOffset;
	memcpy(pData, &chNull, (1 * sizeof(TCHAR)));

	GlobalUnlock(hglbHDrop);

	pstg->pUnkForRelease = nullptr;
	pstg->hGlobal = hglbHDrop;
	pstg->tymed = TYMED_HGLOBAL;

	return S_OK;
}

/* Builds a CIDA structure. Returns the structure and its size
via arguments.
Returns S_OK on success; E_FAIL on failure. */
HRESULT BuildShellIDList(FORMATETC *pftc, STGMEDIUM *pstg, PCIDLIST_ABSOLUTE pidlDirectory,
	const std::vector<PCITEMID_CHILD> &pidlList)
{
	if (pftc == nullptr || pstg == nullptr || pidlDirectory == nullptr || pidlList.empty())
	{
		return E_FAIL;
	}

	SetFORMATETC(pftc, (CLIPFORMAT) RegisterClipboardFormat(CFSTR_SHELLIDLIST), nullptr,
		DVASPECT_CONTENT, -1, TYMED_HGLOBAL);

	/* First, we need to decide how much memory to
	allocate to the structure. This is based on
	the number of items that will be stored in
	this structure. */
	UINT uSize = 0;

	UINT nItems = static_cast<UINT>(pidlList.size());

	/* Size of the base structure + offset array. */
	UINT uBaseSize = sizeof(CIDA) + (sizeof(UINT) * nItems);

	uSize += uBaseSize;

	/* Size of the parent pidl. */
	uSize += ILGetSize(pidlDirectory);

	/* Add the total size of the child pidl's. */
	for (auto pidl : pidlList)
	{
		uSize += ILGetSize(pidl);
	}

	HGLOBAL hglbIDList = GlobalAlloc(GMEM_MOVEABLE, uSize);

	if (hglbIDList == nullptr)
	{
		return E_FAIL;
	}

	auto pcidaData = static_cast<LPVOID>(GlobalLock(hglbIDList));

	CIDA *pcida = static_cast<CIDA *>(pcidaData);

	pcida->cidl = nItems;

	UINT *pOffsets = pcida->aoffset;

	pOffsets[0] = uBaseSize;

	LPBYTE pData;

	pData = (LPBYTE)(((LPBYTE) pcida) + pcida->aoffset[0]);

	memcpy(pData, (LPVOID) pidlDirectory, ILGetSize(pidlDirectory));

	UINT uPreviousSize;
	int i = 0;

	uPreviousSize = ILGetSize(pidlDirectory);

	/* Store each of the pidl's. */
	for (auto pidl : pidlList)
	{
		pOffsets[i + 1] = pOffsets[i] + uPreviousSize;

		pData = (LPBYTE)(((LPBYTE) pcida) + pcida->aoffset[i + 1]);

		memcpy(pData, (LPVOID) pidl, ILGetSize(pidl));

		uPreviousSize = ILGetSize(pidl);

		i++;
	}

	GlobalUnlock(hglbIDList);

	pstg->pUnkForRelease = nullptr;
	pstg->hGlobal = hglbIDList;
	pstg->tymed = TYMED_HGLOBAL;

	return S_OK;
}

HRESULT BindToIdl(PCIDLIST_ABSOLUTE pidl, REFIID riid, void **ppv)
{
	IShellFolder *pDesktop = nullptr;
	HRESULT hr = SHGetDesktopFolder(&pDesktop);

	if (SUCCEEDED(hr))
	{
		/* See http://blogs.msdn.com/b/oldnewthing/archive/2011/08/30/10202076.aspx. */
		if (pidl->mkid.cb)
		{
			hr = pDesktop->BindToObject(pidl, nullptr, riid, ppv);
		}
		else
		{
			hr = pDesktop->QueryInterface(riid, ppv);
		}

		pDesktop->Release();
	}

	return hr;
}

HRESULT GetUIObjectOf(IShellFolder *pShellFolder, HWND hwndOwner, UINT cidl,
	PCUITEMID_CHILD_ARRAY apidl, REFIID riid, void **ppv)
{
	return pShellFolder->GetUIObjectOf(hwndOwner, cidl, apidl, riid, nullptr, ppv);
}

HRESULT GetShellItemDetailsEx(IShellFolder2 *pShellFolder, const SHCOLUMNID *pscid,
	PCUITEMID_CHILD pidl, TCHAR *szDetail, size_t cchMax, BOOL friendlyDate)
{
	VARIANT vt;
	HRESULT hr = pShellFolder->GetDetailsEx(pidl, pscid, &vt);

	if (SUCCEEDED(hr))
	{
		hr = ConvertVariantToString(&vt, szDetail, cchMax, friendlyDate);
		VariantClear(&vt);
	}

	return hr;
}

HRESULT ConvertVariantToString(const VARIANT *vt, TCHAR *szDetail, size_t cchMax, BOOL friendlyDate)
{
	HRESULT hr;

	switch (vt->vt)
	{
	case VT_DATE:
		/* Although dates can be converted directly to strings, a custom
		conversion function is used for two reasons:
		1. To ensure that the date is in the local timezone (dates are
		in UTC by default).
		2. To ensure that the date display is consistent with how other
		dates in the application are displayed. */
		hr = ConvertDateVariantToString(vt->date, szDetail, cchMax, friendlyDate);
		break;

	case VT_ARRAY | VT_BSTR:
		hr = ConvertVariantStringArrayToString(vt->parray, szDetail, cchMax);
		break;

	default:
		hr = ConvertGenericVariantToString(vt, szDetail, cchMax);
		break;
	}

	return hr;
}

HRESULT ConvertDateVariantToString(DATE date, TCHAR *szDetail, size_t cchMax, BOOL friendlyDate)
{
	SYSTEMTIME systemTime;
	int variantConverted = VariantTimeToSystemTime(date, &systemTime);

	if (!variantConverted)
	{
		return E_FAIL;
	}

	SYSTEMTIME localSystemTime;
	BOOL systemTimeConverted =
		SystemTimeToTzSpecificLocalTime(nullptr, &systemTime, &localSystemTime);

	if (!systemTimeConverted)
	{
		return E_FAIL;
	}

	BOOL dateStringCreated =
		CreateSystemTimeString(&localSystemTime, szDetail, cchMax, friendlyDate);

	if (!dateStringCreated)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT ConvertVariantStringArrayToString(SAFEARRAY *array, TCHAR *szDetail, size_t cchMax)
{
	UINT dimensions = SafeArrayGetDim(array);

	if (dimensions != 1)
	{
		return E_INVALIDARG;
	}

	HRESULT hr;
	LONG lowerBound;
	hr = SafeArrayGetLBound(array, 1, &lowerBound);

	if (!SUCCEEDED(hr))
	{
		return hr;
	}

	LONG upperBound;
	hr = SafeArrayGetUBound(array, 1, &upperBound);

	if (!SUCCEEDED(hr))
	{
		return hr;
	}

	std::vector<std::wstring> strings;

	for (long i = lowerBound; i <= upperBound; i++)
	{
		BSTR element;
		hr = SafeArrayGetElement(array, &i, reinterpret_cast<void *>(&element));

		if (SUCCEEDED(hr))
		{
			strings.emplace_back(element);

			SysFreeString(element);
		}
	}

	std::wstring finalString = boost::algorithm::join(strings, L"; ");

	StringCchCopy(szDetail, cchMax, finalString.c_str());

	return S_OK;
}

HRESULT ConvertGenericVariantToString(const VARIANT *vt, TCHAR *szDetail, size_t cchMax)
{
	VARIANT vtDest;
	VariantInit(&vtDest);
	HRESULT hr = VariantChangeType(&vtDest, vt, 0, VT_BSTR);

	if (SUCCEEDED(hr))
	{
		hr = StringCchCopy(szDetail, cchMax, V_BSTR(&vtDest));
		VariantClear(&vtDest);
	}

	return hr;
}

HRESULT GetDateDetailsEx(IShellFolder2 *shellFolder2, PCITEMID_CHILD pidlChild,
	const SHCOLUMNID *column, FILETIME &filetime)
{
	VARIANT dateVariant;
	HRESULT hr = shellFolder2->GetDetailsEx(pidlChild, column, &dateVariant);

	if (FAILED(hr))
	{
		return hr;
	}

	FILETIME date;
	hr = VariantToFileTime(dateVariant, PSTF_UTC, &date);

	if (FAILED(hr))
	{
		return hr;
	}

	filetime = date;

	return hr;
}

BOOL GetBooleanVariant(IShellFolder2 *shellFolder2, PCITEMID_CHILD pidlChild,
	const SHCOLUMNID *column, BOOL defaultValue)
{
	VARIANT boolVariant;
	HRESULT hr = shellFolder2->GetDetailsEx(pidlChild, column, &boolVariant);

	if (FAILED(hr))
	{
		return defaultValue;
	}

	BOOL convertedValue;
	hr = VariantToBoolean(boolVariant, &convertedValue);

	if (FAILED(hr))
	{
		return defaultValue;
	}

	return convertedValue;
}

// Returns either the parsing path for the specified item, or its in
// folder name. The in folder name will be returned when the parsing
// path is a GUID (which typically shouldn't be displayed to the user).
std::optional<std::wstring> GetFolderPathForDisplay(PCIDLIST_ABSOLUTE pidl)
{
	std::wstring parsingPath;
	HRESULT hr = GetDisplayName(pidl, SHGDN_FORPARSING, parsingPath);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	if (IsPathGUID(parsingPath.c_str()))
	{
		hr = GetDisplayName(pidl, SHGDN_INFOLDER, parsingPath);

		if (FAILED(hr))
		{
			return std::nullopt;
		}
	}

	return parsingPath;
}

/* Returns TRUE if a path is a GUID;
i.e. of the form:

::{20D04FE0-3AEA-1069-A2D8-08002B30309D}
(My Computer GUID, Windows 7)
*/
BOOL IsPathGUID(const TCHAR *szPath)
{
	if (szPath == nullptr)
	{
		return FALSE;
	}

	if (lstrlen(szPath) > 2)
	{
		if (szPath[0] == ':' && szPath[1] == ':')
		{
			return TRUE;
		}
	}

	return FALSE;
}

/*
A path passed into this function may be one of the
following:
 - A real path, either complete (e.g. C:\Windows), or
   relative (e.g. \Windows)
 - A virtual Path (e.g. ::{21EC2020-3AEA-1069-A2DD-08002B30309D} - Control Panel)
 - A URL (e.g. http://www.google.com.au)
 - A real path that is either canonicalized (i.e. contains
   "." or ".."), or contains embedded environments variables
   (i.e. %systemroot%\system32)

Basic procedure:
 1. Check if the path is an identifier for a virtual folder
   (e.g. ::{21EC2020-3AEA-1069-A2DD-08002B30309D}). If it is,
   it is copied straight to the output. No additional checks are
   performed to verify whether or not the path given actually
   exists (i.e. "::{}" will be copied straight to the output
   even though it's not a valid folder)
 2. If it isn't, check if it's a friendly name for a virtual
   folder (e.g. "my computer", "control panel", etc), else
 3. The path is expanded (if possible)
 4. Any special character sequences ("..", ".") are removed
 5. If the path is a URL, pass it straight out, else
 6. If the path is relative, add it onto onto the current directory
*/
void DecodePath(const TCHAR *szInitialPath, const TCHAR *szCurrentDirectory, TCHAR *szParsingPath,
	size_t cchDest)
{
	TCHAR szExpandedPath[MAX_PATH];
	TCHAR szCanonicalPath[MAX_PATH];
	HRESULT hr;
	BOOL bRelative;
	BOOL bRet;

	/* If the path starts with "::", then this is a GUID for
	a particular folder. Copy it straight to the output. */
	if (lstrlen(szInitialPath) >= 2 && szInitialPath[0] == ':' && szInitialPath[1] == ':')
	{
		StringCchCopy(szParsingPath, cchDest, szInitialPath);
	}
	else
	{
		std::wstring virtualParsingPath;
		hr = DecodeFriendlyPath(szInitialPath, virtualParsingPath);

		if (SUCCEEDED(hr))
		{
			StringCchCopy(szParsingPath, cchDest, virtualParsingPath.c_str());
		}
		else
		{
			/* Attempt to expand the path (in the event that
			it contains embedded environment variables). */
			bRet = MyExpandEnvironmentStrings(
				szInitialPath, szExpandedPath, SIZEOF_ARRAY(szExpandedPath));

			if (!bRet)
			{
				StringCchCopy(szExpandedPath, SIZEOF_ARRAY(szExpandedPath), szInitialPath);
			}

			/* Canonicalizing the path will remove any "." and
			".." components. */
			PathCanonicalize(szCanonicalPath, szExpandedPath);

			if (PathIsURL(szCanonicalPath))
			{
				StringCchCopy(szParsingPath, cchDest, szCanonicalPath);
			}
			else
			{
				bRelative = PathIsRelative(szCanonicalPath);

				/* If the path is relative, prepend it
				with the current directory. */
				if (bRelative)
				{
					StringCchCopy(szParsingPath, cchDest, szCurrentDirectory);
					PathAppend(szParsingPath, szCanonicalPath);
				}
				else
				{
					StringCchCopy(szParsingPath, cchDest, szCanonicalPath);
				}
			}
		}
	}
}

BOOL ArePidlsEquivalent(PCIDLIST_ABSOLUTE pidl1, PCIDLIST_ABSOLUTE pidl2)
{
	IShellFolder *pDesktopFolder = nullptr;
	HRESULT hr;
	BOOL ret = FALSE;

	hr = SHGetDesktopFolder(&pDesktopFolder);

	if (SUCCEEDED(hr))
	{
		hr = pDesktopFolder->CompareIDs(0, pidl1, pidl2);

		if (HRESULT_CODE(hr) == 0)
		{
			ret = TRUE;
		}

		pDesktopFolder->Release();
	}

	return ret;
}

HRESULT AddJumpListTasks(const std::list<JumpListTaskInformation> &taskList)
{
	wil::com_ptr_nothrow<ICustomDestinationList> customDestinationList;
	HRESULT hr = CoCreateInstance(
		CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&customDestinationList));

	if (FAILED(hr))
	{
		return hr;
	}

	wil::com_ptr_nothrow<IObjectArray> removedItems;
	UINT minSlots;
	hr = customDestinationList->BeginList(&minSlots, IID_PPV_ARGS(&removedItems));

	if (FAILED(hr))
	{
		return hr;
	}

	wil::com_ptr_nothrow<IObjectCollection> objectCollection;
	hr = CoCreateInstance(CLSID_EnumerableObjectCollection, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&objectCollection));

	if (FAILED(hr))
	{
		return hr;
	}

	AddJumpListTasksInternal(objectCollection.get(), taskList);

	wil::com_ptr_nothrow<IObjectArray> items;
	hr = objectCollection->QueryInterface(IID_PPV_ARGS(&items));

	if (FAILED(hr))
	{
		return hr;
	}

	hr = customDestinationList->AddUserTasks(items.get());

	if (FAILED(hr))
	{
		return hr;
	}

	hr = customDestinationList->CommitList();

	return hr;
}

bool AddJumpListTasksInternal(
	IObjectCollection *objectCollection, const std::list<JumpListTaskInformation> &taskList)
{
	bool allSucceeded = true;

	for (const auto &jtli : taskList)
	{
		HRESULT hr = AddJumpListTaskInternal(objectCollection, jtli.pszName, jtli.pszPath,
			jtli.pszArguments, jtli.pszIconPath, jtli.iIcon);

		if (FAILED(hr))
		{
			allSucceeded = false;
		}
	}

	return allSucceeded;
}

HRESULT AddJumpListTaskInternal(IObjectCollection *objectCollection, const TCHAR *name,
	const TCHAR *path, const TCHAR *arguments, const TCHAR *iconPath, int iconIndex)
{
	wil::com_ptr_nothrow<IShellLink> shellLink;
	HRESULT hr =
		CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));

	if (FAILED(hr))
	{
		return hr;
	}

	shellLink->SetPath(path);
	shellLink->SetArguments(arguments);
	shellLink->SetIconLocation(iconPath, iconIndex);

	wil::com_ptr_nothrow<IPropertyStore> propertyStore;
	hr = shellLink->QueryInterface(IID_PPV_ARGS(&propertyStore));

	if (FAILED(hr))
	{
		return hr;
	}

	wil::unique_prop_variant titleProperty;
	hr = InitPropVariantFromString(name, &titleProperty);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = propertyStore->SetValue(PKEY_Title, titleProperty);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = propertyStore->Commit();

	if (FAILED(hr))
	{
		return hr;
	}

	hr = objectCollection->AddObject(shellLink.get());

	return hr;
}

/* Returns a list of DLL's/IUnknown interfaces. Note that
is up to the caller to free both the DLL's and objects
returned.

http://www.ureader.com/msg/16601280.aspx

Also note that a set of blacklisted CLSID entries can be
provided. Any entries in this set will be ignored (i.e. they
won't be loaded). Each entry should be a CLSID with the enclosing
braces included. */
BOOL LoadContextMenuHandlers(const TCHAR *szRegKey,
	std::list<ContextMenuHandler> &contextMenuHandlers,
	const std::vector<std::wstring> &blacklistedCLSIDEntries)
{
	HKEY hKey = nullptr;
	BOOL bSuccess = FALSE;

	LONG lRes = RegOpenKeyEx(HKEY_CLASSES_ROOT, szRegKey, 0, KEY_READ, &hKey);

	if (lRes == ERROR_SUCCESS)
	{
		TCHAR szKeyName[512];
		int iIndex = 0;

		DWORD dwLen = SIZEOF_ARRAY(szKeyName);

		while ((lRes = RegEnumKeyEx(
					hKey, iIndex, szKeyName, &dwLen, nullptr, nullptr, nullptr, nullptr))
			== ERROR_SUCCESS)
		{
			HKEY hSubKey;
			TCHAR szSubKey[512];
			TCHAR szCLSID[256];
			LONG lSubKeyRes;

			StringCchPrintf(szSubKey, SIZEOF_ARRAY(szSubKey), _T("%s\\%s"), szRegKey, szKeyName);

			lSubKeyRes = RegOpenKeyEx(HKEY_CLASSES_ROOT, szSubKey, 0, KEY_READ, &hSubKey);

			if (lSubKeyRes == ERROR_SUCCESS)
			{
				lSubKeyRes =
					RegistrySettings::ReadString(hSubKey, nullptr, szCLSID, SIZEOF_ARRAY(szCLSID));

				if (lSubKeyRes == ERROR_SUCCESS)
				{
					if (std::none_of(blacklistedCLSIDEntries.begin(), blacklistedCLSIDEntries.end(),
							[&szCLSID](const std::wstring &blacklistedEntry) {
								return boost::iequals(szCLSID, blacklistedEntry);
							}))
					{
						ContextMenuHandler contextMenuHandler;

						BOOL bRes = LoadIUnknownFromCLSID(szCLSID, &contextMenuHandler);

						if (bRes)
						{
							contextMenuHandlers.push_back(contextMenuHandler);
						}
					}
				}

				RegCloseKey(hSubKey);
			}

			dwLen = SIZEOF_ARRAY(szKeyName);
			iIndex++;
		}

		RegCloseKey(hKey);

		bSuccess = TRUE;
	}

	return bSuccess;
}

/* Extracts an IUnknown interface from a class object,
based on its CLSID. If the CLSID exists in
HKLM\Software\Classes\CLSID, the DLL for this object
will attempted to be loaded.
Regardless of whether or not a DLL was actually
loaded, the object will be initialized with a call
to CoCreateInstance. */
BOOL LoadIUnknownFromCLSID(const TCHAR *szCLSID, ContextMenuHandler *pContextMenuHandler)
{
	HKEY hCLSIDKey;
	HKEY hDllKey;
	HMODULE hDLL = nullptr;
	TCHAR szCLSIDKey[512];
	LONG lRes;
	BOOL bSuccess = FALSE;

	StringCchPrintf(szCLSIDKey, SIZEOF_ARRAY(szCLSIDKey), _T("%s\\%s"),
		_T("Software\\Classes\\CLSID"), szCLSID);

	/* Open the CLSID key. */
	lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szCLSIDKey, 0, KEY_READ, &hCLSIDKey);

	if (lRes == ERROR_SUCCESS)
	{
		lRes = RegOpenKeyEx(hCLSIDKey, _T("InProcServer32"), 0, KEY_READ, &hDllKey);

		if (lRes == ERROR_SUCCESS)
		{
			TCHAR szDLL[MAX_PATH];

			lRes = RegistrySettings::ReadString(hDllKey, nullptr, szDLL, SIZEOF_ARRAY(szDLL));

			if (lRes == ERROR_SUCCESS)
			{
				/* Now, load the DLL it refers to. */
				hDLL = LoadLibrary(szDLL);
			}

			RegCloseKey(hDllKey);
		}

		RegCloseKey(hCLSIDKey);
	}

	CLSID clsid;

	/* Regardless of whether or not any DLL was
	loaded, attempt to create the object. */
	HRESULT hr = CLSIDFromString(szCLSID, &clsid);

	if (hr == NO_ERROR)
	{
		IUnknown *pUnknown = nullptr;

		hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pUnknown));

		if (hr == S_OK)
		{
			bSuccess = TRUE;

			pContextMenuHandler->hDLL = hDLL;
			pContextMenuHandler->pUnknown = pUnknown;
		}
	}

	if (!bSuccess)
	{
		if (hDLL != nullptr)
		{
			FreeLibrary(hDLL);
		}
	}

	return bSuccess;
}

HRESULT GetItemInfoTip(const TCHAR *szItemPath, TCHAR *szInfoTip, size_t cchMax)
{
	unique_pidl_absolute pidlItem;
	HRESULT hr = SHParseDisplayName(szItemPath, nullptr, wil::out_param(pidlItem), 0, nullptr);

	if (SUCCEEDED(hr))
	{
		hr = GetItemInfoTip(pidlItem.get(), szInfoTip, cchMax);
	}

	return hr;
}

HRESULT GetItemInfoTip(PCIDLIST_ABSOLUTE pidlComplete, TCHAR *szInfoTip, size_t cchMax)
{
	IShellFolder *pShellFolder = nullptr;
	IQueryInfo *pQueryInfo = nullptr;
	PCITEMID_CHILD pidlRelative = nullptr;
	LPWSTR ppwszTip = nullptr;
	HRESULT hr;

	hr = SHBindToParent(pidlComplete, IID_PPV_ARGS(&pShellFolder), &pidlRelative);

	if (SUCCEEDED(hr))
	{
		hr = GetUIObjectOf(pShellFolder, nullptr, 1, &pidlRelative, IID_PPV_ARGS(&pQueryInfo));

		if (SUCCEEDED(hr))
		{
			hr = pQueryInfo->GetInfoTip(QITIPF_USESLOWTIP, &ppwszTip);

			if (SUCCEEDED(hr))
			{
				if (ppwszTip)
				{
					StringCchCopy(szInfoTip, cchMax, ppwszTip);
					CoTaskMemFree(ppwszTip);
				}
				else
				{
					StringCchCopy(szInfoTip, cchMax, _T(""));
				}
			}

			pQueryInfo->Release();
		}
		pShellFolder->Release();
	}

	return hr;
}

HRESULT ShowMultipleFileProperties(
	PCIDLIST_ABSOLUTE pidlDirectory, PCITEMID_CHILD *ppidl, HWND hwndOwner, int nFiles)
{
	return ExecuteActionFromContextMenu(
		pidlDirectory, ppidl, hwndOwner, nFiles, _T("properties"), 0);
}

HRESULT ExecuteActionFromContextMenu(PCIDLIST_ABSOLUTE pidlDirectory, PCITEMID_CHILD *ppidl,
	HWND hwndOwner, int nFiles, const TCHAR *szAction, DWORD fMask)
{
	IShellFolder *pShellParentFolder = nullptr;
	IShellFolder *pShellFolder = nullptr;
	IContextMenu *pContext = nullptr;
	PCITEMID_CHILD pidlRelative = nullptr;
	CMINVOKECOMMANDINFO cmici;
	HRESULT hr = S_FALSE;
	char szActionA[32];

	if (nFiles == 0)
	{
		hr = SHBindToParent(pidlDirectory, IID_PPV_ARGS(&pShellParentFolder), &pidlRelative);

		if (SUCCEEDED(hr))
		{
			hr = GetUIObjectOf(
				pShellParentFolder, hwndOwner, 1, &pidlRelative, IID_PPV_ARGS(&pContext));

			pShellParentFolder->Release();
			pShellParentFolder = nullptr;
		}
	}
	else
	{
		hr = BindToIdl(pidlDirectory, IID_PPV_ARGS(&pShellFolder));

		if (SUCCEEDED(hr))
		{
			hr = GetUIObjectOf(pShellFolder, hwndOwner, nFiles, ppidl, IID_PPV_ARGS(&pContext));
			pShellFolder->Release();
		}
	}

	if (SUCCEEDED(hr))
	{
		/* Action string MUST be ANSI. */
		WideCharToMultiByte(
			CP_ACP, 0, szAction, -1, szActionA, SIZEOF_ARRAY(szActionA), nullptr, nullptr);

		cmici.cbSize = sizeof(CMINVOKECOMMANDINFO);
		cmici.fMask = fMask;
		cmici.hwnd = hwndOwner;
		cmici.lpVerb = szActionA;
		cmici.lpParameters = nullptr;
		cmici.lpDirectory = nullptr;
		cmici.nShow = SW_SHOW;

		hr = pContext->InvokeCommand(&cmici);

		pContext->Release();
		pContext = nullptr;
	}

	return hr;
}

BOOL CompareVirtualFolders(const TCHAR *szDirectory, UINT uFolderCSIDL)
{
	std::wstring parsingPath;
	GetCsidlDisplayName(uFolderCSIDL, SHGDN_FORPARSING, parsingPath);

	if (parsingPath == szDirectory)
	{
		return TRUE;
	}

	return FALSE;
}

bool IsChildOfLibrariesFolder(PCIDLIST_ABSOLUTE pidl)
{
	unique_pidl_absolute pidlLibraries;
	HRESULT hr =
		SHGetKnownFolderIDList(FOLDERID_Libraries, 0, nullptr, wil::out_param(pidlLibraries));

	if (FAILED(hr))
	{
		return false;
	}

	return ILIsParent(pidlLibraries.get(), pidl, FALSE);
}

class FileSystemBindData : public IFileSystemBindData
{
public:
	static wil::com_ptr_nothrow<FileSystemBindData> Create(const WIN32_FIND_DATA *wfd)
	{
		wil::com_ptr_nothrow<FileSystemBindData> fsBindData;
		fsBindData.attach(new FileSystemBindData(wfd));
		return fsBindData;
	}

	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppvObject)
	{
		// clang-format off
		static const QITAB qit[] = {
			QITABENT(FileSystemBindData, IFileSystemBindData),
			{ nullptr }
		};
		// clang-format on

		return QISearch(this, qit, riid, ppvObject);
	}

	IFACEMETHODIMP_(ULONG) AddRef(void)
	{
		return InterlockedIncrement(&m_refCount);
	}

	IFACEMETHODIMP_(ULONG) Release(void)
	{
		ULONG refCount = InterlockedDecrement(&m_refCount);

		if (refCount == 0)
		{
			delete this;
		}

		return refCount;
	}

	IFACEMETHODIMP SetFindData(const WIN32_FIND_DATAW *wfd)
	{
		m_wfd = *wfd;
		return S_OK;
	}

	IFACEMETHODIMP GetFindData(WIN32_FIND_DATAW *wfd)
	{
		*wfd = m_wfd;
		return S_OK;
	}

private:
	ULONG m_refCount;
	WIN32_FIND_DATA m_wfd;

	FileSystemBindData(const WIN32_FIND_DATA *wfd) : m_refCount(1), m_wfd(*wfd)
	{
	}
};

// This performs the same function as SHSimpleIDListFromPath(), which is deprecated.
HRESULT CreateSimplePidl(const std::wstring &path, PIDLIST_ABSOLUTE *pidl)
{
	wil::com_ptr_nothrow<IBindCtx> bindCtx;
	RETURN_IF_FAILED(CreateBindCtx(0, &bindCtx));

	BIND_OPTS opts = { sizeof(opts), 0, STGM_CREATE, 0 };
	RETURN_IF_FAILED(bindCtx->SetBindOptions(&opts));

	WIN32_FIND_DATA wfd = {};
	wfd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	auto fsBindData = FileSystemBindData::Create(&wfd);

	RETURN_IF_FAILED(
		bindCtx->RegisterObjectParam(const_cast<PWSTR>(STR_FILE_SYS_BIND_DATA), fsBindData.get()));

	return SHParseDisplayName(path.c_str(), bindCtx.get(), pidl, 0, nullptr);
}

// This performs the same function as SHGetRealIDL, which is deprecated.
HRESULT SimplePidlToFullPidl(PCIDLIST_ABSOLUTE simplePidl, PIDLIST_ABSOLUTE *fullPidl)
{
	wil::com_ptr_nothrow<IShellItem2> shellItem2;
	RETURN_IF_FAILED(SHCreateItemFromIDList(simplePidl, IID_PPV_ARGS(&shellItem2)));
	RETURN_IF_FAILED(shellItem2->Update(nullptr));

	wil::com_ptr_nothrow<IParentAndItem> parentAndItem;
	RETURN_IF_FAILED(shellItem2->QueryInterface(IID_PPV_ARGS(&parentAndItem)));

	unique_pidl_absolute parent;
	unique_pidl_child child;
	RETURN_IF_FAILED(
		parentAndItem->GetParentAndItem(wil::out_param(parent), nullptr, wil::out_param(child)));

	*fullPidl = ILCombine(parent.get(), child.get());

	return S_OK;
}

std::vector<unique_pidl_absolute> DeepCopyPidls(const std::vector<PCIDLIST_ABSOLUTE> &pidls)
{
	std::vector<unique_pidl_absolute> copiedPidls;
	copiedPidls.reserve(pidls.size());
	std::transform(pidls.begin(), pidls.end(), std::back_inserter(copiedPidls),
		[](const PCIDLIST_ABSOLUTE &pidl) {
			return unique_pidl_absolute(ILCloneFull(pidl));
		});
	return copiedPidls;
}

std::vector<unique_pidl_absolute> DeepCopyPidls(const std::vector<unique_pidl_absolute> &pidls)
{
	std::vector<unique_pidl_absolute> copiedPidls;
	copiedPidls.reserve(pidls.size());
	std::transform(pidls.begin(), pidls.end(), std::back_inserter(copiedPidls),
		[](const unique_pidl_absolute &pidl) {
			return unique_pidl_absolute(ILCloneFull(pidl.get()));
		});
	return copiedPidls;
}

std::vector<PCIDLIST_ABSOLUTE> ShallowCopyPidls(const std::vector<unique_pidl_absolute> &pidls)
{
	std::vector<PCIDLIST_ABSOLUTE> rawPidls;
	rawPidls.reserve(pidls.size());
	std::transform(pidls.begin(), pidls.end(), std::back_inserter(rawPidls),
		[](const unique_pidl_absolute &pidl) {
			return pidl.get();
		});
	return rawPidls;
}