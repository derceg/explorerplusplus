/******************************************************************
 *
 * Project: Helper
 * File: ShellHelper.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Provides various shell related functionality.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <boost\algorithm\string\predicate.hpp>
#include "FileOperations.h"
#include "Helper.h"
#include "Macros.h"
#include "ProcessHelper.h"
#include "RegistrySettings.h"
#include "ShellHelper.h"


HRESULT AddJumpListTasksInternal(IObjectCollection *poc,
	const std::list<JumpListTaskInformation> &TaskList);
HRESULT AddJumpListTaskInternal(IObjectCollection *poc,const TCHAR *pszName,
	const TCHAR *pszPath,const TCHAR *pszArguments,const TCHAR *pszIconPath,int iIcon);

HRESULT GetIdlFromParsingName(const TCHAR *szParsingName,LPITEMIDLIST *pidl)
{
	if(szParsingName == NULL ||
		pidl == NULL)
	{
		return E_FAIL;
	}

	IShellFolder *pDesktopFolder = NULL;
	HRESULT hr;

	hr = SHGetDesktopFolder(&pDesktopFolder);

	if(SUCCEEDED(hr))
	{
		/* For some reason, ParseDisplayName
		takes a pointer to a non-constant
		string, so copy the incoming string. */
		TCHAR szParsingNameTemp[MAX_PATH];
		StringCchCopy(szParsingNameTemp, SIZEOF_ARRAY(szParsingNameTemp), szParsingName);

		hr = pDesktopFolder->ParseDisplayName(NULL,NULL,
			szParsingNameTemp,NULL,pidl,NULL);

		pDesktopFolder->Release();
	}

	return hr;
}

HRESULT GetDisplayName(const TCHAR *szParsingPath,TCHAR *szDisplayName,UINT cchMax,DWORD uFlags)
{
	if(szParsingPath == NULL ||
		szDisplayName == NULL)
	{
		return E_FAIL;
	}

	LPITEMIDLIST pidl = NULL;
	HRESULT hr;

	hr = GetIdlFromParsingName(szParsingPath,&pidl);

	if(SUCCEEDED(hr))
	{
		hr = GetDisplayName(pidl,szDisplayName,cchMax,uFlags);

		CoTaskMemFree(pidl);
	}

	return hr;
}

HRESULT GetDisplayName(LPCITEMIDLIST pidlDirectory,TCHAR *szDisplayName,UINT cchMax,DWORD uFlags)
{
	if(pidlDirectory == NULL ||
		szDisplayName == NULL)
	{
		return E_FAIL;
	}

	IShellFolder *pShellFolder = NULL;
	LPITEMIDLIST pidlRelative = NULL;
	STRRET str;
	HRESULT hr;

	hr = SHBindToParent(pidlDirectory, IID_PPV_ARGS(&pShellFolder),
	(LPCITEMIDLIST *)&pidlRelative);

	if(SUCCEEDED(hr))
	{
		hr = pShellFolder->GetDisplayNameOf(pidlRelative,uFlags,&str);

		if(SUCCEEDED(hr))
		{
			hr = StrRetToBuf(&str,pidlDirectory,szDisplayName,cchMax);
		}

		pShellFolder->Release();
	}

	return hr;
}

HRESULT GetCsidlDisplayName(int csidl, TCHAR *szFolderName, UINT cchMax, DWORD uParsingFlags)
{
	LPITEMIDLIST pidl = NULL;
	HRESULT hr = SHGetFolderLocation(NULL, csidl, NULL, 0, &pidl);

	if(SUCCEEDED(hr))
	{
		hr = GetDisplayName(pidl, szFolderName, cchMax, uParsingFlags);
		CoTaskMemFree(pidl);
	}

	return hr;
}

HRESULT GetItemAttributes(const TCHAR *szItemParsingPath,SFGAOF *pItemAttributes)
{
	if(szItemParsingPath == NULL ||
		pItemAttributes == NULL)
	{
		return E_FAIL;
	}

	LPITEMIDLIST pidl = NULL;
	HRESULT hr;

	hr = GetIdlFromParsingName(szItemParsingPath,&pidl);

	if(SUCCEEDED(hr))
	{
		hr = GetItemAttributes(pidl,pItemAttributes);

		CoTaskMemFree(pidl);
	}

	return hr;
}

