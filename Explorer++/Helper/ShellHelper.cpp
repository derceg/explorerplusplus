/******************************************************************
 *
 * Project: Helper
 * File: ShellHelper.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides various shell related functionality.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Helper.h"
#include "ShellHelper.h"
#include "FileOperations.h"
#include "Buffer.h"
#include "RegistrySettings.h"


HRESULT AddJumpListTasksInternal(IObjectCollection *poc,
	std::list<JumpListTaskInformation> TaskList);
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
	WCHAR szParsingNameW[MAX_PATH];
	HRESULT hr;

	hr = SHGetDesktopFolder(&pDesktopFolder);

	if(SUCCEEDED(hr))
	{
		#ifndef UNICODE
		MultiByteToWideChar(CP_ACP,0,szParsingName,
		-1,szParsingNameW,SIZEOF_ARRAY(szParsingNameW));
		#else
		StringCchCopy(szParsingNameW,SIZEOF_ARRAY(szParsingNameW),szParsingName);
		#endif

		hr = pDesktopFolder->ParseDisplayName(NULL,NULL,
		szParsingNameW,NULL,pidl,NULL);

		pDesktopFolder->Release();
	}

	return hr;
}

HRESULT GetDisplayName(TCHAR *szParsingPath,TCHAR *szDisplayName,DWORD uFlags)
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
		hr = GetDisplayName(pidl,szDisplayName,uFlags);
	}

	CoTaskMemFree(pidl);

	return hr;
}

HRESULT GetDisplayName(LPITEMIDLIST pidlDirectory,TCHAR *szDisplayName,DWORD uFlags)
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

	hr = SHBindToParent(pidlDirectory,IID_IShellFolder,(void **)&pShellFolder,
	(LPCITEMIDLIST *)&pidlRelative);

	if(SUCCEEDED(hr))
	{
		hr = pShellFolder->GetDisplayNameOf(pidlRelative,uFlags,&str);

		if(SUCCEEDED(hr))
		{
			StrRetToBuf(&str,pidlDirectory,szDisplayName,MAX_PATH);
		}

		pShellFolder->Release();
	}

	return hr;
}

HRESULT GetItemAttributes(TCHAR *szItemParsingPath,SFGAOF *pItemAttributes)
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

HRESULT GetItemAttributes(LPITEMIDLIST pidl,SFGAOF *pItemAttributes)
{
	if(pidl == NULL ||
		pItemAttributes == NULL)
	{
		return E_FAIL;
	}

	IShellFolder	*pShellFolder = NULL;
	LPITEMIDLIST	pidlRelative = NULL;
	HRESULT			hr;

	hr = SHBindToParent(pidl,IID_IShellFolder,(void **)&pShellFolder,
	(LPCITEMIDLIST *)&pidlRelative);

	if(SUCCEEDED(hr))
	{
		hr = pShellFolder->GetAttributesOf(1,(LPCITEMIDLIST *)&pidlRelative,pItemAttributes);

		pShellFolder->Release();
	}

	return hr;
}

BOOL ExecuteFileAction(HWND hwnd,TCHAR *szVerb,TCHAR *szParameters,TCHAR *szStartDirectory,LPCITEMIDLIST pidl)
{
	SHELLEXECUTEINFO ExecInfo;

	ExecInfo.cbSize			= sizeof(SHELLEXECUTEINFO);
	ExecInfo.fMask			= SEE_MASK_INVOKEIDLIST;
	ExecInfo.lpVerb			= szVerb;
	ExecInfo.lpIDList		= (LPVOID)pidl;
	ExecInfo.hwnd			= hwnd;
	ExecInfo.nShow			= SW_SHOW;
	ExecInfo.lpParameters	= szParameters;
	ExecInfo.lpDirectory	= szStartDirectory;
	ExecInfo.lpFile			= NULL;
	ExecInfo.hInstApp		= NULL;

	return ShellExecuteEx(&ExecInfo);
}

void GetVirtualFolderParsingPath(UINT uFolderCSIDL,TCHAR *szParsingPath)
{
	IShellFolder *pShellFolder		= NULL;
	IShellFolder *pDesktopFolder	= NULL;
	LPITEMIDLIST pidl				= NULL;
	LPITEMIDLIST pidlRelative		= NULL;
	STRRET str;
	HRESULT hr;

	hr = SHGetDesktopFolder(&pDesktopFolder);

	if(SUCCEEDED(hr))
	{
		hr = SHGetFolderLocation(NULL,uFolderCSIDL,NULL,0,&pidl);

		if(SUCCEEDED(hr))
		{
			hr = SHBindToParent(pidl,IID_IShellFolder,(void **)&pShellFolder,
			(LPCITEMIDLIST *)&pidlRelative);

			if(SUCCEEDED(hr))
			{
				hr = pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);

				if(SUCCEEDED(hr))
				{
					StrRetToBuf(&str,pidlRelative,szParsingPath,MAX_PATH);
				}

				pShellFolder->Release();
			}

			CoTaskMemFree(pidl);
		}

		pDesktopFolder->Release();
	}
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

BOOL CheckIdl(LPITEMIDLIST pidl)
{
	LPITEMIDLIST	pidlCheck = NULL;
	TCHAR			szTabText[MAX_PATH];

	if(!SUCCEEDED(GetDisplayName(pidl,szTabText,SHGDN_FORPARSING)))
		return FALSE;

	if(!SUCCEEDED(GetIdlFromParsingName(szTabText,&pidlCheck)))
		return FALSE;

	CoTaskMemFree(pidlCheck);

	return TRUE;
}

BOOL IsIdlDirectory(LPITEMIDLIST pidl)
{
	SFGAOF Attributes;

	Attributes = SFGAO_FOLDER;

	GetItemAttributes(pidl,&Attributes);

	if(Attributes & SFGAO_FOLDER)
		return TRUE;

	return FALSE;
}

HRESULT DecodeFriendlyPath(TCHAR *szFriendlyPath,TCHAR *szParsingPath)
{
	LPITEMIDLIST pidl = NULL;
	TCHAR szName[MAX_PATH];

	SHGetFolderLocation(NULL,CSIDL_CONTROLS,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_CONTROLS,szParsingPath);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_BITBUCKET,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_BITBUCKET,szParsingPath);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_DRIVES,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_DRIVES,szParsingPath);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_NETWORK,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_NETWORK,szParsingPath);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_CONNECTIONS,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_CONNECTIONS,szParsingPath);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_PRINTERS,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_PRINTERS,szParsingPath);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_FAVORITES,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_FAVORITES,szParsingPath);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_MYPICTURES,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_MYPICTURES,szParsingPath);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_MYMUSIC,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_MYMUSIC,szParsingPath);
		return S_OK;
	}

	SHGetFolderLocation(NULL,CSIDL_MYVIDEO,NULL,0,&pidl);
	GetDisplayName(pidl,szName,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szName,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_MYVIDEO,szParsingPath);
		return S_OK;
	}

	if(CompareString(LOCALE_INVARIANT,NORM_IGNORECASE,
		FRIENDLY_NAME_DESKTOP,-1,szFriendlyPath,-1) == CSTR_EQUAL)
	{
		GetVirtualFolderParsingPath(CSIDL_DESKTOP,szParsingPath);
		return S_OK;
	}

	if(CompareString(LOCALE_INVARIANT,NORM_IGNORECASE,
		FRIENDLY_NAME_PICTURES,-1,szFriendlyPath,-1) == CSTR_EQUAL)
	{
		GetVirtualFolderParsingPath(CSIDL_MYPICTURES,szParsingPath);
		return S_OK;
	}

	if(CompareString(LOCALE_INVARIANT,NORM_IGNORECASE,
		FRIENDLY_NAME_MUSIC,-1,szFriendlyPath,-1) == CSTR_EQUAL)
	{
		GetVirtualFolderParsingPath(CSIDL_MYMUSIC,szParsingPath);
		return S_OK;
	}

	if(CompareString(LOCALE_INVARIANT,NORM_IGNORECASE,
		FRIENDLY_NAME_VIDEOS,-1,szFriendlyPath,-1) == CSTR_EQUAL)
	{
		GetVirtualFolderParsingPath(CSIDL_MYVIDEO,szParsingPath);
		return S_OK;
	}

	if(CompareString(LOCALE_INVARIANT,NORM_IGNORECASE,
		FRIENDLY_NAME_DOCUMENTS,-1,szFriendlyPath,-1) == CSTR_EQUAL)
	{
		GetVirtualFolderParsingPath(CSIDL_MYDOCUMENTS,szParsingPath);
		return S_OK;
	}

	return E_FAIL;
}

BOOL IsDirectoryRoot(TCHAR *Path)
{
	TCHAR *Drives	= NULL;
	DWORD Size;
	BOOL Break		= FALSE;

	/* Find out how much space is needed to hold the list of drive
	names. */
	Size = GetLogicalDriveStrings(0,NULL);

	Drives = (TCHAR *)malloc((Size + 1) * sizeof(TCHAR));

	if(Drives == NULL)
		return FALSE;

	/* Ask the system for a list of the current drives in the system.
	This list is returned as a double NULL terminated buffer. */
	Size = GetLogicalDriveStrings(Size,Drives);

	if(Size != 0)
	{
		while(Break == FALSE)
		{
			/* String list containing drive list is double NULL terminated.
			Detect if the character after the end of the current string is
			another NULL byte. */
			if(*(Drives) == '\0')
			{
				/* Break out of the loop at the next iteration (double NULL byte
				set has being found). */
				Break = TRUE;
			}
			else
			{
				if(StrCmpI(Drives,Path) == 0)
				{
					/* The path name of this drive matches up with the supplied
					path. Thus, the supplied path is a directory root. */
					return TRUE;
				}
			}

			Drives += lstrlen(Drives) + 1;
		}
	}

	/* No matches found. The path specified is not a directory root. */
	return FALSE;
}

