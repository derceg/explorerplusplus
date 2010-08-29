/******************************************************************
 *
 * Project: Explorer++
 * File: Misc.cpp
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
#include "Misc.h"
#include "Explorer++.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/Bookmark.h"
#include "MainResource.h"


extern list<TabDirectory_t> g_TabDirs;

void CContainer::HandleTreeViewSelection(void)
{
	HTREEITEM		hItem;
	LPITEMIDLIST	pidlDirectory = NULL;
	TCHAR			szDirectory[MAX_PATH];
	TCHAR			szRoot[MAX_PATH];
	UINT			uDriveType;
	BOOL			bNetworkPath = FALSE;

	if(!m_bSynchronizeTreeview)
	{
		return;
	}

	pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

	GetDisplayName(pidlDirectory,szDirectory,SHGDN_FORPARSING);

	if(PathIsUNC(szDirectory))
	{
		bNetworkPath = TRUE;
	}
	else
	{
		PathStripToRoot(szRoot);
		uDriveType = GetDriveType(szRoot);

		bNetworkPath = (uDriveType == DRIVE_REMOTE);
	}

	/* To improve performance, do not automatically sync the
	treeview with network or UNC paths. */
	if(!bNetworkPath)
	{
		hItem = m_pMyTreeView->LocateItem(pidlDirectory);

		if(hItem != NULL)
		{
			/* TVN_SELCHANGED is NOT sent when the new selected
			item is the same as the old selected item. It is only
			sent when the two are different.
			Therefore, the only case to handle is when the treeview
			selection is changed by browsing using the listview. */
			if(TreeView_GetSelection(m_hTreeView) != hItem)
				m_bSelectingTreeViewDirectory = TRUE;

			SendMessage(m_hTreeView,TVM_SELECTITEM,(WPARAM)TVGN_CARET,(LPARAM)hItem);
		}
	}

	CoTaskMemFree(pidlDirectory);
}

HRESULT CContainer::RestoreTabs(ILoadSave *pLoadSave)
{
	list<TabDirectory_t>::iterator	itr;
	TCHAR							szDirectory[MAX_PATH];
	HRESULT							hr;
	int								nTabsCreated = 0;
	int								i = 0;

	if(!g_TabDirs.empty())
	{
		for(itr = g_TabDirs.begin();itr != g_TabDirs.end();itr++)
		{
			StringCchCopy(szDirectory,SIZEOF_ARRAY(szDirectory),itr->Dir);

			if(lstrcmp(itr->Dir,_T("..")) == 0)
			{
				/* Get the parent of the current directory,
				and browse to it. */
				GetCurrentDirectory(SIZEOF_ARRAY(szDirectory),szDirectory);
				PathRemoveFileSpec(szDirectory);
			}
			else if(lstrcmp(itr->Dir,_T(".")) == 0)
			{
				GetCurrentDirectory(SIZEOF_ARRAY(szDirectory),szDirectory);
			}

			hr = CreateNewTab(szDirectory,NULL,NULL,TRUE,NULL);

			if(hr == S_OK)
				nTabsCreated++;
		}
	}
	else
	{
		if(m_StartupMode == STARTUP_PREVIOUSTABS)
			nTabsCreated = pLoadSave->LoadPreviousTabs();
	}

	if(nTabsCreated == 0)
	{
		hr = CreateNewTab(m_DefaultTabDirectory,NULL,NULL,TRUE,NULL);

		if(FAILED(hr))
			hr = CreateNewTab(m_DefaultTabDirectoryStatic,NULL,NULL,TRUE,NULL);

		if(hr == S_OK)
		{
			nTabsCreated++;
		}
	}

	if(nTabsCreated == 0)
	{
		/* Should never end up here. */
		return E_FAIL;
	}

	/* Tabs created on startup will NOT have
	any automatic updates. The only thing that
	needs to be done is to monitor each
	directory. The tab that is finally switched
	to will have an associated update of window
	states. */
	for(i = 0;i < nTabsCreated;i++)
	{
		TC_ITEM tcItem;

		tcItem.mask	= TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);

		HandleDirectoryMonitoring((int)tcItem.lParam);
	}

	if(!m_bAlwaysShowTabBar)
	{
		if(nTabsCreated == 1)
		{
			m_bShowTabBar = FALSE;
		}
	}

	/* m_iLastSelectedTab is the tab that was selected when the
	program was last closed. */
	if(m_iLastSelectedTab >= TabCtrl_GetItemCount(m_hTabCtrl) ||
		m_iLastSelectedTab < 0)
		m_iLastSelectedTab = 0;

	/* Set the focus back to the tab that had the focus when the program
	was last closed. */
	TabCtrl_SetCurSel(m_hTabCtrl,m_iLastSelectedTab);

	m_iTabSelectedItem = m_iLastSelectedTab;

	OnTabChangeInternal(TRUE);

	return S_OK;
}

int CContainer::CreateListViewFileList(TCHAR *FileNameList,unsigned int BufferSize)
{
	IBufferManager	*pBufferManager = NULL;
	TCHAR			FullFileName[MAX_PATH];
	int				LastItemFound;

	pBufferManager = new CBufferManager();

	/* Find each selected file and call the selected function for the file. */
	LastItemFound = -1;
	while((LastItemFound = ListView_GetNextItem(m_hActiveListView,
	LastItemFound,LVNI_SELECTED)) != -1)
	{
		m_pActiveShellBrowser->QueryFullItemName(LastItemFound,FullFileName);
		pBufferManager->WriteListEntry(FullFileName);
	}

	if(FileNameList != NULL)
	{
		pBufferManager->QueryBuffer(FileNameList,BufferSize);
	}

	return 0;
}

void CContainer::ValidateLoadedSettings(void)
{
	if(m_TreeViewWidth <= 0)
		m_TreeViewWidth = DEFAULT_TREEVIEW_WIDTH;

	if(m_DisplayWindowHeight < MINIMUM_DISPLAYWINDOW_HEIGHT)
		m_DisplayWindowHeight = DEFAULT_DISPLAYWINDOW_HEIGHT;

	ValidateColumns();
	ValidateToolbarSettings();
}

void CContainer::ValidateToolbarSettings(void)
{
	list<ToolbarButton_t>::iterator	itr;
	BOOL							bCorrupted = FALSE;
	int								*ButtonMap;
	int								nButtons;
	int								i = 0;

	nButtons = sizeof(ToolbarButtonSet) / sizeof(ToolbarButtonSet[0]);

	ButtonMap = (int *)malloc(sizeof(int) * nButtons);

	for(i = 0;i < nButtons;i++)
	{
		ButtonMap[i] = 0;
	}

	/* Two things to check:
	- Firstly, that all items id's are within range,
	- and secondly, that no item is duplicated (except for
	separators).
	An empty set of items (i.e. no buttons on the toolbar) is
	allowed. */
	for(itr = m_tbInitial.begin();itr != m_tbInitial.end();itr++)
	{
		if(itr->iItemID < TOOLBAR_ID_START)
		{
			bCorrupted = TRUE;
			break;
		}
	}

	if(!bCorrupted)
	{
		for(i = 0;i < nButtons;i++)
		{
			if(ButtonMap[i] > 1)
			{
				bCorrupted = TRUE;
				break;
			}
		}
	}

	if(bCorrupted)
		SetInitialToolbarButtons();

	free(ButtonMap);
}

void CContainer::ValidateColumns(void)
{
	ValidateSingleColumnSet(VALIDATE_REALFOLDER_COLUMNS,&m_RealFolderColumnList);
	ValidateSingleColumnSet(VALIDATE_CONTROLPANEL_COLUMNS,&m_ControlPanelColumnList);
	ValidateSingleColumnSet(VALIDATE_MYCOMPUTER_COLUMNS,&m_MyComputerColumnList);
	ValidateSingleColumnSet(VALIDATE_RECYCLEBIN_COLUMNS,&m_RecycleBinColumnList);
	ValidateSingleColumnSet(VALIDATE_PRINTERS_COLUMNS,&m_PrintersColumnList);
	ValidateSingleColumnSet(VALIDATE_NETWORKCONNECTIONS_COLUMNS,&m_NetworkConnectionsColumnList);
	ValidateSingleColumnSet(VALIDATE_MYNETWORKPLACES_COLUMNS,&m_MyNetworkPlacesColumnList);
}

