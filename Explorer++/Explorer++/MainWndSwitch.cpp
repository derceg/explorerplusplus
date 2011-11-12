/******************************************************************
 *
 * Project: Explorer++
 * File: Settings.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Processes and forwards incoming messages for the main
 * window.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "SearchDialog.h"
#include "AboutDialog.h"
#include "FilterDialog.h"
#include "CustomizeColorsDialog.h"
#include "SplitFileDialog.h"
#include "DestroyFilesDialog.h"
#include "MergeFilesDialog.h"
#include "HelpFileMissingDialog.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"


LRESULT CALLBACK WndProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam);

/* TODO: Remove once custom menu image lists
have been corrected. */
extern HIMAGELIST himlMenu;

LRESULT CALLBACK WndProcStub(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	Explorerplusplus *pContainer = (Explorerplusplus *)GetWindowLongPtr(hwnd,GWLP_USERDATA);

	switch(Msg)
	{
		case WM_NCCREATE:
			/* Create a new Explorerplusplus object to assign to this window. */
			pContainer = new Explorerplusplus(hwnd);

			if(!pContainer)
			{
				PostQuitMessage(0);
				return 0;
			}

			/* Store the Explorerplusplus object pointer into the extra window bytes
			for this window. */
			SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)pContainer);
			break;
	}

	/* Jump across to the member window function (will handle all requests). */
	if(pContainer != NULL)
		return pContainer->WindowProcedure(hwnd,Msg,wParam,lParam);
	else
		return DefWindowProc(hwnd,Msg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::WindowProcedure(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	if(Msg == m_uTaskbarButtonCreatedMessage)
	{
		HMODULE hUser32;
		ChangeWindowMessageFilterProc ChangeWindowMessageFilter;

		if((m_dwMajorVersion == WINDOWS_VISTA_SEVEN_MAJORVERSION &&
			m_dwMinorVersion == 0) ||
			m_dwMajorVersion < WINDOWS_VISTA_SEVEN_MAJORVERSION)
		{
			return 0;
		}

		if(!m_bShowTaskbarThumbnails)
		{
			return 0;
		}

		hUser32 = LoadLibrary(_T("user32.dll"));

		if(hUser32 != NULL)
		{
			/* If directly targeting Windows 7, this can be switched
			to static, rather than dynamic linking. */
			ChangeWindowMessageFilter = (ChangeWindowMessageFilterProc)GetProcAddress(hUser32,"ChangeWindowsMessageFilter");

			if(ChangeWindowMessageFilter != NULL)
			{
				ChangeWindowMessageFilter(WM_DWMSENDICONICTHUMBNAIL,MSGFLT_ADD);
				ChangeWindowMessageFilter(WM_DWMSENDICONICLIVEPREVIEWBITMAP,MSGFLT_ADD);
			}

			FreeLibrary(hUser32);
		}

		if(m_pTaskbarList3 != NULL)
		{
			m_pTaskbarList3->Release();
		}

		CoCreateInstance(CLSID_TaskbarList,NULL,CLSCTX_INPROC_SERVER,
			IID_ITaskbarList4,(LPVOID *)&m_pTaskbarList3);

		m_pTaskbarList3->HrInit();

		m_bTaskbarInitialised = TRUE;

		/* Add each of the jump list tasks. */
		SetupJumplistTasks();

		std::list<TabProxyInfo_t>::iterator itr;
		LPITEMIDLIST pidlDirectory = NULL;
		BOOL bActive;

		/* Register each of the tabs. */
		for(itr = m_TabProxyList.begin();itr != m_TabProxyList.end();itr++)
		{
			bActive = (itr->iTabId == m_iObjectIndex);

			RegisterTab(itr->hProxy,EMPTY_STRING,bActive);
			HandleTabText(itr->iTabId);
			SetTabIcon(itr->iTabId);

			CoTaskMemFree(pidlDirectory);
		}

		return 0;
	}
	else
	{
		switch(Msg)
		{
		case WM_CREATE:
			OnWindowCreate();
			break;

		case WM_SETFOCUS:
			OnSetFocus();
			return 0;
			break;

		case CBN_KEYDOWN:
			OnComboBoxKeyDown(wParam);
			break;

		case WM_INITMENU:
			m_pCustomMenu->OnInitMenu(wParam);
			SetProgramMenuItemStates((HMENU)wParam);
			break;

		case WM_MENUSELECT:
			StatusBarMenuSelect(wParam,lParam);
			break;

		case WM_MEASUREITEM:
			return OnMeasureItem(wParam,lParam);
			break;

		case WM_DRAWITEM:
			return OnDrawItem(wParam,lParam);
			break;

		case WM_DEVICECHANGE:
			OnDeviceChange(wParam,lParam);
			break;

		case WM_USER_UPDATEWINDOWS:
			UpdateWindowStates();
			break;

		case WM_USER_FILESADDED:
			/* Runs in the context of the main thread. Either
			occurs after the specified tab index has been
			freed (in which case nothing happens), or before. */
			if(CheckTabIdStatus((int)wParam))
				m_pShellBrowser[wParam]->DirectoryAltered();
			break;

		case WM_USER_RELEASEBROWSER:
			m_pShellBrowser[(int)wParam]->Release();
			m_pShellBrowser[(int)wParam] = NULL;
			m_pFolderView[(int)wParam]->Release();
			m_pFolderView[(int)wParam] = NULL;
			break;

		case WM_USER_TREEVIEW_GAINEDFOCUS:
			m_hLastActiveWindow = m_hTreeView;
			break;

		case WM_USER_TABMCLICK:
			OnTabMClick(wParam,lParam);
			break;

		case WM_USER_DISPLAYWINDOWRESIZED:
			OnDisplayWindowResized(wParam);
			break;

		case WM_USER_STARTEDBROWSING:
			OnStartedBrowsing((int)wParam,(TCHAR *)lParam);
			break;

		case WM_USER_NEWITEMINSERTED:
			OnShellNewItemCreated(lParam);
			break;

		case WM_USER_FOLDEREMPTY:
			{
				if((BOOL)lParam == TRUE)
					NListView::ListView_SetBackgroundImage(m_hListView[(int)wParam],IDB_FOLDEREMPTY);
				else
					NListView::ListView_SetBackgroundImage(m_hListView[(int)wParam],NULL);
			}
			break;

		case WM_USER_FILTERINGAPPLIED:
			{
				if((BOOL)lParam == TRUE)
					NListView::ListView_SetBackgroundImage(m_hListView[(int)wParam],IDB_FILTERINGAPPLIED);
				else
					NListView::ListView_SetBackgroundImage(m_hListView[(int)wParam],NULL);
			}
			break;

		case WM_USER_GETCOLUMNNAMEINDEX:
			return LookupColumnNameStringIndex((int)wParam);
			break;

		case WM_USER_DIRECTORYMODIFIED:
			OnDirectoryModified((int)wParam);
			break;

		case WM_USER_ASSOCCHANGED:
			OnAssocChanged();
			break;

		case WM_USER_HOLDERRESIZED:
			{
				RECT	rc;

				m_TreeViewWidth = (int)lParam + TREEVIEW_DRAG_OFFSET;

				GetClientRect(m_hContainer,&rc);

				SendMessage(m_hContainer,WM_SIZE,SIZE_RESTORED,(LPARAM)MAKELPARAM(rc.right,rc.bottom));
			}
			break;

		case WM_APP_FOLDERSIZECOMPLETED:
			{
				DWFolderSizeCompletion_t *pDWFolderSizeCompletion = NULL;
				TCHAR szFolderSize[32];
				TCHAR szSizeString[64];
				TCHAR szTotalSize[64];
				BOOL bValid = FALSE;

				pDWFolderSizeCompletion = (DWFolderSizeCompletion_t *)wParam;

				std::list<DWFolderSize_t>::iterator itr;

				/* First, make sure we should still display the
				results (we won't if the listview selection has
				changed, or this folder size was calculated for
				a tab other than the current one). */
				for(itr = m_DWFolderSizes.begin();itr != m_DWFolderSizes.end();itr++)
				{
					if(itr->uId == pDWFolderSizeCompletion->uId)
					{
						if(itr->iTabId == m_iObjectIndex)
						{
							bValid = itr->bValid;
						}

						m_DWFolderSizes.erase(itr);

						break;
					}
				}

				if(bValid)
				{
					FormatSizeString(pDWFolderSizeCompletion->liFolderSize,szFolderSize,
						SIZEOF_ARRAY(szFolderSize),m_bForceSize,m_SizeDisplayFormat);

					LoadString(g_hLanguageModule,IDS_GENERAL_TOTALSIZE,
						szTotalSize,SIZEOF_ARRAY(szTotalSize));

					StringCchPrintf(szSizeString,SIZEOF_ARRAY(szSizeString),
						_T("%s: %s"),szTotalSize,szFolderSize);

					/* TODO: The line index should be stored in some other (variable) way. */
					DisplayWindow_SetLine(m_hDisplayWindow,FOLDER_SIZE_LINE_INDEX,szSizeString);
				}

				free(pDWFolderSizeCompletion);
			}
			break;

		case WM_COPYDATA:
			{
				COPYDATASTRUCT *pcds = NULL;
				HRESULT hr;

				pcds = (COPYDATASTRUCT *)lParam;

				if(pcds->lpData != NULL)
				{
					BrowseFolder((TCHAR *)pcds->lpData,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);
				}
				else
				{
					hr = BrowseFolder(m_DefaultTabDirectory,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);

					if(FAILED(hr))
						BrowseFolder(m_DefaultTabDirectoryStatic,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);
				}
			}
			break;

		case WM_NDW_RCLICK:
			OnNdwRClick(wParam,lParam);
			break;

		case WM_NDW_ICONRCLICK:
			OnNdwIconRClick(wParam,lParam);
			break;

		case WM_CHANGECBCHAIN:
			OnChangeCBChain(wParam,lParam);
			break;

		case WM_DRAWCLIPBOARD:
			OnDrawClipboard();
			break;

		case WM_APPCOMMAND:
			OnAppCommand(wParam,lParam);
			break;

		case WM_MENUCOMMAND:
			OnMenuCommand(wParam,lParam);
			break;

		case WM_COMMAND:
			return CommandHandler(hwnd,Msg,wParam,lParam);
			break;

		case WM_NOTIFY:
			return NotifyHandler(hwnd,Msg,wParam,lParam);
			break;

		case WM_SIZE:
			return OnSize(LOWORD(lParam),HIWORD(lParam));
			break;

		case WM_CLOSE:
			return OnClose();
			break;

		case WM_DESTROY:
			return OnDestroy();
			break;
		}
	}

	return DefWindowProc(hwnd,Msg,wParam,lParam);
}

void Explorerplusplus::OnMenuCommand(WPARAM wParam,LPARAM lParam)
{
	UINT	uMenuID;

	uMenuID = GetMenuItemID((HMENU)lParam,(int)wParam);

	if(uMenuID >= MENU_BOOKMARK_STARTID &&
	uMenuID <= MENU_BOOKMARK_ENDID)
	{
		TCHAR szDirectory[MAX_PATH];

		GetBookmarkMenuItemDirectory((HMENU)lParam,uMenuID,
			szDirectory,SIZEOF_ARRAY(szDirectory));

		BrowseFolder(szDirectory,SBSP_ABSOLUTE);
	}
}

/*
 * WM_COMMAND handler for main window.
 */
LRESULT CALLBACK Explorerplusplus::CommandHandler(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	/* Has a bookmark menu item been clicked? */
	if(!HIWORD(wParam) && LOWORD(wParam) >= MENU_BOOKMARK_STARTID &&
	LOWORD(wParam) <= MENU_BOOKMARK_ENDID)
	{
		TCHAR	szDirectory[MAX_PATH];

		GetBookmarkMenuItemDirectory(m_hBookmarksMenu,LOWORD(wParam),
			szDirectory,SIZEOF_ARRAY(szDirectory));

		ExpandAndBrowsePath(szDirectory);
	}
	else if(!HIWORD(wParam) && LOWORD(wParam) >= MENU_HEADER_STARTID &&
	LOWORD(wParam) <= MENU_HEADER_ENDID)
	{
		int iOffset;

		iOffset = LOWORD(wParam) - MENU_HEADER_STARTID;

		std::list<Column_t>			m_pActiveColumnList;
		std::list<Column_t>::iterator	itr;
		int							iItem = 0;
		unsigned int				*pHeaderList = NULL;

		m_pActiveShellBrowser->ExportCurrentColumns(&m_pActiveColumnList);

		GetColumnHeaderMenuList(&pHeaderList);

		/* Loop through all current items to find the item that was clicked, and
		flip its active state. */
		for(itr = m_pActiveColumnList.begin();itr != m_pActiveColumnList.end();itr++)
		{
			if(itr->id == pHeaderList[iOffset])
			{
				itr->bChecked = !itr->bChecked;
				break;
			}

			iItem++;
		}

		/* If it was the first column that was changed, need to refresh
		all columns. */
		if(iOffset == 0)
		{
			m_pActiveShellBrowser->ImportColumns(&m_pActiveColumnList,TRUE);

			RefreshTab(m_iObjectIndex);
		}
		else
		{
			m_pActiveShellBrowser->ImportColumns(&m_pActiveColumnList,FALSE);
		}
	}

	/* Was one of the items on the bookmarks toolbar clicked? */
	if(LOWORD(wParam) >= TOOLBAR_BOOKMARK_START &&
	LOWORD(wParam) <= (TOOLBAR_BOOKMARK_START +
	(MENU_BOOKMARK_ENDID - MENU_BOOKMARK_STARTID - 1)))
	{
		Bookmark_t	Bookmark;
		TBBUTTON	tbButton;
		int			iIndex;

		iIndex = (int)SendMessage(m_hBookmarksToolbar,TB_COMMANDTOINDEX,LOWORD(wParam),0);

		if(iIndex != -1)
		{
			SendMessage(m_hBookmarksToolbar,TB_GETBUTTON,iIndex,(LPARAM)&tbButton);

			m_Bookmark.RetrieveBookmark((void *)tbButton.dwData,&Bookmark);

			/* If the toolbar item is a bookmark, simply navigate
			to its directory. If it's a folder, open a menu with
			its sub-items on. */
			if(Bookmark.Type == BOOKMARK_TYPE_BOOKMARK)
			{
				ExpandAndBrowsePath(Bookmark.szLocation);
			}
			else
			{
				HMENU		hMenu;
				Bookmark_t	ChildBookmark;
				RECT		rc;
				HRESULT		hr;

				hMenu = CreatePopupMenu();

				MENUINFO mi;
				mi.cbSize	= sizeof(MENUINFO);
				mi.fMask	= MIM_STYLE;
				mi.dwStyle	= MNS_NOTIFYBYPOS;
				SetMenuInfo(hMenu,&mi);

				hr = m_Bookmark.GetChild(&Bookmark,&ChildBookmark);

				if(SUCCEEDED(hr))
				{
					InsertBookmarksIntoMenuInternal(hMenu,&ChildBookmark,0,MENU_BOOKMARK_STARTID);
				}
				else
				{
					InsertBookmarksIntoMenuInternal(hMenu,NULL,0,MENU_BOOKMARK_STARTID);
				}

				SendMessage(m_hBookmarksToolbar,TB_GETITEMRECT,iIndex,(LPARAM)&rc);
				MapWindowPoints(m_hBookmarksToolbar,NULL,(LPPOINT)&rc,2);

				TrackPopupMenu(hMenu,TPM_LEFTALIGN,rc.left,rc.bottom,0,hwnd,NULL);
			}
		}
	}

	if(LOWORD(wParam) >= TOOLBAR_APPLICATIONS_ID_START)
	{
		int iIndex;

		iIndex = (int)SendMessage(m_hApplicationToolbar,
			TB_COMMANDTOINDEX,LOWORD(wParam),0);

		ApplicationToolbarOpenItem(iIndex,EMPTY_STRING);
	}
	else if(LOWORD(wParam) >= TOOLBAR_DRIVES_ID_START)
	{
		/* Was one of the buttons on the drives toolbar clicked? */
		TBBUTTON	tbButton;
		TCHAR		*pszDrivePath = NULL;
		LRESULT		lResult;
		int			iIndex;

		iIndex = (int)SendMessage(m_hDrivesToolbar,TB_COMMANDTOINDEX,LOWORD(wParam),0);

		if(iIndex != -1)
		{
			lResult = SendMessage(m_hDrivesToolbar,TB_GETBUTTON,iIndex,(LPARAM)&tbButton);

			if(lResult)
			{
				pszDrivePath = (TCHAR *)tbButton.dwData;

				BrowseFolder(pszDrivePath,SBSP_ABSOLUTE);
			}
		}
	}

	switch(LOWORD(wParam))
	{
		case TOOLBAR_NEWTAB:
		case IDM_FILE_NEWTAB:
			OnNewTab();
			break;

		case TABTOOLBAR_CLOSE:
		case IDM_FILE_CLOSETAB:
			OnCloseTab();
			break;

		case IDM_FILE_CLONEWINDOW:
			OnCloneWindow();
			break;

		case IDM_FILE_SAVEDIRECTORYLISTING:
			OnSaveDirectoryListing();
			break;

		case TOOLBAR_SHOWCOMMANDPROMPT:
		case IDM_FILE_SHOWCOMMANDPROMPT:
			StartCommandPrompt(m_CurrentDirectory);
			break;

		case IDM_FILE_COPYFOLDERPATH:
			CopyTextToClipboard(m_CurrentDirectory);
			break;

		case IDM_FILE_COPYITEMPATH:
			OnCopyItemPath();
			break;

		case IDM_FILE_COPYUNIVERSALFILEPATHS:
			OnCopyUniversalPaths();
			break;

		case IDM_FILE_COPYCOLUMNTEXT:
			CopyColumnInfoToClipboard();
			break;

		case IDM_FILE_SETFILEATTRIBUTES:
			OnSetFileAttributes();
			break;

		case TOOLBAR_DELETE:
		case IDM_FILE_DELETE:
			OnFileDelete(FALSE);
			break;

		case TOOLBAR_DELETEPERMANENTLY:
		case IDM_FILE_DELETEPERMANENTLY:
			OnFileDelete(TRUE);
			break;

		case IDM_FILE_RENAME:
		case IDA_FILE_RENAME:
			OnFileRename();
			break;

		case TOOLBAR_PROPERTIES:
		case IDM_FILE_PROPERTIES:
		case IDM_RCLICK_PROPERTIES:
			OnShowFileProperties();
			break;

		case IDM_FILE_EXIT:
			SendMessage(hwnd,WM_CLOSE,0,0);
			break;

		case IDM_EDIT_UNDO:
			m_FileActionHandler.Undo();
			break;

		case TOOLBAR_COPY:
		case IDM_EDIT_COPY:
			OnCopy(TRUE);
			break;

		case TOOLBAR_CUT:
		case IDM_EDIT_CUT:
			OnCopy(FALSE);
			break;

		case TOOLBAR_PASTE:
		case IDM_EDIT_PASTE:
			OnPaste();
			break;

		case IDM_EDIT_PASTESHORTCUT:
			PasteLinksToClipboardFiles(m_CurrentDirectory);
			break;

		case IDM_EDIT_PASTEHARDLINK:
			PasteHardLinks(m_CurrentDirectory);
			break;

		case IDM_EDIT_COPYTOFOLDER:
		case TOOLBAR_COPYTO:
			CopyToFolder(FALSE);
			break;

		case TOOLBAR_MOVETO:
		case IDM_EDIT_MOVETOFOLDER:
			CopyToFolder(TRUE);
			break;

		case IDM_EDIT_SELECTALL:
			m_bCountingUp = TRUE;
			NListView::ListView_SelectAllItems(m_hActiveListView,TRUE);
			SetFocus(m_hActiveListView);
			break;

		case IDM_EDIT_INVERTSELECTION:
			m_bInverted = TRUE;
			m_nSelectedOnInvert = m_nSelected;
			NListView::ListView_InvertSelection(m_hActiveListView);
			SetFocus(m_hActiveListView);
			break;

		case IDM_EDIT_SELECTALLFOLDERS:
			HighlightSimilarFiles(m_hActiveListView);
			SetFocus(m_hActiveListView);
			break;

		case IDM_EDIT_SELECTNONE:
			m_bCountingDown = TRUE;
			NListView::ListView_SelectAllItems(m_hActiveListView,FALSE);
			SetFocus(m_hActiveListView);
			break;

		case IDM_EDIT_WILDCARDSELECTION:
			OnWildcardSelect(TRUE);
			break;

		case IDM_EDIT_WILDCARDDESELECT:
			OnWildcardSelect(FALSE);
			break;

		case IDM_EDIT_SHOWFILESLACK:
			OnSaveFileSlack();
			break;

		case IDM_EDIT_RESOLVELINK:
			OnResolveLink();
			break;

		case IDM_VIEW_STATUSBAR:
			m_bShowStatusBar = !m_bShowStatusBar;
			lShowWindow(m_hStatusBar,m_bShowStatusBar);
			ResizeWindows();
			break;

		case IDM_VIEW_FOLDERS:
		case TOOLBAR_FOLDERS:
			ToggleFolders();
			break;

		case IDM_VIEW_DISPLAYWINDOW:
			m_bShowDisplayWindow = !m_bShowDisplayWindow;
			lShowWindow(m_hDisplayWindow,m_bShowDisplayWindow);
			ResizeWindows();
			break;

		case IDM_TOOLBARS_ADDRESSBAR:
			m_bShowAddressBar = !m_bShowAddressBar;
			ShowMainRebarBand(m_hAddressToolbar,m_bShowAddressBar);
			AdjustFolderPanePosition();
			ResizeWindows();
			break;

		case IDM_TOOLBARS_MAINTOOLBAR:
			m_bShowMainToolbar = !m_bShowMainToolbar;
			ShowMainRebarBand(m_hMainToolbar,m_bShowMainToolbar);
			AdjustFolderPanePosition();
			ResizeWindows();
			break;

		case IDM_TOOLBARS_BOOKMARKSTOOLBAR:
			m_bShowBookmarksToolbar = !m_bShowBookmarksToolbar;
			ShowMainRebarBand(m_hBookmarksToolbar,m_bShowBookmarksToolbar);
			AdjustFolderPanePosition();
			ResizeWindows();
			break;

		case IDM_TOOLBARS_DRIVES:
			m_bShowDrivesToolbar = !m_bShowDrivesToolbar;
			ShowMainRebarBand(m_hDrivesToolbar,m_bShowDrivesToolbar);
			AdjustFolderPanePosition();
			ResizeWindows();
			break;

		case IDM_TOOLBARS_APPLICATIONTOOLBAR:
			m_bShowApplicationToolbar = !m_bShowApplicationToolbar;
			ShowMainRebarBand(m_hApplicationToolbar,m_bShowApplicationToolbar);
			AdjustFolderPanePosition();
			ResizeWindows();
			break;

		case IDM_TOOLBARS_LOCKTOOLBARS:
			OnLockToolbars();
			break;

		case IDM_TOOLBARS_CUSTOMIZE:
			SendMessage(m_hMainToolbar,TB_CUSTOMIZE,0,0);
			break;

		case IDM_VIEW_EXTRALARGEICONS:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_EXTRALARGEICONS);
			break;

		case IDM_VIEW_LARGEICONS:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_LARGEICONS);
			break;

		case IDM_VIEW_ICONS:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_ICONS);
			break;

		case IDM_VIEW_SMALLICONS:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_SMALLICONS);
			break;

		case IDM_VIEW_LIST:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_LIST);
			break;

		case IDM_VIEW_DETAILS:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_DETAILS);
			break;

		case IDM_VIEW_THUMBNAILS:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_THUMBNAILS);
			break;

		case IDM_VIEW_TILES:
			m_pFolderView[m_iObjectIndex]->SetCurrentViewMode(VM_TILES);
			break;

		case IDM_VIEW_CHANGEDISPLAYCOLOURS:
			DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_DISPLAYCOLOURS),
				hwnd,ChangeDisplayColoursStub,(LPARAM)this);
			break;

		case IDM_FILTER_FILTERRESULTS:
			{
				CFilterDialog FilterDialog(g_hLanguageModule,IDD_FILTER,hwnd,this);

				FilterDialog.ShowModalDialog();
			}
			break;

		case IDM_FILTER_APPLYFILTER:
			SetFilterStatus();
			break;

		case IDM_SORTBY_NAME:
			OnSortBy(FSM_NAME);
			break;

		case IDM_SORTBY_SIZE:
			OnSortBy(FSM_SIZE);
			break;

		case IDM_SORTBY_TYPE:
			OnSortBy(FSM_TYPE);
			break;

		case IDM_SORTBY_DATEMODIFIED:
			OnSortBy(FSM_DATEMODIFIED);
			break;

		case IDM_SORTBY_TOTALSIZE:
			OnSortBy(FSM_TOTALSIZE);
			break;

		case IDM_SORTBY_FREESPACE:
			OnSortBy(FSM_FREESPACE);
			break;

		case IDM_SORTBY_DATEDELETED:
			OnSortBy(FSM_DATEDELETED);
			break;

		case IDM_SORTBY_ORIGINALLOCATION:
			OnSortBy(FSM_ORIGINALLOCATION);
			break;

		case IDM_SORTBY_ATTRIBUTES:
			OnSortBy(FSM_ATTRIBUTES);
			break;

		case IDM_SORTBY_REALSIZE:
			OnSortBy(FSM_REALSIZE);
			break;

		case IDM_SORTBY_SHORTNAME:
			OnSortBy(FSM_SHORTNAME);
			break;

		case IDM_SORTBY_OWNER:
			OnSortBy(FSM_OWNER);
			break;

		case IDM_SORTBY_PRODUCTNAME:
			OnSortBy(FSM_PRODUCTNAME);
			break;

		case IDM_SORTBY_COMPANY:
			OnSortBy(FSM_COMPANY);
			break;

		case IDM_SORTBY_DESCRIPTION:
			OnSortBy(FSM_DESCRIPTION);
			break;

		case IDM_SORTBY_FILEVERSION:
			OnSortBy(FSM_FILEVERSION);
			break;

		case IDM_SORTBY_PRODUCTVERSION:
			OnSortBy(FSM_PRODUCTVERSION);
			break;

		case IDM_SORTBY_SHORTCUTTO:
			OnSortBy(FSM_SHORTCUTTO);
			break;

		case IDM_SORTBY_HARDLINKS:
			OnSortBy(FSM_HARDLINKS);
			break;

		case IDM_SORTBY_EXTENSION:
			OnSortBy(FSM_EXTENSION);
			break;

		case IDM_SORTBY_CREATED:
			OnSortBy(FSM_CREATED);
			break;

		case IDM_SORTBY_ACCESSED:
			OnSortBy(FSM_ACCESSED);
			break;

		case IDM_SORTBY_TITLE:
			OnSortBy(FSM_TITLE);
			break;

		case IDM_SORTBY_SUBJECT:
			OnSortBy(FSM_SUBJECT);
			break;

		case IDM_SORTBY_AUTHOR:
			OnSortBy(FSM_AUTHOR);
			break;

		case IDM_SORTBY_KEYWORDS:
			OnSortBy(FSM_KEYWORDS);
			break;

		case IDM_SORTBY_COMMENTS:
			OnSortBy(FSM_COMMENTS);
			break;

		case IDM_SORTBY_CAMERAMODEL:
			OnSortBy(FSM_CAMERAMODEL);
			break;

		case IDM_SORTBY_DATETAKEN:
			OnSortBy(FSM_DATETAKEN);
			break;

		case IDM_SORTBY_WIDTH:
			OnSortBy(FSM_WIDTH);
			break;

		case IDM_SORTBY_HEIGHT:
			OnSortBy(FSM_HEIGHT);
			break;

		case IDM_SORTBY_VIRTUALCOMMENTS:
			OnSortBy(FSM_VIRTUALCOMMENTS);
			break;

		case IDM_SORTBY_FILESYSTEM:
			OnSortBy(FSM_FILESYSTEM);
			break;

		case IDM_SORTBY_VIRTUALTYPE:
			OnSortBy(FSM_VIRTUALTYPE);
			break;

		case IDM_SORTBY_NUMPRINTERDOCUMENTS:
			OnSortBy(FSM_NUMPRINTERDOCUMENTS);
			break;

		case IDM_SORTBY_PRINTERSTATUS:
			OnSortBy(FSM_PRINTERSTATUS);
			break;

		case IDM_SORTBY_PRINTERCOMMENTS:
			OnSortBy(FSM_PRINTERCOMMENTS);
			break;

		case IDM_SORTBY_PRINTERLOCATION:
			OnSortBy(FSM_PRINTERLOCATION);
			break;

		case IDM_SORTBY_NETWORKADAPTER_STATUS:
			OnSortBy(FSM_NETWORKADAPTER_STATUS);
			break;

		case IDM_GROUPBY_NAME:
			OnGroupBy(FSM_NAME);
			break;

		case IDM_GROUPBY_SIZE:
			OnGroupBy(FSM_SIZE);
			break;

		case IDM_GROUPBY_TYPE:
			OnGroupBy(FSM_TYPE);
			break;

		case IDM_GROUPBY_DATEMODIFIED:
			OnGroupBy(FSM_DATEMODIFIED);
			break;

		case IDM_GROUPBY_TOTALSIZE:
			OnGroupBy(FSM_TOTALSIZE);
			break;

		case IDM_GROUPBY_FREESPACE:
			OnGroupBy(FSM_FREESPACE);
			break;

		case IDM_GROUPBY_DATEDELETED:
			OnGroupBy(FSM_DATEDELETED);
			break;

		case IDM_GROUPBY_ORIGINALLOCATION:
			OnGroupBy(FSM_ORIGINALLOCATION);
			break;

		case IDM_GROUPBY_ATTRIBUTES:
			OnGroupBy(FSM_ATTRIBUTES);
			break;

		case IDM_GROUPBY_REALSIZE:
			OnGroupBy(FSM_REALSIZE);
			break;

		case IDM_GROUPBY_SHORTNAME:
			OnGroupBy(FSM_SHORTNAME);
			break;

		case IDM_GROUPBY_OWNER:
			OnGroupBy(FSM_OWNER);
			break;

		case IDM_GROUPBY_PRODUCTNAME:
			OnGroupBy(FSM_PRODUCTNAME);
			break;

		case IDM_GROUPBY_COMPANY:
			OnGroupBy(FSM_COMPANY);
			break;

		case IDM_GROUPBY_DESCRIPTION:
			OnGroupBy(FSM_DESCRIPTION);
			break;

		case IDM_GROUPBY_FILEVERSION:
			OnGroupBy(FSM_FILEVERSION);
			break;

		case IDM_GROUPBY_PRODUCTVERSION:
			OnGroupBy(FSM_PRODUCTVERSION);
			break;

		case IDM_GROUPBY_SHORTCUTTO:
			OnGroupBy(FSM_SHORTCUTTO);
			break;

		case IDM_GROUPBY_HARDLINKS:
			OnGroupBy(FSM_HARDLINKS);
			break;

		case IDM_GROUPBY_EXTENSION:
			OnGroupBy(FSM_EXTENSION);
			break;

		case IDM_GROUPBY_CREATED:
			OnGroupBy(FSM_CREATED);
			break;

		case IDM_GROUPBY_ACCESSED:
			OnGroupBy(FSM_ACCESSED);
			break;

		case IDM_GROUPBY_TITLE:
			OnGroupBy(FSM_TITLE);
			break;

		case IDM_GROUPBY_SUBJECT:
			OnGroupBy(FSM_SUBJECT);
			break;

		case IDM_GROUPBY_AUTHOR:
			OnGroupBy(FSM_AUTHOR);
			break;

		case IDM_GROUPBY_KEYWORDS:
			OnGroupBy(FSM_KEYWORDS);
			break;

		case IDM_GROUPBY_COMMENTS:
			OnGroupBy(FSM_COMMENTS);
			break;

		case IDM_GROUPBY_CAMERAMODEL:
			OnGroupBy(FSM_CAMERAMODEL);
			break;

		case IDM_GROUPBY_DATETAKEN:
			OnGroupBy(FSM_DATETAKEN);
			break;

		case IDM_GROUPBY_WIDTH:
			OnGroupBy(FSM_WIDTH);
			break;

		case IDM_GROUPBY_HEIGHT:
			OnGroupBy(FSM_HEIGHT);
			break;

		case IDM_GROUPBY_VIRTUALCOMMENTS:
			OnGroupBy(FSM_VIRTUALCOMMENTS);
			break;

		case IDM_GROUPBY_FILESYSTEM:
			OnGroupBy(FSM_FILESYSTEM);
			break;

		case IDM_GROUPBY_VIRTUALTYPE:
			OnGroupBy(FSM_VIRTUALTYPE);
			break;

		case IDM_GROUPBY_NUMPRINTERDOCUMENTS:
			OnGroupBy(FSM_NUMPRINTERDOCUMENTS);
			break;

		case IDM_GROUPBY_PRINTERSTATUS:
			OnGroupBy(FSM_PRINTERSTATUS);
			break;

		case IDM_GROUPBY_PRINTERCOMMENTS:
			OnGroupBy(FSM_PRINTERCOMMENTS);
			break;

		case IDM_GROUPBY_PRINTERLOCATION:
			OnGroupBy(FSM_PRINTERLOCATION);
			break;

		case IDM_GROUPBY_NETWORKADAPTER_STATUS:
			OnGroupBy(FSM_NETWORKADAPTER_STATUS);
			break;

		case IDM_ARRANGEICONSBY_ASCENDING:
			OnSortByAscending(TRUE);
			break;

		case IDM_ARRANGEICONSBY_DESCENDING:
			OnSortByAscending(FALSE);
			break;
			
		case IDM_ARRANGEICONSBY_AUTOARRANGE:
			m_pActiveShellBrowser->ToggleAutoArrange();
			break;

		case IDM_ARRANGEICONSBY_SHOWINGROUPS:
			m_pActiveShellBrowser->ToggleGrouping();
			break;

		case IDM_VIEW_SHOWHIDDENFILES:
			ShowHiddenFiles();
			break;

		case TOOLBAR_REFRESH:
		case IDM_VIEW_REFRESH:
			OnRefresh();
			break;

		case IDM_SORTBY_MORE:
		case IDM_VIEW_SELECTCOLUMNS:
			OnSelectColumns();
			break;

		case IDM_VIEW_AUTOSIZECOLUMNS:
			OnAutoSizeColumns();
			break;

		case IDM_VIEW_SAVECOLUMNLAYOUTASDEFAULT:
			{
				/* Dump the columns from the current tab, and save
				them as the default columns for the appropriate folder
				type.. */
				IShellFolder *pShellFolder = NULL;
				std::list<Column_t> pActiveColumnList;
				LPITEMIDLIST pidl = NULL;
				LPITEMIDLIST pidlDrives = NULL;
				LPITEMIDLIST pidlControls = NULL;
				LPITEMIDLIST pidlBitBucket = NULL;
				LPITEMIDLIST pidlPrinters = NULL;
				LPITEMIDLIST pidlConnections = NULL;
				LPITEMIDLIST pidlNetwork = NULL;

				m_pActiveShellBrowser->ExportCurrentColumns(&pActiveColumnList);

				pidl = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

				SHGetFolderLocation(NULL,CSIDL_DRIVES,NULL,0,&pidlDrives);
				SHGetFolderLocation(NULL,CSIDL_CONTROLS,NULL,0,&pidlControls);
				SHGetFolderLocation(NULL,CSIDL_BITBUCKET,NULL,0,&pidlBitBucket);
				SHGetFolderLocation(NULL,CSIDL_PRINTERS,NULL,0,&pidlPrinters);
				SHGetFolderLocation(NULL,CSIDL_CONNECTIONS,NULL,0,&pidlConnections);
				SHGetFolderLocation(NULL,CSIDL_NETWORK,NULL,0,&pidlNetwork);

				SHGetDesktopFolder(&pShellFolder);

				if(pShellFolder->CompareIDs(SHCIDS_CANONICALONLY,pidl,pidlDrives) == 0)
				{
					m_MyComputerColumnList = pActiveColumnList;
				}
				else if(pShellFolder->CompareIDs(SHCIDS_CANONICALONLY,pidl,pidlControls) == 0)
				{
					m_ControlPanelColumnList = pActiveColumnList;
				}
				else if(pShellFolder->CompareIDs(SHCIDS_CANONICALONLY,pidl,pidlBitBucket) == 0)
				{
					m_RecycleBinColumnList = pActiveColumnList;
				}
				else if(pShellFolder->CompareIDs(SHCIDS_CANONICALONLY,pidl,pidlPrinters) == 0)
				{
					m_PrintersColumnList = pActiveColumnList;
				}
				else if(pShellFolder->CompareIDs(SHCIDS_CANONICALONLY,pidl,pidlConnections) == 0)
				{
					m_NetworkConnectionsColumnList = pActiveColumnList;
				}
				else if(pShellFolder->CompareIDs(SHCIDS_CANONICALONLY,pidl,pidlNetwork) == 0)
				{
					m_MyNetworkPlacesColumnList = pActiveColumnList;
				}
				else
				{
					m_RealFolderColumnList = pActiveColumnList;
				}

				pActiveColumnList.clear();

				pShellFolder->Release();

				CoTaskMemFree(pidl);
			}
			break;

		case TOOLBAR_NEWFOLDER:
		case IDM_ACTIONS_NEWFOLDER:
			OnCreateNewFolder();
			break;

		case IDM_ACTIONS_MERGEFILES:
			{
				TCHAR szCurrentDirectory[MAX_PATH];
				m_pActiveShellBrowser->QueryCurrentDirectory(MAX_PATH,szCurrentDirectory);

				std::list<std::wstring>	FullFilenameList;
				int iItem = -1;

				while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
				{
					TCHAR szFullFilename[MAX_PATH];
					m_pActiveShellBrowser->QueryFullItemName(iItem,szFullFilename);
					FullFilenameList.push_back(szFullFilename);
				}

				CMergeFilesDialog CMergeFilesDialog(g_hLanguageModule,IDD_MERGEFILES,
					m_hContainer,szCurrentDirectory,FullFilenameList,m_bShowFriendlyDatesGlobal);

				CMergeFilesDialog.ShowModalDialog();
			}
			break;

		case IDM_ACTIONS_SPLITFILE:
			{
				int iSelected = ListView_GetNextItem(m_hActiveListView,-1,LVNI_SELECTED);

				if(iSelected != -1)
				{
					TCHAR szFullFilename[MAX_PATH];
					m_pActiveShellBrowser->QueryFullItemName(iSelected,szFullFilename);

					CSplitFileDialog SplitFileDialog(g_hLanguageModule,IDD_SPLITFILE,hwnd,szFullFilename);

					SplitFileDialog.ShowModalDialog();
				}
			}
			break;

		case IDM_ACTIONS_DESTROYFILES:
			{
				std::list<std::wstring>	FullFilenameList;
				int iItem = -1;

				while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
				{
					TCHAR szFullFilename[MAX_PATH];
					m_pActiveShellBrowser->QueryFullItemName(iItem,szFullFilename);
					FullFilenameList.push_back(szFullFilename);
				}

				CDestroyFilesDialog CDestroyFilesDialog(g_hLanguageModule,IDD_DESTROYFILES,
					m_hContainer,FullFilenameList,m_bShowFriendlyDatesGlobal);

				CDestroyFilesDialog.ShowModalDialog();
			}
			break;

		case IDM_GO_BACK:
		case TOOLBAR_BACK:
			OnBrowseBack();
			break;

		case IDM_GO_FORWARD:
		case TOOLBAR_FORWARD:
			OnBrowseForward();
			break;

		case IDM_GO_UPONELEVEL:
		case TOOLBAR_UP:
			OnNavigateUp();
			break;

		case IDM_GO_MYCOMPUTER:
			GotoFolder(CSIDL_DRIVES);
			break;

		case IDM_GO_MYDOCUMENTS:
			GotoFolder(CSIDL_PERSONAL);
			break;

		case IDM_GO_MYMUSIC:
			GotoFolder(CSIDL_MYMUSIC);
			break;

		case IDM_GO_MYPICTURES:
			GotoFolder(CSIDL_MYPICTURES);
			break;

		case IDM_GO_DESKTOP:
			GotoFolder(CSIDL_DESKTOP);
			break;

		case IDM_GO_RECYCLEBIN:
			GotoFolder(CSIDL_BITBUCKET);
			break;

		case IDM_GO_CONTROLPANEL:
			GotoFolder(CSIDL_CONTROLS);
			break;

		case IDM_GO_PRINTERS:
			GotoFolder(CSIDL_PRINTERS);
			break;

		case IDM_GO_CDBURNING:
			GotoFolder(CSIDL_CDBURN_AREA);
			break;

		case IDM_GO_MYNETWORKPLACES:
			GotoFolder(CSIDL_NETWORK);
			break;

		case IDM_GO_NETWORKCONNECTIONS:
			GotoFolder(CSIDL_CONNECTIONS);
			break;

		case TOOLBAR_ADDBOOKMARK:
		case IDM_BOOKMARKS_BOOKMARKTHISTAB:
			{
				AddBookmarkInfo_t abi;

				abi.pContainer		= (void *)this;
				abi.pParentBookmark	= NULL;
				abi.pidlDirectory	= m_pActiveShellBrowser->QueryCurrentDirectoryIdl();
				abi.bExpandInitial	= FALSE;

				DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_ADD_BOOKMARK),
					hwnd,BookmarkTabDlgProcStub,(LPARAM)&abi);

				CoTaskMemFree(abi.pidlDirectory);

				/* It's possible the dialog may have been cancelled
				or closed, but that the bookmarks menu still needs
				to be updated. This is the case when (for example),
				a new bookmark folder is created, and the dialog
				is cancelled. */
				InsertBookmarksIntoMenu();
			}
			break;

		case TOOLBAR_ORGANIZEBOOKMARKS:
		case IDM_BOOKMARKS_ORGANIZEBOOKMARKS:
			DialogBoxParam(g_hLanguageModule,MAKEINTRESOURCE(IDD_ORGANIZE_BOOKMARKS),
				m_hContainer,OrganizeBookmarksStub,(LPARAM)this);
			break;

		case TOOLBAR_SEARCH:
		case IDM_TOOLS_SEARCH:
			if(g_hwndSearch == NULL)
			{
				TCHAR szCurrentDirectory[MAX_PATH];

				m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(szCurrentDirectory),
					szCurrentDirectory);

				CSearchDialog *SearchDialog = new CSearchDialog(g_hLanguageModule,
					IDD_SEARCH,hwnd,szCurrentDirectory,this);

				g_hwndSearch = SearchDialog->ShowModelessDialog(this);
			}
			else
			{
				SetFocus(g_hwndSearch);
			}
			break;

		case IDM_TOOLS_CUSTOMIZECOLORS:
			{
				CCustomizeColorsDialog CustomizeColorsDialog(g_hLanguageModule,IDD_CUSTOMIZECOLORS,hwnd,&m_ColorRuleList);

				CustomizeColorsDialog.ShowModalDialog();

				/* Causes the active listview to redraw (therefore
				applying any updated color schemes). */
				InvalidateRect(m_hActiveListView,NULL,FALSE);
			}
			break;

		case IDM_TOOLS_OPTIONS:
			if(g_hwndOptions == NULL)
			{
				OnShowOptions();
			}
			else
			{
				SetFocus(g_hwndOptions);
			}
			break;

		case IDM_HELP_HELP:
			{
				TCHAR szHelpFile[MAX_PATH];
				GetCurrentProcessImageName(szHelpFile,SIZEOF_ARRAY(szHelpFile));
				PathRemoveFileSpec(szHelpFile);
				PathAppend(szHelpFile,NExplorerplusplus::HELP_FILE_NAME);

				LPITEMIDLIST pidl = NULL;
				HRESULT hr = GetIdlFromParsingName(szHelpFile,&pidl);

				bool bOpenedHelpFile = false;

				if(SUCCEEDED(hr))
				{
					BOOL bRes = ExecuteFileAction(m_hContainer,NULL,NULL,NULL,pidl);

					if(bRes)
					{
						bOpenedHelpFile = true;
					}
				}

				if(!bOpenedHelpFile)
				{
					CHelpFileMissingDialog HelpFileMissingDialog(g_hLanguageModule,IDD_HELPFILEMISSING,hwnd);
					HelpFileMissingDialog.ShowModalDialog();
				}
			}
			break;

		case IDM_HELP_ABOUT:
			{
				CAboutDialog AboutDialog(g_hLanguageModule,IDD_ABOUT,hwnd);
				AboutDialog.ShowModalDialog();
			}
			break;


		case IDA_NEXTTAB:
			SelectAdjacentTab(TRUE);
			break;

		case IDA_PREVIOUSTAB:
			SelectAdjacentTab(FALSE);
			break;

		case IDA_ADDRESSBAR:
			SetFocus(m_hAddressBar);
			break;

		case IDA_COMBODROPDOWN:
			SetFocus(m_hAddressBar);
			SendMessage(m_hAddressBar,CB_SHOWDROPDOWN,TRUE,0);
			break;

		case IDA_PREVIOUSWINDOW:
			OnPreviousWindow();
			break;

		case IDA_NEXTWINDOW:
			OnNextWindow();
			break;

		case IDA_RCLICK:
			OnIdaRClick();
			break;

		case IDA_TAB_DUPLICATETAB:
			OnDuplicateTab(m_iTabSelectedItem);
			break;

		case IDA_HOME:
			OnHome();
			break;

		case IDA_TAB1:
			OnSelectTab(0);
			break;

		case IDA_TAB2:
			OnSelectTab(1);
			break;

		case IDA_TAB3:
			OnSelectTab(2);
			break;

		case IDA_TAB4:
			OnSelectTab(3);
			break;

		case IDA_TAB5:
			OnSelectTab(4);
			break;

		case IDA_TAB6:
			OnSelectTab(5);
			break;

		case IDA_TAB7:
			OnSelectTab(6);
			break;

		case IDA_TAB8:
			OnSelectTab(7);
			break;

		case IDA_LASTTAB:
			OnSelectTab(-1);
			break;

		case TOOLBAR_VIEWS:
			OnToolbarViews();
			break;

		case TOOLBAR_ADDRESSBAR_GO:
			OnAddressBarGo();
			break;

		/* Messages from the context menu that
		is used with the bookmarks toolbar. */
		case IDM_BT_OPEN:
			BookmarkToolbarOpenItem(m_iSelectedRClick,FALSE);
			break;

		case IDM_BT_OPENINNEWTAB:
			BookmarkToolbarOpenItem(m_iSelectedRClick,TRUE);
			break;

		case IDM_BT_DELETE:
			BookmarkToolbarDeleteItem(m_iSelectedRClick);
			break;

		case IDM_BT_PROPERTIES:
			BookmarkToolbarShowItemProperties(m_iSelectedRClick);
			break;

		case IDM_BT_NEWBOOKMARK:
			BookmarkToolbarNewBookmark(m_iSelectedRClick);
			break;

		case IDM_BT_NEWFOLDER:
			BookmarkToolbarNewFolder(m_iSelectedRClick);
			break;

		case IDM_APP_OPEN:
			ApplicationToolbarOpenItem(m_iSelectedRClick,EMPTY_STRING);
			break;

		case IDM_APP_NEW:
			ApplicationToolbarNewButton();
			break;

		case IDM_APP_DELETE:
			ApplicationToolbarDeleteItem(m_iSelectedRClick);
			break;

		case IDM_APP_PROPERTIES:
			ApplicationToolbarShowItemProperties(m_iSelectedRClick);
			break;

		/* Listview column header context menu. */
		case IDM_HEADER_MORE:
			OnSelectColumns();
			break;

		/* Display window menus. */
		case IDM_DW_HIDEDISPLAYWINDOW:
			m_bShowDisplayWindow = FALSE;
			lShowWindow(m_hDisplayWindow,m_bShowDisplayWindow);
			ResizeWindows();
			break;
	}

	switch(HIWORD(wParam))
	{
		case CBN_DROPDOWN:
			AddPathsToComboBoxEx(m_hAddressBar,m_CurrentDirectory);
			break;
	}

	return 1;
}