int GetDefaultFolderIconIndex(void)
{
	return GetDefaultIcon(DEFAULT_ICON_FOLDER);
}

int GetDefaultFileIconIndex(void)
{
	return GetDefaultIcon(DEFAULT_ICON_FILE);
}

int GetDefaultIcon(int iIconType)
{
	SHFILEINFO shfi;
	DWORD dwFileAttributes;

	switch(iIconType)
	{
		case DEFAULT_ICON_FOLDER:
			dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_NORMAL;
			break;

		case DEFAULT_ICON_FILE:
			dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			break;

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

HRESULT GetFileInfoTip(HWND hwnd,LPCITEMIDLIST pidlDirectory,LPCITEMIDLIST *pridl,
TCHAR *szInfoTip,UINT cchMax)
{
	if(pidlDirectory == NULL ||
		pridl == NULL ||
		szInfoTip == NULL ||
		cchMax == 0)
	{
		return E_FAIL;
	}

	IShellFolder *pDesktopFolder = NULL;
	IShellFolder *pShellFolder = NULL;
	IQueryInfo *pQueryInfo = NULL;
	LPWSTR ppwszTip = NULL;
	HRESULT hr;

	hr = SHGetDesktopFolder(&pDesktopFolder);

	if(SUCCEEDED(hr))
	{
		if(IsNamespaceRoot(pidlDirectory))
		{
			hr = SHGetDesktopFolder(&pShellFolder);
		}
		else
		{
			hr = pDesktopFolder->BindToObject(pidlDirectory,NULL,
				IID_IShellFolder,(void **)&pShellFolder);
		}

		if(SUCCEEDED(hr))
		{
			hr = pShellFolder->GetUIObjectOf(hwnd,1,pridl,
			IID_IQueryInfo,0,(void **)&pQueryInfo);

			if(SUCCEEDED(hr))
			{
				hr = pQueryInfo->GetInfoTip(QITIPF_USESLOWTIP,&ppwszTip);

				if(SUCCEEDED(hr) && ppwszTip != NULL)
				{
					#ifndef UNICODE
					WideCharToMultiByte(CP_ACP,0,ppwszTip,-1,szInfoTip,
					cchMax,NULL,NULL);
					#else
					StringCchCopy(szInfoTip,cchMax,ppwszTip);
					#endif

					CoTaskMemFree((LPVOID)ppwszTip);
				}
				else
				{
					/* On Windows XP, seem to be able to disable folder infotips...
					If this is done, hr will return success, but ppwszTip will be
					NULL. Just copy in the empty string, which will cause nothing to
					be shown. */
					StringCchCopy(szInfoTip,cchMax,EMPTY_STRING);
				}

				pQueryInfo->Release();
			}

			pShellFolder->Release();
		}

		pDesktopFolder->Release();
	}

	return hr;
}

HRESULT GetCsidlFolderName(UINT csidl,TCHAR *szFolderName,DWORD uParsingFlags)
{
	if(szFolderName == NULL)
	{
		return E_FAIL;
	}

	LPITEMIDLIST pidl = NULL;
	HRESULT hr;

	hr = SHGetFolderLocation(NULL,csidl,NULL,0,&pidl);

	/* Don't use SUCCEEDED(hr). */
	if(hr == S_OK)
	{
		hr = GetDisplayName(pidl,szFolderName,uParsingFlags);

		CoTaskMemFree(pidl);
	}

	return hr;
}

BOOL MyExpandEnvironmentStrings(TCHAR *szSrc,TCHAR *szExpandedPath,DWORD nSize)
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

DWORD DetermineCurrentDragEffect(DWORD grfKeyState,DWORD dwCurrentEffect,
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

HRESULT BuildHDropList(OUT FORMATETC *pftc,OUT STGMEDIUM *pstg,
	IN list<std::wstring> FilenameList)
{
	if(pftc == NULL ||
		pstg == NULL ||
		FilenameList.size() == 0)
	{
		return E_FAIL;
	}

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
HRESULT BuildShellIDList(OUT FORMATETC *pftc,OUT STGMEDIUM *pstg,
	IN LPCITEMIDLIST pidlDirectory,
	IN list<LPITEMIDLIST> pidlList)
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

HRESULT BindToShellFolder(LPCITEMIDLIST pidlDirectory,IShellFolder **pShellFolder)
{
	if(pidlDirectory == NULL ||
		pShellFolder == NULL)
	{
		return E_FAIL;
	}

	IShellFolder *pDesktopFolder = NULL;
	HRESULT hr;

	*pShellFolder = NULL;

	hr = SHGetDesktopFolder(&pDesktopFolder);

	if(SUCCEEDED(hr))
	{
		if(IsNamespaceRoot(pidlDirectory))
		{
			hr = SHGetDesktopFolder(pShellFolder);
		}
		else
		{
			hr = pDesktopFolder->BindToObject(pidlDirectory,NULL,
				IID_IShellFolder,(LPVOID *)pShellFolder);
		}
	}

	return hr;
}

/* Returns TRUE if a path is a GUID;
i.e. of the form:

::{20D04FE0-3AEA-1069-A2D8-08002B30309D}
(My Computer GUID, Windows 7)
*/
BOOL IsPathGUID(TCHAR *szPath)
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
void DecodePath(TCHAR *szInitialPath,TCHAR *szCurrentDirectory,TCHAR *szParsingPath,size_t cchDest)
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
		hr = DecodeFriendlyPath(szInitialPath,szVirtualParsingPath);

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
	if(pidl1 == NULL || pidl2 == NULL)
	{
		return FALSE;
	}

	IShellFolder *pDesktopFolder = NULL;
	HRESULT hr;
	BOOL ret = FALSE;

	hr = SHGetDesktopFolder(&pDesktopFolder);

	if(SUCCEEDED(hr))
	{
		hr = pDesktopFolder->CompareIDs(0,pidl1,pidl2);

		if(short(HRESULT_CODE(hr) == 0))
		{
			ret = TRUE;
		}

		pDesktopFolder->Release();
	}

	return ret;
}

void SetFORMATETC(FORMATETC *pftc,CLIPFORMAT cfFormat,
	DVTARGETDEVICE *ptd,DWORD dwAspect,LONG lindex,
	DWORD tymed)
{
	if(pftc == NULL)
	{
		return;
	}

	pftc->cfFormat	= cfFormat;
	pftc->tymed		= tymed;
	pftc->lindex	= lindex;
	pftc->dwAspect	= dwAspect;
	pftc->ptd		= ptd;
}

HRESULT AddJumpListTasks(std::list<JumpListTaskInformation> TaskList)
{
	if(TaskList.size() == 0)
	{
		return E_FAIL;
	}

	ICustomDestinationList *pCustomDestinationList = NULL;
	HRESULT hr;

	hr = CoCreateInstance(CLSID_DestinationList,NULL,CLSCTX_INPROC_SERVER,
		IID_ICustomDestinationList,(LPVOID *)&pCustomDestinationList);

	if(SUCCEEDED(hr))
	{
		IObjectArray *poa = NULL;
		UINT uMinSlots;

		hr = pCustomDestinationList->BeginList(&uMinSlots,IID_IObjectArray,(void **)&poa);

		if(SUCCEEDED(hr))
		{
			poa->Release();

			IObjectCollection *poc = NULL;

			hr = CoCreateInstance(CLSID_EnumerableObjectCollection,NULL,CLSCTX_INPROC_SERVER,
				IID_IObjectCollection,(LPVOID *)&poc);

			if(SUCCEEDED(hr))
			{
				AddJumpListTasksInternal(poc,TaskList);

				hr = poc->QueryInterface(IID_IObjectArray,(void **)&poa);

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
	std::list<JumpListTaskInformation> TaskList)
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
		IID_IShellLink,(LPVOID *)&pShellLink);

	if(SUCCEEDED(hr))
	{
		pShellLink->SetPath(pszPath);
		pShellLink->SetArguments(pszArguments);
		pShellLink->SetIconLocation(pszIconPath,iIcon);

		IPropertyStore *pps = NULL;
		PROPVARIANT pv;

		hr = pShellLink->QueryInterface(IID_IPropertyStore,(void **)&pps);

		if(SUCCEEDED(hr))
		{
			InitPropVariantFromString(pszName,&pv);

			/* See: http://msdn.microsoft.com/en-us/library/bb787584(VS.85).aspx */
			PROPERTYKEY PKEY_Title;
			CLSIDFromString(L"{F29F85E0-4FF9-1068-AB91-08002B27B3D9}",&PKEY_Title.fmtid);
			PKEY_Title.pid = 2;

			pps->SetValue(PKEY_Title,pv);
			pps->Commit();

			poc->AddObject(pShellLink);

			pps->Release();
		}

		pShellLink->Release();
	}

	return hr;
}

/* Returns a list of DLL's/IUnknown interfaces. Note that
is up to the caller to free both the DLL's and objects
returned.

http://www.ureader.com/msg/16601280.aspx */
BOOL LoadContextMenuHandlers(IN TCHAR *szRegKey,
	OUT list<ContextMenuHandler_t> *pContextMenuHandlers)
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
					ContextMenuHandler_t ContextMenuHandler;

					BOOL bRes = LoadIUnknownFromCLSID(szCLSID,&ContextMenuHandler);

					if(bRes)
					{
						pContextMenuHandlers->push_back(ContextMenuHandler);
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
BOOL LoadIUnknownFromCLSID(IN TCHAR *szCLSID,
OUT ContextMenuHandler_t *pContextMenuHandler)
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
			IID_IUnknown,(LPVOID *)&pUnknown);

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