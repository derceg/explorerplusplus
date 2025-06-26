// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "ColumnStorage.h"
#include "Config.h"
#include "MainTabView.h"
#include "ShellBrowser/NavigateParams.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabBacking.h"
#include "TabContainer.h"
#include "TabStorage.h"

void Explorerplusplus::InitializeTabs()
{
	m_tabBacking = TabBacking::Create(m_hContainer, this, this, m_app->GetResourceLoader(),
		m_config, m_app->GetTabEvents());

	auto *mainTabView =
		MainTabView::Create(m_tabBacking->GetHWND(), m_config, m_app->GetResourceLoader());
	m_connections.push_back(mainTabView->sizeUpdatedSignal.AddObserver([this] { UpdateLayout(); }));

	auto *tabContainer = TabContainer::Create(mainTabView, this, &m_shellBrowserFactory,
		m_app->GetTabEvents(), m_app->GetShellBrowserEvents(), m_app->GetNavigationEvents(),
		m_app->GetTabRestorer(), m_app->GetCachedIcons(), m_app->GetBookmarkTree(),
		m_app->GetAcceleratorManager(), m_config, m_app->GetResourceLoader());
	m_browserPane = std::make_unique<BrowserPane>(tabContainer);

	m_connections.push_back(m_config->alwaysShowTabBar.addObserver(
		std::bind(&Explorerplusplus::MaybeUpdateTabBarVisibility, this)));
	m_connections.push_back(m_app->GetTabEvents()->AddCreatedObserver(
		std::bind(&Explorerplusplus::MaybeUpdateTabBarVisibility, this),
		TabEventScope::ForBrowser(*this)));
	m_connections.push_back(m_app->GetTabEvents()->AddRemovedObserver(
		std::bind(&Explorerplusplus::MaybeUpdateTabBarVisibility, this),
		TabEventScope::ForBrowser(*this)));

	m_connections.push_back(m_app->GetTabEvents()->AddCreatedObserver(
		std::bind_front(&Explorerplusplus::OnTabCreated, this), TabEventScope::ForBrowser(*this),
		boost::signals2::at_front));
	m_connections.push_back(m_app->GetTabEvents()->AddSelectedObserver(
		std::bind_front(&Explorerplusplus::OnTabSelected, this), TabEventScope::ForBrowser(*this),
		boost::signals2::at_front));

	m_connections.push_back(m_app->GetNavigationEvents()->AddCommittedObserver(
		std::bind_front(&Explorerplusplus::OnNavigationCommitted, this),
		NavigationEventScope::ForActiveShellBrowser(*this), boost::signals2::at_front));

	m_connections.push_back(m_app->GetShellBrowserEvents()->AddItemsChangedObserver(
		std::bind_front(&Explorerplusplus::OnDirectoryContentsChanged, this),
		ShellBrowserEventScope::ForActiveShellBrowser(*this), boost::signals2::at_front));
	m_connections.push_back(m_app->GetShellBrowserEvents()->AddSelectionChangedObserver(
		std::bind_front(&Explorerplusplus::OnTabListViewSelectionChanged, this),
		ShellBrowserEventScope::ForBrowser(*this), boost::signals2::at_front));

	auto updateLayoutObserverMethod = [this](BOOL newValue)
	{
		UNREFERENCED_PARAMETER(newValue);

		UpdateLayout();
	};

	m_connections.push_back(m_config->showTabBarAtBottom.addObserver(updateLayoutObserverMethod));
	m_connections.push_back(m_config->extendTabControl.addObserver(updateLayoutObserverMethod));
}

void Explorerplusplus::MaybeUpdateTabBarVisibility()
{
	if (!m_config->alwaysShowTabBar.get()
		&& (GetActivePane()->GetTabContainer()->GetNumTabs() == 1))
	{
		HideTabBar();
	}
	else
	{
		ShowTabBar();
	}
}

void Explorerplusplus::OnTabCreated(const Tab &tab)
{
	/* TODO: This subclass needs to be removed. */
	SetWindowSubclass(tab.GetShellBrowserImpl()->GetListView(), ListViewProcStub, 0,
		reinterpret_cast<DWORD_PTR>(this));

	// A tab has been created, so this call is needed in order to set the size and position of the
	// tab's listview control.
	UpdateLayout();
}

