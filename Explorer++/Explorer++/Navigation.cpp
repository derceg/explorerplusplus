// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Navigation.h"
#include "CoreInterface.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainer.h"
#include "../Helper/ShellHelper.h"

Navigation::Navigation(IExplorerplusplus *expp) : m_expp(expp), m_tabContainer(nullptr)
{
	m_expp->AddTabsInitializedObserver([this] {
		m_tabContainer = m_expp->GetTabContainer();

		m_connections.push_back(m_tabContainer->tabNavigationCompletedSignal.AddObserver(
			boost::bind(&Navigation::OnNavigationCompleted, this, _1)));
		m_connections.push_back(m_tabContainer->tabNavigationFailedSignal.AddObserver(
			boost::bind(&Navigation::OnNavigationFailed, this, _1)));
	});
}

void Navigation::OnNavigateUp()
{
	Tab &tab = m_tabContainer->GetSelectedTab();
	std::wstring directory = tab.GetShellBrowser()->GetDirectory();

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
		hr = GetVirtualParentPath(
			tab.GetShellBrowser()->GetDirectoryIdl().get(), wil::out_param(pidlParent));

		if (SUCCEEDED(hr))
		{
			hr = m_tabContainer->CreateNewTab(pidlParent.get(), TabSettings(_selected = true),
				nullptr, std::nullopt, &resultingTabId);
		}
	}

	if (SUCCEEDED(hr))
	{
		const Tab &resultingTab = m_tabContainer->GetTab(resultingTabId);
		int folderId = resultingTab.GetShellBrowser()->GetUniqueFolderId();

		TCHAR directoryFileName[MAX_PATH];
		StringCchCopy(directoryFileName, std::size(directoryFileName), directory.c_str());
		PathStripPath(directoryFileName);

		// This may overwrite an existing entry. That's ok, as any existing entry will be for a
		// previous folder and no longer relevant.
		m_selectionInfoMap[resultingTab.GetId()] = { folderId, directoryFileName };
	}
}

void Navigation::OnNavigationCompleted(const Tab &tab)
{
	auto itr = m_selectionInfoMap.find(tab.GetId());

	if (itr == m_selectionInfoMap.end())
	{
		return;
	}

	if (itr->second.folderId != tab.GetShellBrowser()->GetUniqueFolderId())
	{
		return;
	}

	// Because this is called immediately after navigation has completed (i.e. after the enumeration
	// has finished and the associated items have been inserted), the user won't have had a chance
	// to select any items. So this call can't change the user's selection.
	tab.GetShellBrowser()->SelectFiles(itr->second.filename.c_str());

	m_selectionInfoMap.erase(itr);
}

void Navigation::OnNavigationFailed(const Tab &tab)
{
	auto itr = m_selectionInfoMap.find(tab.GetId());

	if (itr == m_selectionInfoMap.end())
	{
		return;
	}

	if (itr->second.folderId != tab.GetShellBrowser()->GetUniqueFolderId())
	{
		return;
	}

	m_selectionInfoMap.erase(itr);
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
	/* Create a new instance of this program, with the
	specified path as an argument. */
	std::wstring path;
	GetDisplayName(pidlDirectory, SHGDN_FORPARSING, path);

	TCHAR szParameters[512];
	StringCchPrintf(szParameters, SIZEOF_ARRAY(szParameters), _T("\"%s\""), path.c_str());

	ExecuteAndShowCurrentProcess(m_expp->GetMainWindow(), szParameters);
}