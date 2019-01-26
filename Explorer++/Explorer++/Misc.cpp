/******************************************************************
 *
 * Project: Explorer++
 * File: Misc.cpp
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
#include "Explorer++.h"
#include "DefaultColumns.h"
#include "MainResource.h"
#include "SelectColumnsDialog.h"
#include "../Helper/Controls.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Logging.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"
#include "../DisplayWindow/DisplayWindow.h"
#include <shobjidl.h>
#include <list>


void Explorerplusplus::ValidateLoadedSettings(void)
{
	if(m_TreeViewWidth <= 0)
		m_TreeViewWidth = DEFAULT_TREEVIEW_WIDTH;

	if(m_DisplayWindowWidth < MINIMUM_DISPLAYWINDOW_WIDTH)
		m_DisplayWindowWidth = DEFAULT_DISPLAYWINDOW_WIDTH;

	if(m_DisplayWindowHeight < MINIMUM_DISPLAYWINDOW_HEIGHT)
		m_DisplayWindowHeight = DEFAULT_DISPLAYWINDOW_HEIGHT;

	ValidateColumns();
	ValidateToolbarSettings();
}

void Explorerplusplus::ValidateToolbarSettings(void)
{
	BOOL bCorrupted = FALSE;

	for(auto itr = m_tbInitial.begin();itr != m_tbInitial.end();itr++)
	{
		if(itr->iItemID < TOOLBAR_ID_START)
		{
			bCorrupted = TRUE;
			break;
		}
	}

	if(bCorrupted)
	{
		SetInitialToolbarButtons();
	}
}

void Explorerplusplus::ValidateColumns(void)
{
	ValidateSingleColumnSet(VALIDATE_REALFOLDER_COLUMNS,&m_RealFolderColumnList);
	ValidateSingleColumnSet(VALIDATE_CONTROLPANEL_COLUMNS,&m_ControlPanelColumnList);
	ValidateSingleColumnSet(VALIDATE_MYCOMPUTER_COLUMNS,&m_MyComputerColumnList);
	ValidateSingleColumnSet(VALIDATE_RECYCLEBIN_COLUMNS,&m_RecycleBinColumnList);
	ValidateSingleColumnSet(VALIDATE_PRINTERS_COLUMNS,&m_PrintersColumnList);
	ValidateSingleColumnSet(VALIDATE_NETWORKCONNECTIONS_COLUMNS,&m_NetworkConnectionsColumnList);
	ValidateSingleColumnSet(VALIDATE_MYNETWORKPLACES_COLUMNS,&m_MyNetworkPlacesColumnList);
}

void Explorerplusplus::ValidateSingleColumnSet(int iColumnSet,std::list<Column_t> *pColumnList)
{
	std::list<Column_t>::iterator	itr;
	Column_t					Column;
	int							*pColumnMap = NULL;
	BOOL						bFound = FALSE;
	const Column_t				*pColumns = NULL;
	unsigned int				iTotalColumnSize = 0;
	unsigned int				i = 0;

	switch(iColumnSet)
	{
		case VALIDATE_REALFOLDER_COLUMNS:
			iTotalColumnSize = SIZEOF_ARRAY(REAL_FOLDER_DEFAULT_COLUMNS);
			pColumns = REAL_FOLDER_DEFAULT_COLUMNS;
			break;

		case VALIDATE_CONTROLPANEL_COLUMNS:
			iTotalColumnSize = SIZEOF_ARRAY(CONTROL_PANEL_DEFAULT_COLUMNS);
			pColumns = CONTROL_PANEL_DEFAULT_COLUMNS;
			break;

		case VALIDATE_MYCOMPUTER_COLUMNS:
			iTotalColumnSize = SIZEOF_ARRAY(MY_COMPUTER_DEFAULT_COLUMNS);
			pColumns = MY_COMPUTER_DEFAULT_COLUMNS;
			break;

		case VALIDATE_RECYCLEBIN_COLUMNS:
			iTotalColumnSize = SIZEOF_ARRAY(RECYCLE_BIN_DEFAULT_COLUMNS);
			pColumns = RECYCLE_BIN_DEFAULT_COLUMNS;
			break;

		case VALIDATE_PRINTERS_COLUMNS:
			iTotalColumnSize = SIZEOF_ARRAY(PRINTERS_DEFAULT_COLUMNS);
			pColumns = PRINTERS_DEFAULT_COLUMNS;
			break;

		case VALIDATE_NETWORKCONNECTIONS_COLUMNS:
			iTotalColumnSize = SIZEOF_ARRAY(NETWORK_CONNECTIONS_DEFAULT_COLUMNS);
			pColumns = NETWORK_CONNECTIONS_DEFAULT_COLUMNS;
			break;

		case VALIDATE_MYNETWORKPLACES_COLUMNS:
			iTotalColumnSize = SIZEOF_ARRAY(MY_NETWORK_PLACES_DEFAULT_COLUMNS);
			pColumns = MY_NETWORK_PLACES_DEFAULT_COLUMNS;
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

void Explorerplusplus::ApplyLoadedSettings(void)
{
	m_pMyTreeView->SetShowHidden(m_bShowHiddenGlobal);
	ApplyDisplayWindowPosition();
}

void Explorerplusplus::ApplyDisplayWindowPosition(void)
{
	SendMessage(m_hDisplayWindow, WM_USER_DISPLAYWINDOWMOVED, m_bDisplayWindowVertical, NULL);
}

void Explorerplusplus::ApplyToolbarSettings(void)
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

void Explorerplusplus::AdjustFolderPanePosition(void)
{
	RECT rcMainWindow;
	int IndentTop		= 0;
	int IndentBottom	= 0;
	int height;

	GetClientRect(m_hContainer,&rcMainWindow);
	height = GetRectHeight(&rcMainWindow);

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

	if(m_bShowDisplayWindow && !m_bDisplayWindowVertical)
	{
		RECT rcDisplayWindow;
		GetWindowRect(m_hDisplayWindow, &rcDisplayWindow);

		IndentBottom += rcDisplayWindow.bottom - rcDisplayWindow.top;
	}

	if(m_bShowFolders)
	{
		RECT rcHolder;
		GetClientRect(m_hHolder,&rcHolder);

		SetWindowPos(m_hHolder,NULL,0,IndentTop,rcHolder.right,
		height-IndentBottom-IndentTop,SWP_SHOWWINDOW|SWP_NOZORDER);
	}
}

LRESULT Explorerplusplus::StatusBarMenuSelect(WPARAM wParam,LPARAM lParam)
{
	/* Is the menu been closed? .*/
	if(HIWORD(wParam) == 0xFFFF && lParam == 0)
	{
		m_pStatusBar->HandleStatusBarMenuClose();
	}
	else
	{
		m_pStatusBar->HandleStatusBarMenuOpen();

		TCHAR szBuffer[512];
		LoadString(m_hLanguageModule,LOWORD(wParam),
			szBuffer,SIZEOF_ARRAY(szBuffer));
		SetWindowText(m_hStatusBar,szBuffer);
	}

	return 0;
}

