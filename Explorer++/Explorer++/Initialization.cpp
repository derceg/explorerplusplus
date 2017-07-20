/******************************************************************
 *
 * Project: Explorer++
 * File: Initialization.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Includes miscellaneous functions related to
 * the top-level GUI component.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "Explorer++.h"
#include "MainImages.h"
#include "CustomizeColorsDialog.h"
#include "BookmarkHelper.h"
#include "../DisplayWindow/DisplayWindow.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/Macros.h"
#include "MainResource.h"


DWORD WINAPI WorkerThreadProc(LPVOID pParam);
void CALLBACK InitializeCOMAPC(ULONG_PTR dwParam);

extern HIMAGELIST himlMenu;

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
	ILoadSave *pLoadSave = NULL;

	InitializeTaskbarThumbnails();

	InitializeBookmarks();

	LoadAllSettings(&pLoadSave);
	ApplyToolbarSettings();

	m_hIconThread = CreateWorkerThread();
	m_hTreeViewIconThread = CreateWorkerThread();
	m_hFolderSizeThread = CreateWorkerThread();

	/* These need to occur after the language module
	has been initialized, but before the tabs are
	restored. */
	SetMenu(m_hContainer, LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_MAINMENU)));
	m_hArrangeSubMenu = GetSubMenu(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_ARRANGEMENU)), 0);
	m_hArrangeSubMenuRClick = GetSubMenu(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_ARRANGEMENU)), 0);
	m_hGroupBySubMenu = GetSubMenu(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_GROUPBY_MENU)), 0);
	m_hGroupBySubMenuRClick = GetSubMenu(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_GROUPBY_MENU)), 0);
	m_hTabRightClickMenu = GetSubMenu(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_TAB_RCLICK)), 0);
	m_hToolbarRightClickMenu = GetSubMenu(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_TOOLBAR_MENU)), 0);
	m_hViewsMenu = GetSubMenu(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_VIEWS_MENU)), 0);

	HBITMAP hb;

	/* Large and small image lists for the main toolbar. */
	m_himlToolbarSmall = ImageList_Create(TOOLBAR_IMAGE_SIZE_SMALL_X, TOOLBAR_IMAGE_SIZE_SMALL_Y, ILC_COLOR32 | ILC_MASK, 0, 47);
	hb = LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(IDB_SHELLIMAGES));
	ImageList_Add(m_himlToolbarSmall, hb, NULL);
	DeleteObject(hb);

	m_himlToolbarLarge = ImageList_Create(TOOLBAR_IMAGE_SIZE_LARGE_X, TOOLBAR_IMAGE_SIZE_LARGE_Y, ILC_COLOR32 | ILC_MASK, 0, 47);
	hb = LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(IDB_SHELLIMAGES_LARGE));
	ImageList_Add(m_himlToolbarLarge, hb, NULL);
	DeleteObject(hb);

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

	/* Taskbar thumbnails can only be shown in
	Windows 7, so we'll set the internal setting to
	false if we're running on an earlier version
	of Windows. */
	if(!(m_dwMajorVersion == WINDOWS_VISTA_SEVEN_MAJORVERSION &&
		m_dwMinorVersion >= 1))
	{
		m_bShowTaskbarThumbnails = FALSE;
	}

	/* The internal variable that controls whether or not
	taskbar thumbnails are shown in Windows 7 should only
	be set once during execution (i.e. when Explorer++
	starts up).
	Therefore, we'll only ever show the user a provisional
	setting, to stop them from changing the actual value. */
	m_bShowTaskbarThumbnailsProvisional = m_bShowTaskbarThumbnails;

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




	/* Mark the main menus as owner drawn. */
	InitializeMenus();

	InitializeArrangeMenuItems();

	/* Place the main window in the clipboard chain. This
	will allow the 'Paste' button to be enabled/disabled
	dynamically. */
	m_hNextClipboardViewer = SetClipboardViewer(m_hContainer);

	SetFocus(m_hActiveListView);
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

	m_bBroadcastIPBookmarkNotifications = true;

	m_pipbin = new CIPBookmarkItemNotifier(m_hContainer,this);
	CBookmarkItemNotifier::GetInstance().AddObserver(m_pipbin);

	m_pipbo = new CIPBookmarkObserver(m_bfAllBookmarks,this);
}

