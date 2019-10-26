// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "BookmarkHelper.h"
#include "Config.h"
#include "CustomizeColorsDialog.h"
#include "Explorer++_internal.h"
#include "LoadSaveInterface.h"
#include "MainImages.h"
#include "MainResource.h"
#include "MainWindow.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ViewModes.h"
#include "TaskbarThumbnails.h"
#include "../DisplayWindow/DisplayWindow.h"
#include "../Helper/Controls.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/iDirectoryMonitor.h"
#include "../Helper/Macros.h"
#include <list>
#include <map>


DWORD WINAPI WorkerThreadProc(LPVOID pParam);
void CALLBACK InitializeCOMAPC(ULONG_PTR dwParam);

const std::map<UINT, int> MAIN_MENU_IMAGE_MAPPINGS = {
	{ IDM_FILE_NEWTAB, SHELLIMAGES_NEWTAB },
	{ IDM_FILE_OPENCOMMANDPROMPT, SHELLIMAGES_CMD },
	{ IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR, SHELLIMAGES_CMDADMIN },
	{ IDM_FILE_DELETE, SHELLIMAGES_DELETE },
	{ IDM_FILE_DELETEPERMANENTLY, SHELLIMAGES_DELETEPERMANENTLY },
	{ IDM_FILE_RENAME, SHELLIMAGES_RENAME },
	{ IDM_FILE_PROPERTIES, SHELLIMAGES_PROPERTIES },

	{ IDM_EDIT_UNDO, SHELLIMAGES_UNDO },
	{ IDM_EDIT_COPY, SHELLIMAGES_COPY },
	{ IDM_EDIT_CUT, SHELLIMAGES_CUT },
	{ IDM_EDIT_PASTE, SHELLIMAGES_PASTE },
	{ IDM_EDIT_PASTESHORTCUT, SHELLIMAGES_PASTESHORTCUT },
	{ IDM_EDIT_COPYTOFOLDER, SHELLIMAGES_COPYTO },
	{ IDM_EDIT_MOVETOFOLDER, SHELLIMAGES_MOVETO },

	{ IDM_ACTIONS_NEWFOLDER, SHELLIMAGES_NEWFOLDER },

	{ IDM_VIEW_REFRESH, SHELLIMAGES_REFRESH },

	{ IDM_FILTER_FILTERRESULTS, SHELLIMAGES_FILTER },

	{ IDM_GO_BACK, SHELLIMAGES_BACK },
	{ IDM_GO_FORWARD, SHELLIMAGES_FORWARD },
	{ IDM_GO_UPONELEVEL, SHELLIMAGES_UP },

	{ IDM_BOOKMARKS_BOOKMARKTHISTAB, SHELLIMAGES_ADDFAV },
	{ IDM_BOOKMARKS_MANAGEBOOKMARKS, SHELLIMAGES_FAV },

	{ IDM_TOOLS_SEARCH, SHELLIMAGES_SEARCH },
	{ IDM_TOOLS_CUSTOMIZECOLORS, SHELLIMAGES_CUSTOMIZECOLORS },
	{ IDM_TOOLS_OPTIONS, SHELLIMAGES_OPTIONS },

	{ IDM_HELP_HELP, SHELLIMAGES_HELP }
};

DWORD WINAPI WorkerThreadProc(LPVOID pParam)
{
	UNREFERENCED_PARAMETER(pParam);

	/* OLE initialization is no longer done from within
	this function. This is because of the fact that the
	first APC may run BEFORE this thread initialization
	function. If this occurs, OLE will not be initialized,
	and possible errors may occur.
	OLE is now initialized using an APC that is queued
	immediately after this thread is created. As APC's
	are run sequentially, it is guaranteed that the
	initialization APC will run before any other APC,
	thus acting like this initialization function. */

	/* WARNING: Warning C4127 (conditional expression is
	constant) temporarily disabled for this function. */
	#pragma warning(push)
	#pragma warning(disable:4127)
	while(TRUE)
	{
		SleepEx(INFINITE, TRUE);
	}
	#pragma warning(pop)

	return 0;
}