void Explorerplusplus::CopyToFolder(BOOL bMove)
{
	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		return;
	}

	std::list<std::wstring> FullFilenameList;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
	{
		TCHAR szFullFilename[MAX_PATH];
		m_pActiveShellBrowser->QueryFullItemName(iItem,szFullFilename,SIZEOF_ARRAY(szFullFilename));

		FullFilenameList.push_back(szFullFilename);
	}

	TCHAR szTemp[128];
	LoadString(m_hLanguageModule,IDS_GENERAL_COPY_TO_FOLDER_TITLE,szTemp,SIZEOF_ARRAY(szTemp));
	NFileOperations::CopyFilesToFolder(m_hContainer,szTemp,FullFilenameList,bMove);
}

LRESULT Explorerplusplus::OnDeviceChange(WPARAM wParam,LPARAM lParam)
{
	/* Forward this notification out to all tabs (if a
	tab is currently in my computer, it will need to
	update its contents). */
	int nTabs = TabCtrl_GetItemCount(m_hTabCtrl);

	for(int i = 0;i < nTabs;i++)
	{
		TCITEM tcItem;
		tcItem.mask = TCIF_PARAM;
		TabCtrl_GetItem(m_hTabCtrl,i,&tcItem);

		m_pShellBrowser[static_cast<int>(tcItem.lParam)]->OnDeviceChange(wParam,lParam);
	}

	/* Forward the message to the treeview, so that
	it can handle the message as well. */
	SendMessage(m_hTreeView,WM_DEVICECHANGE,wParam,lParam);

	switch(wParam)
	{
	case DBT_DEVICEARRIVAL:
		CHardwareChangeNotifier::GetInstance().NotifyDeviceArrival(reinterpret_cast<DEV_BROADCAST_HDR *>(lParam));
		break;

	case DBT_DEVICEREMOVECOMPLETE:
		CHardwareChangeNotifier::GetInstance().NotifyDeviceRemovalComplete(reinterpret_cast<DEV_BROADCAST_HDR *>(lParam));
		break;
	}

	return TRUE;
}

