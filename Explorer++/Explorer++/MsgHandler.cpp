/******************************************************************
 *
 * Project: Explorer++
 * File: MsgHandler.cpp
 * License: GPL - See LICENSE in the top level directory
 *
 * Handles messages passed back from the main GUI components.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/Controls.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/Logging.h"


/* The treeview is offset by a small
amount on the left. */
static const int TREEVIEW_X_CLEARANCE = 1;

/* The offset from the top of the parent
window to the treeview. */
static const int TREEVIEW_Y_CLEARANCE = 20;

/* The spacing between the right edge of
the treeview and the holder window. */
static const int TREEVIEW_HOLDER_CLEARANCE = 4;

/* Width and height of the toolbar on
the folders pane. */
static const int FOLDERS_TOOLBAR_WIDTH = 16;
static const int FOLDERS_TOOLBAR_HEIGHT = 16;

static const int FOLDERS_TOOLBAR_X_OFFSET = -20;
static const int FOLDERS_TOOLBAR_Y_OFFSET = 3;

static const int TAB_TOOLBAR_X_OFFSET = -20;
static const int TAB_TOOLBAR_Y_OFFSET = 5;

/* Width and height of the toolbar that
appears on the tab control. */
static const int TAB_TOOLBAR_WIDTH = 20;
static const int TAB_TOOLBAR_HEIGHT = 20;

void CALLBACK UninitializeCOMAPC(ULONG_PTR dwParam);

void CALLBACK UninitializeCOMAPC(ULONG_PTR dwParam)
{
	UNREFERENCED_PARAMETER(dwParam);

	CoUninitialize();
}

void Explorerplusplus::TestConfigFile(void)
{
	m_bLoadSettingsFromXML = TestConfigFileInternal();
}

BOOL TestConfigFileInternal(void)
{
	HANDLE	hConfigFile;
	TCHAR	szConfigFile[MAX_PATH];
	BOOL	bLoadSettingsFromXML = FALSE;

	/* To ensure the configuration file is loaded from the same directory
	as the executable, determine the fully qualified path of the executable,
	then save the configuration file in that directory. */
	GetProcessImageName(GetCurrentProcessId(),szConfigFile,SIZEOF_ARRAY(szConfigFile));

	PathRemoveFileSpec(szConfigFile);
	PathAppend(szConfigFile,NExplorerplusplus::XML_FILENAME);

	hConfigFile = CreateFile(szConfigFile,GENERIC_READ,FILE_SHARE_READ,NULL,
		OPEN_EXISTING,0,NULL);

	if(hConfigFile != INVALID_HANDLE_VALUE)
	{
		bLoadSettingsFromXML = TRUE;

		CloseHandle(hConfigFile);
	}

	return bLoadSettingsFromXML;
}

void Explorerplusplus::LoadAllSettings(ILoadSave **pLoadSave)
{
	/* Tests for the existence of the configuration
	file. If the file is present, a flag is set
	indicating that the config file should be used
	to load settings. */
	TestConfigFile();

	/* Initialize the LoadSave interface. Note
	that this interface must be regenerated when
	saving, as it's possible for the save/load
	methods to be different. */
	if(m_bLoadSettingsFromXML)
	{
		*pLoadSave = new CLoadSaveXML(this,TRUE);

		/* When loading from the config file, also
		set the option to save back to it on exit. */
		m_bSavePreferencesToXMLFile = TRUE;
	}
	else
	{
		*pLoadSave = new CLoadSaveRegistry(this);
	}

	(*pLoadSave)->LoadBookmarks();
	(*pLoadSave)->LoadGenericSettings();
	(*pLoadSave)->LoadDefaultColumns();
	(*pLoadSave)->LoadApplicationToolbar();
	(*pLoadSave)->LoadToolbarInformation();
	(*pLoadSave)->LoadColorRules();
	(*pLoadSave)->LoadDialogStates();

	ValidateLoadedSettings();
}

void Explorerplusplus::OpenItem(const TCHAR *szItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow)
{
	LPITEMIDLIST	pidlItem = NULL;
	HRESULT			hr;

	hr = GetIdlFromParsingName(szItem,&pidlItem);

	if(SUCCEEDED(hr))
	{
		OpenItem(pidlItem,bOpenInNewTab,bOpenInNewWindow);

		CoTaskMemFree(pidlItem);
	}
}

