// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabContextMenu.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "MenuView.h"
#include "RenameTabDialog.h"
#include "ResourceIconModel.h"
#include "ResourceLoader.h"
#include "ShellBrowser/NavigateParams.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "Tab.h"
#include "TabContainer.h"
#include "TabEvents.h"
#include "../Helper/DpiCompatibility.h"
#include <ranges>

TabContextMenu::TabContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	Tab *tab, TabContainer *tabContainer, TabEvents *tabEvents,
	const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_tab(tab),
	m_tabContainer(tabContainer),
	m_tabEvents(tabEvents),
	m_resourceLoader(resourceLoader)
{
	BuildMenu(resourceLoader);

	m_connections.push_back(m_menuView->AddItemSelectedObserver(
		std::bind(&TabContextMenu::OnMenuItemSelected, this, std::placeholders::_1)));
	m_connections.push_back(
		tabEvents->AddRemovedObserver(std::bind_front(&TabContextMenu::OnTabClosed, this),
			TabEventScope::ForBrowser(*m_tab->GetBrowser())));
}

void TabContextMenu::BuildMenu(const ResourceLoader *resourceLoader)
{
	int size = DpiCompatibility::GetInstance().GetSystemMetricsForDpi(SM_CXSMICON,
		USER_DEFAULT_SCREEN_DPI);

	m_menuView->AppendItem(IDM_TAB_CONTEXT_MENU_NEW_TAB,
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_NEW_TAB),
		std::make_unique<ResourceIconModel>(Icon::NewTab, size, resourceLoader),
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_NEW_TAB_HELP_TEXT),
		GetAcceleratorTextForId(IDM_FILE_NEWTAB));

	m_menuView->AppendItem(IDM_TAB_CONTEXT_MENU_DUPLICATE_TAB,
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_DUPLICATE_TAB), {},
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_DUPLICATE_TAB_HELP_TEXT),
		GetAcceleratorTextForId(IDA_DUPLICATE_TAB));

	m_menuView->AppendItem(IDM_TAB_CONTEXT_MENU_OPEN_PARENT_IN_NEW_TAB,
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_OPEN_PARENT_IN_NEW_TAB), {},
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_OPEN_PARENT_IN_NEW_TAB_HELP_TEXT));

	m_menuView->EnableItem(IDM_TAB_CONTEXT_MENU_OPEN_PARENT_IN_NEW_TAB,
		m_tab->GetShellBrowser()->GetNavigationController()->CanGoUp());

	m_menuView->AppendSeparator();

	m_menuView->AppendItem(IDM_TAB_CONTEXT_MENU_REFRESH,
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_REFRESH),
		std::make_unique<ResourceIconModel>(Icon::Refresh, size, resourceLoader),
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_REFRESH_HELP_TEXT),
		GetAcceleratorTextForId(IDM_VIEW_REFRESH));
	m_menuView->AppendItem(IDM_TAB_CONTEXT_MENU_REFRESH_ALL,
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_REFRESH_ALL), {},
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_REFRESH_ALL_HELP_TEXT));

	m_menuView->AppendSeparator();

	m_menuView->AppendItem(IDM_TAB_CONTEXT_MENU_RENAME_TAB,
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_RENAME_TAB), {},
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_RENAME_TAB_HELP_TEXT));
	m_menuView->AppendItem(IDM_TAB_CONTEXT_MENU_LOCK_TAB,
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_LOCK_TAB), {},
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_LOCK_TAB_HELP_TEXT));
	m_menuView->AppendItem(IDM_TAB_CONTEXT_MENU_LOCK_TAB_AND_ADDRESS,
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_LOCK_TAB_AND_ADDRESS), {},
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_LOCK_TAB_AND_ADDRESS_HELP_TEXT));

	m_menuView->CheckItem(IDM_TAB_CONTEXT_MENU_LOCK_TAB,
		m_tab->GetLockState() == Tab::LockState::Locked);
	m_menuView->CheckItem(IDM_TAB_CONTEXT_MENU_LOCK_TAB_AND_ADDRESS,
		m_tab->GetLockState() == Tab::LockState::AddressLocked);

	m_menuView->AppendSeparator();

	m_menuView->AppendItem(IDM_TAB_CONTEXT_MENU_CLOSE_TAB,
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_CLOSE_TAB),
		std::make_unique<ResourceIconModel>(Icon::CloseTab, size, resourceLoader),
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_CLOSE_TAB_HELP_TEXT),
		GetAcceleratorTextForId(IDM_FILE_CLOSETAB));
	m_menuView->AppendItem(IDM_TAB_CONTEXT_MENU_CLOSE_OTHER_TABS,
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_CLOSE_OTHER_TABS), {},
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_CLOSE_OTHER_TABS_HELP_TEXT));
	m_menuView->AppendItem(IDM_TAB_CONTEXT_MENU_CLOSE_TABS_TO_RIGHT,
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_CLOSE_TABS_TO_RIGHT), {},
		resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_CLOSE_TABS_TO_RIGHT_HELP_TEXT));

	m_menuView->EnableItem(IDM_TAB_CONTEXT_MENU_CLOSE_TAB,
		m_tab->GetLockState() == Tab::LockState::NotLocked);
}

