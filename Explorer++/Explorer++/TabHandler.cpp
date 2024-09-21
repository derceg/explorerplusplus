// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Bookmarks/BookmarkTreeFactory.h"
#include "Config.h"
#include "LoadSaveInterface.h"
#include "MainMenuSubMenuView.h"
#include "MainResource.h"
#include "MenuRanges.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabContainer.h"
#include "TabRestorer.h"
#include "TabRestorerMenu.h"
#include "TabStorage.h"
#include "../Helper/Macros.h"
#include <list>

void Explorerplusplus::InitializeTabs()
{
	/* The tab backing will hold the tab window. */
	CreateTabBacking();

	auto *tabContainer =
		TabContainer::Create(m_hTabBacking, this, this, this, &m_FileActionHandler, &m_cachedIcons,
			BookmarkTreeFactory::GetInstance()->GetBookmarkTree(), m_resourceInstance, m_config);
	m_browserPane = std::make_unique<BrowserPane>(tabContainer);

	tabContainer->tabCreatedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnTabCreated, this), boost::signals2::at_front);
	tabContainer->tabNavigationStartedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnNavigationStartedStatusBar, this),
		boost::signals2::at_front);
	tabContainer->tabNavigationCommittedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnNavigationCommitted, this), boost::signals2::at_front);
	tabContainer->tabNavigationCompletedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnNavigationCompletedStatusBar, this),
		boost::signals2::at_front);
	tabContainer->tabNavigationFailedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnNavigationFailedStatusBar, this),
		boost::signals2::at_front);
	tabContainer->tabSelectedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnTabSelected, this), boost::signals2::at_front);

	tabContainer->tabDirectoryModifiedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnDirectoryModified, this), boost::signals2::at_front);
	tabContainer->tabListViewSelectionChangedSignal.AddObserver(
		std::bind_front(&Explorerplusplus::OnTabListViewSelectionChanged, this),
		boost::signals2::at_front);

	tabContainer->sizeUpdatedSignal.AddObserver([this] { UpdateLayout(); });

	auto updateLayoutObserverMethod = [this](BOOL newValue)
	{
		UNREFERENCED_PARAMETER(newValue);

		UpdateLayout();
	};

	m_connections.push_back(m_config->showTabBarAtBottom.addObserver(updateLayoutObserverMethod));
	m_connections.push_back(m_config->extendTabControl.addObserver(updateLayoutObserverMethod));

	m_tabRestorer = std::make_unique<TabRestorer>(tabContainer);

	// TODO: These should be initialized when the main menu is initialized, but that's not possible
	// at the moment, due to the dependence on m_tabRestorer, which itself depends on tabContainer.
	m_tabRestorerMenuView =
		std::make_unique<MainMenuSubMenuView>(GetMenu(m_hContainer), IDM_FILE_REOPEN_RECENT_TAB);
	m_tabRestorerMenu = std::make_unique<TabRestorerMenu>(m_tabRestorerMenuView.get(),
		m_acceleratorManager, m_tabRestorer.get(), m_resourceInstance, MENU_RECENT_TABS_START_ID,
		MENU_RECENT_TABS_END_ID);

	m_tabsInitializedSignal();
}

void Explorerplusplus::OnTabCreated(int tabId, BOOL switchToNewTab)
{
	UNREFERENCED_PARAMETER(switchToNewTab);

	const Tab &tab = GetActivePane()->GetTabContainer()->GetTab(tabId);

	/* TODO: This subclass needs to be removed. */
	SetWindowSubclass(tab.GetShellBrowser()->GetListView(), ListViewProcStub, 0,
		reinterpret_cast<DWORD_PTR>(this));
}

boost::signals2::connection Explorerplusplus::AddTabsInitializedObserver(
	const TabsInitializedSignal::slot_type &observer)
{
	return m_tabsInitializedSignal.connect(observer);
}

void Explorerplusplus::OnNavigationCommitted(const Tab &tab, const NavigateParams &navigateParams)
{
	UNREFERENCED_PARAMETER(navigateParams);

	if (GetActivePane()->GetTabContainer()->IsTabSelected(tab))
	{
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
	const Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
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

			auto navigateParams = NavigateParams::Normal(pidl.get());
			GetActivePane()->GetTabContainer()->CreateNewTab(navigateParams,
				TabSettings(_selected = true), nullptr, &cols);
			return;
		}
	}

	/* Either no items are selected, or the focused + selected item was not a
	 * folder; open the default tab directory. */
	GetActivePane()->GetTabContainer()->CreateNewTabInDefaultDirectory(
		TabSettings(_selected = true));
}

HRESULT Explorerplusplus::CreateInitialTabs()
{
	if (m_config->startupMode == +StartupMode::PreviousTabs)
	{
		RestorePreviousTabs();
	}

	CreateCommandLineTabs();

	if (GetActivePane()->GetTabContainer()->GetNumTabs() == 0)
	{
		GetActivePane()->GetTabContainer()->CreateNewTabInDefaultDirectory({});
	}

	if (!m_config->alwaysShowTabBar.get() && GetActivePane()->GetTabContainer()->GetNumTabs() == 1)
	{
		m_bShowTabBar = false;
	}

	return S_OK;
}

