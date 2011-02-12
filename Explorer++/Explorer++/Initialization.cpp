/******************************************************************
 *
 * Project: Explorer++
 * File: Initialization.cpp
 * License: GPL - See COPYING in the top level directory
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
#include <shobjidl.h>
#include "Explorer++.h"
#include "CustomizeColorsDialog.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/Bookmark.h"
#include "MainResource.h"


extern HIMAGELIST himlMenu;

void Explorerplusplus::InitializeBookmarks(void)
{
	HMENU	hMenu;
	HMENU	hBookmarkMenu;

	hMenu = GetMenu(m_hContainer);
	hBookmarkMenu = GetSubMenu(hMenu,6);

	InitializeBookmarkToolbarMap();

	InsertBookmarksIntoMenu();
	InsertBookmarkToolbarButtons();
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

	m_hDisplayWindow = CreateDisplayWindow(m_hContainer,
		&m_pDisplayMain,&InitialSettings);
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
	list<ViewMode_t>::iterator itr;
	TCHAR szText[64];

	for(itr = m_ViewModes.begin();itr != m_ViewModes.end();itr++)
	{
		LoadString(g_hLanguageModule,GetViewModeMenuStringId(itr->uViewMode),
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
	SetMenuItemBitmap(hMenu,IDM_FILE_SHOWCOMMANDPROMPT,SHELLIMAGES_CMD);
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
	SetMenuItemBitmap(hMenu,IDM_BOOKMARKS_ORGANIZEBOOKMARKS,SHELLIMAGES_FAV);

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

	/* <--- Bookmarks right click menu ----> */
	SetMenuOwnerDraw(m_hBookmarksRightClickMenu);
	SetMenuItemBitmap(m_hBookmarksRightClickMenu,IDM_BT_DELETE,SHELLIMAGES_DELETE);
	SetMenuItemBitmap(m_hBookmarksRightClickMenu,IDM_BT_PROPERTIES,SHELLIMAGES_PROPERTIES);

	/* <---- Application toolbar right click menu ----> */
	SetMenuOwnerDraw(m_hApplicationRightClickMenu);
	SetMenuItemBitmap(m_hApplicationRightClickMenu,IDM_APP_DELETE,SHELLIMAGES_DELETE);
	SetMenuItemBitmap(m_hApplicationRightClickMenu,IDM_APP_PROPERTIES,SHELLIMAGES_PROPERTIES);

	/* <---- Display window right click menu ----> */
	SetMenuOwnerDraw(m_hDisplayWindowRightClickMenu);

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

void Explorerplusplus::InitializeColorRules(void)
{
	ColorRule_t ColorRule;

	/* TODO: Move text into string table. */
	ColorRule.strDescription		= _T("Compressed files");
	ColorRule.rgbColour				= CF_COMPRESSED;
	ColorRule.dwFilterAttributes	= FILE_ATTRIBUTE_COMPRESSED;
	m_ColorRuleList.push_back(ColorRule);

	/* TODO: Move text into string table. */
	ColorRule.strDescription		= _T("Encrypted files");
	ColorRule.rgbColour				= CF_ENCRYPTED;
	ColorRule.dwFilterAttributes	= FILE_ATTRIBUTE_ENCRYPTED;
	m_ColorRuleList.push_back(ColorRule);
}