void TabContextMenu::OnMenuItemSelected(UINT menuItemId)
{
	if (!m_tab)
	{
		// The tab has been closed, so there's nothing that needs to be done.
		return;
	}

	switch (menuItemId)
	{
	case IDM_TAB_CONTEXT_MENU_NEW_TAB:
		m_tabContainer->CreateNewTabInDefaultDirectory(TabSettings(_selected = true));
		break;

	case IDM_TAB_CONTEXT_MENU_DUPLICATE_TAB:
		m_tabContainer->DuplicateTab(*m_tab);
		break;

	case IDM_TAB_CONTEXT_MENU_OPEN_PARENT_IN_NEW_TAB:
		OnOpenParentInNewTab();
		break;

	case IDM_TAB_CONTEXT_MENU_REFRESH:
		m_tab->GetShellBrowser()->GetNavigationController()->Refresh();
		break;

	case IDM_TAB_CONTEXT_MENU_REFRESH_ALL:
		OnRefreshAllTabs();
		break;

	case IDM_TAB_CONTEXT_MENU_RENAME_TAB:
		OnRenameTab();
		break;

	case IDM_TAB_CONTEXT_MENU_LOCK_TAB:
		OnLockTab();
		break;

	case IDM_TAB_CONTEXT_MENU_LOCK_TAB_AND_ADDRESS:
		OnLockTabAndAddress();
		break;

	case IDM_TAB_CONTEXT_MENU_CLOSE_TAB:
		m_tabContainer->CloseTab(*m_tab);
		break;

	case IDM_TAB_CONTEXT_MENU_CLOSE_OTHER_TABS:
		OnCloseOtherTabs();
		break;

	case IDM_TAB_CONTEXT_MENU_CLOSE_TABS_TO_RIGHT:
		OnCloseTabsToRight();
		break;

	default:
		DCHECK(false);
		break;
	}
}

void TabContextMenu::OnOpenParentInNewTab()
{
	const auto &pidl = m_tab->GetShellBrowser()->GetDirectory();

	unique_pidl_absolute pidlParent;
	HRESULT hr = GetVirtualParentPath(pidl.Raw(), wil::out_param(pidlParent));

	if (SUCCEEDED(hr))
	{
		auto navigateParams = NavigateParams::Normal(pidlParent.get());
		m_tabContainer->CreateNewTab(navigateParams, TabSettings(_selected = true));
	}
}

void TabContextMenu::OnRefreshAllTabs()
{
	for (auto &tab : m_tabContainer->GetAllTabs() | std::views::values)
	{
		tab->GetShellBrowser()->GetNavigationController()->Refresh();
	}
}

void TabContextMenu::OnRenameTab()
{
	RenameTabDialog renameTabDialog(m_tab->GetBrowser()->GetHWND(), m_tab, m_tabEvents,
		m_resourceLoader);
	renameTabDialog.ShowModalDialog();
}

void TabContextMenu::OnLockTab()
{
	if (m_tab->GetLockState() == Tab::LockState::Locked)
	{
		m_tab->SetLockState(Tab::LockState::NotLocked);
	}
	else
	{
		m_tab->SetLockState(Tab::LockState::Locked);
	}
}

void TabContextMenu::OnLockTabAndAddress()
{
	if (m_tab->GetLockState() == Tab::LockState::AddressLocked)
	{
		m_tab->SetLockState(Tab::LockState::NotLocked);
	}
	else
	{
		m_tab->SetLockState(Tab::LockState::AddressLocked);
	}
}

void TabContextMenu::OnCloseOtherTabs()
{
	int index = m_tabContainer->GetTabIndex(*m_tab);
	int numTabs = m_tabContainer->GetNumTabs();

	for (int i = numTabs - 1; i >= 0; i--)
	{
		if (i != index)
		{
			const Tab &currentTab = m_tabContainer->GetTabByIndex(i);
			m_tabContainer->CloseTab(currentTab);
		}
	}
}

void TabContextMenu::OnCloseTabsToRight()
{
	int index = m_tabContainer->GetTabIndex(*m_tab);
	int numTabs = m_tabContainer->GetNumTabs();

	for (int i = numTabs - 1; i > index; i--)
	{
		const Tab &currentTab = m_tabContainer->GetTabByIndex(i);
		m_tabContainer->CloseTab(currentTab);
	}
}

void TabContextMenu::OnTabClosed(const Tab &tab)
{
	if (m_tab == &tab)
	{
		m_tab = nullptr;
	}
}
