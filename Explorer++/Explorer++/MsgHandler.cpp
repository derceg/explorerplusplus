// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AddressBar.h"
#include "ColorRuleHelper.h"
#include "Config.h"
#include "Explorer++_internal.h"
#include "LoadSaveRegistry.h"
#include "LoadSaveXml.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "Navigation.h"
#include "Plugins/PluginManager.h"
#include "ResourceHelper.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/ViewModes.h"
#include "ShellTreeView/ShellTreeView.h"
#include "TabContainer.h"
#include "ToolbarButtons.h"
#include "ViewModeHelper.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/Controls.h"
#include "../Helper/FileOperations.h"
#include "../Helper/iDirectoryMonitor.h"
#include "../Helper/Logging.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/RegistrySettings.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include <boost/range/adaptor/map.hpp>
#include <wil/resource.h>
#include <algorithm>

/* The treeview is offset by a small
amount on the left. */
static const int TREEVIEW_X_CLEARANCE = 1;

/* The spacing between the right edge of
the treeview and the holder window. */
static const int TREEVIEW_HOLDER_CLEARANCE = 4;

const int CLOSE_TOOLBAR_WIDTH = 24;
const int CLOSE_TOOLBAR_HEIGHT = 24;
const int CLOSE_TOOLBAR_X_OFFSET = 4;
const int CLOSE_TOOLBAR_Y_OFFSET = 1;

void Explorerplusplus::TestConfigFile()
{
	m_bLoadSettingsFromXML = TestConfigFileInternal();
}

BOOL TestConfigFileInternal()
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

	hConfigFile = CreateFile(szConfigFile,GENERIC_READ,FILE_SHARE_READ, nullptr,
		OPEN_EXISTING,0, nullptr);

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
		*pLoadSave = new LoadSaveXML(this,TRUE);

		/* When loading from the config file, also
		set the option to save back to it on exit. */
		m_bSavePreferencesToXMLFile = TRUE;
	}
	else
	{
		*pLoadSave = new LoadSaveRegistry(this);
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
	unique_pidl_absolute pidlItem;
	HRESULT hr = SHParseDisplayName(szItem, nullptr, wil::out_param(pidlItem), 0, nullptr);

	if(SUCCEEDED(hr))
	{
		OpenItem(pidlItem.get(),bOpenInNewTab,bOpenInNewWindow);
	}
}

void Explorerplusplus::OpenItem(PCIDLIST_ABSOLUTE pidlItem, BOOL bOpenInNewTab, BOOL bOpenInNewWindow)
{
	BOOL bControlPanelParent = FALSE;

	unique_pidl_absolute pidlControlPanel;
	HRESULT hr = SHGetFolderLocation(nullptr,CSIDL_CONTROLS, nullptr,0,wil::out_param(pidlControlPanel));

	if(SUCCEEDED(hr))
	{
		/* Check if the parent of the item is the control panel.
		If it is, pass it to the shell to open, rather than
		opening it in-place. */
		if(ILIsParent(pidlControlPanel.get(),pidlItem,FALSE) &&
			!CompareIdls(pidlControlPanel.get(),pidlItem))
		{
			bControlPanelParent = TRUE;
		}
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
		unique_pidl_absolute pidlControlPanelCategoryView;
		hr = SHParseDisplayName(CONTROL_PANEL_CATEGORY_VIEW, nullptr, wil::out_param(pidlControlPanelCategoryView), 0, nullptr);

		if (SUCCEEDED(hr))
		{
			/* Check if the parent of the item is the control panel.
			If it is, pass it to the shell to open, rather than
			opening it in-place. */
			if (ILIsParent(pidlControlPanelCategoryView.get(), pidlItem, FALSE) &&
				!CompareIdls(pidlControlPanelCategoryView.get(), pidlItem))
			{
				bControlPanelParent = TRUE;
			}
		}
	}

	SFGAOF uAttributes = SFGAO_FOLDER | SFGAO_STREAM | SFGAO_LINK;
	hr = GetItemAttributes(pidlItem,&uAttributes);

	if(SUCCEEDED(hr))
	{
		if((uAttributes & SFGAO_FOLDER) && (uAttributes & SFGAO_STREAM))
		{
			/* Zip file. */
			if(m_config->handleZipFiles)
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
						((uAttributes & SFGAO_FOLDER) && (uAttributes & SFGAO_STREAM) && m_config->handleZipFiles))
					{
						unique_pidl_absolute pidlTarget;
						hr = SHParseDisplayName(szTargetPath, nullptr, wil::out_param(pidlTarget), 0, nullptr);

						if(SUCCEEDED(hr))
						{
							OpenFolderItem(pidlTarget.get(),bOpenInNewTab,bOpenInNewWindow);
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
				szParsingPath, nullptr,SW_SHOWNORMAL);
		}
		else
		{
			/* File item. */
			OpenFileItem(pidlItem,EMPTY_STRING);
		}
	}
}