void CContainer::ValidateSingleColumnSet(int iColumnSet,list<Column_t> *pColumnList)
{
	list<Column_t>::iterator	itr;
	Column_t					Column;
	int							*pColumnMap = NULL;
	BOOL						bFound = FALSE;
	const Column_t				*pColumns = NULL;
	unsigned int				iTotalColumnSize = 0;
	unsigned int				i = 0;

	switch(iColumnSet)
	{
		case VALIDATE_REALFOLDER_COLUMNS:
			iTotalColumnSize = SIZEOF_ARRAY(g_RealFolderColumns);
			pColumns = g_RealFolderColumns;
			break;

		case VALIDATE_CONTROLPANEL_COLUMNS:
			iTotalColumnSize = SIZEOF_ARRAY(g_ControlPanelColumns);
			pColumns = g_ControlPanelColumns;
			break;

		case VALIDATE_MYCOMPUTER_COLUMNS:
			iTotalColumnSize = SIZEOF_ARRAY(g_MyComputerColumns);
			pColumns = g_MyComputerColumns;
			break;

		case VALIDATE_RECYCLEBIN_COLUMNS:
			iTotalColumnSize = SIZEOF_ARRAY(g_RecycleBinColumns);
			pColumns = g_RecycleBinColumns;
			break;

		case VALIDATE_PRINTERS_COLUMNS:
			iTotalColumnSize = SIZEOF_ARRAY(g_PrintersColumns);
			pColumns = g_PrintersColumns;
			break;

		case VALIDATE_NETWORKCONNECTIONS_COLUMNS:
			iTotalColumnSize = SIZEOF_ARRAY(g_NetworkConnectionsColumns);
			pColumns = g_NetworkConnectionsColumns;
			break;

		case VALIDATE_MYNETWORKPLACES_COLUMNS:
			iTotalColumnSize = SIZEOF_ARRAY(g_MyNetworkPlacesColumns);
			pColumns = g_MyNetworkPlacesColumns;
			break;
	}

	pColumnMap = (int *)malloc(iTotalColumnSize * sizeof(int));

	for(i = 0;i < iTotalColumnSize;i++)
	{
		pColumnMap[i] = 0;
	}

	/* Check that every column that is supposed to appear
	is in the column list. */
	for(i = 0;i < iTotalColumnSize;i++)
	{
		bFound = FALSE;

		for(itr = pColumnList->begin();itr != pColumnList->end();itr++)
		{
			if(itr->id == pColumns[i].id)
			{
				bFound = TRUE;
				break;
			}
		}

		/* The column is not currently in the set. Add it in. */
		if(!bFound)
		{
			Column.id		= pColumns[i].id;
			Column.bChecked	= pColumns[i].bChecked;
			Column.iWidth	= DEFAULT_COLUMN_WIDTH;
			pColumnList->push_back(Column);
		}
	}

	free(pColumnMap);
}

void CContainer::ApplyLoadedSettings(void)
{
	m_pMyTreeView->SetShowHidden(m_bShowHiddenGlobal);
}

void CContainer::ApplyToolbarSettings(void)
{
	BOOL bVisible = FALSE;
	int i = 0;

	/* Set the state of the toolbars contained within
	the main rebar. */
	for(i = 0;i < NUM_MAIN_TOOLBARS;i++)
	{
		switch(m_ToolbarInformation[i].wID)
		{
		case ID_MAINTOOLBAR:
			bVisible = m_bShowMainToolbar;
			break;

		case ID_ADDRESSTOOLBAR:
			bVisible = m_bShowAddressBar;
			break;

		case ID_BOOKMARKSTOOLBAR:
			bVisible = m_bShowBookmarksToolbar;
			break;

		case ID_DRIVESTOOLBAR:
			bVisible = m_bShowDrivesToolbar;
			break;

		case ID_APPLICATIONSTOOLBAR:
			bVisible = m_bShowApplicationToolbar;
			break;
		}

		if(!bVisible)
			AddStyleToToolbar(&m_ToolbarInformation[i].fStyle,RBBS_HIDDEN);
	}

	if(m_bLockToolbars)
	{
		for(i = 0;i < NUM_MAIN_TOOLBARS;i++)
		{
			AddStyleToToolbar(&m_ToolbarInformation[i].fStyle,RBBS_NOGRIPPER);
		}
	}
}

void CContainer::AddStyleToToolbar(UINT *fStyle,UINT fStyleToAdd)
{
	if((*fStyle & fStyleToAdd) != fStyleToAdd)
		*fStyle |= fStyleToAdd;
}

int CContainer::SelectAllFolders(HWND ListView)
{
	TCHAR	FullItemPath[MAX_PATH];
	HRESULT	hr;
	int		nItems;
	int		nFolders = 0;
	int		i = 0;

	ListView_DeselectAllItems(ListView);

	nItems = ListView_GetItemCount(ListView);

	while(i < nItems)
	{
		hr = m_pActiveShellBrowser->QueryFullItemName(i,FullItemPath);

		if(SUCCEEDED(hr))
		{
			if(PathIsDirectory(FullItemPath))
			{
				ListView_SetItemState(ListView,i,LVIS_SELECTED,LVIS_SELECTED);
				nFolders++;
			}
		}

		i++;
	}

	return nFolders;
}

int CContainer::HighlightSimilarFiles(HWND ListView)
{
	TCHAR	FullFileName[MAX_PATH];
	TCHAR	TestFile[MAX_PATH];
	HRESULT	hr;
	BOOL	bSimilarTypes;
	int		iSelected;
	int		nItems;
	int		nSimilar = 0;
	int		i = 0;

	iSelected = ListView_GetNextItem(ListView,
	-1,LVNI_SELECTED);

	if(iSelected == -1)
		return -1;

	hr = m_pActiveShellBrowser->QueryFullItemName(iSelected,TestFile);

	if(SUCCEEDED(hr))
	{
		nItems = ListView_GetItemCount(ListView);

		for(i = 0;i < nItems;i++)
		{
			m_pActiveShellBrowser->QueryFullItemName(i,FullFileName);

			bSimilarTypes = CompareFileTypes(FullFileName,TestFile);

			if(bSimilarTypes)
			{
				ListView_SelectItem(ListView,i,TRUE);
				nSimilar++;
			}
			else
			{
				ListView_SelectItem(ListView,i,FALSE);
			}
		}
	}

	return nSimilar;
}

void CContainer::AdjustFolderPanePosition(void)
{
	RECT rc;
	int IndentTop		= 0;
	int IndentBottom	= 0;
	int height;

	GetClientRect(m_hContainer,&rc);
	height = GetRectHeight(&rc);

	if(m_hMainRebar)
	{
		RECT RebarRect;

		GetWindowRect(m_hMainRebar,&RebarRect);

		IndentTop += RebarRect.bottom - RebarRect.top;
	}

	if(m_bShowStatusBar)
	{
		RECT m_hStatusBarRect;

		GetWindowRect(m_hStatusBar,&m_hStatusBarRect);

		IndentBottom += m_hStatusBarRect.bottom - m_hStatusBarRect.top;
	}

	if(m_bShowDisplayWindow)
	{
		RECT rc;

		GetWindowRect(m_hDisplayWindow,&rc);

		IndentBottom += rc.bottom - rc.top;
	}

	if(m_bShowFolders)
	{
		RECT rc;
		GetClientRect(m_hHolder,&rc);

		SetWindowPos(m_hHolder,NULL,0,IndentTop,rc.right,
		height-IndentBottom-IndentTop,SWP_SHOWWINDOW|SWP_NOZORDER);
	}
}

