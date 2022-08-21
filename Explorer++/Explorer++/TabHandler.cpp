// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "LoadSaveInterface.h"
#include "MenuRanges.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabContainer.h"
#include "TabRestorerUI.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/Macros.h"
#include <list>

static const UINT TAB_WINDOW_HEIGHT_96DPI = 24;

void Explorerplusplus::InitializeTabs()
{
	/* The tab backing will hold the tab window. */
	CreateTabBacking();

	m_tabContainer = TabContainer::Create(m_hTabBacking, this, this, &m_FileActionHandler,
		&m_cachedIcons, &m_bookmarkTree, m_hLanguageModule, m_config);
	m_tabContainer->tabCreatedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnTabCreated, this), boost::signals2::at_front);
	m_tabContainer->tabNavigationStartedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnNavigationStartedStatusBar, this),
		boost::signals2::at_front);
	m_tabContainer->tabNavigationCommittedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnNavigationCommitted, this), boost::signals2::at_front);
	m_tabContainer->tabNavigationCompletedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnNavigationCompletedStatusBar, this),
		boost::signals2::at_front);
	m_tabContainer->tabNavigationFailedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnNavigationFailedStatusBar, this),
		boost::signals2::at_front);
	m_tabContainer->tabSelectedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnTabSelected, this), boost::signals2::at_front);

	m_tabContainer->tabDirectoryModifiedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnDirectoryModified, this), boost::signals2::at_front);
	m_tabContainer->tabListViewSelectionChangedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnTabListViewSelectionChanged, this),
		boost::signals2::at_front);

	auto updateLayoutObserverMethod = [this](BOOL newValue)
	{
		UNREFERENCED_PARAMETER(newValue);

		UpdateLayout();
	};

	m_connections.push_back(m_config->showTabBarAtBottom.addObserver(updateLayoutObserverMethod));
	m_connections.push_back(m_config->extendTabControl.addObserver(updateLayoutObserverMethod));

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_tabContainer->GetHWND());
	int tabWindowHeight = MulDiv(TAB_WINDOW_HEIGHT_96DPI, dpi, USER_DEFAULT_SCREEN_DPI);
	SetWindowPos(m_tabContainer->GetHWND(), nullptr, 0, 0, 0, tabWindowHeight,
		SWP_NOMOVE | SWP_NOZORDER);

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
	SetWindowSubclass(tab.GetShellBrowser()->GetListView(), ListViewProcStub, 0,
		reinterpret_cast<DWORD_PTR>(this));
}

boost::signals2::connection Explorerplusplus::AddTabsInitializedObserver(
	const TabsInitializedSignal::slot_type &observer)
{
	return m_tabsInitializedSignal.connect(observer);
}

void Explorerplusplus::OnNavigationCommitted(const Tab &tab, PCIDLIST_ABSOLUTE pidl,
	bool addHistoryEntry)
{
	UNREFERENCED_PARAMETER(pidl);
	UNREFERENCED_PARAMETER(addHistoryEntry);

	if (m_tabContainer->IsTabSelected(tab))
	{
		std::wstring directory = tab.GetShellBrowser()->GetDirectory();
		SetCurrentDirectory(directory.c_str());

		UpdateWindowStates(tab);
	}

	StopDirectoryMonitoringForTab(tab);

	if (m_config->shellChangeNotificationType == ShellChangeNotificationType::Disabled
		|| (m_config->shellChangeNotificationType == ShellChangeNotificationType::NonFilesystem
			&& !tab.GetShellBrowser()->InVirtualFolder()))
	{
		StartDirectoryMonitoringForTab(tab);
	}
}

/* Creates a new tab. If a folder is selected, that folder is opened in a new
 * tab, else the default directory is opened. */
