// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "MainResource.h"
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
	HRESULT hr = BrowseFolder(m_DefaultTabDirectory, SBSP_ABSOLUTE);

	if(FAILED(hr))
	{
		BrowseFolder(m_DefaultTabDirectoryStatic, SBSP_ABSOLUTE);
	}
}

void Explorerplusplus::OnNavigateUp()
{
	TCHAR szDirectory[MAX_PATH];
	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(szDirectory),
		szDirectory);

	PathStripPath(szDirectory);

	HRESULT hr = BrowseFolder(EMPTY_STRING, SBSP_PARENT | SBSP_SAMEBROWSER);

	if(SUCCEEDED(hr))
	{
		m_pActiveShellBrowser->SelectFiles(szDirectory);
	}
}

/*
* Navigates to the folder specified by the incoming
* csidl.
*/
void Explorerplusplus::GotoFolder(int FolderCSIDL)
{
	LPITEMIDLIST pidl = NULL;
	HRESULT hr = SHGetFolderLocation(NULL, FolderCSIDL, NULL, 0, &pidl);

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
	/* Doesn't matter if we can't get the pidl here,
	as some paths will be relative, or will be filled
	by the shellbrowser (e.g. when browsing back/forward). */
	LPITEMIDLIST pidl = NULL;
	HRESULT hr = GetIdlFromParsingName(szPath, &pidl);

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
	HRESULT hr;
	int iTabObjectIndex = -1;

	if(!m_TabInfo.at(m_selectedTabId).bAddressLocked)
	{
		hr = m_pActiveShellBrowser->BrowseFolder(pidlDirectory, wFlags);

		if(SUCCEEDED(hr))
		{
			PlayNavigationSound();
		}

		iTabObjectIndex = m_selectedTabId;
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
		TCHAR szPath[MAX_PATH];
		TCHAR szParameters[512];

		/* Create a new instance of this program, with the
		specified path as an argument. */
		GetDisplayName(pidlDirectory, szPath, SIZEOF_ARRAY(szPath), SHGDN_FORPARSING);
		StringCchPrintf(szParameters, SIZEOF_ARRAY(szParameters), _T("\"%s\""), szPath);

		ExecuteAndShowCurrentProcess(m_hContainer, szParameters);
	}
	else
	{
		if(!bOpenInNewTab && !m_TabInfo.at(m_selectedTabId).bAddressLocked)
		{
			hr = m_pActiveShellBrowser->BrowseFolder(pidlDirectory, wFlags);

			if(SUCCEEDED(hr))
			{
				PlayNavigationSound();
			}

			iTabObjectIndex = m_selectedTabId;
		}
		else
		{
			if(m_TabInfo.at(m_selectedTabId).bAddressLocked)
			{
				hr = CreateNewTab(pidlDirectory, NULL, NULL, TRUE, &iTabObjectIndex);
			}
			else
			{
				hr = CreateNewTab(pidlDirectory, NULL, NULL, bSwitchToNewTab, &iTabObjectIndex);
			}
		}

		if(SUCCEEDED(hr))
		{
			OnDirChanged(iTabObjectIndex);
		}
	}

	return hr;
}

void Explorerplusplus::PlayNavigationSound() const
{
	if(m_config->playNavigationSound)
	{
		PlaySound(MAKEINTRESOURCE(IDR_WAVE_NAVIGATIONSTART), NULL,
			SND_RESOURCE | SND_ASYNC);
	}
}

void Explorerplusplus::OnDirChanged(int iTabId)
{
	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(m_CurrentDirectory),
		m_CurrentDirectory);
	SetCurrentDirectory(m_CurrentDirectory);

	HandleDirectoryMonitoring(iTabId);

	UpdateArrangeMenuItems();

	m_nSelected = 0;

	/* Set the focus back to the first item. */
	ListView_SetItemState(m_hActiveListView, 0, LVIS_FOCUSED, LVIS_FOCUSED);

	UpdateWindowStates();

	InvalidateTaskbarThumbnailBitmap(iTabId);

	SetTabIcon();
}

void Explorerplusplus::OnStartedBrowsing(int iTabId, const TCHAR *szFolderPath)
{
	TCHAR	szLoadingText[512];

	if (iTabId == m_selectedTabId)
	{
		TCHAR szTemp[64];
		LoadString(m_hLanguageModule, IDS_GENERAL_LOADING, szTemp, SIZEOF_ARRAY(szTemp));
		StringCchPrintf(szLoadingText, SIZEOF_ARRAY(szLoadingText), szTemp, szFolderPath);

		/* Browsing of a folder has started. Set the status bar text to indicate that
		the folder is been loaded. */
		SendMessage(m_hStatusBar, SB_SETTEXT, (WPARAM)0 | 0, (LPARAM)szLoadingText);

		/* Clear the text in all other parts of the status bar. */
		SendMessage(m_hStatusBar, SB_SETTEXT, (WPARAM)1 | 0, (LPARAM)EMPTY_STRING);
		SendMessage(m_hStatusBar, SB_SETTEXT, (WPARAM)2 | 0, (LPARAM)EMPTY_STRING);
	}
}