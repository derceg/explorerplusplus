// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Navigation.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"

Navigation::Navigation(IExplorerplusplus *expp) :
	m_expp(expp),
	m_tabContainer(nullptr)
{
	m_expp->AddTabsInitializedObserver([this] {
		m_tabContainer = m_expp->GetTabContainer();
	});
}

void Navigation::OnNavigateUp()
{
	Tab &tab = m_tabContainer->GetSelectedTab();
	HRESULT hr = E_FAIL;
	int resultingTabId = -1;

	if (tab.GetLockState() != Tab::LockState::AddressLocked)
	{
		hr = tab.GetShellBrowser()->GetNavigationController()->GoUp();

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

HRESULT Navigation::BrowseFolderInCurrentTab(const TCHAR *szPath)
{
	Tab &tab = m_tabContainer->GetSelectedTab();
	return tab.GetShellBrowser()->GetNavigationController()->BrowseFolder(szPath);
}

HRESULT Navigation::BrowseFolderInCurrentTab(PCIDLIST_ABSOLUTE pidlDirectory)
{
	Tab &tab = m_tabContainer->GetSelectedTab();
	return tab.GetShellBrowser()->GetNavigationController()->BrowseFolder(pidlDirectory);
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