// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "Config.h"
#include "DisplayWindow/DisplayWindow.h"
#include "Explorer++_internal.h"
#include "LoadSaveInterface.h"
#include "MainResource.h"
#include "MainWindow.h"
#include "MenuHelper.h"
#include "MenuRanges.h"
#include "ShellBrowser/ViewModes.h"
#include "TaskbarThumbnails.h"
#include "UiTheming.h"
#include "ViewModeHelper.h"
#include "../Helper/iDirectoryMonitor.h"
#include "../Helper/ImageHelper.h"
#include "../Helper/Macros.h"

/*
* Main window creation.
*
* Settings are loaded very early on. Any
* initial settings must be in place before
* this.
*/
void Explorerplusplus::OnCreate()
{
	InitializeMainToolbars();

	ILoadSave *pLoadSave = nullptr;
	LoadAllSettings(&pLoadSave);
	ApplyToolbarSettings();

	m_iconResourceLoader = std::make_unique<IconResourceLoader>(m_config->iconTheme);

	SetLanguageModule();

	m_bookmarksMainMenu = std::make_unique<BookmarksMainMenu>(this, &m_bookmarkTree, MenuIdRange{ MENU_BOOKMARK_STARTID, MENU_BOOKMARK_ENDID });

	m_navigation = std::make_unique<Navigation>(this);

	m_mainWindow = MainWindow::Create(m_hContainer, m_config, m_hLanguageModule, this);

	InitializeMainMenu();

	CreateDirectoryMonitor(&m_pDirMon);

	CreateStatusBar();
	CreateMainControls();
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
	ResizeWindows();

	m_taskbarThumbnails = TaskbarThumbnails::Create(this, m_tabContainer, m_hLanguageModule, m_config);

	RestoreTabs(pLoadSave);
	delete pLoadSave;

	// Register for any shell changes. This should be done after the tabs have
	// been created.
	SHChangeNotifyEntry shcne;
	shcne.fRecursive = TRUE;
	shcne.pidl = nullptr;
	m_SHChangeNotifyID = SHChangeNotifyRegister(m_hContainer, SHCNRF_ShellLevel,
		SHCNE_ASSOCCHANGED, WM_APP_ASSOCCHANGED, 1, &shcne);

	/* Place the main window in the clipboard chain. This
	will allow the 'Paste' button to be enabled/disabled
	dynamically. */
	m_hNextClipboardViewer = SetClipboardViewer(m_hContainer);

	SetFocus(m_hActiveListView);

	m_uiTheming = std::make_unique<UiTheming>(this, m_tabContainer);

	InitializePlugins();

	SetTimer(m_hContainer, AUTOSAVE_TIMER_ID, AUTOSAVE_TIMEOUT, nullptr);

	m_InitializationFinished.set(true);
}

void Explorerplusplus::InitializeDisplayWindow()
{
	DWInitialSettings_t InitialSettings;
	InitialSettings.CentreColor		= m_config->displayWindowCentreColor;
	InitialSettings.SurroundColor	= m_config->displayWindowSurroundColor;
	InitialSettings.TextColor		= m_config->displayWindowTextColor;
	InitialSettings.hFont			= m_config->displayWindowFont;
	InitialSettings.hIcon			= (HICON)LoadImage(GetModuleHandle(nullptr),
		MAKEINTRESOURCE(IDI_DISPLAYWINDOW),IMAGE_ICON,
		0,0,LR_CREATEDIBSECTION);

	m_hDisplayWindow = CreateDisplayWindow(m_hContainer,&InitialSettings);
}

HMENU Explorerplusplus::BuildViewsMenu()
{
	HMENU viewsMenu = GetSubMenu(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_VIEWS_MENU)), 0);
	AddViewModesToMenu(viewsMenu);
	DeleteMenu(viewsMenu, IDM_VIEW_PLACEHOLDER, MF_BYCOMMAND);

	return viewsMenu;
}

void Explorerplusplus::AddViewModesToMenu(HMENU menu)
{
	/* Insert the view mode (icons, small icons, details, etc) menus in. */
	MENUITEMINFO mii;
	TCHAR szText[64];

	for (auto viewMode : VIEW_MODES)
	{
		LoadString(m_hLanguageModule, GetViewModeMenuStringId(viewMode),
			szText, SIZEOF_ARRAY(szText));

		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_ID | MIIM_STRING;
		mii.wID = GetViewModeMenuId(viewMode);
		mii.dwTypeData = szText;
		InsertMenuItem(menu, IDM_VIEW_PLACEHOLDER, FALSE, &mii);
	}
}