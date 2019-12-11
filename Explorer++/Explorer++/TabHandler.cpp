// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "LoadSaveInterface.h"
#include "MainResource.h"
#include "MenuRanges.h"
#include "RenameTabDialog.h"
#include "ShellBrowser/ShellBrowser.h"
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

static const UINT TAB_WINDOW_HEIGHT_96DPI = 24;

extern std::vector<std::wstring> g_commandLineDirectories;

void Explorerplusplus::InitializeTabs()
{
	/* The tab backing will hold the tab window. */
	CreateTabBacking();

	m_tabContainer = TabContainer::Create(m_hTabBacking, this, m_navigation.get(), this, &m_cachedIcons, m_hLanguageModule, m_config);
	m_tabContainer->tabCreatedSignal.AddObserver(boost::bind(&Explorerplusplus::OnTabCreated, this, _1, _2), boost::signals2::at_front);
	m_tabContainer->tabNavigationCompletedSignal.AddObserver(boost::bind(&Explorerplusplus::OnNavigationCompleted, this, _1), boost::signals2::at_front);
	m_tabContainer->tabSelectedSignal.AddObserver(boost::bind(&Explorerplusplus::OnTabSelected, this, _1), boost::signals2::at_front);

	UINT dpi = m_dpiCompat.GetDpiForWindow(m_tabContainer->GetHWND());
	int tabWindowHeight = MulDiv(TAB_WINDOW_HEIGHT_96DPI, dpi, USER_DEFAULT_SCREEN_DPI);
	SetWindowPos(m_tabContainer->GetHWND(), nullptr, 0, 0, 0, tabWindowHeight, SWP_NOMOVE | SWP_NOZORDER);

	m_tabRestorer = std::make_unique<TabRestorer>(m_tabContainer);
	m_tabRestorerUI = std::make_unique<TabRestorerUI>(m_hLanguageModule, this, m_tabRestorer.get(),
		MENU_RECENT_TABS_STARTID, MENU_RECENT_TABS_ENDID);

	m_tabsInitializedSignal();
}

void Explorerplusplus::OnTabCreated(int tabId, BOOL switchToNewTab)
{
	UNREFERENCED_PARAMETER(switchToNewTab);

	const Tab &tab = m_tabContainer->GetTab(tabId);

	/* TODO: This subclass needs to be removed. */
	SetWindowSubclass(tab.GetShellBrowser()->GetListView(), ListViewProcStub, 0, reinterpret_cast<DWORD_PTR>(this));
}

boost::signals2::connection Explorerplusplus::AddTabsInitializedObserver(const TabsInitializedSignal::slot_type &observer)
{
	return m_tabsInitializedSignal.connect(observer);
}

void Explorerplusplus::OnNavigationCompleted(const Tab &tab)
{
	if (m_tabContainer->IsTabSelected(tab))
	{
		m_CurrentDirectory = tab.GetShellBrowser()->GetDirectory();
		SetCurrentDirectory(m_CurrentDirectory.c_str());

		m_nSelected = 0;

		UpdateSortMenuItems(tab);
		UpdateWindowStates(tab);
	}

	HandleDirectoryMonitoring(tab.GetId());
}

/* Creates a new tab. If a folder is selected, that folder is opened in a new
 * tab, else the default directory is opened. */
HRESULT Explorerplusplus::OnNewTab()
{
	const Tab &selectedTab = m_tabContainer->GetSelectedTab();
	int selectionIndex = ListView_GetNextItem(selectedTab.GetShellBrowser()->GetListView(),
		-1, LVNI_FOCUSED | LVNI_SELECTED);

	if(selectionIndex != -1)
	{
		auto fileFindData = selectedTab.GetShellBrowser()->GetItemFileFindData(selectionIndex);

		/* If the selected item is a folder, open that folder in a new tab, else
		 * just use the default new tab directory. */
		if(WI_IsFlagSet(fileFindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
		{
			auto pidl = selectedTab.GetShellBrowser()->GetItemCompleteIdl(selectionIndex);
			return m_tabContainer->CreateNewTab(pidl.get(), TabSettings(_selected = true));
		}
	}

	/* Either no items are selected, or the focused + selected item was not a
	 * folder; open the default tab directory. */
	HRESULT hr = m_tabContainer->CreateNewTab(m_config->defaultTabDirectory.c_str(), TabSettings(_selected = true));

	if (FAILED(hr))
	{
		hr = m_tabContainer->CreateNewTab(m_config->defaultTabDirectoryStatic.c_str(), TabSettings(_selected = true));
	}

	return hr;
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

			hr = m_tabContainer->CreateNewTab(szDirectory, TabSettings(_selected = true));

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
		hr = m_tabContainer->CreateNewTab(m_config->defaultTabDirectory.c_str(), TabSettings(_selected = true));

		if(FAILED(hr))
			hr = m_tabContainer->CreateNewTab(m_config->defaultTabDirectoryStatic.c_str(), TabSettings(_selected = true));

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
	m_tabContainer->SelectTabAtIndex(m_iLastSelectedTab);

	return S_OK;
}

void Explorerplusplus::OnTabSelected(const Tab &tab)
{
	/* Hide the old listview. */
	ShowWindow(m_hActiveListView,SW_HIDE);

	m_hActiveListView = tab.GetShellBrowser()->GetListView();
	m_pActiveShellBrowser = tab.GetShellBrowser();

	/* The selected tab has changed, so update the current
	directory. Although this is not needed internally, context
	menu extensions may need the current directory to be
	set correctly. */
	m_CurrentDirectory = tab.GetShellBrowser()->GetDirectory();
	SetCurrentDirectory(m_CurrentDirectory.c_str());

	m_nSelected = tab.GetShellBrowser()->GetNumSelected();

	UpdateSortMenuItems(tab);
	UpdateWindowStates(tab);

	/* Show the new listview. */
	ShowWindow(m_hActiveListView,SW_SHOW);
	SetFocus(m_hActiveListView);
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

	m_tabContainer->SelectTabAtIndex(newIndex);
}

bool Explorerplusplus::OnCloseTab()
{
	const Tab &tab = m_tabContainer->GetSelectedTab();
	return m_tabContainer->CloseTab(tab);
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

HRESULT Explorerplusplus::CreateNewTab(PCIDLIST_ABSOLUTE pidlDirectory, bool selected)
{
	return m_tabContainer->CreateNewTab(pidlDirectory, TabSettings(_selected = selected));
}