void Explorerplusplus::OpenItem(LPCITEMIDLIST pidlItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow)
{
	SFGAOF uAttributes = SFGAO_FOLDER|SFGAO_STREAM|SFGAO_LINK;
	LPITEMIDLIST pidlControlPanel = NULL;
	HRESULT	hr;
	BOOL bControlPanelParent = FALSE;

	hr = SHGetFolderLocation(NULL,CSIDL_CONTROLS,NULL,0,&pidlControlPanel);

	if(SUCCEEDED(hr))
	{
		/* Check if the parent of the item is the control panel.
		If it is, pass it to the shell to open, rather than
		opening it in-place. */
		if(ILIsParent(pidlControlPanel,pidlItem,FALSE) &&
			!CompareIdls(pidlControlPanel,pidlItem))
		{
			bControlPanelParent = TRUE;
		}

		CoTaskMemFree(pidlControlPanel);
	}

	/* On Vista and later, the Control Panel was split into
	two completely separate views:
	 - Icon View
	 - Category View
	Icon view is essentially the same view provided in
	Windows XP and earlier (i.e. a simple, flat listing of
	all the items in the control panel).
	Category view, on the other hand, groups similar
	Control Panel items under several broad categories.
	It is important to note that both these 'views' are
	represented by different GUID's, and are NOT the
	same folder.
	 - Icon View:
	   ::{21EC2020-3AEA-1069-A2DD-08002B30309D} (Vista and Win 7)
	   ::{26EE0668-A00A-44D7-9371-BEB064C98683}\0 (Win 7)
	 - Category View:
	   ::{26EE0668-A00A-44D7-9371-BEB064C98683} (Vista and Win 7)
	*/
	if (!bControlPanelParent)
	{
		hr = GetIdlFromParsingName(CONTROL_PANEL_CATEGORY_VIEW, &pidlControlPanel);

		if (SUCCEEDED(hr))
		{
			/* Check if the parent of the item is the control panel.
			If it is, pass it to the shell to open, rather than
			opening it in-place. */
			if (ILIsParent(pidlControlPanel, pidlItem, FALSE) &&
				!CompareIdls(pidlControlPanel, pidlItem))
			{
				bControlPanelParent = TRUE;
			}

			CoTaskMemFree(pidlControlPanel);
		}
	}

	hr = GetItemAttributes(pidlItem,&uAttributes);

	if(SUCCEEDED(hr))
	{
		if((uAttributes & SFGAO_FOLDER) && (uAttributes & SFGAO_STREAM))
		{
			/* Zip file. */
			if(m_bHandleZipFiles)
			{
				OpenFolderItem(pidlItem,bOpenInNewTab,bOpenInNewWindow);
			}
			else
			{
				OpenFileItem(pidlItem,EMPTY_STRING);
			}
		}
		else if(((uAttributes & SFGAO_FOLDER) && !bControlPanelParent))
		{
			/* Open folders. */
			OpenFolderItem(pidlItem,bOpenInNewTab,bOpenInNewWindow);
		}
		else if(uAttributes & SFGAO_LINK && !bControlPanelParent)
		{
			/* This item is a shortcut. */
			TCHAR	szItemPath[MAX_PATH];
			TCHAR	szTargetPath[MAX_PATH];

			GetDisplayName(pidlItem,szItemPath,SIZEOF_ARRAY(szItemPath),SHGDN_FORPARSING);

			hr = NFileOperations::ResolveLink(m_hContainer,0,szItemPath,szTargetPath,SIZEOF_ARRAY(szTargetPath));

			if(hr == S_OK)
			{
				/* The target of the shortcut was found
				successfully. Query it to determine whether
				it is a folder or not. */
				uAttributes = SFGAO_FOLDER|SFGAO_STREAM;
				hr = GetItemAttributes(szTargetPath,&uAttributes);

				/* Note this is functionally equivalent to
				recursively calling this function again.
				However, the link may be arbitrarily deep
				(or point to itself). Therefore, DO NOT
				call this function recursively with itself
				without some way of stopping. */
				if(SUCCEEDED(hr))
				{
					/* Is this a link to a folder or zip file? */
					if(((uAttributes & SFGAO_FOLDER) && !(uAttributes & SFGAO_STREAM)) ||
						((uAttributes & SFGAO_FOLDER) && (uAttributes & SFGAO_STREAM) && m_bHandleZipFiles))
					{
						LPITEMIDLIST	pidlTarget = NULL;

						hr = GetIdlFromParsingName(szTargetPath,&pidlTarget);

						if(SUCCEEDED(hr))
						{
							OpenFolderItem(pidlTarget,bOpenInNewTab,bOpenInNewWindow);

							CoTaskMemFree(pidlTarget);
						}
					}
					else
					{
						hr = E_FAIL;
					}
				}
			}

			if(FAILED(hr))
			{
				/* It is possible the target may not resolve,
				yet the shortcut is still valid. This is the
				case with shortcut URL's for example.
				Also, even if the shortcut points to a dead
				folder, it should still attempted to be
				opened. */
				OpenFileItem(pidlItem,EMPTY_STRING);
			}
		}
		else if(bControlPanelParent && (uAttributes & SFGAO_FOLDER))
		{
			TCHAR szParsingPath[MAX_PATH];
			TCHAR szExplorerPath[MAX_PATH];

			GetDisplayName(pidlItem,szParsingPath,SIZEOF_ARRAY(szParsingPath),SHGDN_FORPARSING);

			MyExpandEnvironmentStrings(_T("%windir%\\explorer.exe"),
				szExplorerPath,SIZEOF_ARRAY(szExplorerPath));

			/* Invoke Windows Explorer directly. Note that only folder
			items need to be passed directly to Explorer. Two central
			reasons:
			1. Explorer can only open folder items.
			2. Non-folder items can be opened directly (regardless of
			whether or not they're children of the control panel). */
			ShellExecute(m_hContainer,_T("open"),szExplorerPath,
				szParsingPath,NULL,SW_SHOWNORMAL);
		}
		else
		{
			/* File item. */
			OpenFileItem(pidlItem,EMPTY_STRING);
		}
	}
}

void Explorerplusplus::OpenFolderItem(LPCITEMIDLIST pidlItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow)
{
	if(bOpenInNewWindow)
		BrowseFolder(pidlItem,SBSP_SAMEBROWSER,FALSE,FALSE,TRUE);
	else if(m_bAlwaysOpenNewTab || bOpenInNewTab)
		BrowseFolder(pidlItem,SBSP_SAMEBROWSER,TRUE,TRUE,FALSE);
	else
		BrowseFolder(pidlItem,SBSP_SAMEBROWSER);
}

void Explorerplusplus::OpenFileItem(LPCITEMIDLIST pidlItem,const TCHAR *szParameters)
{
	TCHAR			szItemDirectory[MAX_PATH];
	LPITEMIDLIST	pidlParent = NULL;

	pidlParent = ILClone(pidlItem);

	ILRemoveLastID(pidlParent);

	GetDisplayName(pidlParent,szItemDirectory,SIZEOF_ARRAY(szItemDirectory),SHGDN_FORPARSING);

	ExecuteFileAction(m_hContainer,EMPTY_STRING,szParameters,szItemDirectory,pidlItem);

	CoTaskMemFree(pidlParent);
}