LRESULT CContainer::StatusBarMenuSelect(WPARAM wParam,LPARAM lParam)
{
	/* Is the menu been closed? .*/
	if(HIWORD(wParam) == 0xFFFF && lParam == 0)
	{
		HandleStatusBarMenuClose();
	}
	else
	{
		TCHAR szBuffer[512];

		HandleStatusBarMenuOpen();

		/* Load the menu help string. */
		LoadString(g_hLanguageModule,LOWORD(wParam),
		szBuffer,SIZEOF_ARRAY(szBuffer));

		/* Send the help string to the status bar. */
		SetWindowText(m_hStatusBar,szBuffer);
	}

	return 0;
}

void CContainer::HandleStatusBarMenuOpen(void)
{
	StatusBarPart_t	sbp;
	TCHAR			szPartText[512];
	int				nParts;
	int				aWidths = -1;
	int				i = 0;

	if(!m_bAlteredStatusBarParts)
	{
		/* Get the number of parts in the status bar window. */
		nParts = (int)SendMessage(m_hStatusBar,SB_GETPARTS,0,0);

		/* For each status bar part, get the text and store it.
		The text for that part will be automatically restored
		when the menu is closed. */
		for(i = 0;i < nParts;i++)
		{
			SendMessage(m_hStatusBar,SB_GETTEXT,i,(LPARAM)szPartText);

			StringCchCopy(sbp.szText,SIZEOF_ARRAY(sbp.szText),
				szPartText);

			m_StatusBarParts.push_back(sbp);
		}

		/* Set the number of status bar parts to one. This single
		part will contain the help menu string for the selected
		menu. */
		SendMessage(m_hStatusBar,SB_SETPARTS,1,(LPARAM)&aWidths);

		m_bAlteredStatusBarParts = TRUE;
	}
}

void CContainer::HandleStatusBarMenuClose(void)
{
	list<StatusBarPart_t>::iterator	itr;
	RECT							rcClient;
	int								i = 0;

	GetClientRect(m_hContainer,&rcClient);

	/* Restore the normal status bar parts. */
	SetStatusBarParts(GetRectWidth(&rcClient));

	for(itr = m_StatusBarParts.begin();itr != m_StatusBarParts.end();itr++)
	{
		/* Restore the text that was present before the menu was opened. */
		SendMessage(m_hStatusBar,SB_SETTEXT,(WPARAM)i|0,(LPARAM)itr->szText);

		i++;
	}

	m_StatusBarParts.clear();

	m_bAlteredStatusBarParts = FALSE;
}

void CContainer::ShowHiddenFiles(void)
{
	m_pActiveShellBrowser->ToggleShowHidden();

	RefreshTab(m_iObjectIndex);
}

void CContainer::CopyToFolder(BOOL bMove)
{
	TCHAR	*FileNameList = NULL;
	int		nSelected;

	nSelected = ListView_GetSelectedCount(m_hActiveListView);

	if(nSelected == 0)
		return;

	FileNameList = (TCHAR *)malloc(nSelected * MAX_PATH * sizeof(TCHAR));

	if(FileNameList != NULL)
	{
		CreateListViewFileList(FileNameList,nSelected * MAX_PATH);
		CopyFilesToFolder(m_hContainer,FileNameList,bMove);

		free(FileNameList);
	}
}

HRESULT CContainer::OnDeviceChange(WPARAM wParam,LPARAM lParam)
{
	TCITEM tcItem;
	int nTabs;
	int i = 0;

	/* Forward this notification out to all tabs (if a
	tab is currently in my computer, it will need to
	update its contents). */
	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	for(i = 0;i < nTabs;i++)
	{
		tcItem.mask = TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);

		m_pShellBrowser[(int)tcItem.lParam]->OnDeviceChange(wParam,lParam);
	}

	/* Forward the message to the treeview, so that
	it can handle the message as well. */
	SendMessage(m_hTreeView,WM_DEVICECHANGE,wParam,lParam);

	switch(wParam)
	{
		/* Device has being added/inserted into the system. Update the
		drives toolbar as necessary. */
		case DBT_DEVICEARRIVAL:
			{
				DEV_BROADCAST_HDR *dbh = NULL;

				dbh = (DEV_BROADCAST_HDR *)lParam;

				if(dbh->dbch_devicetype == DBT_DEVTYP_VOLUME)
				{
					DEV_BROADCAST_VOLUME	*pdbv = NULL;
					TCHAR					chDrive;
					TCHAR					szDrive[4];

					pdbv = (DEV_BROADCAST_VOLUME *)dbh;

					/* Build a string that will form the drive name. */
					chDrive = GetDriveNameFromMask(pdbv->dbcv_unitmask);
					StringCchPrintf(szDrive,SIZEOF_ARRAY(szDrive),
						_T("%c:\\"),chDrive);

					/* Is there a change in media, or a change
					in the physical device?
					If this is only a change in media, simply
					update any icons that may need updating;
					else if this is a change in the physical
					device, add the device in the required
					areas. */
					if(pdbv->dbcv_flags & DBTF_MEDIA)
					{
						UpdateDrivesToolbarIcon(szDrive);
					}
					else
					{
						/* Add the drive to the toolbar. The button
						will automatically be inserted into its
						correct, sorted position. */
						InsertDriveIntoDrivesToolbar(szDrive);

						UpdateToolbarBandSizing(m_hMainRebar,m_hDrivesToolbar);
					}
				}
			}
			break;

		case DBT_DEVICEREMOVECOMPLETE:
			{
				DEV_BROADCAST_HDR				*dbh = NULL;

				dbh = (DEV_BROADCAST_HDR *)lParam;

				switch(dbh->dbch_devicetype)
				{
					case DBT_DEVTYP_VOLUME:
						{
							DEV_BROADCAST_VOLUME	*pdbv = NULL;
							TCHAR					chDrive;
							TCHAR					szDrive[4];

							pdbv = (DEV_BROADCAST_VOLUME *)dbh;

							/* Build a string that will form the drive name. */
							chDrive = GetDriveNameFromMask(pdbv->dbcv_unitmask);
							StringCchPrintf(szDrive,SIZEOF_ARRAY(szDrive),_T("%c:\\"),chDrive);

							/* Media changed or drive removed? */
							if(pdbv->dbcv_flags & DBTF_MEDIA)
							{
								UpdateDrivesToolbarIcon(szDrive);
							}
							else
							{
								/* The device was removed from the system.
								Remove its button from the drive toolbar. */
								RemoveDriveFromDrivesToolbar(szDrive);

								UpdateToolbarBandSizing(m_hMainRebar,m_hDrivesToolbar);
							}
						}
						break;
				}

				return TRUE;
			}
			break;
	}

	return E_NOTIMPL;
}