void Explorerplusplus::OnNavigationCommitted(const NavigationRequest *request)
{
	const auto *tab = request->GetShellBrowser()->GetTab();
	UpdateWindowStates(*tab);
}

/* Creates a new tab. If a folder is selected, that folder is opened in a new
 * tab, else the default directory is opened. */
void Explorerplusplus::OnNewTab()
{
	const Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	int selectionIndex = ListView_GetNextItem(selectedTab.GetShellBrowserImpl()->GetListView(), -1,
		LVNI_FOCUSED | LVNI_SELECTED);

	if (selectionIndex != -1)
	{
		auto fileFindData = selectedTab.GetShellBrowserImpl()->GetItemFileFindData(selectionIndex);

		/* If the selected item is a folder, open that folder in a new tab, else
		 * just use the default new tab directory. */
		if (WI_IsFlagSet(fileFindData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
		{
			auto pidl = selectedTab.GetShellBrowserImpl()->GetItemCompleteIdl(selectionIndex);
			FolderColumns cols = selectedTab.GetShellBrowserImpl()->GetColumns();

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

void Explorerplusplus::CreateInitialTabs(const WindowStorageData *storageData)
{
	if (storageData)
	{
		CreateTabsFromStorageData(*storageData);
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
}

void Explorerplusplus::CreateTabsFromStorageData(const WindowStorageData &storageData)
{
	int index = 0;

	for (const auto &loadedTab : storageData.tabs)
	{
		// It's important that the index is set on the tab. That's because the
		// openNewTabNextToCurrent setting will alter the index at which a tab is created. If that
		// setting was enabled and the index wasn't explicitly set here, the first tab would be
		// created and selected, and each additional tab would be created to the immediate right of
		// the first tab.
		auto tabSettings = loadedTab.tabSettings;
		tabSettings.index = index;

		auto validatedColumns = loadedTab.columns;
		ValidateColumns(validatedColumns);

		if (loadedTab.pidl.HasValue())
		{
			auto navigateParams = NavigateParams::Normal(loadedTab.pidl.Raw());
			GetActivePane()->GetTabContainer()->CreateNewTab(navigateParams, tabSettings,
				&loadedTab.folderSettings, &validatedColumns);
		}
		else
		{
			GetActivePane()->GetTabContainer()->CreateNewTab(loadedTab.directory, tabSettings,
				&loadedTab.folderSettings, &validatedColumns);
		}

		index++;
	}

	if (storageData.selectedTab >= 0
		&& storageData.selectedTab < GetActivePane()->GetTabContainer()->GetNumTabs())
	{
		GetActivePane()->GetTabContainer()->SelectTabAtIndex(storageData.selectedTab);
	}
}

void Explorerplusplus::CreateCommandLineTabs()
{
	// It's implicitly assumed that this will succeed. Although the documentation states that
	// GetCurrentDirectory() can fail, I'm not sure under what circumstances it ever would.
	auto currentDirectory = GetCurrentDirectoryWrapper();
	CHECK(currentDirectory);

	for (const auto &fileToSelect : m_app->GetCommandLineSettings()->filesToSelect)
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

		if (ArePidlsEquivalent(newTab.GetShellBrowserImpl()->GetDirectoryIdl().get(),
				parentPidl.get()))
		{
			newTab.GetShellBrowserImpl()->SelectItems({ fullPidl.get() });
		}
	}

	for (const auto &directory : m_app->GetCommandLineSettings()->directories)
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

	m_hActiveListView = tab.GetShellBrowserImpl()->GetListView();
	m_pActiveShellBrowser = tab.GetShellBrowserImpl();

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

void Explorerplusplus::OnTabListViewSelectionChanged(const ShellBrowser *shellBrowser)
{
	const auto *tab = shellBrowser->GetTab();

	/* The selection for this tab has changed, so invalidate any
	folder size calculations that are occurring for this tab
	(applies only to folder sizes that will be shown in the display
	window). */
	for (auto &item : m_DWFolderSizes)
	{
		if (item.iTabId == tab->GetId())
		{
			item.bValid = FALSE;
		}
	}

	if (GetActivePane()->GetTabContainer()->IsTabSelected(*tab))
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