BOOL Explorerplusplus::OnSize(int MainWindowWidth,int MainWindowHeight)
{
	RECT			rc;
	TCITEM			tcItem;
	UINT			uFlags;
	int				IndentBottom = 0;
	int				IndentTop = 0;
	int				IndentLeft = 0;
	int				iIndentRebar = 0;
	int				iHolderWidth;
	int				iHolderHeight;
	int				iHolderTop;
	int				iTabBackingWidth;
	int				iTabBackingLeft;
	int				nTabs;
	int				i = 0;

	if (!m_InitializationFinished)
	{
		return TRUE;
	}

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

	/* <---- Tab control + backing ----> */

	if(m_bExtendTabControl)
	{
		iTabBackingLeft = 0;
		iTabBackingWidth = MainWindowWidth;
	}
	else
	{
		iTabBackingLeft = IndentLeft;
		iTabBackingWidth = MainWindowWidth - IndentLeft;
	}

	uFlags = m_bShowTabBar?SWP_SHOWWINDOW:SWP_HIDEWINDOW;

	int iTabTop;

	if(!m_bShowTabBarAtBottom)
	{
		iTabTop = iIndentRebar;
	}
	else
	{
		iTabTop = MainWindowHeight - IndentBottom - TAB_WINDOW_HEIGHT;
	}

	/* If we're showing the tab bar at the bottom of the listview,
	the only thing that will change is the top coordinate. */
	SetWindowPos(m_hTabBacking,m_hDisplayWindow,iTabBackingLeft,
		iTabTop,iTabBackingWidth,
		TAB_WINDOW_HEIGHT,uFlags);

	SetWindowPos(m_hTabCtrl,NULL,0,0,iTabBackingWidth - 25,
		TAB_WINDOW_HEIGHT,SWP_SHOWWINDOW|SWP_NOZORDER);

	/* Tab close button. */
	SetWindowPos(m_hTabWindowToolbar,NULL,iTabBackingWidth + TAB_TOOLBAR_X_OFFSET,
	TAB_TOOLBAR_Y_OFFSET,TAB_TOOLBAR_WIDTH,TAB_TOOLBAR_HEIGHT,SWP_SHOWWINDOW|SWP_NOZORDER);

	if(m_bExtendTabControl &&
		!m_bShowTabBarAtBottom)
	{
		iHolderTop = IndentTop;
	}
	else
	{
		iHolderTop = iIndentRebar;
	}

	/* <---- Holder window + child windows ----> */

	if(m_bExtendTabControl &&
		m_bShowTabBarAtBottom &&
		m_bShowTabBar)
	{
		iHolderHeight = MainWindowHeight - IndentBottom - iHolderTop - TAB_WINDOW_HEIGHT;
	}
	else
	{
		iHolderHeight = MainWindowHeight - IndentBottom - iHolderTop;
	}

	iHolderWidth = m_TreeViewWidth;

	SetWindowPos(m_hHolder,NULL,0,iHolderTop,
		iHolderWidth,iHolderHeight,SWP_NOZORDER);

	/* The treeview is only slightly smaller than the holder
	window, in both the x and y-directions. */
	SetWindowPos(m_hTreeView,NULL,TREEVIEW_X_CLEARANCE,TREEVIEW_Y_CLEARANCE,
		iHolderWidth - TREEVIEW_HOLDER_CLEARANCE - TREEVIEW_X_CLEARANCE,
		iHolderHeight - TREEVIEW_Y_CLEARANCE,SWP_NOZORDER);

	SetWindowPos(m_hFoldersToolbar,NULL,
		iHolderWidth + FOLDERS_TOOLBAR_X_OFFSET,FOLDERS_TOOLBAR_Y_OFFSET,
		FOLDERS_TOOLBAR_WIDTH,FOLDERS_TOOLBAR_HEIGHT,SWP_SHOWWINDOW|SWP_NOZORDER);


	/* <---- Display window ----> */

	SetWindowPos(m_hDisplayWindow,NULL,0,MainWindowHeight - IndentBottom,
		MainWindowWidth,m_DisplayWindowHeight,SWP_SHOWWINDOW|SWP_NOZORDER);


	/* <---- ALL listview windows ----> */

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	for(i = 0;i < nTabs;i++)
	{
		tcItem.mask = TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);

		uFlags = SWP_NOZORDER;

		if((int)tcItem.lParam == m_selectedTabId)
			uFlags |= SWP_SHOWWINDOW;

		if(!m_bShowTabBarAtBottom)
		{
			SetWindowPos(m_hListView.at((int)tcItem.lParam),NULL,IndentLeft,IndentTop,
				MainWindowWidth - IndentLeft,MainWindowHeight - IndentBottom - IndentTop,
				uFlags);
		}
		else
		{
			if(m_bShowTabBar)
			{
				SetWindowPos(m_hListView.at((int)tcItem.lParam),NULL,IndentLeft,IndentTop,
					MainWindowWidth - IndentLeft,MainWindowHeight - IndentBottom - IndentTop - TAB_WINDOW_HEIGHT,
					uFlags);
			}
			else
			{
				SetWindowPos(m_hListView.at((int)tcItem.lParam),NULL,IndentLeft,IndentTop,
					MainWindowWidth - IndentLeft,MainWindowHeight - IndentBottom - IndentTop,
					uFlags);
			}
		}
	}


	/* <---- Status bar ----> */

	PinStatusBar(m_hStatusBar,MainWindowWidth,MainWindowHeight);
	SetStatusBarParts(MainWindowWidth);


	/* <---- Main rebar + child windows ----> */

	/* Ensure that the main rebar keeps its width in line with the main
	window (its height will not change). */
	MoveWindow(m_hMainRebar,0,0,MainWindowWidth,0,FALSE);

	SetFocus(m_hLastActiveWindow);

	return TRUE;
}

int Explorerplusplus::OnDestroy(void)
{
	if(m_pClipboardDataObject != NULL)
	{
		if(OleIsCurrentClipboard(m_pClipboardDataObject) == S_OK)
		{
			/* Ensure that any data that was copied to the clipboard
			remains there after we exit. */
			OleFlushClipboard();
		}
	}

	if(m_SHChangeNotifyID != 0)
	{
		SHChangeNotifyDeregister(m_SHChangeNotifyID);
	}

	QueueUserAPC(UninitializeCOMAPC,m_hIconThread,NULL);

	ImageList_Destroy(m_himlToolbarSmall);
	ImageList_Destroy(m_himlToolbarLarge);

	delete m_pStatusBar;

	ChangeClipboardChain(m_hContainer,m_hNextClipboardViewer);
	PostQuitMessage(0);

	return 0;
}

int Explorerplusplus::OnClose(void)
{
	if(m_bConfirmCloseTabs && (TabCtrl_GetItemCount(m_hTabCtrl) > 1))
	{
		TCHAR szTemp[128];
		LoadString(m_hLanguageModule,IDS_GENERAL_CLOSE_ALL_TABS,szTemp,SIZEOF_ARRAY(szTemp));
		int response = MessageBox(m_hContainer,szTemp,NExplorerplusplus::APP_NAME,MB_ICONINFORMATION|MB_YESNO);

		/* If the user clicked no, return without
		closing. */
		if(response == IDNO)
			return 1;
	}

	m_iLastSelectedTab = m_selectedTabIndex;

	m_bShowTaskbarThumbnails = m_bShowTaskbarThumbnailsProvisional;

	SaveAllSettings();

	RevokeDragDrop(m_hTabCtrl);

	DestroyWindow(m_hContainer);

	return 0;
}

void Explorerplusplus::OnDirChanged(int iTabId)
{
	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(m_CurrentDirectory),
		m_CurrentDirectory);
	SetCurrentDirectory(m_CurrentDirectory);

	HandleDirectoryMonitoring(iTabId);

	UpdateArrangeMenuItems();

	m_nSelected = 0;

	/* Set the focus back to the first item. */
	ListView_SetItemState(m_hActiveListView,0,LVIS_FOCUSED,LVIS_FOCUSED);

	UpdateWindowStates();

	InvalidateTaskbarThumbnailBitmap(iTabId);

	SetTabIcon();
}

void Explorerplusplus::OnTabCtrlGetDispInfo(LPARAM lParam)
{
	HWND			ToolTipControl;
	LPNMTTDISPINFO	lpnmtdi;
	NMHDR			*nmhdr = NULL;
	static TCHAR	szTabToolTip[512];
	TCITEM			tcItem;

	lpnmtdi = (LPNMTTDISPINFO)lParam;
	nmhdr = &lpnmtdi->hdr;

	ToolTipControl = (HWND)SendMessage(m_hTabCtrl,TCM_GETTOOLTIPS,0,0);

	if(nmhdr->hwndFrom == ToolTipControl)
	{
		tcItem.mask = TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,nmhdr->idFrom,&tcItem);

		m_pShellBrowser[(int)tcItem.lParam]->QueryCurrentDirectory(SIZEOF_ARRAY(szTabToolTip),
			szTabToolTip);
		lpnmtdi->lpszText = szTabToolTip;
	}
}

void Explorerplusplus::OnSetFocus(void)
{
	SetFocus(m_hLastActiveWindow);
}

/*
 * Called when the contents of the clipboard change.
 * All cut items are deghosted, and the 'Paste' button
 * is enabled/disabled.
 */
