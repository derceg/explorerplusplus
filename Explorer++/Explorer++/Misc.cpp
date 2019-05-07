// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "DefaultColumns.h"
#include "Explorer++_internal.h"
#include "HardwareChangeNotifier.h"
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
#include "../MyTreeView/MyTreeView.h"
#include <boost/range/adaptor/map.hpp>
#include <shobjidl.h>
#include <list>

void Explorerplusplus::ValidateLoadedSettings(void)
{
	if(m_config->treeViewWidth <= 0)
		m_config->treeViewWidth = Config::DEFAULT_TREEVIEW_WIDTH;

	if(m_config->displayWindowHeight < MINIMUM_DISPLAYWINDOW_HEIGHT)
		m_config->displayWindowHeight = Config::DEFAULT_DISPLAYWINDOW_HEIGHT;

	ValidateColumns();
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
	m_pMyTreeView->SetShowHidden(m_config->defaultFolderSettings.showHidden);
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
			bVisible = m_config->showMainToolbar;
			break;

		case ID_ADDRESSTOOLBAR:
			bVisible = m_config->showAddressBar;
			break;

		case ID_BOOKMARKSTOOLBAR:
			bVisible = m_config->showBookmarksToolbar;
			break;

		case ID_DRIVESTOOLBAR:
			bVisible = m_config->showDrivesToolbar;
			break;

		case ID_APPLICATIONSTOOLBAR:
			bVisible = m_config->showApplicationToolbar;
			break;
		}

		if(!bVisible)
			AddStyleToToolbar(&m_ToolbarInformation[i].fStyle,RBBS_HIDDEN);
	}

	if(m_config->lockToolbars)
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

	if(m_config->showStatusBar)
	{
		RECT m_hStatusBarRect;

		GetWindowRect(m_hStatusBar,&m_hStatusBarRect);

		IndentBottom += m_hStatusBarRect.bottom - m_hStatusBarRect.top;
	}

	if(m_config->showDisplayWindow)
	{
		RECT rcDisplayWindow;

		GetWindowRect(m_hDisplayWindow,&rcDisplayWindow);

		IndentBottom += rcDisplayWindow.bottom - rcDisplayWindow.top;
	}

	if(m_config->showFolders)
	{
		RECT rcHolder;
		GetClientRect(m_hHolder,&rcHolder);

		SetWindowPos(m_hHolder,NULL,0,IndentTop,rcHolder.right,
		height-IndentBottom-IndentTop,SWP_SHOWWINDOW|SWP_NOZORDER);
	}
}

void Explorerplusplus::CopyToFolder(bool move)
{
	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		return;
	}

	std::vector<PIDLPointer> pidlPtrs;
	std::vector<LPCITEMIDLIST> pidls;
	int iItem = -1;

	while ((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
	{
		PIDLPointer pidlPtr(m_pActiveShellBrowser->QueryItemCompleteIdl(iItem));

		if (!pidlPtr)
		{
			continue;
		}

		pidls.push_back(pidlPtr.get());
		pidlPtrs.push_back(std::move(pidlPtr));
	}

	TCHAR szTemp[128];
	LoadString(m_hLanguageModule,IDS_GENERAL_COPY_TO_FOLDER_TITLE,szTemp,SIZEOF_ARRAY(szTemp));
	NFileOperations::CopyFilesToFolder(m_hContainer,szTemp,pidls,move);
}

LRESULT Explorerplusplus::OnDeviceChange(WPARAM wParam,LPARAM lParam)
{
	/* Forward this notification out to all tabs (if a
	tab is currently in my computer, it will need to
	update its contents). */
	for (auto &tab : m_tabContainer->GetAllTabs() | boost::adaptors::map_values)
	{
		tab.GetShellBrowser()->OnDeviceChange(wParam,lParam);
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

	Tab *tab = pContainer->m_tabContainer->GetTabOptional(pDirectoryAltered->iIndex);

	if (tab)
	{
		TCHAR szDirectory[MAX_PATH];
		tab->GetShellBrowser()->QueryCurrentDirectory(SIZEOF_ARRAY(szDirectory), szDirectory);
		LOG(debug) << _T("Directory change notification received for \"") << szDirectory << _T("\", Action = ") << dwAction
			<< _T(", Filename = \"") << szFileName << _T("\"");

		tab->GetShellBrowser()->FilesModified(dwAction,
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
	CSelectColumnsDialog SelectColumnsDialog(m_hLanguageModule,IDD_SELECTCOLUMNS,m_hContainer,this,m_tabContainer,this);
	SelectColumnsDialog.ShowModalDialog();

	UpdateArrangeMenuItems();
}

CStatusBar *Explorerplusplus::GetStatusBar()
{
	return m_pStatusBar;
}