void CContainer::OpenAllSelectedItems(BOOL bOpenInNewTab)
{
	BOOL	m_bSeenDirectory = FALSE;
	DWORD	dwAttributes;
	int		iItem = -1;
	int		iFolderItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVIS_SELECTED)) != -1)
	{
		dwAttributes = m_pActiveShellBrowser->QueryFileAttributes(iItem);

		if((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		{
			m_bSeenDirectory = TRUE;
			iFolderItem = iItem;
		}
		else
		{
			OpenListViewItem(iItem,FALSE,FALSE);
		}
	}

	if(m_bSeenDirectory)
		OpenListViewItem(iFolderItem,bOpenInNewTab,FALSE);
}

void CContainer::SetComboBoxExTitleString(HWND CbEx,LPITEMIDLIST pidl,TCHAR *szDisplayText)
{
	COMBOBOXEXITEM	cbItem;
	SHFILEINFO		shfi;

	SHGetFileInfo((LPTSTR)pidl,NULL,&shfi,NULL,SHGFI_PIDL|SHGFI_SYSICONINDEX);

	SendMessage(CbEx,CB_RESETCONTENT,0,0);

	cbItem.mask				= CBEIF_TEXT|CBEIF_IMAGE|CBEIF_INDENT|CBEIF_SELECTEDIMAGE;
	cbItem.iItem			= -1;
	cbItem.iImage			= shfi.iIcon;
	cbItem.iSelectedImage	= shfi.iIcon;
	cbItem.iIndent			= 1;
	cbItem.iOverlay			= 1;
	cbItem.pszText			= szDisplayText;

	SendMessage(CbEx,CBEM_SETITEM,(WPARAM)0,(LPARAM)&cbItem);
}

HRESULT CContainer::TestListViewSelectionAttributes(SFGAOF *pItemAttributes)
{
	LPITEMIDLIST	pidlDirectory = NULL;
	LPITEMIDLIST	ridl = NULL;
	LPITEMIDLIST	pidlComplete = NULL;
	HRESULT			hr = E_FAIL;
	int				iSelected;

	iSelected = ListView_GetNextItem(m_hActiveListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
	{
		pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

		ridl = m_pActiveShellBrowser->QueryItemRelativeIdl(iSelected);

		pidlComplete = ILCombine(pidlDirectory,ridl);

		hr = GetItemAttributes(pidlComplete,pItemAttributes);

		CoTaskMemFree(pidlComplete);
		CoTaskMemFree(ridl);
		CoTaskMemFree(pidlDirectory);
	}

	return hr;
}

HRESULT CContainer::TestTreeViewSelectionAttributes(SFGAOF *pItemAttributes)
{
	HTREEITEM		hItem;
	LPITEMIDLIST	pidl = NULL;
	HRESULT			hr = E_FAIL;

	hItem = TreeView_GetSelection(m_hTreeView);

	if(hItem != NULL)
	{
		pidl = m_pMyTreeView->BuildPath(hItem);

		hr = GetItemAttributes(pidl,pItemAttributes);

		CoTaskMemFree(pidl);
	}

	return hr;
}

BOOL CContainer::IsRenamePossible(void)
{
	SFGAOF	ItemAttributes;
	HRESULT	hr;

	ItemAttributes = SFGAO_CANRENAME;

	hr = GetSelectionAttributes(&ItemAttributes);

	if(SUCCEEDED(hr))
		return (ItemAttributes & SFGAO_CANRENAME) == SFGAO_CANRENAME;

	return FALSE;
}

BOOL CContainer::IsDeletionPossible(void)
{
	SFGAOF	ItemAttributes;
	HRESULT	hr;

	ItemAttributes = SFGAO_CANDELETE;

	hr = GetSelectionAttributes(&ItemAttributes);

	if(SUCCEEDED(hr))
		return (ItemAttributes & SFGAO_CANDELETE) == SFGAO_CANDELETE;

	return FALSE;
}

HRESULT CContainer::GetSelectionAttributes(SFGAOF *pItemAttributes)
{
	HWND	hFocus;
	HRESULT	hr = E_FAIL;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
		hr = TestListViewSelectionAttributes(pItemAttributes);
	else if(hFocus == m_hTreeView)
		hr = TestTreeViewSelectionAttributes(pItemAttributes);

	return hr;
}

BOOL CContainer::CanShowFileProperties(void)
{
	SFGAOF	ItemAttributes;
	HRESULT	hr;

	ItemAttributes = SFGAO_HASPROPSHEET;

	hr = GetSelectionAttributes(&ItemAttributes);

	if(SUCCEEDED(hr))
		return (ItemAttributes & SFGAO_HASPROPSHEET) == SFGAO_HASPROPSHEET;

	return FALSE;
}

BOOL CContainer::CanCutOrCopySelection(void)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		return m_nSelected && AreAllSelectedFilesReal();
	}
	else if(hFocus == m_hTreeView)
	{
		HTREEITEM		hItem;
		LPITEMIDLIST	pidl = NULL;
		SFGAOF			Attributes;
		HRESULT			hr;

		hItem = TreeView_GetSelection(m_hTreeView);

		if(hItem != NULL)
		{
			pidl = m_pMyTreeView->BuildPath(hItem);

			Attributes = SFGAO_CANCOPY;// | SFGAO_CANMOVE;

			hr = GetItemAttributes(pidl,&Attributes);

			CoTaskMemFree(pidl);

			if(hr == S_OK)
				return TRUE;
		}

		return FALSE;
	}

	return FALSE;
}

BOOL CContainer::CanPaste(void)
{
	HWND hFocus = GetFocus();

	std::list<FORMATETC> ftcList;
	CDropHandler::GetDropFormats(&ftcList);

	BOOL bDataAvailable = FALSE;

	/* Check whether the drop source has the type of data
	that is needed for this drag operation. */
	for each(auto ftc in ftcList)
	{
		if(IsClipboardFormatAvailable(ftc.cfFormat))
		{
			bDataAvailable = TRUE;
			break;
		}
	}

	if(hFocus == m_hActiveListView)
	{
		return bDataAvailable && m_pActiveShellBrowser->CanCreate();
	}
	else if(hFocus == m_hTreeView)
	{
		HTREEITEM		hItem;
		LPITEMIDLIST	pidl = NULL;
		SFGAOF			Attributes;
		HRESULT			hr;

		hItem = TreeView_GetSelection(m_hTreeView);

		if(hItem != NULL)
		{
			pidl = m_pMyTreeView->BuildPath(hItem);

			Attributes = SFGAO_FILESYSTEM;

			hr = GetItemAttributes(pidl,&Attributes);

			CoTaskMemFree(pidl);

			if(hr == S_OK)
				return bDataAvailable;
		}
	}

	return FALSE;
}

BOOL CContainer::AreAllSelectedFilesReal(void)
{
	int iItem = -1;

	if(m_nSelected == 0)
		return FALSE;

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
	{
		if(!m_pActiveShellBrowser->IsFileReal(iItem))
			return FALSE;
	}

	return TRUE;
}

HRESULT CContainer::HandleStatusText(void)
{
	FolderInfo_t	FolderInfo;
	int				nTotal;
	int				nFilesSelected;
	int				nFoldersSelected;
	TCHAR			szItemsSelected[64];
	TCHAR			lpszSizeBuffer[32];
	TCHAR			szBuffer[64];
	TCHAR			szTemp[64];
	TCHAR			*szNumSelected = NULL;
	int				res;

	nTotal				= m_pActiveShellBrowser->QueryNumItems();
	nFilesSelected		= m_pActiveShellBrowser->QueryNumSelectedFiles();
	nFoldersSelected	= m_pActiveShellBrowser->QueryNumSelectedFolders();

	if((nFilesSelected + nFoldersSelected) != 0)
	{
		szNumSelected = PrintComma(nFilesSelected + nFoldersSelected);

		if((nFilesSelected + nFoldersSelected) == 1)
		{
			LoadString(g_hLanguageModule,IDS_GENERAL_SELECTED_ONEITEM,szTemp,
				SIZEOF_ARRAY(szTemp));

			/* One item selected. Form:
			1 item selected */
			StringCchPrintf(szItemsSelected,SIZEOF_ARRAY(szItemsSelected),
			_T("%s %s"),szNumSelected,szTemp);
		}
		else
		{
			LoadString(g_hLanguageModule,IDS_GENERAL_SELECTED_MOREITEMS,szTemp,
				SIZEOF_ARRAY(szTemp));

			/* More than one item selected. Form:
			n items selected */
			StringCchPrintf(szItemsSelected,SIZEOF_ARRAY(szItemsSelected),
			_T("%s %s"),szNumSelected,szTemp);
		}
	}
	else
	{
		szNumSelected = PrintComma(nTotal);

		if(nTotal == 1)
		{
			LoadString(g_hLanguageModule,IDS_GENERAL_ONEITEM,szTemp,
				SIZEOF_ARRAY(szTemp));

			/* Text: '1 item' */
			StringCchPrintf(szItemsSelected,SIZEOF_ARRAY(szItemsSelected),
			_T("%s %s"),szNumSelected,szTemp);
		}
		else
		{
			LoadString(g_hLanguageModule,IDS_GENERAL_MOREITEMS,szTemp,
				SIZEOF_ARRAY(szTemp));

			/* Text: 'n Items' */
			StringCchPrintf(szItemsSelected,SIZEOF_ARRAY(szItemsSelected),
			_T("%s %s"),szNumSelected,szTemp);
		}
	}

	SendMessage(m_hStatusBar,SB_SETTEXT,(WPARAM)0|0,(LPARAM)szItemsSelected);

	if(m_pActiveShellBrowser->InVirtualFolder())
	{
		LoadString(g_hLanguageModule,IDS_GENERAL_VIRTUALFOLDER,lpszSizeBuffer,
			SIZEOF_ARRAY(lpszSizeBuffer));
	}
	else
	{
		m_pActiveShellBrowser->QueryFolderInfo(&FolderInfo);

		if((nFilesSelected + nFoldersSelected) == 0)
		{
			/* No items(files or folders) selected. */
			FormatSizeString(FolderInfo.TotalFolderSize,lpszSizeBuffer,
				SIZEOF_ARRAY(lpszSizeBuffer),m_bForceSize,m_SizeDisplayFormat);
		}
		else
		{
			if(nFilesSelected == 0)
			{
				/* Only folders selected. Don't show any size in the status bar. */
				StringCchCopy(lpszSizeBuffer,SIZEOF_ARRAY(lpszSizeBuffer),EMPTY_STRING);
			}
			else
			{
				/* Mixture of files and folders selected. Show size of currently
				selected files. */
				FormatSizeString(FolderInfo.TotalSelectionSize,lpszSizeBuffer,
					SIZEOF_ARRAY(lpszSizeBuffer),m_bForceSize,m_SizeDisplayFormat);
			}
		}
	}

	SendMessage(m_hStatusBar,SB_SETTEXT,(WPARAM)1|0,(LPARAM)lpszSizeBuffer);

	res = CreateDriveFreeSpaceString(m_CurrentDirectory,szBuffer,SIZEOF_ARRAY(szBuffer));

	if(res == -1)
		StringCchCopy(szBuffer,SIZEOF_ARRAY(szBuffer),EMPTY_STRING);

	SendMessage(m_hStatusBar,SB_SETTEXT,(WPARAM)2|0,(LPARAM)szBuffer);

	return S_OK;
}

