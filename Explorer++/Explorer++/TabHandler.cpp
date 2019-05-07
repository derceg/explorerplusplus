// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "LoadSaveInterface.h"
#include "MainImages.h"
#include "MainResource.h"
#include "RenameTabDialog.h"
#include "ShellBrowser/iShellView.h"
#include "ShellBrowser/SortModes.h"
#include "TabContainer.h"
#include "../Helper/Helper.h"
#include "../Helper/iDirectoryMonitor.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/TabHelper.h"
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/map.hpp>
#include <algorithm>
#include <list>

extern std::vector<std::wstring> g_commandLineDirectories;

void Explorerplusplus::InitializeTabs()
{
	/* The tab backing will hold the tab window. */
	CreateTabBacking();

	m_tabContainer = CTabContainer::Create(m_hTabBacking, this, this, this, m_hLanguageModule, m_config);

	/* Create the toolbar that will appear on the tab control.
	Only contains the close button used to close tabs. */
	TCHAR szTabCloseTip[64];
	LoadString(m_hLanguageModule,IDS_TAB_CLOSE_TIP,szTabCloseTip,SIZEOF_ARRAY(szTabCloseTip));
	m_hTabWindowToolbar	= CreateTabToolbar(m_hTabBacking,TABTOOLBAR_CLOSE,szTabCloseTip);
}

int Explorerplusplus::GetSelectedTabId() const
{
	return m_selectedTabId;
}

int Explorerplusplus::GetSelectedTabIndex() const
{
	return m_selectedTabIndex;
}

void Explorerplusplus::SelectTab(const Tab &tab)
{
	int index = m_tabContainer->GetTabIndex(tab);
	SelectTabAtIndex(index);
}

/*
* Creates a new tab. If a folder is selected,
* that folder is opened in a new tab, else
* the default directory is opened.
*/
void Explorerplusplus::OnNewTab()
{
	int		iSelected;
	HRESULT	hr;
	BOOL	bFolderSelected = FALSE;

	iSelected = ListView_GetNextItem(m_hActiveListView,
		-1, LVNI_FOCUSED | LVNI_SELECTED);

	if(iSelected != -1)
	{
		TCHAR FullItemPath[MAX_PATH];

		/* An item is selected, so get its full pathname. */
		m_pActiveShellBrowser->QueryFullItemName(iSelected, FullItemPath, SIZEOF_ARRAY(FullItemPath));

		/* If the selected item is a folder, open that folder
		in a new tab, else just use the default new tab directory. */
		if(PathIsDirectory(FullItemPath))
		{
			bFolderSelected = TRUE;
			CreateNewTab(FullItemPath, TabSettings(_selected = true));
		}
	}

	/* Either no items are selected, or the focused + selected
	item was not a folder; open the default tab directory. */
	if(!bFolderSelected)
	{
		hr = CreateNewTab(m_config->defaultTabDirectory.c_str(), TabSettings(_selected = true));

		if (FAILED(hr))
		{
			CreateNewTab(m_config->defaultTabDirectoryStatic.c_str(), TabSettings(_selected = true));
		}
	}
}

HRESULT Explorerplusplus::CreateNewTab(const TCHAR *TabDirectory,
	const TabSettings &tabSettings, const FolderSettings *folderSettings,
	const InitialColumns *initialColumns, int *newTabId)
{
	LPITEMIDLIST	pidl = NULL;
	TCHAR			szExpandedPath[MAX_PATH];
	HRESULT			hr;
	BOOL			bRet;

	/* Attempt to expand the path (in the event that
	it contains embedded environment variables). */
	bRet = MyExpandEnvironmentStrings(TabDirectory,
		szExpandedPath,SIZEOF_ARRAY(szExpandedPath));

	if(!bRet)
	{
		StringCchCopy(szExpandedPath,
			SIZEOF_ARRAY(szExpandedPath),TabDirectory);
	}

	if(!SUCCEEDED(GetIdlFromParsingName(szExpandedPath,&pidl)))
		return E_FAIL;

	hr = CreateNewTab(pidl, tabSettings, folderSettings, initialColumns, newTabId);

	CoTaskMemFree(pidl);

	return hr;
}

