// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabContainerBackgroundContextMenu.h"
#include "Bookmarks/BookmarkHelper.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceHelper.h"
#include "TabContainer.h"
#include "TabRestorer.h"

TabContainerBackgroundContextMenu::TabContainerBackgroundContextMenu(MenuView *menuView,
	TabContainer *tabContainer, BookmarkTree *bookmarkTree, CoreInterface *coreInterface) :
	MenuBase(menuView),
	m_tabContainer(tabContainer),
	m_bookmarkTree(bookmarkTree),
	m_coreInterface(coreInterface)
{
	BuildMenu();

	m_connections.push_back(m_menuView->AddItemSelectedObserver(std::bind(
		&TabContainerBackgroundContextMenu::OnMenuItemSelected, this, std::placeholders::_1)));
}

void TabContainerBackgroundContextMenu::BuildMenu()
{
	m_menuView->AppendItem(IDM_TAB_CONTAINER_NEW_TAB,
		ResourceHelper::LoadString(m_coreInterface->GetResourceInstance(),
			IDS_TAB_CONTAINER_MENU_NEW_TAB));
	m_menuView->AppendItem(IDM_TAB_CONTAINER_REOPEN_CLOSED_TAB,
		ResourceHelper::LoadString(m_coreInterface->GetResourceInstance(),
			IDS_TAB_CONTAINER_MENU_REOPEN_CLOSED_TAB));
	m_menuView->AppendItem(IDM_TAB_CONTAINER_BOOKMARK_ALL_TABS,
		ResourceHelper::LoadString(m_coreInterface->GetResourceInstance(),
			IDS_TAB_CONTAINER_MENU_BOOKMARK_ALL_TABS));

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
			m_coreInterface->GetMainWindow(), m_coreInterface);
		break;

	default:
		DCHECK(false);
		break;
	}
}
