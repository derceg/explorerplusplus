// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Navigation.h"
#include "MainResource.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"

Navigation::Navigation(std::shared_ptr<Config> config, IExplorerplusplus *expp) :
	m_config(config),
	m_expp(expp),
	m_tabContainer(nullptr)
{
	m_expp->AddTabsInitializedObserver([this] {
		m_tabContainer = m_expp->GetTabContainer();
		m_tabContainer->tabCreatedSignal.AddObserver(boost::bind(&Navigation::OnTabCreated, this, _1, _2), boost::signals2::at_front);
	});
}

void Navigation::OnTabCreated(int tabId, BOOL switchToNewTab)
{
	UNREFERENCED_PARAMETER(switchToNewTab);

	const Tab &tab = m_tabContainer->GetTab(tabId);
	navigationCompletedSignal.m_signal(tab);
}

void Navigation::OnBrowseBack()
{
	BrowseFolderInCurrentTab(EMPTY_STRING, SBSP_NAVIGATEBACK);
}

void Navigation::OnBrowseForward()
{
	BrowseFolderInCurrentTab(EMPTY_STRING, SBSP_NAVIGATEFORWARD);
}

void Navigation::OnNavigateHome()
{
	HRESULT hr = BrowseFolderInCurrentTab(m_config->defaultTabDirectory.c_str(), SBSP_ABSOLUTE);

	if(FAILED(hr))
	{
		BrowseFolderInCurrentTab(m_config->defaultTabDirectoryStatic.c_str(), SBSP_ABSOLUTE);
	}
}

void Navigation::OnNavigateUp()
{
	HRESULT hr = BrowseFolderInCurrentTab(EMPTY_STRING, SBSP_PARENT);

	if(SUCCEEDED(hr))
	{
		Tab &tab = m_tabContainer->GetSelectedTab();
		std::wstring directory = tab.GetShellBrowser()->GetDirectory();

		TCHAR directoryFileName[MAX_PATH];
		StringCchCopy(directoryFileName, std::size(directoryFileName), directory.c_str());
		PathStripPath(directoryFileName);
		tab.GetShellBrowser()->SelectFiles(directoryFileName);
	}
}

/*
* Navigates to the folder specified by the incoming
* csidl.
*/
void Navigation::OnGotoFolder(int FolderCSIDL)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHGetFolderLocation(NULL, FolderCSIDL, NULL, 0, wil::out_param(pidl));

	/* Don't use SUCCEEDED(hr). */
	if(hr == S_OK)
	{
		BrowseFolderInCurrentTab(pidl.get(), SBSP_ABSOLUTE);
	}
}

HRESULT Navigation::BrowseFolderInCurrentTab(const TCHAR *szPath, UINT wFlags)
{
	Tab &tab = m_tabContainer->GetSelectedTab();
	return BrowseFolder(tab, szPath, wFlags);
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
HRESULT Navigation::BrowseFolder(Tab &tab, const TCHAR *szPath, UINT wFlags)
{
	/* Doesn't matter if we can't get the pidl here,
	as some paths will be relative, or will be filled
	by the shellbrowser (e.g. when browsing back/forward). */
	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(szPath, nullptr, wil::out_param(pidl), 0, nullptr);

	BrowseFolder(tab, pidl.get(), wFlags);

	return hr;
}

HRESULT Navigation::BrowseFolderInCurrentTab(PCIDLIST_ABSOLUTE pidlDirectory, UINT wFlags)
{
	Tab &tab = m_tabContainer->GetSelectedTab();
	return BrowseFolder(tab, pidlDirectory, wFlags);
}

/* ALL calls to browse a folder in a particular tab MUST
pass through this function. This ensures that tabs that
have their addresses locked will not change directory (a
new tab will be created instead). */
HRESULT Navigation::BrowseFolder(Tab &tab, PCIDLIST_ABSOLUTE pidlDirectory, UINT wFlags)
{
	HRESULT hr = E_FAIL;
	int resultingTabId = -1;

	if(!tab.GetAddressLocked())
	{
		hr = tab.GetShellBrowser()->BrowseFolder(pidlDirectory, wFlags);

		if(SUCCEEDED(hr))
		{
			PlayNavigationSound();
		}

		resultingTabId = tab.GetId();
	}
	else
	{
		hr = m_tabContainer->CreateNewTab(pidlDirectory, TabSettings(_selected = true), nullptr, boost::none, &resultingTabId);
	}

	if(SUCCEEDED(hr))
	{
		const Tab &resultingTab = m_tabContainer->GetTab(resultingTabId);
		navigationCompletedSignal.m_signal(resultingTab);
	}

	return hr;
}

void Navigation::OpenDirectoryInNewWindow(PCIDLIST_ABSOLUTE pidlDirectory)
{
	TCHAR szPath[MAX_PATH];
	TCHAR szParameters[512];

	/* Create a new instance of this program, with the
	specified path as an argument. */
	GetDisplayName(pidlDirectory, szPath, SIZEOF_ARRAY(szPath), SHGDN_FORPARSING);
	StringCchPrintf(szParameters, SIZEOF_ARRAY(szParameters), _T("\"%s\""), szPath);

	ExecuteAndShowCurrentProcess(m_expp->GetMainWindow(), szParameters);
}

void Navigation::PlayNavigationSound() const
{
	if(m_config->playNavigationSound)
	{
		PlaySound(MAKEINTRESOURCE(IDR_WAVE_NAVIGATIONSTART), NULL,
			SND_RESOURCE | SND_ASYNC);
	}
}