/* Creates a new tab. If the settings argument is NULL,
the global settings will be used. */
HRESULT Explorerplusplus::CreateNewTab(LPCITEMIDLIST pidlDirectory,
	const TabSettings &tabSettings, const FolderSettings *folderSettings,
	const InitialColumns *initialColumns, int *newTabId)
{
	if (!CheckIdl(pidlDirectory) || !IsIdlDirectory(pidlDirectory))
	{
		return E_FAIL;
	}

	int tabId = m_tabIdCounter++;
	auto item = m_tabContainer->GetTabs().emplace(std::make_pair(tabId, tabId));

	Tab &tab = item.first->second;

	if (tabSettings.locked)
	{
		tab.SetLocked(*tabSettings.locked);
	}

	if (tabSettings.addressLocked)
	{
		tab.SetAddressLocked(*tabSettings.addressLocked);
	}

	if (tabSettings.name && !tabSettings.name->empty())
	{
		tab.SetCustomName(*tabSettings.name);
	}

	tab.listView = CreateMainListView(m_hContainer);

	if (tab.listView == NULL)
	{
		return E_FAIL;
	}

	/* Set the listview to its initial size. */
	SetListViewInitialPosition(tab.listView);

	FolderSettings folderSettingsFinal;

	if (folderSettings)
	{
		folderSettingsFinal = *folderSettings;
	}
	else
	{
		folderSettingsFinal = GetDefaultFolderSettings(pidlDirectory);
	}

	InitialColumns initialColumnsFinal;

	if (initialColumns)
	{
		initialColumnsFinal = *initialColumns;
	}
	else
	{
		initialColumnsFinal.pControlPanelColumnList = &m_ControlPanelColumnList;
		initialColumnsFinal.pMyComputerColumnList = &m_MyComputerColumnList;
		initialColumnsFinal.pMyNetworkPlacesColumnList = &m_MyNetworkPlacesColumnList;
		initialColumnsFinal.pNetworkConnectionsColumnList = &m_NetworkConnectionsColumnList;
		initialColumnsFinal.pPrintersColumnList = &m_PrintersColumnList;
		initialColumnsFinal.pRealFolderColumnList = &m_RealFolderColumnList;
		initialColumnsFinal.pRecycleBinColumnList = &m_RecycleBinColumnList;
	}

	tab.SetShellBrowser(CShellBrowser::CreateNew(m_hContainer, tab.listView, &m_cachedIcons,
		m_config, folderSettingsFinal, initialColumnsFinal));

	tab.GetShellBrowser()->SetId(tab.GetId());
	tab.GetShellBrowser()->SetResourceModule(m_hLanguageModule);

	int index;

	if (tabSettings.index)
	{
		index = *tabSettings.index;
	}
	else
	{
		if (m_config->openNewTabNextToCurrent)
		{
			index = m_selectedTabIndex + 1;
		}
		else
		{
			index = TabCtrl_GetItemCount(m_tabContainer->GetHWND());
		}
	}

	/* Browse folder sends a message back to the main window, which
	attempts to contact the new tab (needs to be created before browsing
	the folder). */
	m_tabContainer->InsertNewTab(index, tab.GetId(), pidlDirectory, tabSettings.name);

	bool selected = false;

	if (tabSettings.selected)
	{
		selected = *tabSettings.selected;
	}

	if(selected)
	{
		if(m_iPreviousTabSelectionId != -1)
		{
			m_tabSelectionHistory.push_back(m_iPreviousTabSelectionId);
		}

		/* Select the newly created tab. */
		TabCtrl_SetCurSel(m_tabContainer->GetHWND(),index);

		/* Hide the previously active tab, and show the
		newly created one. */
		ShowWindow(m_hActiveListView,SW_HIDE);
		ShowWindow(tab.listView,SW_SHOW);

		m_selectedTabId			= tab.GetId();
		m_selectedTabIndex		= index;

		m_hActiveListView		= tab.listView;
		m_pActiveShellBrowser	= tab.GetShellBrowser();

		SetFocus(tab.listView);

		m_iPreviousTabSelectionId = tab.GetId();
	}

	HRESULT hr = tab.GetShellBrowser()->BrowseFolder(pidlDirectory, SBSP_ABSOLUTE);

	if (selected)
	{
		tab.GetShellBrowser()->QueryCurrentDirectory(SIZEOF_ARRAY(m_CurrentDirectory), m_CurrentDirectory);
	}

	if(hr != S_OK)
	{
		/* Folder was not browsed. Likely that the path does not exist
		(or is locked, cannot be found, etc). */
		return E_FAIL;
	}

	if (newTabId)
	{
		*newTabId = tab.GetId();
	}

	// There's no need to manually disconnect this. Either it will be
	// disconnected when the tab is closed and the tab object (and
	// associated signal) is destroyed or when the tab is destroyed
	// during application shutdown.
	tab.AddTabUpdatedObserver(boost::bind(&Explorerplusplus::OnTabUpdated, this, _1, _2));
	m_tabCreatedSignal(tab.GetId(), selected);

	OnNavigationCompleted(tab);

	return S_OK;
}