HRESULT GetItemAttributes(LPCITEMIDLIST pidl,SFGAOF *pItemAttributes)
{
	if(pidl == NULL ||
		pItemAttributes == NULL)
	{
		return E_FAIL;
	}

	IShellFolder	*pShellFolder = NULL;
	LPITEMIDLIST	pidlRelative = NULL;
	HRESULT			hr;

	hr = SHBindToParent(pidl, IID_PPV_ARGS(&pShellFolder),
	(LPCITEMIDLIST *)&pidlRelative);

	if(SUCCEEDED(hr))
	{
		hr = pShellFolder->GetAttributesOf(1,(LPCITEMIDLIST *)&pidlRelative,pItemAttributes);

		pShellFolder->Release();
	}

	return hr;
}

BOOL ExecuteFileAction(HWND hwnd,const TCHAR *szVerb,const TCHAR *szParameters,const TCHAR *szStartDirectory,LPCITEMIDLIST pidl)
{
	SHELLEXECUTEINFO sei;

	sei.cbSize			= sizeof(SHELLEXECUTEINFO);
	sei.fMask			= SEE_MASK_INVOKEIDLIST;
	sei.lpVerb			= szVerb;
	sei.lpIDList		= (LPVOID)pidl;
	sei.hwnd			= hwnd;
	sei.nShow			= SW_SHOW;
	sei.lpParameters	= szParameters;
	sei.lpDirectory		= szStartDirectory;
	sei.lpFile			= NULL;
	sei.hInstApp		= NULL;

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

	sei.cbSize			= sizeof(sei);
	sei.fMask			= SEE_MASK_DEFAULT;
	sei.lpVerb			= _T("open");
	sei.lpFile			= szProcess;
	sei.lpParameters	= szParameters;
	sei.lpDirectory		= NULL;
	sei.hwnd			= hwnd;
	sei.nShow			= SW_SHOW;

	return ShellExecuteEx(&sei);
}

HRESULT GetVirtualParentPath(LPITEMIDLIST pidlDirectory,LPITEMIDLIST *pidlParent)
{
	if(IsNamespaceRoot(pidlDirectory))
	{
		*pidlParent = NULL;
	}
	else
	{
		ILRemoveLastID(pidlDirectory);
		*pidlParent = ILClone(pidlDirectory);
	}

	return S_OK;
}

BOOL IsNamespaceRoot(LPCITEMIDLIST pidl)
{
	LPITEMIDLIST pidlDesktop	= NULL;
	BOOL bNamespaceRoot			= FALSE;
	HRESULT hr;

	hr = SHGetFolderLocation(NULL,CSIDL_DESKTOP,NULL,0,&pidlDesktop);

	if(SUCCEEDED(hr))
	{
		bNamespaceRoot = CompareIdls(pidl,pidlDesktop);

		CoTaskMemFree(pidlDesktop);
	}

	return bNamespaceRoot;
}

BOOL CheckIdl(LPCITEMIDLIST pidl)
{
	LPITEMIDLIST	pidlCheck = NULL;
	TCHAR			szTabText[MAX_PATH];

	if(!SUCCEEDED(GetDisplayName(pidl,szTabText,SIZEOF_ARRAY(szTabText),SHGDN_FORPARSING)))
		return FALSE;

	if(!SUCCEEDED(GetIdlFromParsingName(szTabText,&pidlCheck)))
		return FALSE;

	CoTaskMemFree(pidlCheck);

	return TRUE;
}

BOOL IsIdlDirectory(LPCITEMIDLIST pidl)
{
	SFGAOF Attributes;

	Attributes = SFGAO_FOLDER;

	GetItemAttributes(pidl,&Attributes);

	if(Attributes & SFGAO_FOLDER)
		return TRUE;

	return FALSE;
}