void Explorerplusplus::OnDrawClipboard(void)
{
	if(m_pClipboardDataObject != NULL)
	{
		if(OleIsCurrentClipboard(m_pClipboardDataObject) == S_FALSE)
		{
			/* Deghost all items that have been 'cut'. */
			for each(auto strFile in m_CutFileNameList)
			{
				/* Only deghost the items if the tab they
				are/were in still exists. */
				if(CheckTabIdStatus(m_iCutTabInternal))
				{
					int iItem = m_pShellBrowser[m_iCutTabInternal]->LocateFileItemIndex(strFile.c_str());

					/* It is possible that the ghosted file
					does NOT exist within the current folder.
					This is the case when (for example), a file
					is cut, and the folder is changed, in which
					case the item is no longer available. */
					if(iItem != -1)
						m_pShellBrowser[m_iCutTabInternal]->DeghostItem(iItem);
				}
			}

			m_CutFileNameList.clear();

			/* Deghost any cut treeview items. */
			if(m_hCutTreeViewItem != NULL)
			{
				TVITEM tvItem;

				tvItem.mask			= TVIF_HANDLE|TVIF_STATE;
				tvItem.hItem		= m_hCutTreeViewItem;
				tvItem.state		= 0;
				tvItem.stateMask	= TVIS_CUT;
				TreeView_SetItem(m_hTreeView,&tvItem);

				m_hCutTreeViewItem = NULL;
			}

			m_pClipboardDataObject->Release();
			m_pClipboardDataObject = NULL;
		}
	}

	SendMessage(m_hMainToolbar,TB_ENABLEBUTTON,(WPARAM)TOOLBAR_PASTE,
		!m_pActiveShellBrowser->InVirtualFolder() && IsClipboardFormatAvailable(CF_HDROP));

	if(m_hNextClipboardViewer != NULL)
	{
		/* Forward the message to the next window in the chain. */
		SendMessage(m_hNextClipboardViewer, WM_DRAWCLIPBOARD, 0, 0);
	}
}

/*
 * Called when the clipboard chain is changed (i.e. a window
 * is added/removed).
 */
void Explorerplusplus::OnChangeCBChain(WPARAM wParam,LPARAM lParam)
{
	if((HWND)wParam == m_hNextClipboardViewer)
		m_hNextClipboardViewer = (HWND)lParam;
	else if(m_hNextClipboardViewer != NULL)
		SendMessage(m_hNextClipboardViewer,WM_CHANGECBCHAIN,wParam,lParam);
}

void Explorerplusplus::HandleDirectoryMonitoring(int iTabId)
{
	DirectoryAltered_t	*pDirectoryAltered = NULL;
	TCHAR				szDirectoryToWatch[MAX_PATH];
	int					iDirMonitorId;

	iDirMonitorId		= m_pShellBrowser[iTabId]->GetDirMonitorId();
			
	/* Stop monitoring the directory that was browsed from. */
	m_pDirMon->StopDirectoryMonitor(iDirMonitorId);

	m_pShellBrowser[iTabId]->QueryCurrentDirectory(SIZEOF_ARRAY(szDirectoryToWatch),
		szDirectoryToWatch);

	/* Don't watch virtual folders (the 'recycle bin' may be an
	exception to this). */
	if(m_pShellBrowser[iTabId]->InVirtualFolder())
	{
		iDirMonitorId = -1;
	}
	else
	{
		pDirectoryAltered = (DirectoryAltered_t *)malloc(sizeof(DirectoryAltered_t));

		pDirectoryAltered->iIndex		= iTabId;
		pDirectoryAltered->iFolderIndex	= m_pShellBrowser[iTabId]->GetFolderIndex();
		pDirectoryAltered->pData		= this;

		/* Start monitoring the directory that was opened. */
		LOG(debug) << _T("Starting directory monitoring for \"") << szDirectoryToWatch << _T("\"");
		iDirMonitorId = m_pDirMon->WatchDirectory(szDirectoryToWatch,FILE_NOTIFY_CHANGE_FILE_NAME|
			FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_ATTRIBUTES|
			FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_LAST_ACCESS|FILE_NOTIFY_CHANGE_CREATION|
			FILE_NOTIFY_CHANGE_SECURITY,DirectoryAlteredCallback,FALSE,(void *)pDirectoryAltered);
	}

	m_pShellBrowser[iTabId]->SetDirMonitorId(iDirMonitorId);
}

void Explorerplusplus::OnTbnDropDown(LPARAM lParam)
{
	NMTOOLBAR		*nmTB = NULL;
	LPITEMIDLIST	pidl = NULL;
	POINT			ptOrigin;
	RECT			rc;
	HRESULT			hr;

	nmTB = (NMTOOLBAR *)lParam;

	GetWindowRect(m_hMainToolbar,&rc);

	ptOrigin.x = rc.left;
	ptOrigin.y = rc.bottom - 4;

	if(nmTB->iItem == TOOLBAR_BACK)
	{
		hr = m_pActiveShellBrowser->CreateHistoryPopup(m_hContainer,&pidl,&ptOrigin,TRUE);

		if(SUCCEEDED(hr))
		{
			BrowseFolder(pidl,SBSP_ABSOLUTE|SBSP_WRITENOHISTORY);

			CoTaskMemFree(pidl);
		}
	}
	else if(nmTB->iItem == TOOLBAR_FORWARD)
	{
		SendMessage(m_hMainToolbar,TB_GETRECT,(WPARAM)TOOLBAR_BACK,(LPARAM)&rc);

		ptOrigin.x += rc.right;

		hr = m_pActiveShellBrowser->CreateHistoryPopup(m_hContainer,&pidl,&ptOrigin,FALSE);

		if(SUCCEEDED(hr))
		{
			BrowseFolder(pidl,SBSP_ABSOLUTE|SBSP_WRITENOHISTORY);

			CoTaskMemFree(pidl);
		}
	}
	else if(nmTB->iItem == TOOLBAR_VIEWS)
	{
		ShowToolbarViewsDropdown();
	}
}

void Explorerplusplus::OnDisplayWindowResized(WPARAM wParam)
{
	RECT	rc;

	if((int)wParam >= MINIMUM_DISPLAYWINDOW_HEIGHT)
		m_DisplayWindowHeight = (int)wParam;

	GetClientRect(m_hContainer,&rc);

	SendMessage(m_hContainer,WM_SIZE,SIZE_RESTORED,(LPARAM)MAKELPARAM(rc.right,rc.bottom));
}

void Explorerplusplus::OnStartedBrowsing(int iTabId, const TCHAR *szFolderPath)
{
	TCHAR	szLoadingText[512];

	if(iTabId == m_selectedTabId)
	{
		TCHAR szTemp[64];
		LoadString(m_hLanguageModule,IDS_GENERAL_LOADING,szTemp,SIZEOF_ARRAY(szTemp));
		StringCchPrintf(szLoadingText,SIZEOF_ARRAY(szLoadingText),szTemp,szFolderPath);

		/* Browsing of a folder has started. Set the status bar text to indicate that
		the folder is been loaded. */
		SendMessage(m_hStatusBar,SB_SETTEXT,(WPARAM)0|0,(LPARAM)szLoadingText);

		/* Clear the text in all other parts of the status bar. */
		SendMessage(m_hStatusBar,SB_SETTEXT,(WPARAM)1|0,(LPARAM)EMPTY_STRING);
		SendMessage(m_hStatusBar,SB_SETTEXT,(WPARAM)2|0,(LPARAM)EMPTY_STRING);
	}
}

/*
 * Sizes all columns in the active listview
 * based on their text.
 */