FolderSettings Explorerplusplus::GetDefaultFolderSettings(LPCITEMIDLIST pidlDirectory) const
{
	FolderSettings folderSettings = m_config->defaultFolderSettings;
	folderSettings.sortMode = GetDefaultSortMode(pidlDirectory);

	return folderSettings;
}

boost::signals2::connection Explorerplusplus::AddTabCreatedObserver(const TabCreatedSignal::slot_type &observer)
{
	return m_tabCreatedSignal.connect(observer);
}

boost::signals2::connection Explorerplusplus::AddTabSelectedObserver(const TabSelectedSignal::slot_type &observer)
{
	return m_tabSelectedSignal.connect(observer);
}

boost::signals2::connection Explorerplusplus::AddTabUpdatedObserver(const TabUpdatedSignal::slot_type &observer)
{
	return m_tabUpdatedSignal.connect(observer);
}

boost::signals2::connection Explorerplusplus::AddTabRemovedObserver(const TabRemovedSignal::slot_type &observer)
{
	return m_tabRemovedSignal.connect(observer);
}

HRESULT Explorerplusplus::RestoreTabs(ILoadSave *pLoadSave)
{
	TCHAR							szDirectory[MAX_PATH];
	HRESULT							hr;
	int								nTabsCreated = 0;

	if(!g_commandLineDirectories.empty())
	{
		for(const auto &strDirectory : g_commandLineDirectories)
		{
			StringCchCopy(szDirectory,SIZEOF_ARRAY(szDirectory),strDirectory.c_str());

			if(lstrcmp(strDirectory.c_str(),_T("..")) == 0)
			{
				/* Get the parent of the current directory,
				and browse to it. */
				GetCurrentDirectory(SIZEOF_ARRAY(szDirectory),szDirectory);
				PathRemoveFileSpec(szDirectory);
			}
			else if(lstrcmp(strDirectory.c_str(),_T(".")) == 0)
			{
				GetCurrentDirectory(SIZEOF_ARRAY(szDirectory),szDirectory);
			}

			hr = CreateNewTab(szDirectory, TabSettings(_selected = true));

			if(hr == S_OK)
				nTabsCreated++;
		}
	}
	else
	{
		if(m_config->startupMode == STARTUP_PREVIOUSTABS)
			nTabsCreated = pLoadSave->LoadPreviousTabs();
	}

	if(nTabsCreated == 0)
	{
		hr = CreateNewTab(m_config->defaultTabDirectory.c_str(), TabSettings(_selected = true));

		if(FAILED(hr))
			hr = CreateNewTab(m_config->defaultTabDirectoryStatic.c_str(), TabSettings(_selected = true));

		if(hr == S_OK)
		{
			nTabsCreated++;
		}
	}

	if(nTabsCreated == 0)
	{
		/* Should never end up here. */
		return E_FAIL;
	}

	/* Tabs created on startup will NOT have
	any automatic updates. The only thing that
	needs to be done is to monitor each
	directory. The tab that is finally switched
	to will have an associated update of window
	states. */
	for (auto &tab : m_tabContainer->GetAllTabs() | boost::adaptors::map_values)
	{
		HandleDirectoryMonitoring(tab.GetId());
	}

	if(!m_config->alwaysShowTabBar.get())
	{
		if(nTabsCreated == 1)
		{
			m_bShowTabBar = FALSE;
		}
	}

	/* m_iLastSelectedTab is the tab that was selected when the
	program was last closed. */
	if(m_iLastSelectedTab >= m_tabContainer->GetNumTabs() ||
		m_iLastSelectedTab < 0)
	{
		m_iLastSelectedTab = 0;
	}

	/* Set the focus back to the tab that had the focus when the program
	was last closed. */
	SelectTabAtIndex(m_iLastSelectedTab);

	return S_OK;
}

void Explorerplusplus::SelectTabAtIndex(int index)
{
	assert(index >= 0 && index < m_tabContainer->GetNumTabs());

	int previousIndex = TabCtrl_SetCurSel(m_tabContainer->GetHWND(), index);

	if (previousIndex == -1)
	{
		return;
	}

	OnTabSelectionChanged();
}

