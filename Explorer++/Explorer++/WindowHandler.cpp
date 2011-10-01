/******************************************************************
 *
 * Project: Explorer++
 * File: WindowHandler.cpp
 * License: GPL - See COPYING in the top level directory
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
#include "MainResource.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/Controls.h"
#include "../Helper/Macros.h"


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
			LoadString(g_hLanguageModule,IDS_ADDRESSBAR,szBandText,SIZEOF_ARRAY(szBandText));
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
			ToolbarSize = (DWORD)SendMessage(m_hDrivesToolbar,TB_GETBUTTONSIZE,0,0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_hDrivesToolbar,TB_GETMAXSIZE,0,(LPARAM)&sz);

			if(m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_hDrivesToolbar;
			break;

		case ID_APPLICATIONSTOOLBAR:
			CreateApplicationToolbar();
			ToolbarSize = (DWORD)SendMessage(m_hApplicationToolbar,TB_GETBUTTONSIZE,0,0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_hApplicationToolbar,TB_GETMAXSIZE,0,(LPARAM)&sz);

			if(m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_hApplicationToolbar;
			break;
		}

		m_ToolbarInformation[i].cbSize = sizeof(REBARBANDINFO);
		SendMessage(m_hMainRebar,RB_INSERTBAND,(WPARAM)-1,(LPARAM)&m_ToolbarInformation[i]);
	}
}

void Explorerplusplus::CreateMainToolbar(void)
{
	m_hMainToolbar = CreateToolbar(m_hMainRebar,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|
		TBSTYLE_TOOLTIPS|TBSTYLE_LIST|TBSTYLE_TRANSPARENT|TBSTYLE_FLAT|CCS_NODIVIDER|
		CCS_NORESIZE|CCS_ADJUSTABLE,TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS|
		TBSTYLE_EX_DOUBLEBUFFER|TBSTYLE_EX_HIDECLIPPEDBUTTONS);

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

	SendMessage(m_hMainToolbar,TB_SETBITMAPSIZE,0,MAKELONG(cx,cy));	
	SendMessage(m_hMainToolbar,TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON),0);

	/* Add the custom buttons to the toolbars image list. */
	SendMessage(m_hMainToolbar,TB_SETIMAGELIST,0,(LPARAM)*phiml);

	AddStringsToMainToolbar();
	InsertToolbarButtons();

	if(!m_bLoadSettingsFromXML)
	{
		if(m_bAttemptToolbarRestore)
		{
			TBSAVEPARAMS	tbSave;

			tbSave.hkr			= HKEY_CURRENT_USER;
			tbSave.pszSubKey	= REG_SETTINGS_KEY;
			tbSave.pszValueName	= _T("ToolbarState");

			SendMessage(m_hMainToolbar,TB_SAVERESTORE,FALSE,(LPARAM)&tbSave);
		}
	}
}

void Explorerplusplus::CreateBookmarksToolbar(void)
{
	m_hBookmarksToolbar = CreateToolbar(m_hMainRebar,BookmarkToolbarStyles,
		TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS|
		TBSTYLE_EX_DOUBLEBUFFER|TBSTYLE_EX_HIDECLIPPEDBUTTONS);

	m_pBookmarksToolbar = new CBookmarksToolbar(m_hBookmarksToolbar,
		*m_bfAllBookmarks,m_guidBookmarksToolbar,TOOLBAR_BOOKMARK_START,TOOLBAR_BOOKMARK_END);
}

