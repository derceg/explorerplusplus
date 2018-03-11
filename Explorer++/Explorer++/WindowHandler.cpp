/******************************************************************
 *
 * Project: Explorer++
 * File: WindowHandler.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Manages the creation of windows as well
 * as housing various window procedures.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"
#include "BookmarksToolbar.h"
#include "DrivesToolbar.h"
#include "ApplicationToolbar.h"
#include "MainResource.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/Controls.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/Macros.h"


static const int TOOLBAR_BOOKMARK_START = TOOLBAR_ID_START + 1000;
static const int TOOLBAR_BOOKMARK_END = TOOLBAR_BOOKMARK_START + 1000;
static const int TOOLBAR_DRIVES_ID_START = TOOLBAR_BOOKMARK_END + 1;
static const int TOOLBAR_DRIVES_ID_END = TOOLBAR_DRIVES_ID_START + 1000;
static const int TOOLBAR_APPLICATIONS_ID_START = TOOLBAR_DRIVES_ID_END + 1;
static const int TOOLBAR_APPLICATIONS_ID_END = TOOLBAR_APPLICATIONS_ID_START + 1000;

DWORD BookmarkToolbarStyles	=	WS_CHILD |WS_VISIBLE |WS_CLIPSIBLINGS |WS_CLIPCHILDREN |
								TBSTYLE_TOOLTIPS | TBSTYLE_LIST | TBSTYLE_TRANSPARENT |
								TBSTYLE_FLAT | CCS_NODIVIDER| CCS_NORESIZE;

DWORD RebarStyles			=	WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|
								WS_BORDER|CCS_NODIVIDER|CCS_TOP|CCS_NOPARENTALIGN|
								RBS_BANDBORDERS|RBS_VARHEIGHT;

LRESULT CALLBACK RebarSubclassStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

void Explorerplusplus::CreateMainControls(void)
{
	SIZE	sz;
	RECT	rc;
	DWORD	ToolbarSize;
	TCHAR	szBandText[32];
	int		i = 0;

	/* If the rebar is locked, prevent bands from
	been rearranged. */
	if(m_bLockToolbars)
		RebarStyles |= RBS_FIXEDORDER;

	/* Create and subclass the main rebar control. */
	m_hMainRebar = CreateWindowEx(0,REBARCLASSNAME,EMPTY_STRING,RebarStyles,
		0,0,0,0,m_hContainer,NULL,GetModuleHandle(0),NULL);
	SetWindowSubclass(m_hMainRebar,RebarSubclassStub,0,(DWORD_PTR)this);

	for(i = 0;i < NUM_MAIN_TOOLBARS;i++)
	{
		switch(m_ToolbarInformation[i].wID)
		{
		case ID_MAINTOOLBAR:
			CreateMainToolbar();
			ToolbarSize = (DWORD)SendMessage(m_hMainToolbar,TB_GETBUTTONSIZE,0,0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_hMainToolbar,TB_GETMAXSIZE,0,(LPARAM)&sz);

			if(m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_hMainToolbar;
			break;

		case ID_ADDRESSTOOLBAR:
			CreateAddressBar();
			LoadString(m_hLanguageModule,IDS_ADDRESSBAR,szBandText,SIZEOF_ARRAY(szBandText));
			GetWindowRect(m_hAddressBar,&rc);
			m_ToolbarInformation[i].cyMinChild = GetRectHeight(&rc);
			m_ToolbarInformation[i].lpText = szBandText;
			m_ToolbarInformation[i].hwndChild = m_hAddressBar;
			break;

		case ID_BOOKMARKSTOOLBAR:
			CreateBookmarksToolbar();
			ToolbarSize = (DWORD)SendMessage(m_hBookmarksToolbar,TB_GETBUTTONSIZE,0,0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_hBookmarksToolbar,TB_GETMAXSIZE,0,(LPARAM)&sz);

			if(m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_hBookmarksToolbar;
			break;

		case ID_DRIVESTOOLBAR:
			CreateDrivesToolbar();
			ToolbarSize = (DWORD)SendMessage(m_pDrivesToolbar->GetHWND(),TB_GETBUTTONSIZE,0,0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_pDrivesToolbar->GetHWND(),TB_GETMAXSIZE,0,(LPARAM)&sz);

			if(m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_pDrivesToolbar->GetHWND();
			break;

		case ID_APPLICATIONSTOOLBAR:
			CreateApplicationToolbar();
			ToolbarSize = (DWORD)SendMessage(m_pApplicationToolbar->GetHWND(),TB_GETBUTTONSIZE,0,0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_pApplicationToolbar->GetHWND(),TB_GETMAXSIZE,0,(LPARAM)&sz);

			if(m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_pApplicationToolbar->GetHWND();
			break;
		}

		m_ToolbarInformation[i].cbSize = sizeof(REBARBANDINFO);
		SendMessage(m_hMainRebar,RB_INSERTBAND,(WPARAM)-1,(LPARAM)&m_ToolbarInformation[i]);
	}
}

void Explorerplusplus::CreateBookmarksToolbar(void)
{
	m_hBookmarksToolbar = CreateToolbar(m_hMainRebar,BookmarkToolbarStyles,
		TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS|
		TBSTYLE_EX_DOUBLEBUFFER|TBSTYLE_EX_HIDECLIPPEDBUTTONS);

	m_pBookmarksToolbar = new CBookmarksToolbar(m_hBookmarksToolbar, this,
		*m_bfAllBookmarks,m_guidBookmarksToolbar,TOOLBAR_BOOKMARK_START,TOOLBAR_BOOKMARK_END);
}

void Explorerplusplus::CreateDrivesToolbar(void)
{
	 m_pDrivesToolbar = CDrivesToolbar::Create(m_hMainRebar, TOOLBAR_DRIVES_ID_START,
		TOOLBAR_DRIVES_ID_END, m_hLanguageModule, this);
}

void Explorerplusplus::CreateApplicationToolbar()
{
	 m_pApplicationToolbar = CApplicationToolbar::Create(m_hMainRebar, TOOLBAR_APPLICATIONS_ID_START,
		TOOLBAR_APPLICATIONS_ID_END, m_hLanguageModule, this);
}

void Explorerplusplus::CreateStatusBar(void)
{
	UINT Style = WS_CHILD|WS_CLIPSIBLINGS|SBARS_SIZEGRIP|WS_CLIPCHILDREN;

	if(m_bShowStatusBar)
	{
		Style |= WS_VISIBLE;
	}

	m_hStatusBar = ::CreateStatusBar(m_hContainer,Style);
	m_pStatusBar = new CStatusBar(m_hStatusBar);
}

HWND Explorerplusplus::CreateTabToolbar(HWND hParent,int idCommand,TCHAR *szTip)
{
	HWND TabToolbar = CreateToolbar(hParent,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|
		TBSTYLE_TOOLTIPS|TBSTYLE_LIST|TBSTYLE_TRANSPARENT|TBSTYLE_FLAT|CCS_NODIVIDER|
		CCS_NOPARENTALIGN|CCS_NORESIZE,TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DOUBLEBUFFER);
	
	SendMessage(TabToolbar,TB_SETBITMAPSIZE,0,MAKELONG(7,7));
	SendMessage(TabToolbar,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);
	SendMessage(TabToolbar,TB_SETBUTTONSIZE,0,MAKELPARAM(16,16));

	/* TODO: The image list is been leaked. */
	HIMAGELIST himl = ImageList_Create(7,7,ILC_COLOR32|ILC_MASK,0,2);
	HBITMAP hb = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_TABTOOLBAR_CLOSE));
	int iIndex = ImageList_Add(himl,hb,NULL);
	SendMessage(TabToolbar,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(himl));
	DeleteObject(hb);

	/* Add the close button, used to close tabs. */
	TBBUTTON tbButton;
	tbButton.iBitmap	= iIndex;
	tbButton.idCommand	= idCommand;
	tbButton.fsState	= TBSTATE_ENABLED;
	tbButton.fsStyle	= TBSTYLE_BUTTON|TBSTYLE_AUTOSIZE;
	tbButton.dwData		= 0;
	tbButton.iString	= reinterpret_cast<INT_PTR>(szTip);
	SendMessage(TabToolbar,TB_INSERTBUTTON,0,reinterpret_cast<LPARAM>(&tbButton));

	SendMessage(TabToolbar,TB_AUTOSIZE,0,0);

	return TabToolbar;
}

LRESULT CALLBACK RebarSubclassStub(HWND hwnd,UINT uMsg,
WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	Explorerplusplus *pContainer = (Explorerplusplus *)dwRefData;

	return pContainer->RebarSubclass(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::RebarSubclass(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_INITMENU:
			SendMessage(m_hContainer,WM_INITMENU,wParam,lParam);
			break;

		case WM_MENUSELECT:
			SendMessage(m_hContainer,WM_MENUSELECT,wParam,lParam);
			break;
	
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->code)
			{
				case NM_RCLICK:
					{
						LPNMMOUSE	pnmm;
						LPNMHDR		pnmh;

						pnmm = (LPNMMOUSE)lParam;
						pnmh = &pnmm->hdr;

						if(pnmh->hwndFrom == m_hBookmarksToolbar)
						{
							/* TODO: [Bookmarks] Show bookmarks menu. */
						}
						else if(pnmh->hwndFrom == m_pApplicationToolbar->GetHWND())
						{
							if(pnmm->dwItemSpec == -1)
							{
								OnApplicationToolbarRClick();
							}
						}
						else
						{
							OnMainToolbarRClick();
						}
					}
					return TRUE;
					break;
			}
			break;
	}

	return DefSubclassProc(hwnd,msg,wParam,lParam);
}

void Explorerplusplus::OnApplicationToolbarRClick()
{
	MENUITEMINFO mii;

	TCHAR szTemp[64];
	LoadString(m_hLanguageModule,IDS_APPLICATIONBUTTON_NEW,
		szTemp,SIZEOF_ARRAY(szTemp));

	mii.cbSize		= sizeof(mii);
	mii.fMask		= MIIM_ID|MIIM_STRING;
	mii.dwTypeData	= szTemp;
	mii.wID			= IDM_APP_NEW;

	/* Add the item to the menu. */
	InsertMenuItem(m_hToolbarRightClickMenu,7,TRUE,&mii);

	OnMainToolbarRClick();

	/* Now, remove the item from the menu. */
	DeleteMenu(m_hToolbarRightClickMenu,7,MF_BYPOSITION);
}

void Explorerplusplus::SetStatusBarParts(int width)
{
	int Parts[3];

	Parts[0] = (int)(0.50 * width);
	Parts[1] = (int)(0.75 * width);
	Parts[2] = width;

	SendMessage(m_hStatusBar,SB_SETPARTS,3,(LPARAM)Parts);
}

void Explorerplusplus::ResizeWindows(void)
{
	RECT rc;

	GetClientRect(m_hContainer,&rc);
	SendMessage(m_hContainer,WM_SIZE,
	SIZE_RESTORED,(LPARAM)MAKELPARAM(rc.right,rc.bottom));
}

/* TODO: This should be linked to OnSize(). */
void Explorerplusplus::SetListViewInitialPosition(HWND hListView)
{
	RECT			rc;
	int				MainWindowWidth;
	int				MainWindowHeight;
	int				IndentBottom = 0;
	int				IndentTop = 0;
	int				IndentLeft = 0;
	int				iIndentRebar = 0;

	GetClientRect(m_hContainer,&rc);

	MainWindowWidth = GetRectWidth(&rc);
	MainWindowHeight = GetRectHeight(&rc);

	if(m_hMainRebar)
	{
		GetWindowRect(m_hMainRebar,&rc);
		iIndentRebar += GetRectHeight(&rc);
	}

	if(m_bShowStatusBar)
	{
		GetWindowRect(m_hStatusBar,&rc);
		IndentBottom += GetRectHeight(&rc);
	}

	if(m_bShowDisplayWindow)
	{
		IndentBottom += m_DisplayWindowHeight;
	}

	if(m_bShowFolders)
	{
		GetClientRect(m_hHolder,&rc);
		IndentLeft = GetRectWidth(&rc);
	}

	IndentTop = iIndentRebar;

	if(m_bShowTabBar)
	{
		if(!m_bShowTabBarAtBottom)
		{
			IndentTop += TAB_WINDOW_HEIGHT;
		}
	}

	if(!m_bShowTabBarAtBottom)
	{
		SetWindowPos(hListView,NULL,IndentLeft,IndentTop,
			MainWindowWidth - IndentLeft,MainWindowHeight -
			IndentBottom - IndentTop,
			SWP_HIDEWINDOW|SWP_NOZORDER);
	}
	else
	{
		SetWindowPos(hListView,NULL,IndentLeft,IndentTop,
			MainWindowWidth - IndentLeft,MainWindowHeight -
			IndentBottom - IndentTop - TAB_WINDOW_HEIGHT,
			SWP_HIDEWINDOW|SWP_NOZORDER);
	}
}

void Explorerplusplus::ToggleFolders(void)
{
	m_bShowFolders = !m_bShowFolders;
	lShowWindow(m_hHolder,m_bShowFolders);
	lShowWindow(m_hTreeView,m_bShowFolders);

	SendMessage(m_hMainToolbar,TB_CHECKBUTTON,(WPARAM)TOOLBAR_FOLDERS,(LPARAM)m_bShowFolders);
	ResizeWindows();
}

void Explorerplusplus::AdjustMainToolbarSize(void)
{
	HIMAGELIST *phiml = NULL;
	int cx;
	int cy;

	if(m_bLargeToolbarIcons)
	{
		cx = TOOLBAR_IMAGE_SIZE_LARGE_X;
		cy = TOOLBAR_IMAGE_SIZE_LARGE_Y;
		phiml = &m_himlToolbarLarge;
	}
	else
	{
		cx = TOOLBAR_IMAGE_SIZE_SMALL_X;
		cy = TOOLBAR_IMAGE_SIZE_SMALL_Y;
		phiml = &m_himlToolbarSmall;
	}

	/* Switch the image list. */
	SendMessage(m_hMainToolbar,TB_SETIMAGELIST,0,(LPARAM)*phiml);
	SendMessage(m_hMainToolbar,TB_SETBUTTONSIZE,0,MAKELPARAM(cx,cy));
	SendMessage(m_hMainToolbar,TB_AUTOSIZE,0,0);

	REBARBANDINFO rbi;
	DWORD dwSize;

	dwSize = (DWORD)SendMessage(m_hMainToolbar,TB_GETBUTTONSIZE,0,0);

	rbi.cbSize		= sizeof(rbi);
	rbi.fMask		= RBBIM_CHILDSIZE;
	rbi.cxMinChild	= 0;
	rbi.cyMinChild	= HIWORD(dwSize);
	rbi.cyChild		= HIWORD(dwSize);
	rbi.cyMaxChild	= HIWORD(dwSize);

	SendMessage(m_hMainRebar,RB_SETBANDINFO,0,(LPARAM)&rbi);
}

HMENU Explorerplusplus::CreateRebarHistoryMenu(BOOL bBack)
{
	HMENU hSubMenu = NULL;
	std::list<LPITEMIDLIST> history;
	int iBase;

	if(bBack)
	{
		iBase = ID_REBAR_MENU_BACK_START;
		history = m_pActiveShellBrowser->GetBackHistory();
	}
	else
	{
		iBase = ID_REBAR_MENU_FORWARD_START;
		history = m_pActiveShellBrowser->GetForwardHistory();
	}

	if(history.size() > 0)
	{
		int i = 0;

		hSubMenu = CreateMenu();

		for(auto itr = history.begin();itr != history.end();itr++)
		{
			TCHAR szDisplayName[MAX_PATH];
			GetDisplayName(*itr,szDisplayName,SIZEOF_ARRAY(szDisplayName),SHGDN_INFOLDER);

			MENUITEMINFO mii;
			mii.cbSize		= sizeof(mii);
			mii.fMask		= MIIM_ID|MIIM_STRING;
			mii.wID			= iBase + i + 1;
			mii.dwTypeData	= szDisplayName;
			InsertMenuItem(hSubMenu,i,TRUE,&mii);

			i++;

			CoTaskMemFree(*itr);
		}

		history.clear();
	}

	return hSubMenu;
}

void Explorerplusplus::InitializeMainToolbars(void)
{
	/* Initialize the main toolbar styles and settings here. The visibility and gripper
	styles will be set after the settings have been loaded (needed to keep compatibility
	with versions older than 0.9.5.4). */
	m_ToolbarInformation[0].wID			= ID_MAINTOOLBAR;
	m_ToolbarInformation[0].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_IDEALSIZE|RBBIM_STYLE;
	m_ToolbarInformation[0].fStyle		= RBBS_BREAK|RBBS_USECHEVRON;
	m_ToolbarInformation[0].cx			= 0;
	m_ToolbarInformation[0].cxIdeal		= 0;
	m_ToolbarInformation[0].cxMinChild	= 0;
	m_ToolbarInformation[0].cyIntegral	= 0;
	m_ToolbarInformation[0].cxHeader	= 0;
	m_ToolbarInformation[0].lpText		= NULL;

	m_ToolbarInformation[1].wID			= ID_ADDRESSTOOLBAR;
	m_ToolbarInformation[1].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_STYLE|RBBIM_TEXT;
	m_ToolbarInformation[1].fStyle		= RBBS_BREAK;
	m_ToolbarInformation[1].cx			= 0;
	m_ToolbarInformation[1].cxIdeal		= 0;
	m_ToolbarInformation[1].cxMinChild	= 0;
	m_ToolbarInformation[1].cyIntegral	= 0;
	m_ToolbarInformation[1].cxHeader	= 0;
	m_ToolbarInformation[1].lpText		= NULL;

	m_ToolbarInformation[2].wID			= ID_BOOKMARKSTOOLBAR;
	m_ToolbarInformation[2].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_IDEALSIZE|RBBIM_STYLE;
	m_ToolbarInformation[2].fStyle		= RBBS_BREAK|RBBS_USECHEVRON;
	m_ToolbarInformation[2].cx			= 0;
	m_ToolbarInformation[2].cxIdeal		= 0;
	m_ToolbarInformation[2].cxMinChild	= 0;
	m_ToolbarInformation[2].cyIntegral	= 0;
	m_ToolbarInformation[2].cxHeader	= 0;
	m_ToolbarInformation[2].lpText		= NULL;

	m_ToolbarInformation[3].wID			= ID_DRIVESTOOLBAR;
	m_ToolbarInformation[3].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_IDEALSIZE|RBBIM_STYLE;
	m_ToolbarInformation[3].fStyle		= RBBS_BREAK|RBBS_USECHEVRON;
	m_ToolbarInformation[3].cx			= 0;
	m_ToolbarInformation[3].cxIdeal		= 0;
	m_ToolbarInformation[3].cxMinChild	= 0;
	m_ToolbarInformation[3].cyIntegral	= 0;
	m_ToolbarInformation[3].cxHeader	= 0;
	m_ToolbarInformation[3].lpText		= NULL;

	m_ToolbarInformation[4].wID			= ID_APPLICATIONSTOOLBAR;
	m_ToolbarInformation[4].fMask		= RBBIM_ID|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE|RBBIM_IDEALSIZE|RBBIM_STYLE;
	m_ToolbarInformation[4].fStyle		= RBBS_BREAK|RBBS_USECHEVRON;
	m_ToolbarInformation[4].cx			= 0;
	m_ToolbarInformation[4].cxIdeal		= 0;
	m_ToolbarInformation[4].cxMinChild	= 0;
	m_ToolbarInformation[4].cyIntegral	= 0;
	m_ToolbarInformation[4].cxHeader	= 0;
	m_ToolbarInformation[4].lpText		= NULL;
}