void CALLBACK InitializeCOMAPC(ULONG_PTR dwParam)
{
	UNREFERENCED_PARAMETER(dwParam);

	/* This will be balanced out by a corresponding
	CoUninitialize() when the thread is ended.
	It must be apartment threaded, or some icons (such
	as those used for XML files) may not load properly.
	It *may* be due to the fact that one or more of
	the other threads in use do not initialize COM/
	use the same threading model. */
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
}

/*
* Main window creation.
*
* Settings are loaded very early on. Any
* initial settings must be in place before
* this.
*/
void Explorerplusplus::OnCreate(void)
{
	InitializeMainToolbars();
	InitializeBookmarks();

	ILoadSave *pLoadSave = NULL;
	LoadAllSettings(&pLoadSave);
	ApplyToolbarSettings();

	SetLanguageModule();

	m_navigation = new Navigation(m_config, this);

	m_mainWindow = MainWindow::Create(m_hContainer, m_config, m_hLanguageModule, this, m_navigation);

	m_hTreeViewIconThread = CreateWorkerThread();

	/* These need to occur after the language module
	has been initialized, but before the tabs are
	restored. */
	HMENU mainMenu = LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_MAINMENU));

	if (!g_enablePlugins)
	{
		DeleteMenu(mainMenu, IDM_TOOLS_RUNSCRIPT, MF_BYCOMMAND);
	}

	SetMenu(m_hContainer, mainMenu);

	m_hArrangeSubMenu = GetSubMenu(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_ARRANGEMENU)), 0);
	m_hArrangeSubMenuRClick = GetSubMenu(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_ARRANGEMENU)), 0);
	m_hGroupBySubMenu = GetSubMenu(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_GROUPBY_MENU)), 0);
	m_hGroupBySubMenuRClick = GetSubMenu(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_GROUPBY_MENU)), 0);

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

	/* Settings cannot be applied until
	all child windows have been created. */
	ApplyLoadedSettings();

	m_taskbarThumbnails = TaskbarThumbnails::Create(this, m_tabContainer, m_navigation, m_hLanguageModule, m_config);

	RestoreTabs(pLoadSave);
	delete pLoadSave;

	SHChangeNotifyEntry shcne;

	/* Don't need to specify any file for this notification. */
	shcne.fRecursive = TRUE;
	shcne.pidl = NULL;

	/* Register for any shell changes. This should
	be done after the tabs have been created. */
	m_SHChangeNotifyID = SHChangeNotifyRegister(m_hContainer, SHCNRF_ShellLevel,
		SHCNE_ASSOCCHANGED, WM_APP_ASSOCCHANGED, 1, &shcne);

	InitializeMenus();

	InitializeArrangeMenuItems();

	/* Place the main window in the clipboard chain. This
	will allow the 'Paste' button to be enabled/disabled
	dynamically. */
	m_hNextClipboardViewer = SetClipboardViewer(m_hContainer);

	SetFocus(m_hActiveListView);

	m_uiTheming = std::make_unique<UiTheming>(this, m_tabContainer);

	InitializePlugins();

	SetTimer(m_hContainer, AUTOSAVE_TIMER_ID, AUTOSAVE_TIMEOUT, nullptr);

	m_InitializationFinished = true;
}

/* Creates a low priority worker thread, and initializes COM on that thread. */
HANDLE Explorerplusplus::CreateWorkerThread()
{
	HANDLE hThread = CreateThread(NULL, 0, WorkerThreadProc, NULL, 0, NULL);
	SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);
	QueueUserAPC(InitializeCOMAPC, hThread, NULL);

	return hThread;
}