void Explorerplusplus::InitializeDisplayWindow(void)
{
	DWInitialSettings_t	InitialSettings;

	InitialSettings.CentreColor		= m_DisplayWindowCentreColor;
	InitialSettings.SurroundColor	= m_DisplayWindowSurroundColor;
	InitialSettings.TextColor		= m_DisplayWindowTextColor;
	InitialSettings.hFont			= m_DisplayWindowFont;
	InitialSettings.hIcon			= (HICON)LoadImage(GetModuleHandle(0),
		MAKEINTRESOURCE(IDI_DISPLAYWINDOW),IMAGE_ICON,
		0,0,LR_CREATEDIBSECTION);

	m_hDisplayWindow = CreateDisplayWindow(m_hContainer,&InitialSettings);
}

void Explorerplusplus::InitializeMenus(void)
{
	HMENU	hMenu;
	HBITMAP	hBitmap;
	int		nTopLevelMenus;
	int		i = 0;

	hMenu = GetMenu(m_hContainer);

	/* Insert the view mode (icons, small icons, details, etc) menus in. */
	MENUITEMINFO mii;
	std::list<ViewMode_t>::iterator itr;
	TCHAR szText[64];

	for(itr = m_ViewModes.begin();itr != m_ViewModes.end();itr++)
	{
		LoadString(m_hLanguageModule,GetViewModeMenuStringId(itr->uViewMode),
			szText,SIZEOF_ARRAY(szText));

		mii.cbSize		= sizeof(mii);
		mii.fMask		= MIIM_ID|MIIM_STRING;
		mii.wID			= GetViewModeMenuId(itr->uViewMode);
		mii.dwTypeData	= szText;
		InsertMenuItem(hMenu,IDM_VIEW_PLACEHOLDER,FALSE,&mii);

		InsertMenuItem(m_hViewsMenu,IDM_VIEW_PLACEHOLDER,FALSE,&mii);
	}

	/* Delete the placeholder menu. */
	DeleteMenu(hMenu,IDM_VIEW_PLACEHOLDER,MF_BYCOMMAND);
	DeleteMenu(m_hViewsMenu,IDM_VIEW_PLACEHOLDER,MF_BYCOMMAND);

	nTopLevelMenus = GetMenuItemCount(hMenu);

	/* Loop through each of the top level menus, setting
	all sub menus to owner drawn. Don't set top-level
	parent menus to owner drawn. */
	for(i = 0;i < nTopLevelMenus;i++)
	{
		SetMenuOwnerDraw(GetSubMenu(hMenu,i));
	}

	himlMenu = ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,0,48);

	/* Contains all images used on the menus. */
	hBitmap = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_SHELLIMAGES));

	ImageList_Add(himlMenu,hBitmap,NULL);

	/* <---- Associate menu items with a particular image ----> */

	/* <---- Main menu ----> */

	SetMenuItemBitmap(hMenu,IDM_FILE_NEWTAB,SHELLIMAGES_NEWTAB);
	SetMenuItemBitmap(hMenu,IDM_FILE_OPENCOMMANDPROMPT,SHELLIMAGES_CMD);
	SetMenuItemBitmap(hMenu,IDM_FILE_OPENCOMMANDPROMPTADMINISTRATOR,SHELLIMAGES_CMDADMIN);
	SetMenuItemBitmap(hMenu,IDM_FILE_DELETE,SHELLIMAGES_DELETE);
	SetMenuItemBitmap(hMenu,IDM_FILE_DELETEPERMANENTLY,SHELLIMAGES_DELETEPERMANENTLY);
	SetMenuItemBitmap(hMenu,IDM_FILE_RENAME,SHELLIMAGES_RENAME);
	SetMenuItemBitmap(hMenu,IDM_FILE_PROPERTIES,SHELLIMAGES_PROPERTIES);

	SetMenuItemBitmap(hMenu,IDM_EDIT_UNDO,SHELLIMAGES_UNDO);
	SetMenuItemBitmap(hMenu,IDM_EDIT_COPY,SHELLIMAGES_COPY);
	SetMenuItemBitmap(hMenu,IDM_EDIT_CUT,SHELLIMAGES_CUT);
	SetMenuItemBitmap(hMenu,IDM_EDIT_PASTE,SHELLIMAGES_PASTE);
	SetMenuItemBitmap(hMenu,IDM_EDIT_PASTESHORTCUT,SHELLIMAGES_PASTESHORTCUT);
	SetMenuItemBitmap(hMenu,IDM_EDIT_COPYTOFOLDER,SHELLIMAGES_COPYTO);
	SetMenuItemBitmap(hMenu,IDM_EDIT_MOVETOFOLDER,SHELLIMAGES_MOVETO);

	SetMenuItemBitmap(hMenu,IDM_ACTIONS_NEWFOLDER,SHELLIMAGES_NEWFOLDER);

	SetMenuItemBitmap(hMenu,IDM_VIEW_REFRESH,SHELLIMAGES_REFRESH);

	SetMenuItemBitmap(hMenu,IDM_FILTER_FILTERRESULTS,SHELLIMAGES_FILTER);

	SetMenuItemBitmap(hMenu,IDM_GO_BACK,SHELLIMAGES_BACK);
	SetMenuItemBitmap(hMenu,IDM_GO_FORWARD,SHELLIMAGES_FORWARD);
	SetMenuItemBitmap(hMenu,IDM_GO_UPONELEVEL,SHELLIMAGES_UP);

	SetMenuItemBitmap(hMenu,IDM_BOOKMARKS_BOOKMARKTHISTAB,SHELLIMAGES_ADDFAV);
	SetMenuItemBitmap(hMenu,IDM_BOOKMARKS_MANAGEBOOKMARKS,SHELLIMAGES_FAV);

	SetMenuItemBitmap(hMenu,IDM_TOOLS_SEARCH,SHELLIMAGES_SEARCH);
	SetMenuItemBitmap(hMenu,IDM_TOOLS_CUSTOMIZECOLORS,SHELLIMAGES_CUSTOMIZECOLORS);
	SetMenuItemBitmap(hMenu,IDM_TOOLS_OPTIONS,SHELLIMAGES_OPTIONS);

	SetMenuItemBitmap(hMenu,IDM_HELP_HELP,SHELLIMAGES_HELP);

	SetMenuOwnerDraw(m_hTabRightClickMenu);

	/* <---- Tab right click menu ----> */
	SetMenuItemBitmap(m_hTabRightClickMenu,IDM_FILE_NEWTAB,SHELLIMAGES_NEWTAB);
	SetMenuItemBitmap(m_hTabRightClickMenu,IDM_TAB_REFRESH,SHELLIMAGES_REFRESH);

	/* <---- Toolbar right click menu ----> */
	SetMenuOwnerDraw(m_hToolbarRightClickMenu);

	/* <---- Toolbar views menu ----> */
	SetMenuOwnerDraw(m_hViewsMenu);

	/* CCustomMenu will handle the drawing of all owner drawn menus. */
	m_pCustomMenu = new CCustomMenu(m_hContainer,hMenu,himlMenu);

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

	DeleteObject(hBitmap);

	/* Arrange submenu. */
	SetMenuOwnerDraw(m_hArrangeSubMenu);

	/* Group by submenu. */
	SetMenuOwnerDraw(m_hGroupBySubMenu);
}

void Explorerplusplus::SetDefaultTabSettings(TabInfo_t *pTabInfo)
{
	pTabInfo->bLocked			= FALSE;
	pTabInfo->bAddressLocked	= FALSE;
	pTabInfo->bUseCustomName	= FALSE;
	StringCchCopy(pTabInfo->szName,
		SIZEOF_ARRAY(pTabInfo->szName),EMPTY_STRING);
}