void Explorerplusplus::OnAutoSizeColumns(void)
{
	size_t	nColumns;
	UINT	iCol = 0;

	nColumns = m_pActiveShellBrowser->QueryNumActiveColumns();

	for(iCol = 0;iCol < nColumns;iCol++)
	{
		ListView_SetColumnWidth(m_hActiveListView,iCol,LVSCW_AUTOSIZE);
	}
}

/* Cycle through the current views. */
void Explorerplusplus::OnToolbarViews(void)
{
	CycleViewState(TRUE);
}

void Explorerplusplus::CycleViewState(BOOL bCycleForward)
{
	UINT uViewMode = m_pActiveShellBrowser->GetCurrentViewMode();

	std::list<UINT>::iterator itr;

	for(itr = m_ViewModes.begin();itr != m_ViewModes.end();itr++)
	{
		if(*itr == uViewMode)
			break;
	}

	if(bCycleForward)
	{
		std::list<UINT>::iterator itrEnd;

		itrEnd = m_ViewModes.end();
		itrEnd--;

		if(itr == itrEnd)
			itr = m_ViewModes.begin();
		else
			itr++;
	}
	else
	{
		if(itr == m_ViewModes.begin())
		{
			itr = m_ViewModes.end();
			itr--;
		}
		else
		{
			itr--;
		}
	}

	m_pActiveShellBrowser->SetCurrentViewMode(*itr);
}

void Explorerplusplus::ShowToolbarViewsDropdown(void)
{
	POINT	ptOrigin;
	RECT	rcButton;

	SendMessage(m_hMainToolbar,TB_GETRECT,(WPARAM)TOOLBAR_VIEWS,(LPARAM)&rcButton);

	ptOrigin.x	= rcButton.left;
	ptOrigin.y	= rcButton.bottom;

	ClientToScreen(m_hMainToolbar,&ptOrigin);

	CreateViewsMenu(&ptOrigin);
}

void Explorerplusplus::OnSortByAscending(BOOL bSortAscending)
{
	if(bSortAscending != m_pActiveShellBrowser->GetSortAscending())
	{
		m_pActiveShellBrowser->SetSortAscending(bSortAscending);

		UINT SortMode = m_pActiveShellBrowser->GetSortMode();

		/* It is quicker to re-sort the folder than refresh it. */
		m_pActiveShellBrowser->SortFolder(SortMode);
	}
}

void Explorerplusplus::OnPreviousWindow(void)
{
	if(m_bListViewRenaming)
	{
		SendMessage(ListView_GetEditControl(m_hActiveListView),
			WM_APP_KEYDOWN, VK_TAB, 0);
	}
	else
	{
		HWND hFocus = GetFocus();

		if(hFocus == m_hActiveListView)
		{
			if(m_bShowFolders)
			{
				SetFocus(m_hTreeView);
			}
			else
			{
				if(m_bShowAddressBar)
				{
					SetFocus(m_hAddressBar);
				}
			}
		}
		else if(hFocus == m_hTreeView)
		{
			if(m_bShowAddressBar)
			{
				SetFocus(m_hAddressBar);
			}
			else
			{
				/* Always shown. */
				SetFocus(m_hActiveListView);
			}
		}
		else if(hFocus == (HWND) SendMessage(m_hAddressBar, CBEM_GETEDITCONTROL, 0, 0))
		{
			/* Always shown. */
			SetFocus(m_hActiveListView);
		}
	}
}

/*
 * Shifts focus to the next internal
 * window in the chain.
 */
void Explorerplusplus::OnNextWindow(void)
{
	if(m_bListViewRenaming)
	{
		SendMessage(ListView_GetEditControl(m_hActiveListView),
			WM_APP_KEYDOWN,VK_TAB,0);
	}
	else
	{
		HWND hFocus = GetFocus();

		/* Check if the next target window is visible.
		If it is, select it, else select the next
		window in the chain. */
		if(hFocus == m_hActiveListView)
		{
			if(m_bShowAddressBar)
			{
				SetFocus(m_hAddressBar);
			}
			else
			{
				if(m_bShowFolders)
				{
					SetFocus(m_hTreeView);
				}
			}
		}
		else if(hFocus == m_hTreeView)
		{
			/* Always shown. */
			SetFocus(m_hActiveListView);
		}
		else if(hFocus == (HWND)SendMessage(m_hAddressBar,CBEM_GETEDITCONTROL,0,0))
		{
			if(m_bShowFolders)
			{
				SetFocus(m_hTreeView);
			}
			else
			{
				SetFocus(m_hActiveListView);
			}
		}
	}
}

void Explorerplusplus::SetGoMenuName(HMENU hMenu,UINT uMenuID,UINT csidl)
{
	MENUITEMINFO	mii;
	LPITEMIDLIST	pidl = NULL;
	TCHAR			szFolderName[MAX_PATH];
	HRESULT			hr;

	hr = SHGetFolderLocation(NULL,csidl,NULL,0,&pidl);

	/* Don't use SUCCEEDED(hr). */
	if(hr == S_OK)
	{
		GetDisplayName(pidl,szFolderName,SIZEOF_ARRAY(szFolderName),SHGDN_INFOLDER);

		mii.cbSize		= sizeof(mii);
		mii.fMask		= MIIM_STRING;
		mii.dwTypeData	= szFolderName;
		SetMenuItemInfo(hMenu,uMenuID,FALSE,&mii);

		CoTaskMemFree(pidl);
	}
	else
	{
		DeleteMenu(hMenu,uMenuID,MF_BYCOMMAND);
	}
}

void Explorerplusplus::OnLockToolbars(void)
{
	REBARBANDINFO	rbbi;
	UINT			nBands;
	UINT			i = 0;

	m_bLockToolbars = !m_bLockToolbars;

	nBands = (UINT)SendMessage(m_hMainRebar,RB_GETBANDCOUNT,0,0);

	for(i = 0;i < nBands;i++)
	{
		/* First, retrieve the current style for this band. */
		rbbi.cbSize	= sizeof(REBARBANDINFO);
		rbbi.fMask	= RBBIM_STYLE;
		SendMessage(m_hMainRebar,RB_GETBANDINFO,i,(LPARAM)&rbbi);

		/* Add the gripper style. */
		AddGripperStyle(&rbbi.fStyle,!m_bLockToolbars);

		/* Now, set the new style. */
		SendMessage(m_hMainRebar,RB_SETBANDINFO,i,(LPARAM)&rbbi);
	}

	/* If the rebar is locked, prevent items from
	been rearranged. */
	AddWindowStyle(m_hMainRebar,RBS_FIXEDORDER,m_bLockToolbars);
}

void Explorerplusplus::OnShellNewItemCreated(LPARAM lParam)
{
	HWND	hEdit;
	int		iRenamedItem;

	iRenamedItem = (int)lParam;

	if(iRenamedItem != -1)
	{
		/* Start editing the label for this item. */
		hEdit = ListView_EditLabel(m_hActiveListView,iRenamedItem);
	}
}

