// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabContainerBackgroundContextMenu.h"
#include "Bookmarks/BookmarkHelper.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "MenuView.h"
#include "ResourceLoader.h"
#include "TabContainer.h"
#include "TabRestorer.h"

TabContainerBackgroundContextMenu::TabContainerBackgroundContextMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager, TabContainer *tabContainer,
	TabRestorer *tabRestorer, BookmarkTree *bookmarkTree, BrowserWindow *browser,
	const ResourceLoader *resourceLoader, PlatformContext *platformContext) :
	MenuBase(menuView, acceleratorManager),
	m_tabContainer(tabContainer),
	m_tabRestorer(tabRestorer),
	m_bookmarkTree(bookmarkTree),
	m_browser(browser),
	m_resourceLoader(resourceLoader),
	m_platformContext(platformContext)
{
	BuildMenu();
}

void TabContainerBackgroundContextMenu::BuildMenu()
{
	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_TAB_CONTAINER_MENU_NEW_TAB))
			.SetAcceleratorText(GetAcceleratorTextForId(IDM_FILE_NEWTAB))
			.SetSelectedCallback(
				[this] { m_tabContainer->CreateNewTabInDefaultDirectory({ .selected = true }); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_TAB_CONTAINER_MENU_REOPEN_CLOSED_TAB))
			.SetAcceleratorText(GetAcceleratorTextForId(IDA_RESTORE_LAST_TAB))
			.SetEnabledCallback([this] { return !m_tabRestorer->IsEmpty(); })
			.SetSelectedCallback([this] { m_tabRestorer->RestoreLastTab(); }));

	m_rootMenuView->AppendItem(
		MenuItemParams(m_resourceLoader->LoadString(IDS_TAB_CONTAINER_MENU_BOOKMARK_ALL_TABS))
			.SetAcceleratorText(GetAcceleratorTextForId(IDM_BOOKMARKS_BOOKMARK_ALL_TABS))
			.SetSelectedCallback(
				[this]
				{
					BookmarkHelper::BookmarkAllTabs(m_bookmarkTree, m_browser, m_platformContext,
						m_acceleratorManager, m_resourceLoader);
				}));
}