void Explorerplusplus::InitializeBookmarks(void)
{
	TCHAR szTemp[64];

	GUID RootGuid;

	/* The cast to RPC_WSTR is required for the reason
	discussed here: http://social.msdn.microsoft.com/Forums/vstudio/en-US/d1b4550a-407b-4c09-8560-0ab9ef6ff754/error-while-compiling-c2664. */
	UuidFromString(reinterpret_cast<RPC_WSTR>(NBookmarkHelper::ROOT_GUID),&RootGuid);

	LoadString(m_hLanguageModule,IDS_BOOKMARKS_ALLBOOKMARKS,szTemp,SIZEOF_ARRAY(szTemp));
	m_bfAllBookmarks = CBookmarkFolder::CreateNew(szTemp, RootGuid);

	GUID ToolbarGuid;
	UuidFromString(reinterpret_cast<RPC_WSTR>(NBookmarkHelper::TOOLBAR_GUID),&ToolbarGuid);
	LoadString(m_hLanguageModule,IDS_BOOKMARKS_BOOKMARKSTOOLBAR,szTemp,SIZEOF_ARRAY(szTemp));
	CBookmarkFolder bfBookmarksToolbar = CBookmarkFolder::Create(szTemp,ToolbarGuid);
	m_bfAllBookmarks->InsertBookmarkFolder(bfBookmarksToolbar);
	m_guidBookmarksToolbar = bfBookmarksToolbar.GetGUID();

	GUID MenuGuid;
	UuidFromString(reinterpret_cast<RPC_WSTR>(NBookmarkHelper::MENU_GUID),&MenuGuid);
	LoadString(m_hLanguageModule,IDS_BOOKMARKS_BOOKMARKSMENU,szTemp,SIZEOF_ARRAY(szTemp));
	CBookmarkFolder bfBookmarksMenu = CBookmarkFolder::Create(szTemp,MenuGuid);
	m_bfAllBookmarks->InsertBookmarkFolder(bfBookmarksMenu);
	m_guidBookmarksMenu = bfBookmarksMenu.GetGUID();
}

void Explorerplusplus::InitializeDisplayWindow()
{
	DWInitialSettings_t InitialSettings;
	InitialSettings.CentreColor		= m_config->displayWindowCentreColor;
	InitialSettings.SurroundColor	= m_config->displayWindowSurroundColor;
	InitialSettings.TextColor		= m_config->displayWindowTextColor;
	InitialSettings.hFont			= m_config->displayWindowFont;
	InitialSettings.hIcon			= (HICON)LoadImage(GetModuleHandle(0),
		MAKEINTRESOURCE(IDI_DISPLAYWINDOW),IMAGE_ICON,
		0,0,LR_CREATEDIBSECTION);

	m_hDisplayWindow = CreateDisplayWindow(m_hContainer,&InitialSettings);
}

void Explorerplusplus::InitializeMenus(void)
{
	HMENU hMenu = GetMenu(m_hContainer);

	AddViewModesToMenu(hMenu);

	/* Delete the placeholder menu. */
	DeleteMenu(hMenu,IDM_VIEW_PLACEHOLDER,MF_BYCOMMAND);

	SetMainMenuImages();

	SetGoMenuName(hMenu,IDM_GO_MYCOMPUTER,CSIDL_DRIVES);
	SetGoMenuName(hMenu,IDM_GO_MYDOCUMENTS,CSIDL_PERSONAL);
	SetGoMenuName(hMenu,IDM_GO_MYMUSIC,CSIDL_MYMUSIC);
	SetGoMenuName(hMenu,IDM_GO_MYPICTURES,CSIDL_MYPICTURES);
	SetGoMenuName(hMenu,IDM_GO_DESKTOP,CSIDL_DESKTOP);
	SetGoMenuName(hMenu,IDM_GO_RECYCLEBIN,CSIDL_BITBUCKET);
	SetGoMenuName(hMenu,IDM_GO_CONTROLPANEL,CSIDL_CONTROLS);
	SetGoMenuName(hMenu,IDM_GO_PRINTERS,CSIDL_PRINTERS);
	SetGoMenuName(hMenu,IDM_GO_CDBURNING,CSIDL_CDBURN_AREA);
	SetGoMenuName(hMenu,IDM_GO_MYNETWORKPLACES,CSIDL_NETWORK);
	SetGoMenuName(hMenu,IDM_GO_NETWORKCONNECTIONS,CSIDL_CONNECTIONS);
}

void Explorerplusplus::SetMainMenuImages()
{
	HImageListPtr imageList = GetShellImageList();

	if (!imageList)
	{
		return;
	}

	HMENU mainMenu = GetMenu(m_hContainer);

	for (auto mapping : MAIN_MENU_IMAGE_MAPPINGS)
	{
		SetMenuItemImageFromImageList(mainMenu, mapping.first, imageList.get(), mapping.second, m_menuImages);
	}
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

	for (auto viewMode : m_viewModes)
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