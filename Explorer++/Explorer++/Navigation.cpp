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
	});
}

void Navigation::OnBrowseBack()
{
	OnGoToOffset(-1);
}

void Navigation::OnBrowseForward()
{
	OnGoToOffset(1);
}

void Navigation::OnGoToOffset(int offset)
{
	Tab &tab = m_tabContainer->GetSelectedTab();

	if (tab.GetLockState() != Tab::LockState::AddressLocked)
	{
		tab.GetNavigationController()->GoToOffset(offset);
	}
	else
	{
		auto entry = tab.GetNavigationController()->GetEntry(offset);

		if (entry)
		{
			m_tabContainer->CreateNewTab(entry->GetPidl().get(), TabSettings(_selected = true));
		}
	}
}

void Navigation::OnNavigateHome()
{
	HRESULT hr = BrowseFolderInCurrentTab(m_config->defaultTabDirectory.c_str());

	if(FAILED(hr))
	{
		BrowseFolderInCurrentTab(m_config->defaultTabDirectoryStatic.c_str());
	}
}

void Navigation::OnNavigateUp()
{
	Tab &tab = m_tabContainer->GetSelectedTab();
	HRESULT hr = E_FAIL;
	int resultingTabId = -1;

	if (tab.GetLockState() != Tab::LockState::AddressLocked)
	{
		hr = tab.GetNavigationController()->GoUp();

		resultingTabId = tab.GetId();
	}
	else
	{
		unique_pidl_absolute pidlParent;
		hr = GetVirtualParentPath(tab.GetShellBrowser()->GetDirectoryIdl().get(), wil::out_param(pidlParent));

		if (SUCCEEDED(hr))
		{
			hr = m_tabContainer->CreateNewTab(pidlParent.get(), TabSettings(_selected = true), nullptr, boost::none, &resultingTabId);
		}
	}

	if (SUCCEEDED(hr))
	{
		const Tab &resultingTab = m_tabContainer->GetTab(resultingTabId);
		std::wstring directory = resultingTab.GetShellBrowser()->GetDirectory();

		TCHAR directoryFileName[MAX_PATH];
		StringCchCopy(directoryFileName, std::size(directoryFileName), directory.c_str());
		PathStripPath(directoryFileName);
		resultingTab.GetShellBrowser()->SelectFiles(directoryFileName);
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
		BrowseFolderInCurrentTab(pidl.get());
	}
}

HRESULT Navigation::BrowseFolderInCurrentTab(const TCHAR *szPath)
{
	Tab &tab = m_tabContainer->GetSelectedTab();
	return BrowseFolder(tab, szPath);
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
HRESULT Navigation::BrowseFolder(Tab &tab, const TCHAR *szPath)
{
	/* Doesn't matter if we can't get the pidl here,
	as some paths will be relative, or will be filled
	by the shellbrowser (e.g. when browsing back/forward). */
	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(szPath, nullptr, wil::out_param(pidl), 0, nullptr);

	BrowseFolder(tab, pidl.get());

	return hr;
}

HRESULT Navigation::BrowseFolderInCurrentTab(PCIDLIST_ABSOLUTE pidlDirectory)
{
	Tab &tab = m_tabContainer->GetSelectedTab();
	return BrowseFolder(tab, pidlDirectory);
}

/* ALL calls to browse a folder in a particular tab MUST
pass through this function. This ensures that tabs that
have their addresses locked will not change directory (a
new tab will be created instead). */
HRESULT Navigation::BrowseFolder(Tab &tab, PCIDLIST_ABSOLUTE pidlDirectory)
{
	HRESULT hr = E_FAIL;

	if (tab.GetLockState() != Tab::LockState::AddressLocked)
	{
		hr = tab.GetShellBrowser()->BrowseFolder(pidlDirectory);
	}
	else
	{
		hr = m_tabContainer->CreateNewTab(pidlDirectory, TabSettings(_selected = true));
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