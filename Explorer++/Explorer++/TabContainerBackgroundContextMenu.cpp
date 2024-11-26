// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabContainerBackgroundContextMenu.h"
#include "Bookmarks/BookmarkHelper.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceManager.h"
#include "TabContainer.h"
#include "TabRestorer.h"

TabContainerBackgroundContextMenu::TabContainerBackgroundContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, TabContainer *tabContainer,
	BookmarkTree *bookmarkTree, CoreInterface *coreInterface,
	const IconResourceLoader *iconResourceLoader) :
	MenuBase(menuView, acceleratorManager),
	m_tabContainer(tabContainer),
	m_bookmarkTree(bookmarkTree),
	m_coreInterface(coreInterface),
	m_iconResourceLoader(iconResourceLoader)
{
	BuildMenu();

	m_connections.push_back(m_menuView->AddItemSelectedObserver(std::bind(
		&TabContainerBackgroundContextMenu::OnMenuItemSelected, this, std::placeholders::_1)));
}

void TabContainerBackgroundContextMenu::BuildMenu()
{
	m_menuView->AppendItem(IDM_TAB_CONTAINER_NEW_TAB,
		Resources::LoadString(IDS_TAB_CONTAINER_MENU_NEW_TAB), {}, L"",
		GetAcceleratorTextForId(IDM_FILE_NEWTAB));
	m_menuView->AppendItem(IDM_TAB_CONTAINER_REOPEN_CLOSED_TAB,
		Resources::LoadString(IDS_TAB_CONTAINER_MENU_REOPEN_CLOSED_TAB), {}, L"",
		GetAcceleratorTextForId(IDA_RESTORE_LAST_TAB));
	m_menuView->AppendItem(IDM_TAB_CONTAINER_BOOKMARK_ALL_TABS,
		Resources::LoadString(IDS_TAB_CONTAINER_MENU_BOOKMARK_ALL_TABS), {}, L"",
		GetAcceleratorTextForId(IDM_BOOKMARKS_BOOKMARK_ALL_TABS));

	if (m_coreInterface->GetTabRestorer()->GetClosedTabs().empty())
	{
		m_menuView->EnableItem(IDM_TAB_CONTAINER_REOPEN_CLOSED_TAB, false);
	}
}

void TabContainerBackgroundContextMenu::OnMenuItemSelected(UINT menuItemId)
{
	switch (menuItemId)
	{
	case IDM_TAB_CONTAINER_NEW_TAB:
		m_tabContainer->CreateNewTabInDefaultDirectory(TabSettings(_selected = true));
		break;

	case IDM_TAB_CONTAINER_REOPEN_CLOSED_TAB:
		m_coreInterface->GetTabRestorer()->RestoreLastTab();
		break;

	case IDM_TAB_CONTAINER_BOOKMARK_ALL_TABS:
		BookmarkHelper::BookmarkAllTabs(m_bookmarkTree, m_coreInterface->GetResourceInstance(),
			m_coreInterface->GetMainWindow(), m_coreInterface, m_iconResourceLoader);
		break;

	default:
		DCHECK(false);
		break;
	}
}