BOOL CContainer::CompareVirtualFolders(UINT uFolderCSIDL)
{
	TCHAR szParsingPath[MAX_PATH];

	GetVirtualFolderParsingPath(uFolderCSIDL,szParsingPath);

	if(StrCmp(m_CurrentDirectory,szParsingPath) == 0)
		return TRUE;

	return FALSE;
}

/*
RUNS IN CONTEXT OF DIRECTORY MOINTORING WORKER THREAD.
Possible bugs:
 - The tab may exist at the point of call that checks
   whether or not the tab index has been freed. However,
   it's possible it may not exist directly after.
   Therefore, use a critical section to ensure a tab cannot
   be freed until at least this call completes.

   If this runs before the tab is freed, the tab existence
   check will succeed, the shell browser function will be called
   and this function will exit.
   If this runs after the tab is freed, the tab existence
   check will fail, and the shell browser function won't be called.
*/
void CContainer::DirectoryAlteredCallback(TCHAR *szFileName,DWORD dwAction,
void *pData)
{
	DirectoryAltered_t	*pDirectoryAltered = NULL;
	CContainer			*pContainer = NULL;

	EnterCriticalSection(&g_csDirMonCallback);

	pDirectoryAltered = (DirectoryAltered_t *)pData;
	pContainer = (CContainer *)pDirectoryAltered->pData;

	/* Does this tab still exist? */
	if(pContainer->m_uTabMap[pDirectoryAltered->iIndex] == 1)
	{
		pContainer->m_pShellBrowser[pDirectoryAltered->iIndex]->FilesModified(dwAction,
			szFileName,pDirectoryAltered->iIndex,pDirectoryAltered->iFolderIndex);
	}

	LeaveCriticalSection(&g_csDirMonCallback);
}

void CContainer::HandleFileSelectionDisplay(void)
{
	int nSelected;

	DisplayWindow_ClearTextBuffer(m_hDisplayWindow);

	nSelected = m_pActiveShellBrowser->QueryNumSelected();

	if(nSelected == 0)
		HandleFileSelectionDisplayZero();
	else if(nSelected == 1)
		HandleFileSelectionDisplayOne();
	else if(nSelected > 1)
		HandleFileSelectionDisplayMore();
}

void CContainer::HandleFileSelectionDisplayZero(void)
{
	SHFILEINFO		shfi;
	TCHAR			szFolderName[MAX_PATH];
	TCHAR			szCurrentDirectory[MAX_PATH];
	LPITEMIDLIST	pidlDirectory = NULL;
	LPITEMIDLIST	pidlComputer = NULL;

	/* Clear out any previous data shown in the display window. */
	DisplayWindow_ClearTextBuffer(m_hDisplayWindow);
	DisplayWindow_SetThumbnailFile(m_hDisplayWindow,EMPTY_STRING,FALSE);

	m_pActiveShellBrowser->QueryCurrentDirectory(MAX_PATH,szCurrentDirectory);
	pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

	SHGetFolderLocation(NULL,CSIDL_DRIVES,NULL,0,&pidlComputer);

	if(CompareIdls(pidlDirectory,pidlComputer))
	{
		char szCPUBrand[64];
		WCHAR wszCPUBrand[64];
		TCHAR szDisplay[512];
		TCHAR szTemp[512];
		DWORD dwSize;

		dwSize = SIZEOF_ARRAY(szDisplay);
		GetComputerName(szDisplay,&dwSize);
		DisplayWindow_BufferText(m_hDisplayWindow,szDisplay);


		GetCPUBrandString(szCPUBrand,SIZEOF_ARRAY(szCPUBrand));

		#ifndef UNICODE
		StringCchCopy(wszCPUBrand,SIZEOF_ARRAY(wszCPUBrand),szCPUBrand);
		#else
		MultiByteToWideChar(CP_ACP,0,szCPUBrand,-1,wszCPUBrand,SIZEOF_ARRAY(wszCPUBrand));
		#endif

		StringCchPrintf(szDisplay,SIZEOF_ARRAY(szDisplay),_T("Processor: %s"),wszCPUBrand);
		DisplayWindow_BufferText(m_hDisplayWindow,szDisplay);


		MEMORYSTATUSEX msex;

		msex.dwLength = sizeof(msex);
		GlobalMemoryStatusEx(&msex);

		ULARGE_INTEGER lTotalPhysicalMem;

		lTotalPhysicalMem.QuadPart = msex.ullTotalPhys;
		FormatSizeString(lTotalPhysicalMem,szTemp,
			SIZEOF_ARRAY(szTemp),TRUE);
		StringCchPrintf(szDisplay,SIZEOF_ARRAY(szDisplay),
			_T("Memory: %s"),szTemp);
		DisplayWindow_BufferText(m_hDisplayWindow,szDisplay);
	}
	else
	{
		/* Folder name. */
		GetDisplayName(szCurrentDirectory,szFolderName,SHGDN_INFOLDER);
		DisplayWindow_BufferText(m_hDisplayWindow,szFolderName);

		/* Folder type. */
		SHGetFileInfo((LPTSTR)pidlDirectory,NULL,&shfi,
			sizeof(shfi),SHGFI_PIDL|SHGFI_TYPENAME);
		DisplayWindow_BufferText(m_hDisplayWindow,shfi.szTypeName);
	}

	CoTaskMemFree(pidlDirectory);
}