void Explorerplusplus::OnTabSelectionChanged()
{
	int index = TabCtrl_GetCurSel(m_tabContainer->GetHWND());

	if (index == -1)
	{
		throw std::runtime_error("No selected tab");
	}

	m_selectedTabIndex = index;

	if(m_iPreviousTabSelectionId != -1)
	{
		m_tabSelectionHistory.push_back(m_iPreviousTabSelectionId);
	}

	/* Hide the old listview. */
	ShowWindow(m_hActiveListView,SW_HIDE);

	const Tab &tab = m_tabContainer->GetTabByIndex(index);

	m_selectedTabId = tab.GetId();
	m_hActiveListView = tab.listView;
	m_pActiveShellBrowser = tab.GetShellBrowser();

	/* The selected tab has changed, so update the current
	directory. Although this is not needed internally, context
	menu extensions may need the current directory to be
	set correctly. */
	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(m_CurrentDirectory),
		m_CurrentDirectory);
	SetCurrentDirectory(m_CurrentDirectory);

	m_nSelected = m_pActiveShellBrowser->GetNumSelected();

	SetActiveArrangeMenuItems();
	UpdateArrangeMenuItems();

	UpdateWindowStates();

	/* Show the new listview. */
	ShowWindow(m_hActiveListView,SW_SHOW);
	SetFocus(m_hActiveListView);

	m_iPreviousTabSelectionId = m_selectedTabId;

	m_tabSelectedSignal(tab);
}

void Explorerplusplus::SelectAdjacentTab(BOOL bNextTab)
{
	int nTabs = m_tabContainer->GetNumTabs();
	int newIndex = m_selectedTabIndex;

	if(bNextTab)
	{
		/* If this is the last tab in the order,
		wrap the selection back to the start. */
		if(newIndex == (nTabs - 1))
			newIndex = 0;
		else
			newIndex++;
	}
	else
	{
		/* If this is the first tab in the order,
		wrap the selection back to the end. */
		if(newIndex == 0)
			newIndex = nTabs - 1;
		else
			newIndex--;
	}

	SelectTabAtIndex(newIndex);
}

void Explorerplusplus::OnSelectTabByIndex(int iTab)
{
	int nTabs = m_tabContainer->GetNumTabs();
	int newIndex;

	if(iTab == -1)
	{
		newIndex = nTabs - 1;
	}
	else
	{
		if(iTab < nTabs)
			newIndex = iTab;
		else
			newIndex = nTabs - 1;
	}

	SelectTabAtIndex(newIndex);
}

bool Explorerplusplus::OnCloseTab(void)
{
	const Tab &tab = m_tabContainer->GetSelectedTab();
	return CloseTab(tab);
}

bool Explorerplusplus::CloseTab(const Tab &tab)
{
	const int nTabs = m_tabContainer->GetNumTabs();

	if(nTabs == 1 &&
		m_config->closeMainWindowOnTabClose)
	{
		OnClose();
		return true;
	}

	/* The tab is locked. Don't close it. */
	if(tab.GetLocked() || tab.GetAddressLocked())
	{
		return false;
	}

	RemoveTabFromControl(tab);

	m_pDirMon->StopDirectoryMonitor(tab.GetShellBrowser()->GetDirMonitorId());

	tab.GetShellBrowser()->Release();

	DestroyWindow(tab.listView);

	// This is needed, as the erase() call below will remove the element
	// from the tabs container (which will invalidate the reference
	// passed to the function).
	int tabId = tab.GetId();

	m_tabContainer->GetTabs().erase(tab.GetId());

	m_tabRemovedSignal(tabId);

	return true;
}

void Explorerplusplus::RemoveTabFromControl(const Tab &tab)
{
	m_tabSelectionHistory.erase(std::remove(m_tabSelectionHistory.begin(), m_tabSelectionHistory.end(), tab.GetId()), m_tabSelectionHistory.end());

	const int index = m_tabContainer->GetTabIndex(tab);

	if(m_tabContainer->IsTabSelected(tab))
	{
		int newIndex;

		/* If there was a previously active tab, the focus
		should be switched back to it. */
		if (!m_tabSelectionHistory.empty())
		{
			const int lastTabId = m_tabSelectionHistory.back();
			m_tabSelectionHistory.pop_back();

			const Tab& lastTab = m_tabContainer->GetTab(lastTabId);
			newIndex = m_tabContainer->GetTabIndex(lastTab);
		}
		else
		{
			newIndex = index;

			// If the last tab in the control is what's being closed,
			// the tab before it will be selected.
			if (newIndex == (m_tabContainer->GetNumTabs() - 1))
			{
				newIndex--;
			}
		}

		SelectTabAtIndex(newIndex);

		// This is somewhat hacky. Switching the tab will cause the
		// previously selected tab (i.e. the tab that's about to be
		// closed) to be added to the history list. That's not
		// desirable, so the last entry will be removed here.
		m_tabSelectionHistory.pop_back();
	}

	TCITEM tcItemRemoved;
	tcItemRemoved.mask = TCIF_IMAGE;
	TabCtrl_GetItem(m_tabContainer->GetHWND(), index, &tcItemRemoved);

	TabCtrl_DeleteItem(m_tabContainer->GetHWND(),index);

	TabCtrl_RemoveImage(m_tabContainer->GetHWND(),tcItemRemoved.iImage);

	if (m_selectedTabIndex > index)
	{
		m_selectedTabIndex--;
	}
}