HRESULT Explorerplusplus::TestListViewSelectionAttributes(SFGAOF *pItemAttributes) const
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

HRESULT Explorerplusplus::TestTreeViewSelectionAttributes(SFGAOF *pItemAttributes) const
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

BOOL Explorerplusplus::IsRenamePossible(void) const
{
	return TestItemAttributes(SFGAO_CANRENAME);
}

BOOL Explorerplusplus::IsDeletionPossible(void) const
{
	return TestItemAttributes(SFGAO_CANDELETE);
}

BOOL Explorerplusplus::CanShowFileProperties(void) const
{
	return TestItemAttributes(SFGAO_HASPROPSHEET);
}

/* Returns TRUE if all the specified attributes are set on the selected items. */
BOOL Explorerplusplus::TestItemAttributes(SFGAOF attributes) const
{
	SFGAOF commonAttributes = attributes;
	HRESULT hr = GetSelectionAttributes(&commonAttributes);

	if(SUCCEEDED(hr))
	{
		return (commonAttributes & attributes) == attributes;
	}

	return FALSE;
}

HRESULT Explorerplusplus::GetSelectionAttributes(SFGAOF *pItemAttributes) const
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

BOOL Explorerplusplus::CanCutOrCopySelection(void) const
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

BOOL Explorerplusplus::CanPaste(void) const
{
	HWND hFocus = GetFocus();

	std::list<FORMATETC> ftcList;
	CDropHandler::GetDropFormats(ftcList);

	BOOL bDataAvailable = FALSE;

	/* Check whether the drop source has the type of data
	that is needed for this drag operation. */
	for(const auto &ftc : ftcList)
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

BOOL Explorerplusplus::AreAllSelectedFilesReal(void) const
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

HRESULT Explorerplusplus::UpdateStatusBarText(void)
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
			LoadString(m_hLanguageModule,IDS_GENERAL_SELECTED_ONEITEM,szTemp,
				SIZEOF_ARRAY(szTemp));

			/* One item selected. Form:
			1 item selected */
			StringCchPrintf(szItemsSelected,SIZEOF_ARRAY(szItemsSelected),
			_T("%s %s"),szNumSelected,szTemp);
		}
		else
		{
			LoadString(m_hLanguageModule,IDS_GENERAL_SELECTED_MOREITEMS,szTemp,
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
			LoadString(m_hLanguageModule,IDS_GENERAL_ONEITEM,szTemp,
				SIZEOF_ARRAY(szTemp));

			/* Text: '1 item' */
			StringCchPrintf(szItemsSelected,SIZEOF_ARRAY(szItemsSelected),
			_T("%s %s"),szNumSelected,szTemp);
		}
		else
		{
			LoadString(m_hLanguageModule,IDS_GENERAL_MOREITEMS,szTemp,
				SIZEOF_ARRAY(szTemp));

			/* Text: 'n Items' */
			StringCchPrintf(szItemsSelected,SIZEOF_ARRAY(szItemsSelected),
			_T("%s %s"),szNumSelected,szTemp);
		}
	}

	SendMessage(m_hStatusBar,SB_SETTEXT,(WPARAM)0|0,(LPARAM)szItemsSelected);

	if(m_pActiveShellBrowser->InVirtualFolder())
	{
		LoadString(m_hLanguageModule,IDS_GENERAL_VIRTUALFOLDER,lpszSizeBuffer,
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
void Explorerplusplus::DirectoryAlteredCallback(const TCHAR *szFileName,DWORD dwAction,
void *pData)
{
	DirectoryAltered_t	*pDirectoryAltered = NULL;
	Explorerplusplus			*pContainer = NULL;

	pDirectoryAltered = (DirectoryAltered_t *)pData;
	pContainer = (Explorerplusplus *)pDirectoryAltered->pData;

	auto itr = pContainer->m_pShellBrowser.find(pDirectoryAltered->iIndex);

	if (itr != pContainer->m_pShellBrowser.end())
	{
		TCHAR szDirectory[MAX_PATH];
		itr->second->QueryCurrentDirectory(SIZEOF_ARRAY(szDirectory), szDirectory);
		LOG(debug) << _T("Directory change notification received for \"") << szDirectory << _T("\", Action = ") << dwAction
			<< _T(", Filename = \"") << szFileName << _T("\"");

		itr->second->FilesModified(dwAction,
			szFileName, pDirectoryAltered->iIndex, pDirectoryAltered->iFolderIndex);
	}
}

void FolderSizeCallbackStub(int nFolders,int nFiles,PULARGE_INTEGER lTotalFolderSize,LPVOID pData)
{
	Explorerplusplus::FolderSizeExtraInfo_t *pfsei = reinterpret_cast<Explorerplusplus::FolderSizeExtraInfo_t *>(pData);
	reinterpret_cast<Explorerplusplus *>(pfsei->pContainer)->FolderSizeCallback(pfsei,nFolders,nFiles,lTotalFolderSize);
	free(pfsei);
}

void Explorerplusplus::FolderSizeCallback(FolderSizeExtraInfo_t *pfsei,
int nFolders,int nFiles,PULARGE_INTEGER lTotalFolderSize)
{
	UNREFERENCED_PARAMETER(nFolders);
	UNREFERENCED_PARAMETER(nFiles);

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

void Explorerplusplus::CreateViewsMenu(POINT *ptOrigin)
{
	UINT uViewMode = m_pActiveShellBrowser->GetCurrentViewMode();

	int ItemToCheck = GetViewModeMenuId(uViewMode);
	CheckMenuRadioItem(m_hViewsMenu,IDM_VIEW_THUMBNAILS,IDM_VIEW_EXTRALARGEICONS,
		ItemToCheck,MF_BYCOMMAND);

	TrackPopupMenu(m_hViewsMenu,TPM_LEFTALIGN,ptOrigin->x,ptOrigin->y,
		0,m_hContainer,NULL);
}

int Explorerplusplus::CreateDriveFreeSpaceString(const TCHAR *szPath, TCHAR *szBuffer, int nBuffer)
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
		SIZEOF_ARRAY(szFreeSpace));

	LoadString(m_hLanguageModule,IDS_GENERAL_FREE,szFree,SIZEOF_ARRAY(szFree));

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
BOOL Explorerplusplus::AnyItemsSelected(void)
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

void Explorerplusplus::OnSelectColumns()
{
	CSelectColumnsDialog SelectColumnsDialog(m_hLanguageModule,IDD_SELECTCOLUMNS,m_hContainer,this);
	SelectColumnsDialog.ShowModalDialog();

	UpdateArrangeMenuItems();
}

CStatusBar *Explorerplusplus::GetStatusBar()
{
	return m_pStatusBar;
}