void CContainer::HandleFileSelectionDisplayOne(void)
{
	WIN32_FIND_DATA	*pwfd = NULL;
	SHFILEINFO		shfi;
	TCHAR			szFullItemName[MAX_PATH];
	TCHAR			szFileDate[256];
	TCHAR			szDisplayDate[MAX_STRING_LENGTH];
	TCHAR			szDisplayName[MAX_PATH];
	TCHAR			szDateModified[256];
	int				iSelected;
	list<DWRule_t>::iterator itr;
	list<DWFileType_t>::iterator itrTypes;
	list<DWLine_t>::iterator itrLines;

	iSelected = ListView_GetNextItem(m_hActiveListView,-1,LVNI_SELECTED);

	if(iSelected != -1)
	{
		m_pActiveShellBrowser->QueryDisplayName(iSelected,
			MAX_PATH,szDisplayName);

		/* File name. */
		DisplayWindow_BufferText(m_hDisplayWindow,szDisplayName);

		m_pActiveShellBrowser->QueryFullItemName(iSelected,szFullItemName);

		if(!m_pActiveShellBrowser->InVirtualFolder())
		{
			DWORD dwAttributes;

			m_pActiveShellBrowser->QueryFullItemName(iSelected,szFullItemName);

			pwfd = m_pActiveShellBrowser->QueryFileFindData(iSelected);

			dwAttributes = GetFileAttributes(szFullItemName);

			if(((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
				FILE_ATTRIBUTE_DIRECTORY) && m_bShowFolderSizes)
			{
				FolderSize_t	*pfs = NULL;
				FolderSizeExtraInfo_t	*pfsei = NULL;
				DWFolderSize_t	DWFolderSize;
				TCHAR			szDisplayText[256];
				TCHAR			szTotalSize[64];
				TCHAR			szCalculating[64];
				DWORD			ThreadId;

				pfs = (FolderSize_t *)malloc(sizeof(FolderSize_t));

				if(pfs != NULL)
				{
					pfsei = (FolderSizeExtraInfo_t *)malloc(sizeof(FolderSizeExtraInfo_t));

					if(pfsei != NULL)
					{
						pfsei->pContainer	= (void *)this;
						pfsei->uId			= m_iDWFolderSizeUniqueId;
						pfs->pData			= (LPVOID)pfsei;

						pfs->pfnCallback	= FolderSizeCallbackStub;

						StringCchCopy(pfs->szPath,MAX_PATH,szFullItemName);

						LoadString(g_hLanguageModule,IDS_GENERAL_TOTALSIZE,
							szTotalSize,SIZEOF_ARRAY(szTotalSize));
						LoadString(g_hLanguageModule,IDS_GENERAL_CALCULATING,
							szCalculating,SIZEOF_ARRAY(szCalculating));
						StringCchPrintf(szDisplayText,SIZEOF_ARRAY(szDisplayText),
							_T("%s: %s"),szTotalSize,szCalculating);
						DisplayWindow_BufferText(m_hDisplayWindow,szDisplayText);

						/* Maintain a global list of folder size operations. */
						DWFolderSize.uId	= m_iDWFolderSizeUniqueId;
						DWFolderSize.iTabId	= m_iObjectIndex;
						DWFolderSize.bValid	= TRUE;
						m_DWFolderSizes.push_back(DWFolderSize);

						CreateThread(NULL,0,Thread_CalculateFolderSize,(LPVOID)pfs,0,&ThreadId);

						m_iDWFolderSizeUniqueId++;
					}
					else
					{
						free(pfs);
					}
				}
			}
			else
			{
				SHGetFileInfo(szFullItemName,pwfd->dwFileAttributes,
					&shfi,sizeof(shfi),SHGFI_TYPENAME|SHGFI_USEFILEATTRIBUTES);

				DisplayWindow_BufferText(m_hDisplayWindow,shfi.szTypeName);
			}

			CreateFileTimeString(&pwfd->ftLastWriteTime,
				szFileDate,SIZEOF_ARRAY(szFileDate),
				m_bShowFriendlyDatesGlobal);

			LoadString(g_hLanguageModule,IDS_GENERAL_DATEMODIFIED,szDateModified,
				SIZEOF_ARRAY(szDateModified));

			StringCchPrintf(szDisplayDate,
			SIZEOF_ARRAY(szDisplayDate),
			_T("%s: %s"),szDateModified,szFileDate);

			/* File (modified) date. */
			DisplayWindow_BufferText(m_hDisplayWindow,szDisplayDate);

			if(IsImage(szFullItemName))
			{
				TCHAR szOutput[256];
				UINT uWidth;
				UINT uHeight;
				Image *pimg = NULL;

				pimg = new Image(szFullItemName,FALSE);

				if(pimg->GetLastStatus() == Ok)
				{
					/* String table. */
					uWidth = pimg->GetWidth();
					StringCchPrintf(szOutput,SIZEOF_ARRAY(szOutput),_T("Width: %u pixels"),uWidth);
					DisplayWindow_BufferText(m_hDisplayWindow,szOutput);

					/* String table. */
					uHeight = pimg->GetHeight();
					StringCchPrintf(szOutput,SIZEOF_ARRAY(szOutput),_T("Height: %u pixels"),uHeight);
					DisplayWindow_BufferText(m_hDisplayWindow,szOutput);

					PixelFormat format;
					UINT uBitDepth;

					format = pimg->GetPixelFormat();

					switch(format)
					{
					case PixelFormat1bppIndexed:
						uBitDepth = 1;
						break;

					case PixelFormat4bppIndexed:
						uBitDepth = 4;
						break;

					case PixelFormat8bppIndexed:
						uBitDepth = 8;
						break;

					case PixelFormat16bppARGB1555:
					case PixelFormat16bppGrayScale:
					case PixelFormat16bppRGB555:
					case PixelFormat16bppRGB565:
						uBitDepth = 16;
						break;

					case PixelFormat24bppRGB:
						uBitDepth = 24;
						break;

					case PixelFormat32bppARGB:
					case PixelFormat32bppPARGB:
					case PixelFormat32bppRGB:
						uBitDepth = 32;
						break;

					case PixelFormat48bppRGB:
						uBitDepth = 48;
						break;

					case PixelFormat64bppARGB:
					case PixelFormat64bppPARGB:
						uBitDepth = 64;
						break;

					default:
						uBitDepth = 0;
						break;
					}

					/* String table. */
					if(uBitDepth == -1)
						StringCchPrintf(szOutput,SIZEOF_ARRAY(szOutput),_T("Bit depth: Unknown"));
					else
						StringCchPrintf(szOutput,SIZEOF_ARRAY(szOutput),_T("Bit depth: %u"),uBitDepth);

					DisplayWindow_BufferText(m_hDisplayWindow,szOutput);

					REAL res;

					res = pimg->GetHorizontalResolution();
					StringCchPrintf(szOutput,SIZEOF_ARRAY(szOutput),_T("Horizontal resolution: %.0f dpi"),res);
					DisplayWindow_BufferText(m_hDisplayWindow,szOutput);

					res = pimg->GetVerticalResolution();
					StringCchPrintf(szOutput,SIZEOF_ARRAY(szOutput),_T("Vertical resolution: %.0f dpi"),res);
					DisplayWindow_BufferText(m_hDisplayWindow,szOutput);
				}

				delete pimg;
			}

			/* Only attempt to show file previews for files (not folders). Also, only
			attempt to show a preview if the display window is actually active. */
			if(((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
				FILE_ATTRIBUTE_DIRECTORY) && m_bShowFilePreviews
				&& m_bShowDisplayWindow)
			{
				DisplayWindow_SetThumbnailFile(m_hDisplayWindow,szFullItemName,TRUE);
			}
			else
			{
				DisplayWindow_SetThumbnailFile(m_hDisplayWindow,EMPTY_STRING,FALSE);
			}
		}
		else
		{
			m_pActiveShellBrowser->QueryFullItemName(iSelected,szFullItemName);

			if(PathIsRoot(szFullItemName))
			{
				ULARGE_INTEGER ulTotalNumberOfBytes;
				ULARGE_INTEGER ulTotalNumberOfFreeBytes;
				TCHAR szSize[32];
				TCHAR szFileSystem[MAX_PATH + 1];
				TCHAR szMsg[64];
				BOOL bRet;

				bRet = GetDiskFreeSpaceEx(szFullItemName,NULL,&ulTotalNumberOfBytes,
					&ulTotalNumberOfFreeBytes);

				if(bRet)
				{
					FormatSizeString(ulTotalNumberOfFreeBytes,szSize,
						SIZEOF_ARRAY(szSize),FALSE);
					StringCchPrintf(szMsg,SIZEOF_ARRAY(szMsg),_T("Free Space: %s"),szSize);
					DisplayWindow_BufferText(m_hDisplayWindow,szMsg);

					FormatSizeString(ulTotalNumberOfBytes,szSize,SIZEOF_ARRAY(szSize),FALSE);
					StringCchPrintf(szMsg,SIZEOF_ARRAY(szMsg),_T("Total Size: %s"),szSize);
					DisplayWindow_BufferText(m_hDisplayWindow,szMsg);
				}

				bRet = GetVolumeInformation(szFullItemName,NULL,0,NULL,NULL,NULL,szFileSystem,
					SIZEOF_ARRAY(szFileSystem));

				if(bRet)
				{
					StringCchPrintf(szMsg,SIZEOF_ARRAY(szMsg),_T("File System: %s"),szFileSystem);
					DisplayWindow_BufferText(m_hDisplayWindow,szMsg);
				}
			}
		}
	}
}

void CContainer::FormatDisplayString(TCHAR *szDisplayRaw,int iSelected,
TCHAR *szFullFileName,TCHAR *szDisplayFinal,UINT cchMax)
{
	TCHAR szOutputTemp[512];
	TCHAR szTemp[512];
	BOOL bTrackingBrace = FALSE;
	int iOutputTemp = 0;
	int iTemp = 0;
	int i = 0;

	/* Read the raw buffer, placing characters into temporary buffer.
	If a '{' character is encountered, hold it and what follows in a
	separate buffer. If the end of the string is reached before a
	matching '}' is found, just copy this to the output buffer.
	If a matching '}' is found, but the text does not match any special
	character, just copy it to the output. If a matching '}' is found,
	and the text does match a special symbol, translate it. */

	for(i = 0;i < lstrlen(szDisplayRaw);i++)
	while(i < lstrlen(szDisplayRaw) && iOutputTemp < (SIZEOF_ARRAY(szOutputTemp) - 1))
	{
		if(bTrackingBrace)
		{
			if(iTemp < (SIZEOF_ARRAY(szTemp) - 1))
				szTemp[iTemp++] = szDisplayRaw[i];

			/* Reached a closing brace, so translate
			what we have. */
			if(szDisplayRaw[i] == '}')
			{
				int iCopy;

				szTemp[iTemp++] = '\0';

				TranslateDisplayWindowBuffer(szTemp,SIZEOF_ARRAY(szTemp),iSelected,szFullFileName);

				iCopy = min((unsigned int)lstrlen(szTemp),(unsigned int)(SIZEOF_ARRAY(szOutputTemp) - iOutputTemp - 1));
				memcpy(&szOutputTemp[iOutputTemp],szTemp,iCopy * sizeof(TCHAR));

				iOutputTemp += lstrlen(szTemp);

				bTrackingBrace = FALSE;
			}
		}
		else if(szDisplayRaw[i] == '{')
		{
			iTemp = 0;

			szTemp[iTemp++] = szDisplayRaw[i];

			bTrackingBrace = TRUE;
		}
		else
		{
			szOutputTemp[iOutputTemp++] = szDisplayRaw[i];
		}

		i++;
	}

	szOutputTemp[iOutputTemp++] = '\0';

	if(bTrackingBrace)
		StringCchCat(szOutputTemp,SIZEOF_ARRAY(szOutputTemp),szTemp);

	StringCchCopy(szDisplayFinal,cchMax,szOutputTemp);
}

void CContainer::TranslateDisplayWindowBuffer(TCHAR *szSymbol,UINT cchMax,int iSelected,TCHAR *szFullFileName)
{
	if(lstrcmp(szSymbol,_T("{name}")) == 0)
	{
		TCHAR szOutput[MAX_PATH];

		StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),szFullFileName);
		PathStripPath(szOutput);
		StringCchCopy(szSymbol,cchMax,szOutput);
	}
	else if(lstrcmp(szSymbol,_T("{date_modified}")) == 0)
	{
		WIN32_FIND_DATA	*pwfd = NULL;
		TCHAR szFileDate[256];

		pwfd = m_pActiveShellBrowser->QueryFileFindData(iSelected);

		CreateFileTimeString(&pwfd->ftLastWriteTime,
			szFileDate,SIZEOF_ARRAY(szFileDate),
			m_bShowFriendlyDatesGlobal);

		StringCchCopy(szSymbol,cchMax,szFileDate);
	}
	else if(lstrcmp(szSymbol,_T("{type}")) == 0)
	{
		WIN32_FIND_DATA	*pwfd = NULL;
		SHFILEINFO		shfi;

		pwfd = m_pActiveShellBrowser->QueryFileFindData(iSelected);

		SHGetFileInfo(szFullFileName,pwfd->dwFileAttributes,
			&shfi,sizeof(shfi),SHGFI_TYPENAME|SHGFI_USEFILEATTRIBUTES);

		StringCchCopy(szSymbol,cchMax,shfi.szTypeName);
	}
	else if(lstrcmp(szSymbol,_T("{width}")) == 0)
	{
		Image *pimg = NULL;
		TCHAR szOutput[256];
		UINT uWidth;

		pimg = new Image(szFullFileName,FALSE);

		if(pimg->GetLastStatus() == Ok)
		{
			uWidth = pimg->GetWidth();
			StringCchPrintf(szOutput,SIZEOF_ARRAY(szOutput),_T("%u"),uWidth);
			StringCchCopy(szSymbol,cchMax,szOutput);

			delete pimg;
		}
	}
	else if(lstrcmp(szSymbol,_T("{height}")) == 0)
	{
		Image *pimg = NULL;
		TCHAR szOutput[256];
		UINT uHeight;

		pimg = new Image(szFullFileName,FALSE);

		if(pimg->GetLastStatus() == Ok)
		{
			uHeight = pimg->GetHeight();
			StringCchPrintf(szOutput,SIZEOF_ARRAY(szOutput),_T("%u"),uHeight);
			StringCchCopy(szSymbol,cchMax,szOutput);
			delete pimg;
		}
	}
}