HRESULT DecodeFriendlyPath(const TCHAR *szFriendlyPath,TCHAR *szParsingPath,UINT cchMax)
{
	LPITEMIDLIST pidl = NULL;
	TCHAR szName[MAX_PATH];

	SHGetFolderLocation(NULL,CSIDL_CONTROLS,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SIZEOF_ARRAY(szName),SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetCsidlDisplayName(CSIDL_CONTROLS,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_BITBUCKET,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SIZEOF_ARRAY(szName),SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetCsidlDisplayName(CSIDL_BITBUCKET,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_DRIVES,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SIZEOF_ARRAY(szName),SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetCsidlDisplayName(CSIDL_DRIVES,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_NETWORK,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SIZEOF_ARRAY(szName),SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetCsidlDisplayName(CSIDL_NETWORK,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_CONNECTIONS,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SIZEOF_ARRAY(szName),SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetCsidlDisplayName(CSIDL_CONNECTIONS,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_PRINTERS,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SIZEOF_ARRAY(szName),SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetCsidlDisplayName(CSIDL_PRINTERS,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_FAVORITES,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SIZEOF_ARRAY(szName),SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetCsidlDisplayName(CSIDL_FAVORITES,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_MYPICTURES,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SIZEOF_ARRAY(szName),SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetCsidlDisplayName(CSIDL_MYPICTURES,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_MYMUSIC,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SIZEOF_ARRAY(szName),SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetCsidlDisplayName(CSIDL_MYMUSIC,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_MYVIDEO,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SIZEOF_ARRAY(szName),SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetCsidlDisplayName(CSIDL_MYVIDEO,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	if(CompareString(LOCALE_INVARIANT,NORM_IGNORECASE,
		FRIENDLY_NAME_DESKTOP,-1,szFriendlyPath,-1) == CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_DESKTOP,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	if(CompareString(LOCALE_INVARIANT,NORM_IGNORECASE,
		FRIENDLY_NAME_PICTURES,-1,szFriendlyPath,-1) == CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_MYPICTURES,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	if(CompareString(LOCALE_INVARIANT,NORM_IGNORECASE,
		FRIENDLY_NAME_MUSIC,-1,szFriendlyPath,-1) == CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_MYMUSIC,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	if(CompareString(LOCALE_INVARIANT,NORM_IGNORECASE,
		FRIENDLY_NAME_VIDEOS,-1,szFriendlyPath,-1) == CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_MYVIDEO,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	if(CompareString(LOCALE_INVARIANT,NORM_IGNORECASE,
		FRIENDLY_NAME_DOCUMENTS,-1,szFriendlyPath,-1) == CSTR_EQUAL)
	{
		GetCsidlDisplayName(CSIDL_MYDOCUMENTS,szParsingPath,cchMax,SHGDN_FORPARSING);
		return S_OK;
	}

	return E_FAIL;
}

int GetDefaultFolderIconIndex(void)
{
	return GetDefaultIcon(DEFAULT_ICON_FOLDER);
}

int GetDefaultFileIconIndex(void)
{
	return GetDefaultIcon(DEFAULT_ICON_FILE);
}

int GetDefaultIcon(DefaultIconType defaultIconType)
{
	SHFILEINFO shfi;
	DWORD dwFileAttributes;

	switch(defaultIconType)
	{
		case DEFAULT_ICON_FOLDER:
			dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_NORMAL;
			break;

		case DEFAULT_ICON_FILE:
		default:
			dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			break;
	}

	/* Under unicode, the filename argument cannot be NULL,
	as it is not a valid unicode character. */
	SHGetFileInfo(_T("dummy"),dwFileAttributes,&shfi,
	sizeof(SHFILEINFO),SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);

	return shfi.iIcon;
}

BOOL MyExpandEnvironmentStrings(const TCHAR *szSrc,TCHAR *szExpandedPath,DWORD nSize)
{
	HANDLE hProcess;
	HANDLE hToken;
	BOOL bRet = FALSE;

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,GetCurrentProcessId());

	if(hProcess != NULL)
	{
		bRet = OpenProcessToken(hProcess,TOKEN_IMPERSONATE|TOKEN_QUERY,&hToken);

		if(bRet)
		{
			bRet = ExpandEnvironmentStringsForUser(hToken,szSrc,
				szExpandedPath,nSize);

			CloseHandle(hToken);
		}

		CloseHandle(hProcess);
	}

	return bRet;
}

DWORD DetermineDragEffect(DWORD grfKeyState,DWORD dwCurrentEffect,
BOOL bDataAccept,BOOL bOnSameDrive)
{
	DWORD dwEffect = DROPEFFECT_NONE;

	if(!bDataAccept)
	{
		dwEffect = DROPEFFECT_NONE;
	}
	else
	{
		/* Test the state of modifier keys. */
		if((((grfKeyState & MK_CONTROL) == MK_CONTROL &&
			(grfKeyState & MK_SHIFT) == MK_SHIFT) ||
			(grfKeyState & MK_ALT) == MK_ALT) &&
			dwCurrentEffect & DROPEFFECT_LINK)
		{
			/* Shortcut. */
			dwEffect = DROPEFFECT_LINK;
		}
		else if((grfKeyState & MK_SHIFT) == MK_SHIFT &&
			dwCurrentEffect & DROPEFFECT_MOVE)
		{
			/* Move. */
			dwEffect = DROPEFFECT_MOVE;
		}
		else if((grfKeyState & MK_CONTROL) == MK_CONTROL &&
			dwCurrentEffect & DROPEFFECT_COPY)
		{
			/* Copy. */
			dwEffect = DROPEFFECT_COPY;
		}
		else
		{
			/* No modifier. Determine the drag effect from
			the location of the source and destination
			directories. */
			if(bOnSameDrive && (dwCurrentEffect & DROPEFFECT_MOVE))
				dwEffect = DROPEFFECT_MOVE;
			else if(dwCurrentEffect & DROPEFFECT_COPY)
				dwEffect = DROPEFFECT_COPY;

			if(dwEffect == DROPEFFECT_NONE)
			{
				/* No suitable drop type found above. Use whichever
				method is available. */
				if(dwCurrentEffect & DROPEFFECT_MOVE)
					dwEffect = DROPEFFECT_MOVE;
				else if(dwCurrentEffect & DROPEFFECT_COPY)
					dwEffect = DROPEFFECT_COPY;
				else if(dwCurrentEffect & DROPEFFECT_LINK)
					dwEffect = DROPEFFECT_LINK;
				else
					dwEffect = DROPEFFECT_NONE;
			}
		}
	}

	return dwEffect;
}

HRESULT BuildHDropList(FORMATETC *pftc,STGMEDIUM *pstg,
	const std::list<std::wstring> &FilenameList)
{
	SetFORMATETC(pftc,CF_HDROP,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL);

	UINT uSize = 0;

	uSize = sizeof(DROPFILES);

	for each(auto Filename in FilenameList)
	{
		uSize += static_cast<UINT>((Filename.length() + 1) * sizeof(TCHAR));
	}

	/* The last string is double-null terminated. */
	uSize += (1 * sizeof(TCHAR));

	HGLOBAL hglbHDrop = GlobalAlloc(GMEM_MOVEABLE,uSize);

	if(hglbHDrop == NULL)
	{
		return E_FAIL;
	}

	LPVOID pcidaData = static_cast<LPVOID>(GlobalLock(hglbHDrop));

	DROPFILES *pdf = static_cast<DROPFILES *>(pcidaData);

	pdf->pFiles = sizeof(DROPFILES);
	pdf->fNC = FALSE;
	pdf->pt.x = 0;
	pdf->pt.y = 0;
	pdf->fWide = TRUE;

	LPBYTE pData;
	UINT uOffset = 0;

	TCHAR chNull = '\0';

	for each(auto Filename in FilenameList)
	{
		pData = static_cast<LPBYTE>(pcidaData) + sizeof(DROPFILES) + uOffset;

		memcpy(pData,Filename.c_str(),(Filename.length() + 1) * sizeof(TCHAR));
		uOffset += static_cast<UINT>((Filename.length() + 1) * sizeof(TCHAR));
	}

	/* Copy the last null byte. */
	pData = static_cast<LPBYTE>(pcidaData) + sizeof(DROPFILES) + uOffset;
	memcpy(pData,&chNull,(1 * sizeof(TCHAR)));

	GlobalUnlock(hglbHDrop);

	pstg->pUnkForRelease	= 0;
	pstg->hGlobal			= hglbHDrop;
	pstg->tymed				= TYMED_HGLOBAL;

	return S_OK;
}

/* Builds a CIDA structure. Returns the structure and its size
via arguments.
Returns S_OK on success; E_FAIL on failure. */
HRESULT BuildShellIDList(FORMATETC *pftc,STGMEDIUM *pstg,
	LPCITEMIDLIST pidlDirectory,
	const std::list<LPITEMIDLIST> &pidlList)
{
	if(pftc == NULL ||
		pstg == NULL ||
		pidlDirectory == NULL ||
		pidlList.size() == 0)
	{
		return E_FAIL;
	}

	SetFORMATETC(pftc,(CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST),
		NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL);

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
	for each(auto pidl in pidlList)
	{
		uSize += ILGetSize(pidl);
	}

	HGLOBAL hglbIDList = GlobalAlloc(GMEM_MOVEABLE,uSize);

	if(hglbIDList == NULL)
	{
		return E_FAIL;
	}

	LPVOID pcidaData = static_cast<LPVOID>(GlobalLock(hglbIDList));

	CIDA *pcida = static_cast<CIDA *>(pcidaData);

	pcida->cidl = nItems;

	UINT *pOffsets = pcida->aoffset;

	pOffsets[0] = uBaseSize;

	LPBYTE pData;

	pData = (LPBYTE)(((LPBYTE)pcida) + pcida->aoffset[0]);

	memcpy(pData,(LPVOID)pidlDirectory,
		ILGetSize(pidlDirectory));

	UINT uPreviousSize;
	int i = 0;

	uPreviousSize = ILGetSize(pidlDirectory);

	/* Store each of the pidl's. */
	for each(auto pidl in pidlList)
	{
		pOffsets[i + 1] = pOffsets[i] + uPreviousSize;

		pData = (LPBYTE)(((LPBYTE)pcida) + pcida->aoffset[i + 1]);

		memcpy(pData,(LPVOID)pidl,
			ILGetSize(pidl));

		uPreviousSize = ILGetSize(pidl);

		i++;
	}

	GlobalUnlock(hglbIDList);

	pstg->pUnkForRelease	= 0;
	pstg->hGlobal			= hglbIDList;
	pstg->tymed				= TYMED_HGLOBAL;

	return S_OK;
}

HRESULT BindToIdl(LPCITEMIDLIST pidl, REFIID riid, void **ppv)
{
	IShellFolder *pDesktop = NULL;
	HRESULT hr = SHGetDesktopFolder(&pDesktop);

	if(SUCCEEDED(hr))
	{
		/* See http://blogs.msdn.com/b/oldnewthing/archive/2011/08/30/10202076.aspx. */
		if(pidl->mkid.cb)
		{
			hr = pDesktop->BindToObject(pidl, NULL, riid, ppv);
		}
		else
		{
			hr = pDesktop->QueryInterface(riid, ppv);
		}

		pDesktop->Release();
	}

	return hr;
}

HRESULT GetUIObjectOf(IShellFolder *pShellFolder, HWND hwndOwner,
	UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, void **ppv)
{
	return pShellFolder->GetUIObjectOf(hwndOwner, cidl, apidl, riid, nullptr, ppv);
}

HRESULT GetShellItemDetailsEx(IShellFolder2 *pShellFolder, const SHCOLUMNID *pscid,
	PCUITEMID_CHILD pidl, TCHAR *szDetail, size_t cchMax, BOOL friendlyDate)
{
	VARIANT vt;
	HRESULT hr = pShellFolder->GetDetailsEx(pidl, pscid, &vt);

	if(SUCCEEDED(hr))
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
		hr = ConvertDateVariantToString(vt->date, szDetail, cchMax, friendlyDate);
		break;

	default:
		hr = ConvertGenericVariantToString(vt, szDetail, cchMax);
		break;
	}

	return hr;
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

HRESULT ConvertDateVariantToString(DATE date, TCHAR *szDetail, size_t cchMax, BOOL friendlyDate)
{
	SYSTEMTIME systemTime;
	int variantConverted = VariantTimeToSystemTime(date, &systemTime);

	if (!variantConverted)
	{
		return E_FAIL;
	}

	SYSTEMTIME localSystemTime;
	BOOL systemTimeConverted = SystemTimeToTzSpecificLocalTime(nullptr, &systemTime, &localSystemTime);

	if (!systemTimeConverted)
	{
		return E_FAIL;
	}

	BOOL dateStringCreated = CreateSystemTimeString(&localSystemTime, szDetail, cchMax, friendlyDate);

	if (!dateStringCreated)
	{
		return E_FAIL;
	}

	return S_OK;
}

/* Returns TRUE if a path is a GUID;
i.e. of the form:

::{20D04FE0-3AEA-1069-A2D8-08002B30309D}
(My Computer GUID, Windows 7)
*/
BOOL IsPathGUID(const TCHAR *szPath)
{
	if(szPath == NULL)
	{
		return FALSE;
	}

	if(lstrlen(szPath) > 2)
	{
		if(szPath[0] == ':' &&
			szPath[1] == ':')
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
void DecodePath(const TCHAR *szInitialPath,const TCHAR *szCurrentDirectory,TCHAR *szParsingPath,size_t cchDest)
{
	TCHAR szExpandedPath[MAX_PATH];
	TCHAR szCanonicalPath[MAX_PATH];
	TCHAR szVirtualParsingPath[MAX_PATH];
	HRESULT hr;
	BOOL bRelative;
	BOOL bRet;

	/* If the path starts with "::", then this is a GUID for
	a particular folder. Copy it straight to the output. */
	if(lstrlen(szInitialPath) >= 2 && szInitialPath[0] == ':'
		&& szInitialPath[1] == ':')
	{
		StringCchCopy(szParsingPath,cchDest,szInitialPath);
	}
	else
	{
		hr = DecodeFriendlyPath(szInitialPath,szVirtualParsingPath,SIZEOF_ARRAY(szVirtualParsingPath));

		if(SUCCEEDED(hr))
		{
			StringCchCopy(szParsingPath,cchDest,szVirtualParsingPath);
		}
		else
		{
			/* Attempt to expand the path (in the event that
			it contains embedded environment variables). */
			bRet = MyExpandEnvironmentStrings(szInitialPath,
				szExpandedPath,SIZEOF_ARRAY(szExpandedPath));

			if(!bRet)
			{
				StringCchCopy(szExpandedPath,
					SIZEOF_ARRAY(szExpandedPath),szInitialPath);
			}

			/* Canonicalizing the path will remove any "." and
			".." components. */
			PathCanonicalize(szCanonicalPath,szExpandedPath);

			if(PathIsURL(szCanonicalPath))
			{
				StringCchCopy(szParsingPath,cchDest,szCanonicalPath);
			}
			else
			{
				bRelative = PathIsRelative(szCanonicalPath);

				/* If the path is relative, prepend it
				with the current directory. */
				if(bRelative)
				{
					StringCchCopy(szParsingPath,cchDest,szCurrentDirectory);
					PathAppend(szParsingPath,szCanonicalPath);
				}
				else
				{
					StringCchCopy(szParsingPath,cchDest,szCanonicalPath);
				}
			}
		}
	}
}

BOOL CompareIdls(LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2)
{
	IShellFolder *pDesktopFolder = NULL;
	HRESULT hr;
	BOOL ret = FALSE;

	hr = SHGetDesktopFolder(&pDesktopFolder);

	if(SUCCEEDED(hr))
	{
		hr = pDesktopFolder->CompareIDs(0,pidl1,pidl2);

		if(HRESULT_CODE(hr) == 0)
		{
			ret = TRUE;
		}

		pDesktopFolder->Release();
	}

	return ret;
}

HRESULT AddJumpListTasks(const std::list<JumpListTaskInformation> &TaskList)
{
	if(TaskList.size() == 0)
	{
		return E_FAIL;
	}

	ICustomDestinationList *pCustomDestinationList = NULL;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_DestinationList,NULL,CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pCustomDestinationList));

	if(SUCCEEDED(hr))
	{
		IObjectArray *poa = NULL;
		UINT uMinSlots;

		hr = pCustomDestinationList->BeginList(&uMinSlots, IID_PPV_ARGS(&poa));

		if(SUCCEEDED(hr))
		{
			poa->Release();

			IObjectCollection *poc = NULL;

			hr = CoCreateInstance(CLSID_EnumerableObjectCollection,NULL,CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&poc));

			if(SUCCEEDED(hr))
			{
				AddJumpListTasksInternal(poc,TaskList);

				hr = poc->QueryInterface(IID_PPV_ARGS(&poa));

				if(SUCCEEDED(hr))
				{
					pCustomDestinationList->AddUserTasks(poa);
					pCustomDestinationList->CommitList();

					poa->Release();
				}

				poc->Release();
			}
		}

		pCustomDestinationList->Release();
	}

	return hr;
}

HRESULT AddJumpListTasksInternal(IObjectCollection *poc,
	const std::list<JumpListTaskInformation> &TaskList)
{
	for each(auto jtli in TaskList)
	{
		AddJumpListTaskInternal(poc,jtli.pszName,
			jtli.pszPath,jtli.pszArguments,
			jtli.pszIconPath,jtli.iIcon);
	}

	return S_OK;
}

HRESULT AddJumpListTaskInternal(IObjectCollection *poc,const TCHAR *pszName,
	const TCHAR *pszPath,const TCHAR *pszArguments,const TCHAR *pszIconPath,int iIcon)
{
	if(poc == NULL ||
		pszName == NULL ||
		pszPath == NULL ||
		pszArguments == NULL ||
		pszIconPath == NULL)
	{
		return E_FAIL;
	}

	IShellLink *pShellLink = NULL;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pShellLink));

	if(SUCCEEDED(hr))
	{
		pShellLink->SetPath(pszPath);
		pShellLink->SetArguments(pszArguments);
		pShellLink->SetIconLocation(pszIconPath,iIcon);

		IPropertyStore *pps = NULL;
		PROPVARIANT pv;

		hr = pShellLink->QueryInterface(IID_PPV_ARGS(&pps));

		if(SUCCEEDED(hr))
		{
			hr = InitPropVariantFromString(pszName,&pv);

			if(SUCCEEDED(hr))
			{
				/* See: http://msdn.microsoft.com/en-us/library/bb787584(VS.85).aspx */
				PROPERTYKEY keyTitle;
				keyTitle.pid = 2;
				hr = CLSIDFromString(L"{F29F85E0-4FF9-1068-AB91-08002B27B3D9}",&keyTitle.fmtid);

				if(SUCCEEDED(hr))
				{
					pps->SetValue(keyTitle,pv);
					pps->Commit();

					poc->AddObject(pShellLink);
				}

				PropVariantClear(&pv);
			}

			pps->Release();
		}

		pShellLink->Release();
	}

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
	std::list<ContextMenuHandler_t> &ContextMenuHandlers,
	const std::vector<std::wstring> &blacklistedCLSIDEntries)
{
	HKEY hKey = NULL;
	BOOL bSuccess = FALSE;

	LONG lRes = RegOpenKeyEx(HKEY_CLASSES_ROOT,szRegKey,
		0,KEY_READ,&hKey);

	if(lRes == ERROR_SUCCESS)
	{
		TCHAR szKeyName[512];
		int iIndex = 0;

		DWORD dwLen = SIZEOF_ARRAY(szKeyName);

		while((lRes = RegEnumKeyEx(hKey,iIndex,szKeyName,
			&dwLen,NULL,NULL,NULL,NULL)) == ERROR_SUCCESS)
		{
			HKEY hSubKey;
			TCHAR szSubKey[512];
			TCHAR szCLSID[256];
			LONG lSubKeyRes;

			StringCchPrintf(szSubKey,SIZEOF_ARRAY(szSubKey),
				_T("%s\\%s"),szRegKey,szKeyName);

			lSubKeyRes = RegOpenKeyEx(HKEY_CLASSES_ROOT,szSubKey,0,KEY_READ,&hSubKey);

			if(lSubKeyRes == ERROR_SUCCESS)
			{
				lSubKeyRes = NRegistrySettings::ReadStringFromRegistry(hSubKey,NULL,szCLSID,
					SIZEOF_ARRAY(szCLSID));

				if(lSubKeyRes == ERROR_SUCCESS)
				{
					if (std::none_of(blacklistedCLSIDEntries.begin(), blacklistedCLSIDEntries.end(),
						[&szCLSID](std::wstring blacklistedEntry) { return boost::iequals(szCLSID, blacklistedEntry); }))
					{
						ContextMenuHandler_t ContextMenuHandler;

						BOOL bRes = LoadIUnknownFromCLSID(szCLSID, &ContextMenuHandler);

						if (bRes)
						{
							ContextMenuHandlers.push_back(ContextMenuHandler);
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
BOOL LoadIUnknownFromCLSID(const TCHAR *szCLSID,
	ContextMenuHandler_t *pContextMenuHandler)
{
	HKEY hCLSIDKey;
	HKEY hDllKey;
	HMODULE hDLL = NULL;
	TCHAR szCLSIDKey[512];
	LONG lRes;
	BOOL bSuccess = FALSE;

	StringCchPrintf(szCLSIDKey,SIZEOF_ARRAY(szCLSIDKey),
		_T("%s\\%s"),_T("Software\\Classes\\CLSID"),szCLSID);

	/* Open the CLSID key. */
	lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szCLSIDKey,0,KEY_READ,&hCLSIDKey);

	if(lRes == ERROR_SUCCESS)
	{
		lRes = RegOpenKeyEx(hCLSIDKey,_T("InProcServer32"),0,KEY_READ,&hDllKey);

		if(lRes == ERROR_SUCCESS)
		{
			TCHAR szDLL[MAX_PATH];

			lRes = NRegistrySettings::ReadStringFromRegistry(hDllKey,NULL,szDLL,SIZEOF_ARRAY(szDLL));

			if(lRes == ERROR_SUCCESS)
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
	HRESULT hr = CLSIDFromString(szCLSID,&clsid);

	if(hr == NO_ERROR)
	{
		IUnknown *pUnknown = NULL;

		hr = CoCreateInstance(clsid,NULL,CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&pUnknown));

		if(hr == S_OK)
		{
			bSuccess = TRUE;

			pContextMenuHandler->hDLL = hDLL;
			pContextMenuHandler->pUnknown = pUnknown;
		}
	}

	if(!bSuccess)
	{
		if(hDLL != NULL)
		{
			FreeLibrary(hDLL);
		}
	}

	return bSuccess;
}

HRESULT GetItemInfoTip(const TCHAR *szItemPath, TCHAR *szInfoTip, size_t cchMax)
{
	LPITEMIDLIST	pidlItem = NULL;
	HRESULT			hr;

	hr = GetIdlFromParsingName(szItemPath, &pidlItem);

	if(SUCCEEDED(hr))
	{
		hr = GetItemInfoTip(pidlItem, szInfoTip, cchMax);

		CoTaskMemFree(pidlItem);
	}

	return hr;
}

HRESULT GetItemInfoTip(LPCITEMIDLIST pidlComplete, TCHAR *szInfoTip, size_t cchMax)
{
	IShellFolder	*pShellFolder = NULL;
	IQueryInfo		*pQueryInfo = NULL;
	LPCITEMIDLIST	pidlRelative = NULL;
	LPWSTR			ppwszTip = NULL;
	HRESULT			hr;

	hr = SHBindToParent(pidlComplete, IID_PPV_ARGS(&pShellFolder), &pidlRelative);

	if(SUCCEEDED(hr))
	{
		hr = GetUIObjectOf(pShellFolder, NULL, 1, &pidlRelative,
			IID_PPV_ARGS(&pQueryInfo));

		if(SUCCEEDED(hr))
		{
			hr = pQueryInfo->GetInfoTip(QITIPF_USESLOWTIP, &ppwszTip);

			if(SUCCEEDED(hr) && (ppwszTip != NULL))
			{
				StringCchCopy(szInfoTip, cchMax, ppwszTip);
				CoTaskMemFree(ppwszTip);
			}

			pQueryInfo->Release();
		}
		pShellFolder->Release();
	}

	return hr;
}

HRESULT ExecuteActionFromContextMenu(LPITEMIDLIST pidlDirectory,
	LPCITEMIDLIST *ppidl, HWND hwndOwner, int nFiles, const TCHAR *szAction, DWORD fMask)
{
	IShellFolder		*pShellParentFolder = NULL;
	IShellFolder		*pShellFolder = NULL;
	IContextMenu		*pContext = NULL;
	LPITEMIDLIST		pidlRelative = NULL;
	CMINVOKECOMMANDINFO	cmici;
	HRESULT				hr = S_FALSE;
	char				szActionA[32];

	if(nFiles == 0)
	{
		hr = SHBindToParent(pidlDirectory, IID_PPV_ARGS(&pShellParentFolder),
			(LPCITEMIDLIST *) &pidlRelative);

		if(SUCCEEDED(hr))
		{
			hr = GetUIObjectOf(pShellParentFolder, hwndOwner, 1,
				(LPCITEMIDLIST *) &pidlRelative, IID_PPV_ARGS(&pContext));

			pShellParentFolder->Release();
			pShellParentFolder = NULL;
		}
	}
	else
	{
		hr = BindToIdl(pidlDirectory, IID_PPV_ARGS(&pShellFolder));

		if(SUCCEEDED(hr))
		{
			hr = GetUIObjectOf(pShellFolder, hwndOwner, nFiles,
				ppidl, IID_PPV_ARGS(&pContext));
			pShellFolder->Release();
		}
	}

	if(SUCCEEDED(hr))
	{
		/* Action string MUST be ANSI. */
#ifdef UNICODE
		WideCharToMultiByte(CP_ACP, 0, szAction, -1, szActionA,
			SIZEOF_ARRAY(szActionA), NULL, NULL);
#else
		StringCchCopy(szActionA, SIZEOF_ARRAY(szActionA), szAction);
#endif

		cmici.cbSize = sizeof(CMINVOKECOMMANDINFO);
		cmici.fMask = fMask;
		cmici.hwnd = hwndOwner;
		cmici.lpVerb = szActionA;
		cmici.lpParameters = NULL;
		cmici.lpDirectory = NULL;
		cmici.nShow = SW_SHOW;

		hr = pContext->InvokeCommand(&cmici);

		pContext->Release();
		pContext = NULL;
	}

	return hr;
}