void Explorerplusplus::OnAppCommand(UINT cmd)
{
	switch(cmd)
	{
	case APPCOMMAND_BROWSER_BACKWARD:
		/* This will cancel any menu that may be shown
		at the moment. */
		SendMessage(m_hContainer,WM_CANCELMODE,0,0);

		OnBrowseBack();
		break;

	case APPCOMMAND_BROWSER_FORWARD:
		SendMessage(m_hContainer,WM_CANCELMODE,0,0);
		OnBrowseForward();
		break;

	case APPCOMMAND_BROWSER_HOME:
		OnHome();
		break;

	case APPCOMMAND_BROWSER_FAVORITES:
		break;

	case APPCOMMAND_BROWSER_REFRESH:
		SendMessage(m_hContainer,WM_CANCELMODE,0,0);
		OnRefresh();
		break;

	case APPCOMMAND_BROWSER_SEARCH:
		OnSearch();
		break;

	case APPCOMMAND_CLOSE:
		SendMessage(m_hContainer,WM_CANCELMODE,0,0);
		OnCloseTab();
		break;

	case APPCOMMAND_COPY:
		OnCopy(TRUE);
		break;

	case APPCOMMAND_CUT:
		OnCopy(FALSE);
		break;

	case APPCOMMAND_HELP:
		OnShowHelp();
		break;

	case APPCOMMAND_NEW:
		break;

	case APPCOMMAND_PASTE:
		OnPaste();
		break;

	case APPCOMMAND_UNDO:
		m_FileActionHandler.Undo();
		break;

	case APPCOMMAND_REDO:
		break;
	}
}

void Explorerplusplus::OnRefresh(void)
{
	RefreshTab(m_selectedTabId);
}

void Explorerplusplus::CopyColumnInfoToClipboard(void)
{
	std::list<Column_t> Columns;
	m_pActiveShellBrowser->ExportCurrentColumns(&Columns);

	std::wstring strColumnInfo;
	int nActiveColumns = 0;

	for each(auto Column in Columns)
	{
		if(Column.bChecked)
		{
			TCHAR szText[64];
			LoadString(m_hLanguageModule,LookupColumnNameStringIndex(Column.id),szText,SIZEOF_ARRAY(szText));

			strColumnInfo += std::wstring(szText) + _T("\t");

			nActiveColumns++;
		}
	}

	/* Remove the trailing tab. */
	strColumnInfo = strColumnInfo.substr(0,strColumnInfo.size() - 1);

	strColumnInfo += _T("\r\n");

	int iItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
	{
		for(int i = 0;i < nActiveColumns;i++)
		{
			TCHAR szText[64];
			ListView_GetItemText(m_hActiveListView,iItem,i,szText,
				SIZEOF_ARRAY(szText));

			strColumnInfo += std::wstring(szText) + _T("\t");
		}

		strColumnInfo = strColumnInfo.substr(0,strColumnInfo.size() - 1);

		strColumnInfo += _T("\r\n");
	}

	/* Remove the trailing newline. */
	strColumnInfo = strColumnInfo.substr(0,strColumnInfo.size() - 2);

	CopyTextToClipboard(strColumnInfo);
}

void Explorerplusplus::SetFilterStatus(void)
{
	m_pActiveShellBrowser->SetFilterStatus(!m_pActiveShellBrowser->GetFilterStatus());
}

void Explorerplusplus::OnDirectoryModified(int iTabId)
{
	/* This message is sent when one of the
	tab directories is modified.
	Two cases to handle:
	 1. Tab that sent the notification DOES NOT
	    have focus.
	 2. Tab that sent the notification DOES have
	    focus.

	Case 1 (Tab DOES NOT have focus):
	No updates will be applied. When the tab
	selection changes to the updated tab, the
	view will be synchronized anyhow (since all
	windows are updated when the tab selection
	changes).

	Case 2 (Tab DOES have focus):
	In this case, only the following updates
	need to be applied:
	 - Updated status bar text
	 - Handle file selection display (i.e. update
	   the display window)
	*/

	if(iTabId == m_selectedTabId)
	{
		UpdateStatusBarText();
		UpdateDisplayWindow();
	}
}

void Explorerplusplus::OnIdaRClick(void)
{
	/* Show the context menu (if any)
	for the window that currently has
	the focus.
	Note: The edit box within the address
	bar already handles the r-click menu
	key. */

	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		/* The behaviour of the listview is
		slightly different when compared to
		normal right-clicking.
		If any item(s) in the listview are
		selected when they key is pressed,
		the context menu for those items will
		be shown, rather than the background
		context menu.
		The context menu will be anchored to
		the item that currently has selection.
		If no item is selected, the background
		context menu will be shown (and anchored
		at the current mouse position). */
		POINT ptMenuOrigin = {0,0};

		/* If no items are selected, pass the current mouse
		position. If items are selected, take the one with
		focus, and pass its center point. */
		if(ListView_GetSelectedCount(m_hActiveListView) == 0)
		{
			GetCursorPos(&ptMenuOrigin);
		}
		else
		{
			HIMAGELIST himl;
			POINT ptItem;
			UINT uViewMode;
			int iItem;
			int cx;
			int cy;

			iItem = ListView_GetNextItem(m_hActiveListView,-1,LVNI_FOCUSED);

			if(iItem != -1)
			{
				ListView_GetItemPosition(m_hActiveListView,iItem,&ptItem);

				ClientToScreen(m_hActiveListView,&ptItem);

				uViewMode = m_pActiveShellBrowser->GetCurrentViewMode();

				if(uViewMode == VM_SMALLICONS || uViewMode == VM_LIST ||
					uViewMode == VM_DETAILS)
					himl = ListView_GetImageList(m_hActiveListView,LVSIL_SMALL);
				else
					himl = ListView_GetImageList(m_hActiveListView,LVSIL_NORMAL);

				ImageList_GetIconSize(himl,&cx,&cy);

				/* DON'T free the image list. */

				/* The origin of the menu will be fixed at the centre point
				of the items icon. */
				ptMenuOrigin.x = ptItem.x + cx / 2;
				ptMenuOrigin.y = ptItem.y + cy / 2;
			}
		}

		OnListViewRClick(&ptMenuOrigin);
	}
	else if(hFocus == m_hTreeView)
	{
		HTREEITEM hSelection;
		RECT rcItem;
		POINT ptOrigin;

		hSelection = TreeView_GetSelection(m_hTreeView);

		TreeView_GetItemRect(m_hTreeView,hSelection,&rcItem,TRUE);

		ptOrigin.x = rcItem.left;
		ptOrigin.y = rcItem.top;

		ClientToScreen(m_hTreeView,&ptOrigin);

		ptOrigin.y += (rcItem.bottom - rcItem.top) / 2;

		if(hSelection != NULL)
		{
			OnTreeViewRightClick((WPARAM)hSelection,(LPARAM)&ptOrigin);
		}
	}
}

