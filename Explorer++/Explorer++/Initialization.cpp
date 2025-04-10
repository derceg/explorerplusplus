// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "Config.h"
#include "DisplayWindow/DisplayWindow.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "MainWindow.h"
#include "MenuRanges.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ViewModes.h"
#include "StatusBar.h"
#include "StatusBarView.h"
#include "Tab.h"
#include "TabContainerImpl.h"
#include "TaskbarThumbnails.h"
#include "ThemeManager.h"
#include "ThemeWindowTracker.h"
#include "UiTheming.h"
#include "ViewModeHelper.h"

void Explorerplusplus::Initialize(const WindowStorageData *storageData)
{
	m_bookmarksMainMenu =
		std::make_unique<BookmarksMainMenu>(m_app, this, this, m_app->GetIconResourceLoader(),
			&m_iconFetcher, m_app->GetThemeManager(), m_app->GetBookmarkTree(),
			BookmarkMenuBuilder::MenuIdRange{ MENU_BOOKMARK_START_ID, MENU_BOOKMARK_END_ID });

	m_mainWindow = MainWindow::Create(m_hContainer, m_app, this, this);

	InitializeMainMenu();

	auto *statusBarView = StatusBarView::Create(m_hContainer, m_config);
	m_statusBar = StatusBar::Create(statusBarView, this, m_config, m_app->GetTabEvents(),
		m_app->GetShellBrowserEvents(), m_app->GetNavigationEvents(), m_app->GetResourceLoader());

	CreateMainRebarAndChildren(storageData);
	InitializeDisplayWindow();
	InitializeTabs();
	CreateFolderControls();

	/* All child windows MUST be resized before
	any listview changes take place. If auto arrange
	is turned off in the listview, when it is
	initially sized, all current items will lock
	to the current width. The only was to unlock
	them from this width is to turn auto arrange back on.
	Therefore, the listview MUST be set to the correct
	size initially. */
	UpdateLayout();

	m_taskbarThumbnails =
		std::make_unique<TaskbarThumbnails>(m_app, this, GetActivePane()->GetTabContainerImpl());

	CreateInitialTabs(storageData);

	// Register for any shell changes. This should be done after the tabs have
	// been created.
	SHChangeNotifyEntry shcne;
	shcne.fRecursive = TRUE;
	shcne.pidl = nullptr;
	m_SHChangeNotifyID = SHChangeNotifyRegister(m_hContainer, SHCNRF_ShellLevel, SHCNE_ASSOCCHANGED,
		WM_APP_ASSOC_CHANGED, 1, &shcne);

	SetFocus(m_hActiveListView);

	m_uiTheming = std::make_unique<UiTheming>(m_app, this, GetActivePane()->GetTabContainerImpl());

	InitializePlugins();

	m_themeWindowTracker =
		std::make_unique<ThemeWindowTracker>(m_hContainer, m_app->GetThemeManager());

	m_browserInitialized = true;
	m_browserInitializedSignal();
}

void Explorerplusplus::InitializeDisplayWindow()
{
	m_displayWindow = DisplayWindow::Create(m_hContainer, m_config);

	ApplyDisplayWindowPosition();

	m_connections.push_back(
		m_config->showDisplayWindow.addObserver(std::bind(&Explorerplusplus::UpdateLayout, this)));
}

wil::unique_hmenu Explorerplusplus::BuildViewsMenu()
{
	wil::unique_hmenu viewsMenu(CreatePopupMenu());
	AddViewModesToMenu(viewsMenu.get(), 0, TRUE);

	const Tab &tab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();
	ViewMode currentViewMode = tab.GetShellBrowserImpl()->GetViewMode();

	CheckMenuRadioItem(viewsMenu.get(), IDM_VIEW_EXTRALARGEICONS, IDM_VIEW_TILES,
		GetViewModeMenuId(currentViewMode), MF_BYCOMMAND);

	return viewsMenu;
}

void Explorerplusplus::AddViewModesToMenu(HMENU menu, UINT startPosition, BOOL byPosition)
{
	UINT position = startPosition;

	for (auto viewMode : VIEW_MODES)
	{
		std::wstring text = GetViewModeMenuText(m_app->GetResourceLoader(), viewMode);

		MENUITEMINFO itemInfo;
		itemInfo.cbSize = sizeof(itemInfo);
		itemInfo.fMask = MIIM_ID | MIIM_STRING;
		itemInfo.wID = GetViewModeMenuId(viewMode);
		itemInfo.dwTypeData = text.data();
		InsertMenuItem(menu, position, byPosition, &itemInfo);

		if (byPosition)
		{
			position++;
		}
	}
}