void Explorerplusplus::OpenFolderItem(PCIDLIST_ABSOLUTE pidlItem, BOOL bOpenInNewTab, BOOL bOpenInNewWindow)
{
	if(bOpenInNewWindow)
		m_navigation->OpenDirectoryInNewWindow(pidlItem);
	else if(m_config->alwaysOpenNewTab || bOpenInNewTab)
		m_tabContainer->CreateNewTab(pidlItem, TabSettings(_selected = true));
	else
		m_navigation->BrowseFolderInCurrentTab(pidlItem);
}

void Explorerplusplus::OpenFileItem(PCIDLIST_ABSOLUTE pidlItem,const TCHAR *szParameters)
{
	unique_pidl_absolute pidlParent(ILCloneFull(pidlItem));
	ILRemoveLastID(pidlParent.get());

	TCHAR szItemDirectory[MAX_PATH];
	GetDisplayName(pidlParent.get(),szItemDirectory,SIZEOF_ARRAY(szItemDirectory),SHGDN_FORPARSING);

	ExecuteFileAction(m_hContainer,EMPTY_STRING,szParameters,szItemDirectory,pidlItem);
}

BOOL Explorerplusplus::OnSize(int MainWindowWidth,int MainWindowHeight)
{
	RECT			rc;
	UINT			uFlags;
	int				indentBottom = 0;
	int				indentTop = 0;
	int				indentRight = 0;
	int				indentLeft = 0;
	int				iIndentRebar = 0;
	int				iHolderWidth;
	int				iHolderHeight;
	int				iHolderTop;
	int				iTabBackingWidth;
	int				iTabBackingLeft;

	if (!m_InitializationFinished.get())
	{
		return TRUE;
	}

	if(m_hMainRebar)
	{
		GetWindowRect(m_hMainRebar,&rc);
		iIndentRebar += GetRectHeight(&rc);
	}

	if(m_config->showStatusBar)
	{
		GetWindowRect(m_hStatusBar,&rc);
		indentBottom += GetRectHeight(&rc);
	}

	if(m_config->showDisplayWindow)
	{
		if (m_config->displayWindowVertical)
		{
			indentRight += m_config->displayWindowWidth;
		}
		else
		{
			indentBottom += m_config->displayWindowHeight;
		}
	}

	if(m_config->showFolders)
	{
		GetClientRect(m_hHolder,&rc);
		indentLeft = GetRectWidth(&rc);
	}

	RECT tabWindowRect;
	GetClientRect(m_tabContainer->GetHWND(), &tabWindowRect);

	int tabWindowHeight = GetRectHeight(&tabWindowRect);

	indentTop = iIndentRebar;

	if(m_bShowTabBar)
	{
		if(!m_config->showTabBarAtBottom)
		{
			indentTop += tabWindowHeight;
		}
	}

	/* <---- Tab control + backing ----> */

	if(m_config->extendTabControl)
	{
		iTabBackingLeft = 0;
		iTabBackingWidth = MainWindowWidth;
	}
	else
	{
		iTabBackingLeft = indentLeft;
		iTabBackingWidth = MainWindowWidth - indentLeft - indentRight;
	}

	uFlags = m_bShowTabBar?SWP_SHOWWINDOW:SWP_HIDEWINDOW;

	int iTabTop;

	if(!m_config->showTabBarAtBottom)
	{
		iTabTop = iIndentRebar;
	}
	else
	{
		iTabTop = MainWindowHeight - indentBottom - tabWindowHeight;
	}

	/* If we're showing the tab bar at the bottom of the listview,
	the only thing that will change is the top coordinate. */
	SetWindowPos(m_hTabBacking,m_hDisplayWindow,iTabBackingLeft,
		iTabTop,iTabBackingWidth,tabWindowHeight,uFlags);

	SetWindowPos(m_tabContainer->GetHWND(), nullptr,0,0,iTabBackingWidth - 25,
		tabWindowHeight,SWP_SHOWWINDOW|SWP_NOZORDER);

	UINT dpi = m_dpiCompat.GetDpiForWindow(m_hContainer);

	/* Tab close button. */
	int scaledCloseToolbarWidth = MulDiv(CLOSE_TOOLBAR_WIDTH, dpi, USER_DEFAULT_SCREEN_DPI);
	int scaledCloseToolbarHeight = MulDiv(CLOSE_TOOLBAR_HEIGHT, dpi, USER_DEFAULT_SCREEN_DPI);
	int scaledCloseToolbarXOffset = MulDiv(CLOSE_TOOLBAR_X_OFFSET, dpi, USER_DEFAULT_SCREEN_DPI);
	int scaledCloseToolbarYOffset = MulDiv(CLOSE_TOOLBAR_Y_OFFSET, dpi, USER_DEFAULT_SCREEN_DPI);

	SetWindowPos(m_hTabWindowToolbar, nullptr, iTabBackingWidth - scaledCloseToolbarWidth - scaledCloseToolbarXOffset,
		scaledCloseToolbarYOffset, scaledCloseToolbarWidth, scaledCloseToolbarHeight, SWP_SHOWWINDOW | SWP_NOZORDER);

	if(m_config->extendTabControl &&
		!m_config->showTabBarAtBottom)
	{
		iHolderTop = indentTop;
	}
	else
	{
		iHolderTop = iIndentRebar;
	}

	/* <---- Holder window + child windows ----> */

	if(m_config->extendTabControl &&
		m_config->showTabBarAtBottom &&
		m_bShowTabBar)
	{
		iHolderHeight = MainWindowHeight - indentBottom - iHolderTop - tabWindowHeight;
	}
	else
	{
		iHolderHeight = MainWindowHeight - indentBottom - iHolderTop;
	}

	iHolderWidth = m_config->treeViewWidth;

	SetWindowPos(m_hHolder, nullptr,0,iHolderTop,
		iHolderWidth,iHolderHeight,SWP_NOZORDER);

	/* The treeview is only slightly smaller than the holder
	window, in both the x and y-directions. */
	SetWindowPos(m_hTreeView, nullptr,TREEVIEW_X_CLEARANCE,tabWindowHeight,
		iHolderWidth - TREEVIEW_HOLDER_CLEARANCE - TREEVIEW_X_CLEARANCE,
		iHolderHeight - tabWindowHeight,SWP_NOZORDER);

	SetWindowPos(m_hFoldersToolbar, nullptr, iHolderWidth - scaledCloseToolbarWidth - scaledCloseToolbarXOffset,
		scaledCloseToolbarYOffset, scaledCloseToolbarWidth, scaledCloseToolbarHeight, SWP_SHOWWINDOW | SWP_NOZORDER);


	/* <---- Display window ----> */

	if (m_config->displayWindowVertical)
	{
		SetWindowPos(m_hDisplayWindow,NULL,MainWindowWidth - indentRight,iIndentRebar,
			m_config->displayWindowWidth,MainWindowHeight - iIndentRebar - indentBottom,SWP_SHOWWINDOW|SWP_NOZORDER);
	}
	else
	{
		SetWindowPos(m_hDisplayWindow, nullptr,0,MainWindowHeight - indentBottom,
			MainWindowWidth, m_config->displayWindowHeight,SWP_SHOWWINDOW|SWP_NOZORDER);
	}

	/* <---- ALL listview windows ----> */

	for (auto &tab : m_tabContainer->GetAllTabs() | boost::adaptors::map_values)
	{
		uFlags = SWP_NOZORDER;

		if (m_tabContainer->IsTabSelected(*tab))
		{
			uFlags |= SWP_SHOWWINDOW;
		}

		int width = MainWindowWidth - indentLeft - indentRight;
		int height = MainWindowHeight - indentBottom - indentTop;

		if (m_config->showTabBarAtBottom && m_bShowTabBar)
		{
			height -= tabWindowHeight;
		}

		SetWindowPos(tab->GetShellBrowser()->GetListView(),NULL,indentLeft,indentTop,width,height,uFlags);
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

void Explorerplusplus::OnDpiChanged(const RECT *updatedWindowRect)
{
	SetWindowPos(m_hContainer, nullptr, updatedWindowRect->left, updatedWindowRect->top,
		GetRectWidth(updatedWindowRect), GetRectHeight(updatedWindowRect),
		SWP_NOZORDER | SWP_NOACTIVATE);
}

int Explorerplusplus::OnDestroy()
{
	if(m_pClipboardDataObject != nullptr)
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

	delete m_pStatusBar;

	ChangeClipboardChain(m_hContainer,m_hNextClipboardViewer);
	PostQuitMessage(0);

	return 0;
}

int Explorerplusplus::OnClose()
{
	if(m_config->confirmCloseTabs && (m_tabContainer->GetNumTabs() > 1))
	{
		std::wstring message = ResourceHelper::LoadString(m_hLanguageModule,IDS_GENERAL_CLOSE_ALL_TABS);
		int response = MessageBox(m_hContainer,message.c_str(),NExplorerplusplus::APP_NAME,MB_ICONINFORMATION|MB_YESNO);

		/* If the user clicked no, return without
		closing. */
		if(response == IDNO)
			return 1;
	}

	// It's important that the plugins are destroyed before the main
	// window is destroyed and before this class is destroyed.
	// The first because the API binding classes may interact with the
	// UI on destruction (e.g. to remove menu entries they've added).
	// The second because the API bindings assume they can use the
	// objects passed to them until their destruction. Those objects are
	// destroyed automatically when this class is destroyed, so letting
	// the plugins be destroyed automatically could result in objects
	// being destroyed in the wrong order.
	m_pluginManager.reset();

	KillTimer(m_hContainer, AUTOSAVE_TIMER_ID);

	SaveAllSettings();

	DestroyWindow(m_hContainer);

	return 0;
}

void Explorerplusplus::OnSetFocus()
{
	SetFocus(m_hLastActiveWindow);
}

/*
 * Called when the contents of the clipboard change.
 * All cut items are deghosted, and the 'Paste' button
 * is enabled/disabled.
 */
void Explorerplusplus::OnDrawClipboard()
{
	if(m_pClipboardDataObject != nullptr)
	{
		if(OleIsCurrentClipboard(m_pClipboardDataObject) == S_FALSE)
		{
			/* Deghost all items that have been 'cut'. */
			for(const auto &strFile : m_CutFileNameList)
			{
				Tab *tab = m_tabContainer->GetTabOptional(m_iCutTabInternal);

				/* Only deghost the items if the tab they
				are/were in still exists. */
				if(tab)
				{
					int iItem = tab->GetShellBrowser()->LocateFileItemIndex(strFile.c_str());

					/* It is possible that the ghosted file
					does NOT exist within the current folder.
					This is the case when (for example), a file
					is cut, and the folder is changed, in which
					case the item is no longer available. */
					if(iItem != -1)
						tab->GetShellBrowser()->DeghostItem(iItem);
				}
			}

			m_CutFileNameList.clear();

			/* Deghost any cut treeview items. */
			if(m_hCutTreeViewItem != nullptr)
			{
				TVITEM tvItem;

				tvItem.mask			= TVIF_HANDLE|TVIF_STATE;
				tvItem.hItem		= m_hCutTreeViewItem;
				tvItem.state		= 0;
				tvItem.stateMask	= TVIS_CUT;
				TreeView_SetItem(m_hTreeView,&tvItem);

				m_hCutTreeViewItem = nullptr;
			}

			m_pClipboardDataObject->Release();
			m_pClipboardDataObject = nullptr;
		}
	}

	SendMessage(m_mainToolbar->GetHWND(), TB_ENABLEBUTTON, ToolbarButton::Paste, CanPaste());

	if(m_hNextClipboardViewer != nullptr)
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
	else if(m_hNextClipboardViewer != nullptr)
		SendMessage(m_hNextClipboardViewer,WM_CHANGECBCHAIN,wParam,lParam);
}

void Explorerplusplus::HandleDirectoryMonitoring(int iTabId)
{
	DirectoryAltered_t	*pDirectoryAltered = nullptr;
	int					iDirMonitorId;

	Tab &tab = m_tabContainer->GetTab(iTabId);

	iDirMonitorId		= tab.GetShellBrowser()->GetDirMonitorId();
			
	/* Stop monitoring the directory that was browsed from. */
	m_pDirMon->StopDirectoryMonitor(iDirMonitorId);

	std::wstring directoryToWatch = tab.GetShellBrowser()->GetDirectory();

	/* Don't watch virtual folders (the 'recycle bin' may be an
	exception to this). */
	if(tab.GetShellBrowser()->InVirtualFolder())
	{
		iDirMonitorId = -1;
	}
	else
	{
		pDirectoryAltered = (DirectoryAltered_t *)malloc(sizeof(DirectoryAltered_t));

		pDirectoryAltered->iIndex		= iTabId;
		pDirectoryAltered->iFolderIndex	= tab.GetShellBrowser()->GetUniqueFolderId();
		pDirectoryAltered->pData		= this;

		/* Start monitoring the directory that was opened. */
		LOG(debug) << _T("Starting directory monitoring for \"") << directoryToWatch << _T("\"");
		iDirMonitorId = m_pDirMon->WatchDirectory(directoryToWatch.c_str(),FILE_NOTIFY_CHANGE_FILE_NAME|
			FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_ATTRIBUTES|
			FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_LAST_ACCESS|FILE_NOTIFY_CHANGE_CREATION|
			FILE_NOTIFY_CHANGE_SECURITY,DirectoryAlteredCallback,FALSE,(void *)pDirectoryAltered);
	}

	tab.GetShellBrowser()->SetDirMonitorId(iDirMonitorId);
}

void Explorerplusplus::OnDisplayWindowResized(WPARAM wParam)
{
	if (m_config->displayWindowVertical)
	{
		m_config->displayWindowWidth = max(LOWORD(wParam), MINIMUM_DISPLAYWINDOW_WIDTH);
	}
	else
	{
		m_config->displayWindowHeight = max(HIWORD(wParam), MINIMUM_DISPLAYWINDOW_HEIGHT);
	}

	RECT rc;
	GetClientRect(m_hContainer,&rc);
	SendMessage(m_hContainer,WM_SIZE,SIZE_RESTORED,(LPARAM)MAKELPARAM(rc.right,rc.bottom));
}

/*
 * Sizes all columns in the active listview
 * based on their text.
 */
void Explorerplusplus::OnAutoSizeColumns()
{
	size_t	nColumns;
	UINT	iCol = 0;

	nColumns = m_pActiveShellBrowser->GetNumActiveColumns();

	for(iCol = 0;iCol < nColumns;iCol++)
	{
		ListView_SetColumnWidth(m_hActiveListView,iCol,LVSCW_AUTOSIZE);
	}
}

/* Cycle through the current views. */
void Explorerplusplus::OnToolbarViews()
{
	Tab &selectedTab = m_tabContainer->GetSelectedTab();
	selectedTab.GetShellBrowser()->CycleViewMode(true);
}

void Explorerplusplus::OnSortByAscending(BOOL bSortAscending)
{
	Tab &selectedTab = m_tabContainer->GetSelectedTab();

	if(bSortAscending != selectedTab.GetShellBrowser()->GetSortAscending())
	{
		selectedTab.GetShellBrowser()->SetSortAscending(bSortAscending);

		SortMode sortMode = selectedTab.GetShellBrowser()->GetSortMode();

		/* It is quicker to re-sort the folder than refresh it. */
		selectedTab.GetShellBrowser()->SortFolder(sortMode);
	}
}

void Explorerplusplus::OnPreviousWindow()
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
			if(m_config->showFolders)
			{
				SetFocus(m_hTreeView);
			}
			else
			{
				if(m_config->showAddressBar)
				{
					SetFocus(m_addressBar->GetHWND());
				}
			}
		}
		else if(hFocus == m_hTreeView)
		{
			if(m_config->showAddressBar)
			{
				SetFocus(m_addressBar->GetHWND());
			}
			else
			{
				/* Always shown. */
				SetFocus(m_hActiveListView);
			}
		}
		else if(hFocus == (HWND) SendMessage(m_addressBar->GetHWND(), CBEM_GETEDITCONTROL, 0, 0))
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
void Explorerplusplus::OnNextWindow()
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
			if(m_config->showAddressBar)
			{
				SetFocus(m_addressBar->GetHWND());
			}
			else
			{
				if(m_config->showFolders)
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
		else if(hFocus == (HWND)SendMessage(m_addressBar->GetHWND(),CBEM_GETEDITCONTROL,0,0))
		{
			if(m_config->showFolders)
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

void Explorerplusplus::OnLockToolbars()
{
	REBARBANDINFO	rbbi;
	UINT			nBands;
	UINT			i = 0;

	m_config->lockToolbars = !m_config->lockToolbars;

	nBands = (UINT)SendMessage(m_hMainRebar,RB_GETBANDCOUNT,0,0);

	for(i = 0;i < nBands;i++)
	{
		/* First, retrieve the current style for this band. */
		rbbi.cbSize	= sizeof(REBARBANDINFO);
		rbbi.fMask	= RBBIM_STYLE;
		SendMessage(m_hMainRebar,RB_GETBANDINFO,i,(LPARAM)&rbbi);

		/* Add the gripper style. */
		AddGripperStyle(&rbbi.fStyle,!m_config->lockToolbars);

		/* Now, set the new style. */
		SendMessage(m_hMainRebar,RB_SETBANDINFO,i,(LPARAM)&rbbi);
	}

	/* If the rebar is locked, prevent items from
	been rearranged. */
	AddWindowStyle(m_hMainRebar,RBS_FIXEDORDER, m_config->lockToolbars);
}

void Explorerplusplus::OnShellNewItemCreated(LPARAM lParam)
{
	int iRenamedItem = (int)lParam;

	if(iRenamedItem != -1)
	{
		/* Start editing the label for this item. */
		ListView_EditLabel(m_hActiveListView,iRenamedItem);
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
		OnGoBack();
		break;

	case APPCOMMAND_BROWSER_FORWARD:
		SendMessage(m_hContainer,WM_CANCELMODE,0,0);
		OnGoForward();
		break;

	case APPCOMMAND_BROWSER_HOME:
		OnGoHome();
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

void Explorerplusplus::OnRefresh()
{
	Tab &tab = m_tabContainer->GetSelectedTab();
	tab.GetShellBrowser()->GetNavigationController()->Refresh();
}

void Explorerplusplus::CopyColumnInfoToClipboard()
{
	auto currentColumns = m_pActiveShellBrowser->ExportCurrentColumns();

	std::wstring strColumnInfo;
	int nActiveColumns = 0;

	for(const auto &column : currentColumns)
	{
		if(column.bChecked)
		{
			TCHAR szText[64];
			LoadString(m_hLanguageModule,ShellBrowser::LookupColumnNameStringIndex(column.id),szText,SIZEOF_ARRAY(szText));

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

	BulkClipboardWriter clipboardWriter;
	clipboardWriter.WriteText(strColumnInfo);
}

void Explorerplusplus::ToggleFilterStatus()
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

	const Tab &selectedTab = m_tabContainer->GetSelectedTab();

	if(iTabId == selectedTab.GetId())
	{
		UpdateStatusBarText(selectedTab);
		UpdateDisplayWindow(selectedTab);
	}
}

void Explorerplusplus::OnIdaRClick()
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

				uViewMode = m_pActiveShellBrowser->GetViewMode();

				if(uViewMode == ViewMode::SmallIcons || uViewMode == ViewMode::List ||
					uViewMode == ViewMode::Details)
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

		if(hSelection != nullptr)
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
void Explorerplusplus::OnAssocChanged()
{
	typedef BOOL (WINAPI *FII_PROC)(BOOL);
	FII_PROC fileIconInit;
	HKEY hKey;
	HMODULE hShell32;
	TCHAR szShellIconSize[32];
	TCHAR szTemp[32];
	DWORD dwShellIconSize;
	LONG res;

	hShell32 = LoadLibrary(_T("shell32.dll"));

	fileIconInit = (FII_PROC)GetProcAddress(hShell32,(LPCSTR)660);

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

		if(fileIconInit != nullptr)
			fileIconInit(TRUE);

		/* Now, set it back to the original value. */
		NRegistrySettings::SaveStringToRegistry(hKey,_T("Shell Icon Size"),szShellIconSize);

		if(fileIconInit != nullptr)
			fileIconInit(FALSE);

		RegCloseKey(hKey);
	}

	/* DO NOT free shell32.dll. Doing so will release
	the image lists (among other things). */

	/* When the system image list is refresh, ALL previous
	icons will be discarded. This means that SHGetFileInfo()
	needs to be called to get each files icon again. */

	/* Now, go through each tab, and refresh each icon. */
	for (auto &tab : m_tabContainer->GetAllTabs() | boost::adaptors::map_values)
	{
		tab->GetShellBrowser()->GetNavigationController()->Refresh();
	}

	/* Now, refresh the treeview. */
	m_shellTreeView->RefreshAllIcons();

	/* TODO: Update the address bar. */
}

void Explorerplusplus::OnCloneWindow()
{
	std::wstring currentDirectory = m_pActiveShellBrowser->GetDirectory();

	TCHAR szQuotedCurrentDirectory[MAX_PATH];
	StringCchPrintf(szQuotedCurrentDirectory, SIZEOF_ARRAY(szQuotedCurrentDirectory),
		_T("\"%s\""), currentDirectory.c_str());

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

void Explorerplusplus::OnDisplayWindowIconRClick(POINT *ptClient)
{
	POINT ptScreen = *ptClient;
	BOOL res = ClientToScreen(m_hDisplayWindow, &ptScreen);

	if (!res)
	{
		return;
	}

	OnListViewRClick(&ptScreen);
}

void Explorerplusplus::OnDisplayWindowRClick(POINT *ptClient)
{
	wil::unique_hmenu parentMenu(
		LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_DISPLAYWINDOW_RCLICK)));

	if (!parentMenu)
	{
		return;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	MenuHelper::CheckItem(menu, IDM_DISPLAYWINDOW_VERTICAL, m_config->displayWindowVertical);

	POINT ptScreen = *ptClient;
	BOOL res = ClientToScreen(m_hDisplayWindow, &ptScreen);

	if (!res)
	{
		return;
	}

	TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERTICAL, ptScreen.x, ptScreen.y, 0,
		m_hContainer, nullptr);
}

LRESULT Explorerplusplus::OnCustomDraw(LPARAM lParam)
{
	NMLVCUSTOMDRAW *pnmlvcd = nullptr;
	NMCUSTOMDRAW *pnmcd = nullptr;

	pnmlvcd = (NMLVCUSTOMDRAW *)lParam;

	if(pnmlvcd->nmcd.hdr.hwndFrom == m_hActiveListView)
	{
		pnmcd = &pnmlvcd->nmcd;

		switch(pnmcd->dwDrawStage)
		{
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;

		case CDDS_ITEMPREPAINT:
			{
				DWORD dwAttributes = m_pActiveShellBrowser->GetItemFileFindData(static_cast<int>(pnmcd->dwItemSpec)).dwFileAttributes;

				TCHAR szFileName[MAX_PATH];
				m_pActiveShellBrowser->GetItemFullName(static_cast<int>(pnmcd->dwItemSpec),szFileName,SIZEOF_ARRAY(szFileName));
				PathStripPath(szFileName);

				/* Loop through each filter. Decide whether to change the font of the
				current item based on its filename and/or attributes. */
				for(const auto &colorRule : m_ColorRules)
				{
					BOOL bMatchFileName = FALSE;
					BOOL bMatchAttributes = FALSE;

					/* Only match against the filename if it's not empty. */
					if(!colorRule.strFilterPattern.empty())
					{
						if(CheckWildcardMatch(colorRule.strFilterPattern.c_str(),szFileName,!colorRule.caseInsensitive) == 1)
						{
							bMatchFileName = TRUE;
						}
					}
					else
					{
						bMatchFileName = TRUE;
					}

					if(colorRule.dwFilterAttributes != 0)
					{
						if(colorRule.dwFilterAttributes & dwAttributes)
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
						pnmlvcd->clrText = colorRule.rgbColour;
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

void Explorerplusplus::OnSortBy(SortMode sortMode)
{
	Tab &selectedTab = m_tabContainer->GetSelectedTab();
	SortMode currentSortMode = selectedTab.GetShellBrowser()->GetSortMode();

	if(!selectedTab.GetShellBrowser()->GetShowInGroups() &&
		sortMode == currentSortMode)
	{
		selectedTab.GetShellBrowser()->SetSortAscending(!selectedTab.GetShellBrowser()->GetSortAscending());
	}
	else if(selectedTab.GetShellBrowser()->GetShowInGroups())
	{
		selectedTab.GetShellBrowser()->SetShowInGroups(FALSE);
	}

	selectedTab.GetShellBrowser()->SortFolder(sortMode);
}

void Explorerplusplus::OnGroupBy(SortMode sortMode)
{
	Tab &selectedTab = m_tabContainer->GetSelectedTab();
	SortMode currentSortMode = selectedTab.GetShellBrowser()->GetSortMode();

	/* If group view is already enabled, and the current sort
	mode matches the supplied sort mode, toggle the ascending/
	descending flag. */
	if(selectedTab.GetShellBrowser()->GetShowInGroups() &&
		sortMode == currentSortMode)
	{
		selectedTab.GetShellBrowser()->SetSortAscending(!selectedTab.GetShellBrowser()->GetSortAscending());
	}
	else if(!selectedTab.GetShellBrowser()->GetShowInGroups())
	{
		selectedTab.GetShellBrowser()->SetShowInGroupsFlag(TRUE);
	}

	selectedTab.GetShellBrowser()->SortFolder(sortMode);
}

void Explorerplusplus::SaveAllSettings()
{
	m_iLastSelectedTab = m_tabContainer->GetSelectedTabIndex();

	ILoadSave *pLoadSave = nullptr;

	if(m_bSavePreferencesToXMLFile)
		pLoadSave = new LoadSaveXML(this,FALSE);
	else
		pLoadSave = new LoadSaveRegistry(this);

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

Config *Explorerplusplus::GetConfig() const
{
	return m_config.get();
}

HMODULE Explorerplusplus::GetLanguageModule() const
{
	return m_hLanguageModule;
}

HWND Explorerplusplus::GetMainWindow() const
{
	return m_hContainer;
}

HWND Explorerplusplus::GetActiveListView() const
{
	return m_hActiveListView;
}

ShellBrowser *Explorerplusplus::GetActiveShellBrowser() const
{
	return m_pActiveShellBrowser;
}

IExplorerplusplus *Explorerplusplus::GetCoreInterface()
{
	return this;
}

TabContainer *Explorerplusplus::GetTabContainer() const
{
	return m_tabContainer;
}

TabRestorer *Explorerplusplus::GetTabRestorer() const
{
	return m_tabRestorer.get();
}

HWND Explorerplusplus::GetTreeView() const
{
	return m_hTreeView;
}

IDirectoryMonitor *Explorerplusplus::GetDirectoryMonitor() const
{
	return m_pDirMon;
}

IconResourceLoader *Explorerplusplus::GetIconResourceLoader() const
{
	return m_iconResourceLoader.get();
}

CachedIcons *Explorerplusplus::GetCachedIcons()
{
	return &m_cachedIcons;
}

BOOL Explorerplusplus::GetSavePreferencesToXmlFile() const
{
	return m_bSavePreferencesToXMLFile;
}

void Explorerplusplus::SetSavePreferencesToXmlFile(BOOL savePreferencesToXmlFile)
{
	m_bSavePreferencesToXMLFile = savePreferencesToXmlFile;
}

void Explorerplusplus::OnShowHiddenFiles()
{
	Tab &tab = m_tabContainer->GetSelectedTab();
	tab.GetShellBrowser()->SetShowHidden(!tab.GetShellBrowser()->GetShowHidden());
	tab.GetShellBrowser()->GetNavigationController()->Refresh();
}