void CContainer::OutputInformationOnDisplayWindow(TCHAR *szFullFileName,DWInfoType_t InfoType)
{
	switch(InfoType)
	{
		case DW_NAME:
			{
				TCHAR szOutput[MAX_PATH];

				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),szFullFileName);
				PathStripPath(szOutput);
				DisplayWindow_BufferText(m_hDisplayWindow,szOutput);
			}
			break;

		case DW_TYPE:
			break;
		case DW_SIZE:
			break;

		case DW_DATEMODIFIED:
			break;
		case DW_CREATED:
			break;
		case DW_ACCESSED:
			break;

		case DW_ATTRIBUTES:
			break;
		case DW_REALSIZE:
			break;
		case DW_SHORTNAME:
			break;
		case DW_OWNER:
			break;

		case DW_PRODUCTNAME:
			break;
		case DW_COMPANY:
			break;
		case DW_DESCRIPTION:
			break;
		case DW_FILEVERSION:
			break;
		case DW_PRODUCTVERSION:
			break;

		case DW_SHORTCUTTO:
			break;
		case DW_HARDLINKS:
			break;
		case DW_EXTENSION:
			break;

		case DW_TITLE:
			break;
		case DW_SUBJECT:
			break;
		case DW_AUTHOR:
			break;
		case DW_KEYWORDS:
			break;
		case DW_COMMENT:
			break;

		case DW_CAMERAMODEL:
			break;
		case DW_DATETAKEN:
			break;
		case DW_WIDTH:
			break;
		case DW_HEIGHT:
			break;

		case DW_VIRTUALCOMMENTS:
			break;

		case DW_VIRTUALTYPE:
			break;

		case DW_TOTALSIZE:
			break;
		case DW_FREESPACE:
			break;

		case DW_FILESYSTEM:
			break;

		case DW_NUMPRINTERDOCUMENTS:
			break;

		case DW_PRINTERSTATUS:
			break;

		case DW_PRINTERCOMMENTS:
			break;

		case DW_PRINTERLOCATION:
			break;

		case DW_NETWORKADAPTER_STATUS:
			break;
	}
}

