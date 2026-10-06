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
#include <ranges>

TabContextMenu::TabContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	WeakPtr<Tab> tab, TabContainer *tabContainer, TabEvents *tabEvents,
	const ResourceLoader *resourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_tab(tab),
	m_tabContainer(tabContainer),
	m_tabEvents(tabEvents),
	m_resourceLoader(resourceLoader)
{
	BuildMenu(resourceLoader);
}

void TabContextMenu::BuildMenu(const ResourceLoader *resourceLoader)
{
	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_NEW_TAB))
			.SetKey(Item::NewTab)
			.SetIconModel(
				std::make_unique<ResourceIconModel>(Icon::NewTab, IconSize::Small, resourceLoader))
			.SetHelpText(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_NEW_TAB_HELP_TEXT))
			.SetAcceleratorText(GetAcceleratorTextForId(IDM_FILE_NEWTAB))
			.SetSelectedCallback(
				[this] { m_tabContainer->CreateNewTabInDefaultDirectory({ .selected = true }); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_DUPLICATE_TAB))
			.SetKey(Item::DuplicateTab)
			.SetHelpText(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_DUPLICATE_TAB_HELP_TEXT))
			.SetAcceleratorText(GetAcceleratorTextForId(IDA_DUPLICATE_TAB))
			.SetSelectedCallback([this] { OnDuplicateTab(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_OPEN_PARENT_IN_NEW_TAB))
			.SetKey(Item::OpenParentInNewTab)
			.SetHelpText(
				resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_OPEN_PARENT_IN_NEW_TAB_HELP_TEXT))
			.SetEnabledCallback(
				[this] { return m_tab->GetShellBrowser()->GetNavigationController()->CanGoUp(); })
			.SetSelectedCallback([this] { OnOpenParentInNewTab(); }));

	m_rootMenuView->AppendSeparator();

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_REFRESH))
			.SetKey(Item::Refresh)
			.SetIconModel(
				std::make_unique<ResourceIconModel>(Icon::Refresh, IconSize::Small, resourceLoader))
			.SetHelpText(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_REFRESH_HELP_TEXT))
			.SetAcceleratorText(GetAcceleratorTextForId(IDM_VIEW_REFRESH))
			.SetSelectedCallback([this] { OnRefresh(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_REFRESH_ALL))
			.SetKey(Item::RefreshAll)
			.SetHelpText(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_REFRESH_ALL_HELP_TEXT))
			.SetSelectedCallback([this] { OnRefreshAllTabs(); }));

	m_rootMenuView->AppendSeparator();

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_RENAME_TAB))
			.SetKey(Item::RenameTab)
			.SetHelpText(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_RENAME_TAB_HELP_TEXT))
			.SetSelectedCallback([this] { OnRenameTab(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_LOCK_TAB))
			.SetKey(Item::LockTab)
			.SetHelpText(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_LOCK_TAB_HELP_TEXT))
			.SetCheckedCallback([this] { return m_tab->GetLockState() == Tab::LockState::Locked; })
			.SetSelectedCallback([this] { OnLockTab(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_LOCK_TAB_AND_ADDRESS))
			.SetKey(Item::LockTabAndAddress)
			.SetHelpText(
				resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_LOCK_TAB_AND_ADDRESS_HELP_TEXT))
			.SetCheckedCallback(
				[this] { return m_tab->GetLockState() == Tab::LockState::AddressLocked; })
			.SetSelectedCallback([this] { OnLockTabAndAddress(); }));

	m_rootMenuView->AppendSeparator();

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_CLOSE_TAB))
			.SetKey(Item::CloseTab)
			.SetIconModel(std::make_unique<ResourceIconModel>(Icon::CloseTab, IconSize::Small,
				resourceLoader))
			.SetHelpText(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_CLOSE_TAB_HELP_TEXT))
			.SetAcceleratorText(GetAcceleratorTextForId(IDM_FILE_CLOSETAB))
			.SetEnabledCallback([this] { return m_tab->GetTabContainer()->CanCloseTab(*m_tab); })
			.SetSelectedCallback([this] { OnCloseTab(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_CLOSE_OTHER_TABS))
			.SetKey(Item::CloseOtherTabs)
			.SetHelpText(
				resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_CLOSE_OTHER_TABS_HELP_TEXT))
			.SetSelectedCallback([this] { OnCloseOtherTabs(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_CLOSE_TABS_TO_RIGHT))
			.SetKey(Item::CloseTabsToRight)
			.SetHelpText(
				resourceLoader->LoadString(IDS_TAB_CONTEXT_MENU_CLOSE_TABS_TO_RIGHT_HELP_TEXT))
			.SetSelectedCallback([this] { OnCloseTabsToRight(); }));
}

void TabContextMenu::OnDuplicateTab()
{
	if (!m_tab)
	{
		// The tab has been closed, so there's nothing that needs to be done.
		return;
	}

	m_tabContainer->DuplicateTab(*m_tab);
}

void TabContextMenu::OnOpenParentInNewTab()
{
	if (!m_tab)
	{
		return;
	}

	const auto &pidl = m_tab->GetShellBrowser()->GetDirectory();

	unique_pidl_absolute pidlParent;
	HRESULT hr = GetVirtualParentPath(pidl.Raw(), wil::out_param(pidlParent));

	if (SUCCEEDED(hr))
	{
		auto navigateParams = NavigateParams::Normal(pidlParent.get());
		m_tabContainer->CreateNewTab(navigateParams, { .selected = true });
	}
}

void TabContextMenu::OnRefresh()
{
	if (!m_tab)
	{
		return;
	}

	m_tab->GetShellBrowser()->GetNavigationController()->Refresh();
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
	if (!m_tab)
	{
		return;
	}

	auto *renameTabDialog = RenameTabDialog::Create(m_tab->GetBrowser()->GetHWND(), m_tab.Get(),
		m_tabEvents, m_resourceLoader);
	renameTabDialog->ShowModalDialog();
}

void TabContextMenu::OnLockTab()
{
	if (!m_tab)
	{
		return;
	}

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
	if (!m_tab)
	{
		return;
	}

	if (m_tab->GetLockState() == Tab::LockState::AddressLocked)
	{
		m_tab->SetLockState(Tab::LockState::NotLocked);
	}
	else
	{
		m_tab->SetLockState(Tab::LockState::AddressLocked);
	}
}

void TabContextMenu::OnCloseTab()
{
	if (!m_tab)
	{
		return;
	}

	m_tabContainer->CloseTab(*m_tab);
}

void TabContextMenu::OnCloseOtherTabs()
{
	if (!m_tab)
	{
		return;
	}

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
	if (!m_tab)
	{
		return;
	}

	int index = m_tabContainer->GetTabIndex(*m_tab);
	int numTabs = m_tabContainer->GetNumTabs();

	for (int i = numTabs - 1; i > index; i--)
	{
		const Tab &currentTab = m_tabContainer->GetTabByIndex(i);
		m_tabContainer->CloseTab(currentTab);
	}
}
