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
#include "FileOperations.h"
#include "Buffer.h"

HRESULT GetIdlFromParsingName(TCHAR *szParsingName,LPITEMIDLIST *pidl)
{
	IShellFolder	*pDesktopFolder = NULL;
	WCHAR			szParsingNameW[MAX_PATH];
	HRESULT			hr;

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
	LPITEMIDLIST pidl				= NULL;
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
	IShellFolder	*pShellFolder = NULL;
	LPITEMIDLIST	pidlRelative = NULL;
	STRRET			str;
	HRESULT			hr;

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
	LPITEMIDLIST pidl				= NULL;
	HRESULT hr;

	hr = GetIdlFromParsingName(szItemParsingPath,&pidl);

	if(SUCCEEDED(hr))
	{
		GetItemAttributes(pidl,pItemAttributes);

		CoTaskMemFree(pidl);
	}

	return hr;
}

HRESULT GetItemAttributes(LPITEMIDLIST pidl,SFGAOF *pItemAttributes)
{
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

BOOL ExecuteFileAction(HWND hwnd,TCHAR *szVerb,TCHAR *szStartDirectory,LPCITEMIDLIST pidl)
{
	SHELLEXECUTEINFO ExecInfo;

	ExecInfo.cbSize			= sizeof(SHELLEXECUTEINFO);
	ExecInfo.fMask			= SEE_MASK_INVOKEIDLIST;
	ExecInfo.lpVerb			= szVerb;
	ExecInfo.lpIDList		= (LPVOID)pidl;
	ExecInfo.hwnd			= hwnd;
	ExecInfo.nShow			= SW_SHOW;
	ExecInfo.lpParameters	= EMPTY_STRING;
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
		bNamespaceRoot = ILIsEqual(pidl,pidlDesktop);

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
	LPITEMIDLIST	pidl = NULL;
	TCHAR			szControlPanel[MAX_PATH];
	TCHAR			szRecycleBin[MAX_PATH];
	TCHAR			szComputer[MAX_PATH];
	TCHAR			szNetworkPlaces[MAX_PATH];
	TCHAR			szNetworkConnections[MAX_PATH];
	TCHAR			szPrinters[MAX_PATH];

	SHGetFolderLocation(NULL,CSIDL_CONTROLS,NULL,0,&pidl);
	GetDisplayName(pidl,szControlPanel,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	SHGetFolderLocation(NULL,CSIDL_BITBUCKET,NULL,0,&pidl);
	GetDisplayName(pidl,szRecycleBin,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	SHGetFolderLocation(NULL,CSIDL_DRIVES,NULL,0,&pidl);
	GetDisplayName(pidl,szComputer,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	SHGetFolderLocation(NULL,CSIDL_NETWORK,NULL,0,&pidl);
	GetDisplayName(pidl,szNetworkPlaces,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	SHGetFolderLocation(NULL,CSIDL_CONNECTIONS,NULL,0,&pidl);
	GetDisplayName(pidl,szNetworkConnections,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	SHGetFolderLocation(NULL,CSIDL_PRINTERS,NULL,0,&pidl);
	GetDisplayName(pidl,szPrinters,SHGDN_INFOLDER);
	CoTaskMemFree(pidl);

	if(lstrcmpi(szControlPanel,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_CONTROLS,szParsingPath);
		return S_OK;
	}
	else if(lstrcmpi(szRecycleBin,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_BITBUCKET,szParsingPath);
		return S_OK;
	}
	else if(lstrcmpi(szComputer,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_DRIVES,szParsingPath);
		return S_OK;
	}
	else if(lstrcmpi(szNetworkPlaces,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_NETWORK,szParsingPath);
		return S_OK;
	}
	else if(lstrcmpi(szNetworkConnections,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_CONNECTIONS,szParsingPath);
		return S_OK;
	}
	else if(lstrcmpi(szPrinters,szFriendlyPath) == 0)
	{
		GetVirtualFolderParsingPath(CSIDL_PRINTERS,szParsingPath);
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

HRESULT GetFileInfoTip(HWND hwnd,LPCITEMIDLIST pidlDirectory,LPCITEMIDLIST pridl,
TCHAR *szInfoTip,UINT cchMax)
{
	IShellFolder	*pDesktopFolder = NULL;
	IShellFolder	*pShellFolder = NULL;
	IQueryInfo		*pQueryInfo = NULL;
	LPWSTR			ppwszTip = NULL;
	HRESULT			hr;

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
			hr = pShellFolder->GetUIObjectOf(hwnd,1,&pridl,
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
	LPITEMIDLIST	pidl = NULL;
	HRESULT			hr;

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
	HANDLE	hProcess;
	HANDLE	hToken;
	BOOL	bRet = FALSE;

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