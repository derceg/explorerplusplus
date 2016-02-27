/******************************************************************
*
* Project: Explorer++
* File: Navigation.cpp
* License: GPL - See LICENSE in the top level directory
*
* Navigation-related functionality.
*
* Written by David Erceg
* www.explorerplusplus.com
*
*****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"


void Explorerplusplus::OnBrowseBack()
{
	BrowseFolder(EMPTY_STRING,
		SBSP_NAVIGATEBACK | SBSP_SAMEBROWSER);
}

void Explorerplusplus::OnBrowseForward()
{
	BrowseFolder(EMPTY_STRING,
		SBSP_NAVIGATEFORWARD | SBSP_SAMEBROWSER);
}

void Explorerplusplus::OnHome()
{
	HRESULT hr;

	hr = BrowseFolder(m_DefaultTabDirectory,SBSP_ABSOLUTE);

	if(FAILED(hr))
	{
		BrowseFolder(m_DefaultTabDirectoryStatic,SBSP_ABSOLUTE);
	}
}

void Explorerplusplus::OnNavigateUp()
{
	TCHAR szDirectory[MAX_PATH];
	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(szDirectory),
		szDirectory);
	PathStripPath(szDirectory);

	BrowseFolder(EMPTY_STRING,SBSP_PARENT|SBSP_SAMEBROWSER);

	m_pActiveShellBrowser->SelectFiles(szDirectory);
}

/*
* Navigates to the folder specified by the incoming
* csidl.
*/
void Explorerplusplus::GotoFolder(int FolderCSIDL)
{
	LPITEMIDLIST	pidl = NULL;
	HRESULT			hr;

	hr = SHGetFolderLocation(NULL, FolderCSIDL, NULL, 0, &pidl);

	/* Don't use SUCCEEDED(hr). */
	if(hr == S_OK)
	{
		BrowseFolder(pidl, SBSP_SAMEBROWSER | SBSP_ABSOLUTE);

		CoTaskMemFree(pidl);
	}
}

/*
Browses to the specified folder within the _current_
tab. Also performs path expansion, meaning paths with
embedded environment variables will be handled automatically.

NOTE: All user-facing functions MUST send their paths
through here, rather than converting them to an idl
themselves (so that path expansion and any other required
processing can occur here).

The ONLY times an idl should be sent are:
- When loading directories on startup
- When navigating to a folder on the 'Go' menu
*/
HRESULT Explorerplusplus::BrowseFolder(const TCHAR *szPath, UINT wFlags)
{
	return BrowseFolder(szPath, wFlags, FALSE, FALSE, FALSE);
}

HRESULT Explorerplusplus::BrowseFolder(const TCHAR *szPath, UINT wFlags,
	BOOL bOpenInNewTab, BOOL bSwitchToNewTab, BOOL bOpenInNewWindow)
{
	LPITEMIDLIST	pidl = NULL;
	HRESULT			hr = S_FALSE;

	/* Doesn't matter if we can't get the pidl here,
	as some paths will be relative, or will be filled
	by the shellbrowser (e.g. when browsing back/forward). */
	hr = GetIdlFromParsingName(szPath, &pidl);

	BrowseFolder(pidl, wFlags, bOpenInNewTab, bSwitchToNewTab, bOpenInNewWindow);

	if(SUCCEEDED(hr))
	{
		CoTaskMemFree(pidl);
	}

	return hr;
}

/* ALL calls to browse a folder in the current tab MUST
pass through this function. This ensures that tabs that
have their addresses locked will not change directory. */
HRESULT Explorerplusplus::BrowseFolder(LPCITEMIDLIST pidlDirectory, UINT wFlags)
{
	HRESULT hr = E_FAIL;
	int iTabObjectIndex = -1;

	if(!m_TabInfo[m_iObjectIndex].bAddressLocked)
	{
		hr = m_pActiveShellBrowser->BrowseFolder(pidlDirectory, wFlags);

		if(SUCCEEDED(hr))
		{
			PlayNavigationSound();
		}

		iTabObjectIndex = m_iObjectIndex;
	}
	else
	{
		hr = CreateNewTab(pidlDirectory, NULL, NULL, TRUE, &iTabObjectIndex);
	}

	if(SUCCEEDED(hr))
	{
		OnDirChanged(iTabObjectIndex);
	}

	return hr;
}

HRESULT Explorerplusplus::BrowseFolder(LPCITEMIDLIST pidlDirectory, UINT wFlags,
	BOOL bOpenInNewTab, BOOL bSwitchToNewTab, BOOL bOpenInNewWindow)
{
	HRESULT hr = E_FAIL;
	int iTabObjectIndex = -1;

	if(bOpenInNewWindow)
	{
		/* Create a new instance of this program, with the
		specified path as an argument. */
		SHELLEXECUTEINFO sei;
		TCHAR szCurrentProcess[MAX_PATH];
		TCHAR szPath[MAX_PATH];
		TCHAR szParameters[512];

		GetProcessImageName(GetCurrentProcessId(), szCurrentProcess, SIZEOF_ARRAY(szCurrentProcess));

		GetDisplayName(pidlDirectory, szPath, SIZEOF_ARRAY(szPath), SHGDN_FORPARSING);
		StringCchPrintf(szParameters, SIZEOF_ARRAY(szParameters), _T("\"%s\""), szPath);

		sei.cbSize = sizeof(sei);
		sei.fMask = SEE_MASK_DEFAULT;
		sei.lpVerb = _T("open");
		sei.lpFile = szCurrentProcess;
		sei.lpParameters = szParameters;
		sei.lpDirectory = NULL;
		sei.nShow = SW_SHOW;
		ShellExecuteEx(&sei);
	}
	else
	{
		if(!bOpenInNewTab && !m_TabInfo[m_iObjectIndex].bAddressLocked)
		{
			hr = m_pActiveShellBrowser->BrowseFolder(pidlDirectory, wFlags);

			if(SUCCEEDED(hr))
			{
				PlayNavigationSound();
			}

			iTabObjectIndex = m_iObjectIndex;
		}
		else
		{
			if(m_TabInfo[m_iObjectIndex].bAddressLocked)
				hr = CreateNewTab(pidlDirectory, NULL, NULL, TRUE, &iTabObjectIndex);
			else
				hr = CreateNewTab(pidlDirectory, NULL, NULL, bSwitchToNewTab, &iTabObjectIndex);
		}

		if(SUCCEEDED(hr))
			OnDirChanged(iTabObjectIndex);
	}

	return hr;
}