/* A file association has changed. Rather
than refreshing all tabs, just find all
icons again.

To refresh system image list:
1. Call FileIconInit(TRUE)
2. Change "Shell Icon Size" in "Control Panel\\Desktop\\WindowMetrics"
3. Call FileIconInit(FALSE)

Note that refreshing the system image list affects
the WHOLE PROGRAM. This means that the treeview
needs to have its icons refreshed as well.

References:
http://tech.groups.yahoo.com/group/wtl/message/13911
http://www.eggheadcafe.com/forumarchives/platformsdkshell/Nov2005/post24294253.asp
*/
void Explorerplusplus::OnAssocChanged(void)
{
	typedef BOOL (WINAPI *FII_PROC)(BOOL);
	FII_PROC FileIconInit;
	HKEY hKey;
	TCITEM tcItem;
	HMODULE hShell32;
	TCHAR szShellIconSize[32];
	TCHAR szTemp[32];
	DWORD dwShellIconSize;
	LONG res;
	int i = 0;
	int nTabs;
	int iIndex;

	hShell32 = LoadLibrary(_T("shell32.dll"));

	FileIconInit = (FII_PROC)GetProcAddress(hShell32,(LPCSTR)660);

	res = RegOpenKeyEx(HKEY_CURRENT_USER,
		_T("Control Panel\\Desktop\\WindowMetrics"),
		0,KEY_READ|KEY_WRITE,&hKey);

	if(res == ERROR_SUCCESS)
	{
		NRegistrySettings::ReadStringFromRegistry(hKey,_T("Shell Icon Size"),
			szShellIconSize,SIZEOF_ARRAY(szShellIconSize));

		dwShellIconSize = _wtoi(szShellIconSize);

		/* Increment the value by one, and save it back to the registry. */
		StringCchPrintf(szTemp,SIZEOF_ARRAY(szTemp),_T("%d"),dwShellIconSize + 1);
		NRegistrySettings::SaveStringToRegistry(hKey,_T("Shell Icon Size"),szTemp);

		if(FileIconInit != NULL)
			FileIconInit(TRUE);

		/* Now, set it back to the original value. */
		NRegistrySettings::SaveStringToRegistry(hKey,_T("Shell Icon Size"),szShellIconSize);

		if(FileIconInit != NULL)
			FileIconInit(FALSE);

		RegCloseKey(hKey);
	}

	/* DO NOT free shell32.dll. Doing so will release
	the image lists (among other things). */

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	/* When the system image list is refresh, ALL previous
	icons will be discarded. This means that SHGetFileInfo()
	needs to be called to get each files icon again. */

	/* Now, go through each tab, and refresh each icon. */
	for(i = 0;i < nTabs;i++)
	{
		tcItem.mask = TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);

		iIndex = (int)tcItem.lParam;

		m_pShellBrowser[iIndex]->RefreshAllIcons();
	}

	/* Now, refresh the treeview. */
	m_pMyTreeView->RefreshAllIcons();

	/* Address bar. */
	UpdateAddressBarText();
}

void Explorerplusplus::OnCloneWindow(void)
{
	TCHAR szCurrentDirectory[MAX_PATH];
	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(szCurrentDirectory),
		szCurrentDirectory);

	TCHAR szQuotedCurrentDirectory[MAX_PATH];
	StringCchPrintf(szQuotedCurrentDirectory, SIZEOF_ARRAY(szQuotedCurrentDirectory),
		_T("\"%s\""),szCurrentDirectory);

	ExecuteAndShowCurrentProcess(m_hContainer, szQuotedCurrentDirectory);
}

void Explorerplusplus::ShowMainRebarBand(HWND hwnd,BOOL bShow)
{
	REBARBANDINFO rbi;
	LRESULT lResult;
	UINT nBands;
	UINT i = 0;

	nBands = (UINT)SendMessage(m_hMainRebar,RB_GETBANDCOUNT,0,0);

	for(i = 0;i < nBands;i++)
	{
		rbi.cbSize	= sizeof(rbi);
		rbi.fMask	= RBBIM_CHILD;
		lResult = SendMessage(m_hMainRebar,RB_GETBANDINFO,i,(LPARAM)&rbi);

		if(lResult)
		{
			if(hwnd == rbi.hwndChild)
			{
				SendMessage(m_hMainRebar,RB_SHOWBAND,i,bShow);
				break;
			}
		}
	}
}

void Explorerplusplus::OnNdwIconRClick(POINT *pt)
{
	POINT ptCopy = *pt;
	ClientToScreen(m_hDisplayWindow,&ptCopy);
	OnListViewRClick(&ptCopy);
}

void Explorerplusplus::OnNdwRClick(POINT *pt)
{
	HMENU hMenu = LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_DISPLAYWINDOW_RCLICK));

	if(hMenu != NULL)
	{
		HMENU hPopupMenu = GetSubMenu(hMenu, 0);

		if(hPopupMenu != NULL)
		{
			POINT ptCopy = *pt;
			BOOL bRes = ClientToScreen(m_hDisplayWindow, &ptCopy);

			if(bRes)
			{
				TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERTICAL,
					ptCopy.x, ptCopy.y, 0, m_hContainer, NULL);
			}
		}

		DestroyMenu(hMenu);
	}
}

LRESULT Explorerplusplus::OnCustomDraw(LPARAM lParam)
{
	NMLVCUSTOMDRAW *pnmlvcd = NULL;
	NMCUSTOMDRAW *pnmcd = NULL;

	pnmlvcd = (NMLVCUSTOMDRAW *)lParam;

	if(pnmlvcd->nmcd.hdr.hwndFrom == m_hActiveListView)
	{
		pnmcd = &pnmlvcd->nmcd;

		switch(pnmcd->dwDrawStage)
		{
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT:
			{
				DWORD dwAttributes = m_pActiveShellBrowser->QueryFileAttributes(static_cast<int>(pnmcd->dwItemSpec));

				TCHAR szFileName[MAX_PATH];
				m_pActiveShellBrowser->QueryFullItemName(static_cast<int>(pnmcd->dwItemSpec),szFileName,SIZEOF_ARRAY(szFileName));
				PathStripPath(szFileName);

				/* Loop through each filter. Decide whether to change the font of the
				current item based on its filename and/or attributes. */
				for each(auto ColorRule in m_ColorRules)
				{
					BOOL bMatchFileName = FALSE;
					BOOL bMatchAttributes = FALSE;

					/* Only match against the filename if it's not empty. */
					if(ColorRule.strFilterPattern.size() > 0)
					{
						if(CheckWildcardMatch(ColorRule.strFilterPattern.c_str(),szFileName,!ColorRule.caseInsensitive) == 1)
						{
							bMatchFileName = TRUE;
						}
					}
					else
					{
						bMatchFileName = TRUE;
					}

					if(ColorRule.dwFilterAttributes != 0)
					{
						if(ColorRule.dwFilterAttributes & dwAttributes)
						{
							bMatchAttributes = TRUE;
						}
					}
					else
					{
						bMatchAttributes = TRUE;
					}

					if(bMatchFileName && bMatchAttributes)
					{
						pnmlvcd->clrText = ColorRule.rgbColour;
						return CDRF_NEWFONT;
					}
				}
			}
			break;
		}

		return CDRF_NOTIFYITEMDRAW;
	}

	return 0;
}