void CContainer::HandleFileSelectionDisplayMore(void)
{
	TCHAR			szNumSelected[64] = EMPTY_STRING;
	TCHAR			szTotalSize[64] = EMPTY_STRING;
	TCHAR			szTotalSizeFragment[32] = EMPTY_STRING;
	TCHAR			szMore[64];
	TCHAR			szTotalSizeString[64];
	FolderInfo_t	FolderInfo;
	int				nSelected;

	DisplayWindow_SetThumbnailFile(m_hDisplayWindow,EMPTY_STRING,FALSE);

	nSelected = m_pActiveShellBrowser->QueryNumSelected();

	LoadString(g_hLanguageModule,IDS_GENERAL_SELECTED_MOREITEMS,
		szMore,SIZEOF_ARRAY(szMore));

	StringCchPrintf(szNumSelected,SIZEOF_ARRAY(szNumSelected),
	_T("%d %s"),nSelected,szMore);

	DisplayWindow_BufferText(m_hDisplayWindow,szNumSelected);

	if(!m_pActiveShellBrowser->InVirtualFolder())
	{
		m_pActiveShellBrowser->QueryFolderInfo(&FolderInfo);

		FormatSizeString(FolderInfo.TotalSelectionSize,szTotalSizeFragment,
			SIZEOF_ARRAY(szTotalSizeFragment),m_bForceSize,m_SizeDisplayFormat);

		LoadString(g_hLanguageModule,IDS_GENERAL_TOTALFILESIZE,
		szTotalSizeString,SIZEOF_ARRAY(szTotalSizeString));

		StringCchPrintf(szTotalSize,SIZEOF_ARRAY(szTotalSize),
		_T("%s: %s"),szTotalSizeString,szTotalSizeFragment);
	}

	DisplayWindow_BufferText(m_hDisplayWindow,szTotalSize);
}

void FolderSizeCallbackStub(int nFolders,int nFiles,PULARGE_INTEGER lTotalFolderSize,LPVOID pData)
{
	FolderSizeExtraInfo_t *pfsei = NULL;

	pfsei = (FolderSizeExtraInfo_t *)pData;

	((CContainer *)pfsei->pContainer)->FolderSizeCallback(pfsei,nFolders,nFiles,lTotalFolderSize);

	free(pfsei);
}

void CContainer::FolderSizeCallback(FolderSizeExtraInfo_t *pfsei,
int nFolders,int nFiles,PULARGE_INTEGER lTotalFolderSize)
{
	DWFolderSizeCompletion_t *pDWFolderSizeCompletion = NULL;

	pDWFolderSizeCompletion = (DWFolderSizeCompletion_t *)malloc(sizeof(DWFolderSizeCompletion_t));

	pDWFolderSizeCompletion->liFolderSize = *lTotalFolderSize;
	pDWFolderSizeCompletion->uId = pfsei->uId;

	/* Queue the result back to the main thread, so that
	the folder size can be displayed. It is up to the main
	thread to determine whether the folder size should actually
	be shown. */
	PostMessage(m_hContainer,WM_APP_FOLDERSIZECOMPLETED,
		(WPARAM)pDWFolderSizeCompletion,0);
}

void CContainer::PushGlobalSettingsToTab(int iTabId)
{
	GlobalSettings_t gs;	

	/* These settings are global to the whole program. */
	gs.bShowExtensions		= m_bShowExtensionsGlobal;
	gs.bShowFriendlyDates	= m_bShowFriendlyDatesGlobal;
	gs.bShowFolderSizes		= m_bShowFolderSizes;

	m_pShellBrowser[iTabId]->SetGlobalSettings(&gs);
}

void CContainer::PushGlobalSettingsToAllTabs(void)
{
	TCITEM	tcItem;
	int		nTabs;
	int		i = 0;

	nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	for(i = 0;i < nTabs;i++)
	{
		tcItem.mask	= TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);

		PushGlobalSettingsToTab((int)tcItem.lParam);
	}

	RefreshAllTabs();
}

void CContainer::CreateViewsMenu(POINT *ptOrigin)
{
	UINT	uViewMode;
	int		ItemToCheck;

	m_pFolderView[m_iObjectIndex]->GetCurrentViewMode(&uViewMode);

	ItemToCheck = GetViewModeMenuId(uViewMode);
	CheckMenuRadioItem(m_hViewsMenu,IDM_VIEW_THUMBNAILS,IDM_VIEW_EXTRALARGEICONS,
		ItemToCheck,MF_BYCOMMAND);

	TrackPopupMenu(m_hViewsMenu,TPM_LEFTALIGN,ptOrigin->x,ptOrigin->y,
		0,m_hContainer,NULL);
}

int CContainer::CreateDriveFreeSpaceString(TCHAR *szPath,TCHAR *szBuffer,int nBuffer)
{
	ULARGE_INTEGER	TotalNumberOfBytes;
	ULARGE_INTEGER	TotalNumberOfFreeBytes;
	ULARGE_INTEGER	BytesAvailableToCaller;
	TCHAR			szFreeSpace[32];
	TCHAR			szFree[16];
	TCHAR			szFreeSpaceString[512];

	if(GetDiskFreeSpaceEx(szPath,&BytesAvailableToCaller,
	&TotalNumberOfBytes,&TotalNumberOfFreeBytes) == 0)
	{
		szBuffer = NULL;
		return -1;
	}

	FormatSizeString(TotalNumberOfFreeBytes,szFreeSpace,
		SIZEOF_ARRAY(szFreeSpace),FALSE);

	LoadString(g_hLanguageModule,IDS_GENERAL_FREE,szFree,SIZEOF_ARRAY(szFree));

	StringCchPrintf(szFreeSpaceString,SIZEOF_ARRAY(szFreeSpace),
	_T("%s %s (%.0f%%)"),szFreeSpace,szFree,TotalNumberOfFreeBytes.QuadPart * 100.0 / TotalNumberOfBytes.QuadPart);

	if(nBuffer > lstrlen(szFreeSpaceString))
		StringCchCopy(szBuffer,nBuffer,szFreeSpaceString);
	else
		szBuffer = NULL;

	return lstrlen(szFreeSpaceString);
}

/*
 * Returns TRUE if there are any selected items
 * in the window that currently has focus; FALSE
 * otherwise.
 */
BOOL CContainer::CheckItemSelection(void)
{
	HWND hFocus;

	hFocus = GetFocus();

	if(hFocus == m_hActiveListView)
	{
		if(ListView_GetSelectedCount(m_hActiveListView) > 0)
			return TRUE;
	}
	else if(hFocus == m_hTreeView)
	{
		if(TreeView_GetSelection(m_hTreeView) != NULL)
			return TRUE;
	}

	return FALSE;
}

/* Checks the version of the specified
language DLL against the version of the
main executable and returns TRUE if they
match. */
BOOL CContainer::VerifyLanguageVersion(TCHAR *szLanguageModule)
{
	TCHAR szImageName[MAX_PATH];
	DWORD dwpvProcessLS;
	DWORD dwpvProcessMS;
	DWORD dwpvLanguageLS;
	DWORD dwpvLanguageMS;
	DWORD dwRet;
	BOOL bSuccess1;
	BOOL bSuccess2;

	dwRet = GetCurrentProcessImageName(szImageName,SIZEOF_ARRAY(szImageName));

	if(dwRet != 0)
	{
		bSuccess1 = GetFileProductVersion(szImageName,&dwpvProcessLS,&dwpvProcessMS);
		bSuccess2 = GetFileProductVersion(szLanguageModule,&dwpvLanguageLS,&dwpvLanguageMS);

		if(bSuccess1 && bSuccess2)
		{
			if((dwpvProcessLS == dwpvLanguageLS) &&
				(dwpvProcessMS == dwpvLanguageMS))
				return TRUE;
		}
	}

	return FALSE;
}