void Explorerplusplus::ShowTabBar()
{
	m_bShowTabBar = TRUE;
	UpdateLayout();
}

void Explorerplusplus::HideTabBar()
{
	m_bShowTabBar = FALSE;
	UpdateLayout();
}

HRESULT Explorerplusplus::RefreshTab(const Tab &tab)
{
	HRESULT hr = tab.GetShellBrowser()->Refresh();

	if (SUCCEEDED(hr))
	{
		OnNavigationCompleted(tab);
	}

	return hr;
}

void Explorerplusplus::OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType)
{
	switch (propertyType)
	{
	case Tab::PropertyType::LOCKED:
	case Tab::PropertyType::ADDRESS_LOCKED:
		/* If the tab that was locked/unlocked is the
		currently selected tab, then the tab close
		button on the toolbar will need to be updated. */
		if (m_tabContainer->IsTabSelected(tab))
		{
			UpdateTabToolbar();
		}
		break;
	}

	m_tabUpdatedSignal(tab, propertyType);
}

void Explorerplusplus::UpdateTabToolbar(void)
{
	const int nTabs = m_tabContainer->GetNumTabs();

	const Tab &selectedTab = m_tabContainer->GetSelectedTab();

	if(nTabs > 1 && !(selectedTab.GetLocked() || selectedTab.GetAddressLocked()))
	{
		/* Enable the tab close button. */
		SendMessage(m_hTabWindowToolbar,TB_SETSTATE,
		TABTOOLBAR_CLOSE,TBSTATE_ENABLED);
	}
	else
	{
		/* Disable the tab close toolbar button. */
		SendMessage(m_hTabWindowToolbar,TB_SETSTATE,
		TABTOOLBAR_CLOSE,TBSTATE_INDETERMINATE);
	}
}

void Explorerplusplus::DuplicateTab(const Tab &tab)
{
	TCHAR szTabDirectory[MAX_PATH];

	tab.GetShellBrowser()->QueryCurrentDirectory(SIZEOF_ARRAY(szTabDirectory),
		szTabDirectory);

	CreateNewTab(szTabDirectory);
}

SortMode Explorerplusplus::GetDefaultSortMode(LPCITEMIDLIST pidlDirectory) const
{
	const std::list<Column_t> *pColumns = NULL;

	TCHAR szDirectory[MAX_PATH];
	GetDisplayName(pidlDirectory,szDirectory,SIZEOF_ARRAY(szDirectory),SHGDN_FORPARSING);

	if(CompareVirtualFolders(szDirectory,CSIDL_CONTROLS))
	{
		pColumns = &m_ControlPanelColumnList;
	}
	else if(CompareVirtualFolders(szDirectory,CSIDL_DRIVES))
	{
		pColumns = &m_MyComputerColumnList;
	}
	else if(CompareVirtualFolders(szDirectory,CSIDL_BITBUCKET))
	{
		pColumns = &m_RecycleBinColumnList;
	}
	else if(CompareVirtualFolders(szDirectory,CSIDL_PRINTERS))
	{
		pColumns = &m_PrintersColumnList;
	}
	else if(CompareVirtualFolders(szDirectory,CSIDL_CONNECTIONS))
	{
		pColumns = &m_NetworkConnectionsColumnList;
	}
	else if(CompareVirtualFolders(szDirectory,CSIDL_NETWORK))
	{
		pColumns = &m_MyNetworkPlacesColumnList;
	}
	else
	{
		pColumns = &m_RealFolderColumnList;
	}

	SortMode sortMode = SortMode::Name;

	for(const auto &Column : *pColumns)
	{
		if(Column.bChecked)
		{
			sortMode = CShellBrowser::DetermineColumnSortMode(Column.id);
			break;
		}
	}

	return sortMode;
}