/*
 * WM_NOTIFY handler for the main window.
 */
LRESULT CALLBACK Explorerplusplus::NotifyHandler(HWND hwnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	NMHDR *nmhdr;
	nmhdr = (NMHDR *)lParam;

	switch(nmhdr->code)
	{
		case NM_CLICK:
			if(m_bOneClickActivate && !m_bSelectionFromNowhere)
			{
				OnListViewDoubleClick(&((NMITEMACTIVATE *)lParam)->hdr);
			}
			break;

		case NM_DBLCLK:
			OnListViewDoubleClick(nmhdr);
			break;

		case NM_RCLICK:
			OnRightClick(nmhdr);
			break;

		case NM_CUSTOMDRAW:
			return OnCustomDraw(lParam);
			break;

		case LVN_GETINFOTIP:
			OnListViewGetInfoTip(lParam);
			break;

		case LVN_KEYDOWN:
			return OnListViewKeyDown(lParam);
			break;

		case LVN_ITEMCHANGING:
			{
				UINT uViewMode;
				m_pFolderView[m_iObjectIndex]->GetCurrentViewMode(&uViewMode);
				if(uViewMode == VM_LIST)
				{
					if(m_bBlockNext)
					{
						m_bBlockNext = FALSE;
						return TRUE;
					}
				}
			}
			break;

		case LVN_ITEMCHANGED:
			OnListViewItemChanged(lParam);
			break;

		case LVN_BEGINDRAG:
			OnListViewBeginDrag(lParam,DRAG_TYPE_LEFTCLICK);
			break;

		case LVN_BEGINLABELEDIT:
			return OnListViewBeginLabelEdit(lParam);
			break;

		case LVN_ENDLABELEDIT:
			return OnListViewEndLabelEdit(lParam);
			break;

		case LVN_ENDSCROLL:
			{
				/* There's a bug within Windows XP that causes gridlines
				within a listview to draw incorrectly. See references to
				KB813791. */
				if(m_dwMajorVersion == WINDOWS_XP_MAJORVERSION)
				{
					UINT uViewMode;
					m_pFolderView[m_iObjectIndex]->GetCurrentViewMode(&uViewMode);

					DWORD dwExtendedStyle = ListView_GetExtendedListViewStyle(m_hActiveListView);

					if((uViewMode == VM_DETAILS) &&
						(dwExtendedStyle & LVS_EX_GRIDLINES))
					{
						InvalidateRect(m_hActiveListView,NULL,TRUE);
						UpdateWindow(m_hActiveListView);
					}
				}
			}
			break;

		case LVN_GETDISPINFO:
			OnListViewGetDisplayInfo(lParam);
			break;

		case LVN_COLUMNCLICK:
			OnListViewColumnClick(lParam);
			break;

		case CBEN_DRAGBEGIN:
			OnAddressBarBeginDrag();
			break;

		case TBN_DROPDOWN:
			OnTbnDropDown(lParam);
			break;

		case TBN_INITCUSTOMIZE:
			return TBNRF_HIDEHELP;
			break;

		case TBN_QUERYINSERT:
			return OnTBQueryInsert(lParam);
			break;

		case TBN_QUERYDELETE:
			return OnTBQueryDelete(lParam);
			break;

		case TBN_GETBUTTONINFO:
			return OnTBGetButtonInfo(lParam);
			break;

		case TBN_RESTORE:
			return OnTBRestore(lParam);
			break;

		case TBN_GETINFOTIP:
			OnTBGetInfoTip(lParam);
			break;

		case TBN_RESET:
			OnTBReset();
			break;

		case TBN_ENDADJUST:
			UpdateToolbarBandSizing(m_hMainRebar,((NMHDR *)lParam)->hwndFrom);
			break;

		case RBN_BEGINDRAG:
			SendMessage(m_hMainRebar,RB_DRAGMOVE,0,-1);
			return 0;
			break;

		case RBN_HEIGHTCHANGE:
			/* The listview and treeview may
			need to be moved to accomodate the new
			rebar size. */
			AdjustFolderPanePosition();
			ResizeWindows();
			break;

		case RBN_CHEVRONPUSHED:
			{
				NMREBARCHEVRON *pnmrc = NULL;
				HWND hToolbar = NULL;
				HMENU hMenu;
				HIMAGELIST himlBackup;
				HIMAGELIST himlSmall;
				MENUITEMINFO mii;
				TCHAR szText[512];
				TBBUTTON tbButton;
				RECT rcToolbar;
				RECT rcButton;
				RECT rcIntersect;
				LRESULT lResult;
				BOOL bIntersect;
				int iMenu = 0;
				int nButtons;
				int i = 0;

				hMenu = CreatePopupMenu();

				pnmrc = (NMREBARCHEVRON *)lParam;

				himlBackup = himlMenu;

				Shell_GetImageLists(NULL,&himlSmall);

				switch(pnmrc->wID)
				{
				case ID_MAINTOOLBAR:
					hToolbar = m_hMainToolbar;
					break;

				case ID_BOOKMARKSTOOLBAR:
					hToolbar = m_hBookmarksToolbar;
					break;

				case ID_DRIVESTOOLBAR:
					hToolbar = m_hDrivesToolbar;
					himlMenu = himlSmall;
					break;

				case ID_APPLICATIONSTOOLBAR:
					hToolbar = m_hApplicationToolbar;
					himlMenu = himlSmall;
					break;
				}

				nButtons = (int)SendMessage(hToolbar,TB_BUTTONCOUNT,0,0);

				GetClientRect(hToolbar,&rcToolbar);

				for(i = 0;i < nButtons;i++)
				{
					lResult = SendMessage(hToolbar,TB_GETITEMRECT,i,(LPARAM)&rcButton);

					if(lResult)
					{
						bIntersect = IntersectRect(&rcIntersect,&rcToolbar,&rcButton);

						if(!bIntersect || rcButton.right > rcToolbar.right)
						{
							SendMessage(hToolbar,TB_GETBUTTON,i,(LPARAM)&tbButton);

							if(tbButton.fsStyle & BTNS_SEP)
							{
								mii.cbSize		= sizeof(mii);
								mii.fMask		= MIIM_FTYPE;
								mii.fType		= MFT_SEPARATOR;
								InsertMenuItem(hMenu,i,TRUE,&mii);
							}
							else
							{
								if(IS_INTRESOURCE(tbButton.iString))
								{
									SendMessage(hToolbar,TB_GETSTRING,MAKEWPARAM(SIZEOF_ARRAY(szText),
										tbButton.iString),(LPARAM)szText);
								}
								else
								{
									StringCchCopy(szText,SIZEOF_ARRAY(szText),(LPCWSTR)tbButton.iString);
								}

								HMENU hSubMenu = NULL;
								UINT fMask;

								fMask = MIIM_ID|MIIM_STRING;
								hSubMenu = NULL;

								switch(pnmrc->wID)
								{
								case ID_MAINTOOLBAR:
									{
										switch(tbButton.idCommand)
										{
										case TOOLBAR_BACK:
											hSubMenu = CreateRebarHistoryMenu(TRUE);
											fMask |= MIIM_SUBMENU;
											break;

										case TOOLBAR_FORWARD:
											hSubMenu = CreateRebarHistoryMenu(FALSE);
											fMask |= MIIM_SUBMENU;
											break;

										case TOOLBAR_VIEWS:
											{
												TCHAR szSubMenuText[512];
												BOOL bRes;
												int nItems;
												int j = 0;

												hSubMenu = CreateMenu();

												nItems = GetMenuItemCount(m_hViewsMenu);

												for(j = 0;j < nItems;j++)
												{
													mii.cbSize		= sizeof(mii);
													mii.fMask		= MIIM_ID|MIIM_STRING;
													mii.dwTypeData	= szSubMenuText;
													mii.cch			= SIZEOF_ARRAY(szSubMenuText);
													bRes = GetMenuItemInfo(m_hViewsMenu,j,TRUE,&mii);

													if(bRes)
													{
														mii.cbSize		= sizeof(mii);
														mii.fMask		= MIIM_ID|MIIM_STRING;
														mii.dwTypeData	= szSubMenuText;
														InsertMenuItem(hSubMenu,j,TRUE,&mii);
													}
												}

												SetMenuOwnerDraw(hSubMenu);

												fMask |= MIIM_SUBMENU;
											}
											break;
										}
									}
									break;

								case ID_BOOKMARKSTOOLBAR:
									{
										Bookmark_t	Bookmark;
										int			iIndex;

										iIndex = (int)SendMessage(m_hBookmarksToolbar,TB_COMMANDTOINDEX,tbButton.idCommand,0);

										if(iIndex != -1)
										{
											SendMessage(m_hBookmarksToolbar,TB_GETBUTTON,iIndex,(LPARAM)&tbButton);

											m_Bookmark.RetrieveBookmark((void *)tbButton.dwData,&Bookmark);

											if(Bookmark.Type == BOOKMARK_TYPE_FOLDER)
											{
												Bookmark_t	ChildBookmark;
												HRESULT		hr;

												hSubMenu = CreateMenu();
												m_hBookmarksMenu = CreateMenu();

												MENUINFO mi;
												mi.cbSize	= sizeof(MENUINFO);
												mi.fMask	= MIM_STYLE;
												mi.dwStyle	= MNS_NOTIFYBYPOS;
												SetMenuInfo(hSubMenu,&mi);

												hr = m_Bookmark.GetChild(&Bookmark,&ChildBookmark);

												if(SUCCEEDED(hr))
												{
													InsertBookmarksIntoMenuInternal(hSubMenu,&ChildBookmark,0,MENU_BOOKMARK_STARTID);
													InsertBookmarksIntoMenuInternal(m_hBookmarksMenu,&ChildBookmark,0,MENU_BOOKMARK_STARTID);
												}
												else
												{
													InsertBookmarksIntoMenuInternal(hSubMenu,NULL,0,MENU_BOOKMARK_STARTID);
													InsertBookmarksIntoMenuInternal(m_hBookmarksMenu,NULL,0,MENU_BOOKMARK_STARTID);
												}

												SetMenuOwnerDraw(hSubMenu);

												fMask |= MIIM_SUBMENU;
											}
										}
									}
									break;
								}

								mii.cbSize		= sizeof(mii);
								mii.fMask		= fMask;
								mii.wID			= tbButton.idCommand;
								mii.hSubMenu	= hSubMenu;
								mii.dwTypeData	= szText;
								InsertMenuItem(hMenu,iMenu,TRUE,&mii);

								SetMenuItemOwnerDrawn(hMenu,iMenu);
								SetMenuItemBitmap(hMenu,tbButton.idCommand,tbButton.iBitmap);	
							}
							iMenu++;
						}
					}
				}

				POINT ptMenu;

				ptMenu.x = pnmrc->rc.left;
				ptMenu.y = pnmrc->rc.bottom;

				ClientToScreen(m_hMainRebar,&ptMenu);

				UINT uFlags = TPM_LEFTALIGN|TPM_RETURNCMD;
				int iCmd;

				iCmd = TrackPopupMenu(hMenu,uFlags,
					ptMenu.x,ptMenu.y,0,m_hMainRebar,NULL);

				if(iCmd != 0)
				{
					/* We'll handle the back and forward buttons
					in place, and send the rest of the messages
					back to the main window. */
					if((iCmd >= ID_REBAR_MENU_BACK_START &&
						iCmd <= ID_REBAR_MENU_BACK_END) ||
						(iCmd >= ID_REBAR_MENU_FORWARD_START &&
						iCmd <= ID_REBAR_MENU_FORWARD_END))
					{
						LPITEMIDLIST pidl = NULL;

						if(iCmd >= ID_REBAR_MENU_BACK_START &&
							iCmd <= ID_REBAR_MENU_BACK_END)
						{
							iCmd = -(iCmd - ID_REBAR_MENU_BACK_START);
						}
						else
						{
							iCmd = iCmd - ID_REBAR_MENU_FORWARD_START;
						}

						pidl = m_pActiveShellBrowser->RetrieveHistoryItem(iCmd);

						BrowseFolder(pidl,SBSP_ABSOLUTE|SBSP_WRITENOHISTORY);

						CoTaskMemFree(pidl);
					}
					else
					{
						SendMessage(m_hContainer,WM_COMMAND,MAKEWPARAM(iCmd,0),0);
					}
				}

				DestroyMenu(hMenu);

				himlMenu = himlBackup;
			}
			break;
	}

	return 0;
}