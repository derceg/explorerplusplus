// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Bookmarks/BookmarkTreeFactory.h"
#include "Config.h"
#include "LoadSaveInterface.h"
#include "MenuRanges.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabContainer.h"
#include "TabRestorerUI.h"
#include "../Helper/Macros.h"
#include <list>

void Explorerplusplus::InitializeTabs()
{
	/* The tab backing will hold the tab window. */
	CreateTabBacking();

	m_tabContainer =
		TabContainer::Create(m_hTabBacking, this, this, &m_FileActionHandler, &m_cachedIcons,
			BookmarkTreeFactory::GetInstance()->GetBookmarkTree(), m_resourceInstance, m_config);
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

	m_tabContainer->sizeUpdatedSignal.AddObserver([this] { UpdateLayout(); });

	auto updateLayoutObserverMethod = [this](BOOL newValue)
	{
		UNREFERENCED_PARAMETER(newValue);

		UpdateLayout();
	};

	m_connections.push_back(m_config->showTabBarAtBottom.addObserver(updateLayoutObserverMethod));
	m_connections.push_back(m_config->extendTabControl.addObserver(updateLayoutObserverMethod));

	m_tabRestorer = std::make_unique<TabRestorer>(m_tabContainer);
	m_tabRestorerUI = std::make_unique<TabRestorerUI>(m_resourceInstance, this, m_tabRestorer.get(),
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

void Explorerplusplus::OnNavigationCommitted(const Tab &tab, const NavigateParams &navigateParams)
{
	UNREFERENCED_PARAMETER(navigateParams);

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

			auto navigateParams = NavigateParams::Normal(pidl.get());
			m_tabContainer->CreateNewTab(navigateParams, TabSettings(_selected = true), nullptr,
				&cols);
			return;
		}
	}

	/* Either no items are selected, or the focused + selected item was not a
	 * folder; open the default tab directory. */
	m_tabContainer->CreateNewTabInDefaultDirectory(TabSettings(_selected = true));
}

HRESULT Explorerplusplus::RestoreTabs(ILoadSave *pLoadSave)
{
	// It's implicitly assumed that this will succeed. Although the documentation states that
	// GetCurrentDirectory() can fail, I'm not sure under what circumstances it ever would.
	// Also note that it's important that this is called before creating any tabs, as
	// SetCurrentDirectory() is currently called when navigating/switching to a tab.
	auto currentDirectory = GetCurrentDirectoryWrapper();

	if (m_config->startupMode == StartupMode::PreviousTabs)
	{
		pLoadSave->LoadPreviousTabs();

		// It's possible that the above call might not have loaded any tabs (e.g. because there are
		// no saved settings). So, it's important that tab selection is only set when the last
		// selected tab value is in the appropriate range.
		if (m_iLastSelectedTab >= 0 && m_iLastSelectedTab < m_tabContainer->GetNumTabs())
		{
			m_tabContainer->SelectTabAtIndex(m_iLastSelectedTab);
		}
	}

	for (const auto &fileToSelect : m_commandLineSettings.filesToSelect)
	{
		auto absolutePath = TransformUserEnteredPathToAbsolutePathAndNormalize(fileToSelect,
			currentDirectory.value(), EnvVarsExpansion::DontExpand);

		if (!absolutePath)
		{
			continue;
		}

		unique_pidl_absolute fullPidl;
		HRESULT hr = SHParseDisplayName(absolutePath->c_str(), nullptr, wil::out_param(fullPidl), 0,
			nullptr);

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
		Tab &newTab = m_tabContainer->CreateNewTab(navigateParams, TabSettings(_selected = true));

		if (ArePidlsEquivalent(newTab.GetShellBrowser()->GetDirectoryIdl().get(), parentPidl.get()))
		{
			newTab.GetShellBrowser()->SelectItems({ fullPidl.get() });
		}
	}

	for (const auto &directory : m_commandLineSettings.directories)
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

		m_tabContainer->CreateNewTab(*absolutePath, TabSettings(_selected = true));
	}

	if (m_tabContainer->GetNumTabs() == 0)
	{
		m_tabContainer->CreateNewTabInDefaultDirectory(TabSettings(_selected = true));
	}

	if (!m_config->alwaysShowTabBar.get() && m_tabContainer->GetNumTabs() == 1)
	{
		m_bShowTabBar = false;
	}

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
void Explorerplusplus::CreateNewTab(NavigateParams &navigateParams, bool selected)
{
	m_tabContainer->CreateNewTab(navigateParams, TabSettings(_selected = selected));
}

void Explorerplusplus::SelectTabById(int tabId)
{
	const Tab &tab = m_tabContainer->GetTab(tabId);
	m_tabContainer->SelectTab(tab);
}