void Explorerplusplus::CreateDrivesToolbar(void)
{
	m_hDrivesToolbar = CreateToolbar(m_hMainRebar,BookmarkToolbarStyles,
		TBSTYLE_EX_DOUBLEBUFFER|TBSTYLE_EX_HIDECLIPPEDBUTTONS);

	m_pDrivesToolbar = new CDrivesToolbar(m_hDrivesToolbar,
		TOOLBAR_DRIVES_ID_START,TOOLBAR_DRIVES_ID_END,this);
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
						else if(pnmh->hwndFrom == m_hDrivesToolbar)
						{
							/* The drives toolbar will handle right-clicking of
							its items, so if the NM_RCLICK message is received
							here, it means the toolbar background was clicked. */
							OnMainToolbarRClick();
						}
						else if(pnmh->hwndFrom == m_hApplicationToolbar)
						{
							if(pnmm->dwItemSpec != -1)
							{
								int iItem;

								iItem = (int)SendMessage(m_hApplicationToolbar,
									TB_COMMANDTOINDEX,pnmm->dwItemSpec,0);

								m_iSelectedRClick = iItem;

								POINT ptCursor;
								DWORD dwPos;

								SetFocus(m_hApplicationToolbar);
								dwPos = GetMessagePos();
								ptCursor.x = GET_X_LPARAM(dwPos);
								ptCursor.y = GET_Y_LPARAM(dwPos);

								TrackPopupMenu(m_hApplicationRightClickMenu,TPM_LEFTALIGN,
									ptCursor.x,ptCursor.y,0,m_hMainRebar,NULL);
							}
							else
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

void Explorerplusplus::InsertToolbarButtons(void)
{
	std::list<ToolbarButton_t>::iterator	itr;
	TBBUTTON						*ptbButton = NULL;
	BYTE							StandardStyle;
	unsigned int					iCurrent = 0;

	/* Standard style that all toolbar buttons will have. */
	StandardStyle = BTNS_BUTTON | BTNS_AUTOSIZE;

	ptbButton = (TBBUTTON *)malloc(m_tbInitial.size() * sizeof(TBBUTTON));

	/* Fill out the button information... */
	for(itr = m_tbInitial.begin();itr != m_tbInitial.end();itr++)
	{
		if(iCurrent < m_tbInitial.size())
		{
			if(itr->iItemID == TOOLBAR_SEPARATOR)
			{
				ptbButton[iCurrent].iBitmap		= 0;
				ptbButton[iCurrent].idCommand	= 0;
				ptbButton[iCurrent].fsState		= TBSTATE_ENABLED;
				ptbButton[iCurrent].fsStyle		= BTNS_SEP;
				ptbButton[iCurrent].dwData		= 0;
				ptbButton[iCurrent].iString		= 0;
			}
			else
			{
				ptbButton[iCurrent].iBitmap		= LookupToolbarButtonImage(itr->iItemID);
				ptbButton[iCurrent].idCommand	= itr->iItemID;
				ptbButton[iCurrent].fsState		= TBSTATE_ENABLED;
				ptbButton[iCurrent].fsStyle		= StandardStyle | LookupToolbarButtonExtraStyles(itr->iItemID);
				ptbButton[iCurrent].dwData		= 0;
				ptbButton[iCurrent].iString		= (INT_PTR)itr->iItemID - TOOLBAR_BACK;
			}

			iCurrent++;
		}
	}

	/* Add the buttons to the toolbar. */
	SendMessage(m_hMainToolbar,TB_ADDBUTTONS,(WPARAM)iCurrent,(LPARAM)ptbButton);

	free(ptbButton);
}

BOOL Explorerplusplus::OnTBQueryInsert(LPARAM lParam)
{
	return TRUE;
}

BOOL Explorerplusplus::OnTBQueryDelete(LPARAM lParam)
{
	/* All buttons can be deleted. */
	return TRUE;
}

BOOL Explorerplusplus::OnTBGetButtonInfo(LPARAM lParam)
{
	NMTOOLBAR		*pnmtb = NULL;
	static TCHAR	szText[64];
	int				id;

	pnmtb = (NMTOOLBAR *)lParam;

	/* The cast below is to fix C4018 (signed/unsigned mismatch). */
	if((pnmtb->iItem >= 0) && ((unsigned int)pnmtb->iItem < sizeof(ToolbarButtonSet) / sizeof(ToolbarButtonSet[0])))
	{
		id = ToolbarButtonSet[pnmtb->iItem];

		pnmtb->tbButton.fsState		= TBSTATE_ENABLED;
		pnmtb->tbButton.fsStyle		= BTNS_BUTTON | BTNS_AUTOSIZE | LookupToolbarButtonExtraStyles(id);
		pnmtb->tbButton.idCommand	= id;
		pnmtb->tbButton.iBitmap		= LookupToolbarButtonImage(id);
		pnmtb->tbButton.iString		= (INT_PTR)id - TOOLBAR_BACK;
		pnmtb->tbButton.dwData		= 0;

		StringCchCopy(pnmtb->pszText,pnmtb->cchText,szText);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL Explorerplusplus::OnTBRestore(LPARAM lParam)
{
	return 0;
}

void Explorerplusplus::OnTBReset(void)
{
	int nButtons;
	int i = 0;

	nButtons = (int)SendMessage(m_hMainToolbar,TB_BUTTONCOUNT,0,0);

	for(i = nButtons - 1;i >= 0;i--)
		SendMessage(m_hMainToolbar,TB_DELETEBUTTON,i,0);

	InsertToolbarButtons();
	HandleToolbarItemStates();
}

void Explorerplusplus::OnTBGetInfoTip(LPARAM lParam)
{
	NMTBGETINFOTIP	*ptbgit = NULL;
	LPITEMIDLIST	pidl = NULL;
	TCHAR			szInfoTip[1024];
	TCHAR			szPath[MAX_PATH];

	ptbgit = (NMTBGETINFOTIP *)lParam;

	StringCchCopy(ptbgit->pszText,ptbgit->cchTextMax,EMPTY_STRING);

	/* TODO: String table. */
	if(ptbgit->iItem == TOOLBAR_BACK)
	{
		if(m_pActiveShellBrowser->IsBackHistory())
		{
			pidl = m_pActiveShellBrowser->RetrieveHistoryItemWithoutUpdate(-1);

			GetDisplayName(pidl,szPath,SHGDN_INFOLDER);
			StringCchPrintf(szInfoTip,SIZEOF_ARRAY(szInfoTip),
				_T("Back to %s"),szPath);

			StringCchCopy(ptbgit->pszText,ptbgit->cchTextMax,szInfoTip);
		}
	}
	else if(ptbgit->iItem == TOOLBAR_FORWARD)
	{
		if(m_pActiveShellBrowser->IsForwardHistory())
		{
			pidl = m_pActiveShellBrowser->RetrieveHistoryItemWithoutUpdate(1);

			GetDisplayName(pidl,szPath,SHGDN_INFOLDER);
			StringCchPrintf(szInfoTip,SIZEOF_ARRAY(szInfoTip),
				_T("Forward to %s"),szPath);

			StringCchCopy(ptbgit->pszText,ptbgit->cchTextMax,szInfoTip);
		}
	}
	else if(ptbgit->iItem >= TOOLBAR_APPLICATIONS_ID_START)
	{
		TBBUTTON			tbButton;
		ApplicationButton_t *pab = NULL;
		LRESULT				lResult;
		int					iIndex;

		iIndex = (int)SendMessage(m_hApplicationToolbar,
			TB_COMMANDTOINDEX,ptbgit->iItem,0);

		if(iIndex != -1)
		{
			lResult = SendMessage(m_hApplicationToolbar,
				TB_GETBUTTON,iIndex,(LPARAM)&tbButton);

			if(lResult)
			{
				pab = (ApplicationButton_t *)tbButton.dwData;

				StringCchPrintf(ptbgit->pszText,ptbgit->cchTextMax,_T("%s\n%s"),
					pab->szName,pab->szCommand);
			}
		}
	}
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
	std::list<LPITEMIDLIST> lHistory;
	std::list<LPITEMIDLIST>::iterator itr;
	MENUITEMINFO mii;
	TCHAR szDisplayName[MAX_PATH];
	int iBase;
	int i = 0;

	if(bBack)
	{
		m_pActiveShellBrowser->GetBackHistory(&lHistory);

		iBase = ID_REBAR_MENU_BACK_START;
	}
	else
	{
		m_pActiveShellBrowser->GetForwardHistory(&lHistory);

		iBase = ID_REBAR_MENU_FORWARD_START;
	}

	if(lHistory.size() > 0)
	{
		hSubMenu = CreateMenu();

		for(itr = lHistory.begin();itr != lHistory.end();itr++)
		{
			GetDisplayName(*itr,szDisplayName,SHGDN_INFOLDER);

			mii.cbSize		= sizeof(mii);
			mii.fMask		= MIIM_ID|MIIM_STRING;
			mii.wID			= iBase + i + 1;
			mii.dwTypeData	= szDisplayName;
			InsertMenuItem(hSubMenu,i,TRUE,&mii);

			i++;

			CoTaskMemFree(*itr);
		}

		lHistory.clear();

		SetMenuOwnerDraw(hSubMenu);
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