void Explorerplusplus::RestorePreviousTabs()
{
	int index = 0;

	for (auto &loadedTab : m_loadedTabs)
	{
		loadedTab.tabSettings.index = index;

		if (loadedTab.pidl.HasValue())
		{
			auto navigateParams = NavigateParams::Normal(loadedTab.pidl.Raw());
			GetActivePane()->GetTabContainer()->CreateNewTab(navigateParams, loadedTab.tabSettings,
				&loadedTab.folderSettings, &loadedTab.columns);
		}
		else
		{
			GetActivePane()->GetTabContainer()->CreateNewTab(loadedTab.directory,
				loadedTab.tabSettings, &loadedTab.folderSettings, &loadedTab.columns);
		}

		index++;
	}

	if (m_iLastSelectedTab >= 0
		&& m_iLastSelectedTab < GetActivePane()->GetTabContainer()->GetNumTabs())
	{
		GetActivePane()->GetTabContainer()->SelectTabAtIndex(m_iLastSelectedTab);
	}
}

void Explorerplusplus::CreateCommandLineTabs()
{
	// It's implicitly assumed that this will succeed. Although the documentation states that
	// GetCurrentDirectory() can fail, I'm not sure under what circumstances it ever would.
	auto currentDirectory = GetCurrentDirectoryWrapper();
	CHECK(currentDirectory);

	for (const auto &fileToSelect : m_commandLineSettings->filesToSelect)
	{
		auto absolutePath = TransformUserEnteredPathToAbsolutePathAndNormalize(fileToSelect,
			currentDirectory.value(), EnvVarsExpansion::DontExpand);

		if (!absolutePath)
		{
			continue;
		}

		unique_pidl_absolute fullPidl;
		HRESULT hr = ParseDisplayNameForNavigation(absolutePath->c_str(), fullPidl);

		if (FAILED(hr))
		{
			continue;
		}

		unique_pidl_absolute parentPidl(ILCloneFull(fullPidl.get()));

		BOOL res = ILRemoveLastID(parentPidl.get());

		if (!res)
		{
			continue;
		}

		auto navigateParams = NavigateParams::Normal(parentPidl.get());
		Tab &newTab = GetActivePane()->GetTabContainer()->CreateNewTab(navigateParams,
			TabSettings(_selected = true));

		if (ArePidlsEquivalent(newTab.GetShellBrowser()->GetDirectoryIdl().get(), parentPidl.get()))
		{
			newTab.GetShellBrowser()->SelectItems({ fullPidl.get() });
		}
	}

	for (const auto &directory : m_commandLineSettings->directories)
	{
		// Windows Explorer doesn't expand environment variables passed in on the command line. The
		// command-line interpreter that's being used can expand variables - for example, running:
		//
		// explorer.exe %windir%
		//
		// from cmd.exe will result in %windir% being expanded before being passed to explorer.exe.
		// But if explorer.exe is launched with the string %windir% passed as a parameter, no
		// expansion will occur.
		// Therefore, no expansion is performed here either.
		// One difference from Explorer is that paths here are trimmed, which means that passing
		// "  C:\Windows  " will result in "C:\Windows" being opened.
		auto absolutePath = TransformUserEnteredPathToAbsolutePathAndNormalize(directory,
			currentDirectory.value(), EnvVarsExpansion::DontExpand);

		if (!absolutePath)
		{
			continue;
		}

		GetActivePane()->GetTabContainer()->CreateNewTab(*absolutePath,
			TabSettings(_selected = true));
	}
}

void Explorerplusplus::OnTabSelected(const Tab &tab)
{
	/* Hide the old listview. */
	ShowWindow(m_hActiveListView, SW_HIDE);

	m_hActiveListView = tab.GetShellBrowser()->GetListView();
	m_pActiveShellBrowser = tab.GetShellBrowser();

	UpdateWindowStates(tab);

	/* Show the new listview. */
	ShowWindow(m_hActiveListView, SW_SHOW);
	SetFocus(m_hActiveListView);
}

void Explorerplusplus::OnSelectTabByIndex(int iTab)
{
	int nTabs = GetActivePane()->GetTabContainer()->GetNumTabs();
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

	GetActivePane()->GetTabContainer()->SelectTabAtIndex(newIndex);
}

bool Explorerplusplus::OnCloseTab()
{
	const Tab &tab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	return GetActivePane()->GetTabContainer()->CloseTab(tab);
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

	if (GetActivePane()->GetTabContainer()->IsTabSelected(tab))
	{
		SetTimer(m_hContainer, LISTVIEW_ITEM_CHANGED_TIMER_ID, LISTVIEW_ITEM_CHANGED_TIMEOUT,
			nullptr);
	}
}

// TabNavigationInterface
void Explorerplusplus::CreateNewTab(NavigateParams &navigateParams, bool selected)
{
	GetActivePane()->GetTabContainer()->CreateNewTab(navigateParams,
		TabSettings(_selected = selected));
}

void Explorerplusplus::SelectTabById(int tabId)
{
	const Tab &tab = GetActivePane()->GetTabContainer()->GetTab(tabId);
	GetActivePane()->GetTabContainer()->SelectTab(tab);
}

std::vector<TabStorageData> Explorerplusplus::GetTabListStorageData()
{
	std::vector<TabStorageData> tabListStorageData;

	for (auto tabRef : GetActivePane()->GetTabContainer()->GetAllTabsInOrder())
	{
		const auto &tab = tabRef.get();

		TabStorageData tabStorageData;
		tabStorageData.pidl = tab.GetShellBrowser()->GetDirectoryIdl().get();
		tabStorageData.directory = tab.GetShellBrowser()->GetDirectory();
		tabStorageData.folderSettings = tab.GetShellBrowser()->GetFolderSettings();
		tabStorageData.columns = tab.GetShellBrowser()->ExportAllColumns();

		TabSettings tabSettings;

		if (tab.GetUseCustomName())
		{
			tabSettings.name = tab.GetName();
		}

		tabSettings.lockState = tab.GetLockState();

		tabStorageData.tabSettings = tabSettings;

		tabListStorageData.push_back(tabStorageData);
	}

	return tabListStorageData;
}