void Explorerplusplus::OnNewTab()
{
	const Tab &selectedTab = m_tabContainer->GetSelectedTab();
	int selectionIndex = ListView_GetNextItem(selectedTab.GetShellBrowser()->GetListView(), -1,
		LVNI_FOCUSED | LVNI_SELECTED);

	if (selectionIndex != -1)
	{
		auto fileFindData = selectedTab.GetShellBrowser()->GetItemFileFindData(selectionIndex);

		/* If the selected item is a folder, open that folder in a new tab, else
		 * just use the default new tab directory. */
		if (WI_IsFlagSet(fileFindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
		{
			auto pidl = selectedTab.GetShellBrowser()->GetItemCompleteIdl(selectionIndex);
			FolderColumns cols = selectedTab.GetShellBrowser()->ExportAllColumns();
			m_tabContainer->CreateNewTab(pidl.get(), TabSettings(_selected = true), nullptr, &cols);
			return;
		}
	}

	/* Either no items are selected, or the focused + selected item was not a
	 * folder; open the default tab directory. */
	m_tabContainer->CreateNewTabInDefaultDirectory(TabSettings(_selected = true));
}

HRESULT Explorerplusplus::RestoreTabs(ILoadSave *pLoadSave)
{
	TCHAR szDirectory[MAX_PATH];
	int nTabsCreated = 0;

	if (!m_commandLineSettings.directories.empty())
	{
		for (const auto &strDirectory : m_commandLineSettings.directories)
		{
			StringCchCopy(szDirectory, SIZEOF_ARRAY(szDirectory), strDirectory.c_str());

			if (lstrcmp(strDirectory.c_str(), _T("..")) == 0)
			{
				/* Get the parent of the current directory,
				and browse to it. */
				GetCurrentDirectory(SIZEOF_ARRAY(szDirectory), szDirectory);
				PathRemoveFileSpec(szDirectory);
			}
			else if (lstrcmp(strDirectory.c_str(), _T(".")) == 0)
			{
				GetCurrentDirectory(SIZEOF_ARRAY(szDirectory), szDirectory);
			}

			m_tabContainer->CreateNewTab(szDirectory, TabSettings(_selected = true));
			nTabsCreated++;
		}
	}
	else
	{
		if (m_config->startupMode == StartupMode::PreviousTabs)
		{
			nTabsCreated = pLoadSave->LoadPreviousTabs();
		}
	}

	if (nTabsCreated == 0)
	{
		m_tabContainer->CreateNewTabInDefaultDirectory(TabSettings(_selected = true));
		nTabsCreated++;
	}

	if (!m_config->alwaysShowTabBar.get() && nTabsCreated == 1)
	{
		m_bShowTabBar = false;
	}

	/* m_iLastSelectedTab is the tab that was selected when the
	program was last closed. */
	if (m_iLastSelectedTab >= m_tabContainer->GetNumTabs() || m_iLastSelectedTab < 0)
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
	ShowWindow(m_hActiveListView, SW_HIDE);

	m_hActiveListView = tab.GetShellBrowser()->GetListView();
	m_pActiveShellBrowser = tab.GetShellBrowser();

	/* The selected tab has changed, so update the current
	directory. Although this is not needed internally, context
	menu extensions may need the current directory to be
	set correctly. */
	std::wstring directory = tab.GetShellBrowser()->GetDirectory();
	SetCurrentDirectory(directory.c_str());

	UpdateWindowStates(tab);

	/* Show the new listview. */
	ShowWindow(m_hActiveListView, SW_SHOW);
	SetFocus(m_hActiveListView);
}

void Explorerplusplus::OnSelectTabByIndex(int iTab)
{
	int nTabs = m_tabContainer->GetNumTabs();
	int newIndex;

	if (iTab == -1)
	{
		newIndex = nTabs - 1;
	}
	else
	{
		if (iTab < nTabs)
		{
			newIndex = iTab;
		}
		else
		{
			newIndex = nTabs - 1;
		}
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
	m_bShowTabBar = true;
	UpdateLayout();
}

void Explorerplusplus::HideTabBar()
{
	m_bShowTabBar = false;
	UpdateLayout();
}

void Explorerplusplus::OnTabListViewSelectionChanged(const Tab &tab)
{
	/* The selection for this tab has changed, so invalidate any
	folder size calculations that are occurring for this tab
	(applies only to folder sizes that will be shown in the display
	window). */
	for (auto &item : m_DWFolderSizes)
	{
		if (item.iTabId == tab.GetId())
		{
			item.bValid = FALSE;
		}
	}

	if (m_tabContainer->IsTabSelected(tab))
	{
		SetTimer(m_hContainer, LISTVIEW_ITEM_CHANGED_TIMER_ID, LISTVIEW_ITEM_CHANGED_TIMEOUT,
			nullptr);
	}
}

// TabNavigationInterface
void Explorerplusplus::CreateNewTab(PCIDLIST_ABSOLUTE pidlDirectory, bool selected)
{
	m_tabContainer->CreateNewTab(pidlDirectory, TabSettings(_selected = selected));
}

void Explorerplusplus::SelectTabById(int tabId)
{
	const Tab &tab = m_tabContainer->GetTab(tabId);
	m_tabContainer->SelectTab(tab);
}