int Explorerplusplus::GetViewModeMenuId(UINT uViewMode)
{
	switch(uViewMode)
	{
		case VM_THUMBNAILS:
			return IDM_VIEW_THUMBNAILS;
			break;

		case VM_TILES:
			return IDM_VIEW_TILES;
			break;

		case VM_EXTRALARGEICONS:
			return IDM_VIEW_EXTRALARGEICONS;
			break;

		case VM_LARGEICONS:
			return IDM_VIEW_LARGEICONS;
			break;

		case VM_ICONS:
			return IDM_VIEW_ICONS;
			break;

		case VM_SMALLICONS:
			return IDM_VIEW_SMALLICONS;
			break;

		case VM_LIST:
			return IDM_VIEW_LIST;
			break;

		case VM_DETAILS:
			return IDM_VIEW_DETAILS;
			break;
	}

	return -1;
}

int Explorerplusplus::GetViewModeMenuStringId(UINT uViewMode)
{
	switch(uViewMode)
	{
		case VM_THUMBNAILS:
			return IDS_VIEW_THUMBNAILS;
			break;

		case VM_TILES:
			return IDS_VIEW_TILES;
			break;

		case VM_EXTRALARGEICONS:
			return IDS_VIEW_EXTRALARGEICONS;
			break;

		case VM_LARGEICONS:
			return IDS_VIEW_LARGEICONS;
			break;

		case VM_ICONS:
			return IDS_VIEW_MEDIUMICONS;
			break;

		case VM_SMALLICONS:
			return IDS_VIEW_SMALLICONS;
			break;

		case VM_LIST:
			return IDS_VIEW_LIST;
			break;

		case VM_DETAILS:
			return IDS_VIEW_DETAILS;
			break;
	}

	return -1;
}

void Explorerplusplus::OnSortBy(UINT uSortMode)
{
	UINT uCurrentSortMode = m_pActiveShellBrowser->GetSortMode();

	if(!m_pActiveShellBrowser->IsGroupViewEnabled() &&
		uSortMode == uCurrentSortMode)
	{
		m_pActiveShellBrowser->ToggleSortAscending();
	}
	else if(m_pActiveShellBrowser->IsGroupViewEnabled())
	{
		m_pActiveShellBrowser->SetGrouping(FALSE);
	}

	m_pActiveShellBrowser->SortFolder(uSortMode);
}

void Explorerplusplus::OnGroupBy(UINT uSortMode)
{
	UINT uCurrentSortMode = m_pActiveShellBrowser->GetSortMode();

	/* If group view is already enabled, and the current sort
	mode matches the supplied sort mode, toggle the ascending/
	descending flag. */
	if(m_pActiveShellBrowser->IsGroupViewEnabled() &&
		uSortMode == uCurrentSortMode)
	{
		m_pActiveShellBrowser->ToggleSortAscending();
	}
	else if(!m_pActiveShellBrowser->IsGroupViewEnabled())
	{
		m_pActiveShellBrowser->SetGroupingFlag(TRUE);
	}

	m_pActiveShellBrowser->SortFolder(uSortMode);
}

void Explorerplusplus::SaveAllSettings(void)
{
	ILoadSave *pLoadSave = NULL;

	if(m_bSavePreferencesToXMLFile)
		pLoadSave = new CLoadSaveXML(this,FALSE);
	else
		pLoadSave = new CLoadSaveRegistry(this);

	pLoadSave->SaveGenericSettings();
	pLoadSave->SaveTabs();
	pLoadSave->SaveDefaultColumns();
	pLoadSave->SaveBookmarks();
	pLoadSave->SaveApplicationToolbar();
	pLoadSave->SaveToolbarInformation();
	pLoadSave->SaveColorRules();
	pLoadSave->SaveDialogStates();

	delete pLoadSave;
}

/* Saves directory settings for a particular
tab. */
void Explorerplusplus::SaveDirectorySpecificSettings(int iTab)
{
	TCITEM tcItem;
	BOOL bRet;

	tcItem.mask = TCIF_PARAM;
	bRet = TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

	if(bRet)
	{
		int iIndexInternal = (int)tcItem.lParam;

		DirectorySettings_t ds;

		/* TODO: First check if there are already settings held for this
		tab. If there are, delete them first. */

		ds.pidlDirectory = m_pShellBrowser[iIndexInternal]->QueryCurrentDirectoryIdl();

		ds.dsi.SortMode = m_pShellBrowser[iIndexInternal]->GetSortMode();
		ds.dsi.ViewMode = m_pShellBrowser[iIndexInternal]->GetCurrentViewMode();

		ColumnExport_t ce;

		m_pShellBrowser[iIndexInternal]->ExportAllColumns(&ce);

		ds.dsi.ControlPanelColumnList		= ce.ControlPanelColumnList;
		ds.dsi.MyComputerColumnList			= ce.MyComputerColumnList;
		ds.dsi.MyNetworkPlacesColumnList	= ce.MyNetworkPlacesColumnList;
		ds.dsi.NetworkConnectionsColumnList	= ce.NetworkConnectionsColumnList;
		ds.dsi.PrintersColumnList			= ce.PrintersColumnList;
		ds.dsi.RealFolderColumnList			= ce.RealFolderColumnList;
		ds.dsi.RecycleBinColumnList			= ce.RecycleBinColumnList;

		m_DirectorySettingsList.push_back(ds);
	}
}

/* TODO: This needs to be moved into the actual shell browser. Can't change
settings until it's known that the folder has successfully changed. */
void Explorerplusplus::SetDirectorySpecificSettings(int iTab,LPITEMIDLIST pidlDirectory)
{
	if(m_DirectorySettingsList.size() > 0)
	{
		for each(auto ds in m_DirectorySettingsList)
		{
			if(CompareIdls(pidlDirectory,ds.pidlDirectory))
			{
				TCITEM tcItem;
				BOOL bRet;

				tcItem.mask = TCIF_PARAM;
				bRet = TabCtrl_GetItem(m_hTabCtrl,iTab,&tcItem);

				if(bRet)
				{
					int iIndexInternal = (int)tcItem.lParam;

					m_pShellBrowser[iIndexInternal]->SetSortMode(ds.dsi.SortMode);
					m_pShellBrowser[iIndexInternal]->SetCurrentViewMode(ds.dsi.ViewMode);

					ColumnExport_t ce;

					ce.ControlPanelColumnList = ds.dsi.ControlPanelColumnList;
					ce.MyComputerColumnList = ds.dsi.MyComputerColumnList;
					ce.MyNetworkPlacesColumnList = ds.dsi.MyNetworkPlacesColumnList;
					ce.NetworkConnectionsColumnList = ds.dsi.NetworkConnectionsColumnList;
					ce.PrintersColumnList = ds.dsi.PrintersColumnList;
					ce.RealFolderColumnList = ds.dsi.RealFolderColumnList;
					ce.RecycleBinColumnList = ds.dsi.RecycleBinColumnList;

					m_pShellBrowser[iIndexInternal]->ImportAllColumns(&ce);
				}
			}
		}
	}
}

HWND Explorerplusplus::GetActiveListView() const
{
	return m_hActiveListView;
}

CShellBrowser *Explorerplusplus::GetActiveShellBrowser() const
{
	return m_pActiveShellBrowser;
}

void Explorerplusplus::OnShowHiddenFiles(void)
{
	m_pActiveShellBrowser->ToggleShowHidden();

	RefreshTab(m_selectedTabId);
}