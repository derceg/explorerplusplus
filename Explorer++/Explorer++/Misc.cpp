// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "DisplayWindow/DisplayWindow.h"
#include "Explorer++_internal.h"
#include "HardwareChangeNotifier.h"
#include "MainResource.h"
#include "SelectColumnsDialog.h"
#include "ShellBrowser/ShellBrowser.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Logging.h"
#include "../Helper/Macros.h"
#include "../Helper/WindowHelper.h"
#include <boost/range/adaptor/map.hpp>

void Explorerplusplus::ValidateLoadedSettings()
{
	if (m_config->treeViewWidth <= 0)
	{
		m_config->treeViewWidth = Config::DEFAULT_TREEVIEW_WIDTH;
	}

	if (m_config->displayWindowWidth < MINIMUM_DISPLAYWINDOW_WIDTH)
	{
		m_config->displayWindowWidth = Config::DEFAULT_DISPLAYWINDOW_WIDTH;
	}

	if (m_config->displayWindowHeight < MINIMUM_DISPLAYWINDOW_HEIGHT)
	{
		m_config->displayWindowHeight = Config::DEFAULT_DISPLAYWINDOW_HEIGHT;
	}

	ValidateColumns(m_config->globalFolderSettings.folderColumns);
}

void Explorerplusplus::ValidateColumns(FolderColumns &folderColumns)
{
	ValidateSingleColumnSet(VALIDATE_REALFOLDER_COLUMNS,folderColumns.realFolderColumns);
	ValidateSingleColumnSet(VALIDATE_CONTROLPANEL_COLUMNS,folderColumns.controlPanelColumns);
	ValidateSingleColumnSet(VALIDATE_MYCOMPUTER_COLUMNS,folderColumns.myComputerColumns);
	ValidateSingleColumnSet(VALIDATE_RECYCLEBIN_COLUMNS,folderColumns.recycleBinColumns);
	ValidateSingleColumnSet(VALIDATE_PRINTERS_COLUMNS,folderColumns.printersColumns);
	ValidateSingleColumnSet(VALIDATE_NETWORKCONNECTIONS_COLUMNS,folderColumns.networkConnectionsColumns);
	ValidateSingleColumnSet(VALIDATE_MYNETWORKPLACES_COLUMNS,folderColumns.myNetworkPlacesColumns);
}

void Explorerplusplus::ValidateSingleColumnSet(int iColumnSet, std::vector<Column_t> &columns)
{
	Column_t					column;
	int							*pColumnMap = nullptr;
	BOOL						bFound = FALSE;
	const Column_t				*pColumns = nullptr;
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

		for(auto itr = columns.begin();itr != columns.end();itr++)
		{
			if(itr->type == pColumns[i].type)
			{
				bFound = TRUE;
				break;
			}
		}

		/* The column is not currently in the set. Add it in. */
		if(!bFound)
		{
			column.type		= pColumns[i].type;
			column.bChecked	= pColumns[i].bChecked;
			column.iWidth	= DEFAULT_COLUMN_WIDTH;
			columns.push_back(column);
		}
	}

	free(pColumnMap);
}

void Explorerplusplus::ApplyDisplayWindowPosition()
{
	SendMessage(m_hDisplayWindow, WM_USER_DISPLAYWINDOWMOVED, m_config->displayWindowVertical, NULL);
}

void Explorerplusplus::ApplyToolbarSettings()
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

		if (!bVisible)
		{
			AddStyleToToolbar(&m_ToolbarInformation[i].fStyle, RBBS_HIDDEN);
		}
	}

	if(m_config->lockToolbars)
	{
		for(i = 0;i < NUM_MAIN_TOOLBARS;i++)
		{
			AddStyleToToolbar(&m_ToolbarInformation[i].fStyle,RBBS_NOGRIPPER);
		}
	}
}

void Explorerplusplus::AdjustFolderPanePosition()
{
	RECT rcMainWindow;
	int indentTop		= 0;
	int indentBottom	= 0;
	int height;

	GetClientRect(m_hContainer,&rcMainWindow);
	height = GetRectHeight(&rcMainWindow);

	if(m_hMainRebar)
	{
		RECT rebarRect;

		GetWindowRect(m_hMainRebar,&rebarRect);

		indentTop += rebarRect.bottom - rebarRect.top;
	}

	if(m_config->showStatusBar)
	{
		RECT statusBarRect;

		GetWindowRect(m_hStatusBar,&statusBarRect);

		indentBottom += statusBarRect.bottom - statusBarRect.top;
	}

	if(m_config->showDisplayWindow && !m_config->displayWindowVertical)
	{
		RECT rcDisplayWindow;
		GetWindowRect(m_hDisplayWindow, &rcDisplayWindow);

		indentBottom += rcDisplayWindow.bottom - rcDisplayWindow.top;
	}

	if(m_config->showFolders)
	{
		RECT rcHolder;
		GetClientRect(m_hHolder,&rcHolder);

		SetWindowPos(m_hHolder, nullptr,0,indentTop,rcHolder.right,
		height-indentBottom-indentTop,SWP_SHOWWINDOW|SWP_NOZORDER);
	}
}

void Explorerplusplus::CopyToFolder(bool move)
{
	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		return;
	}

	std::vector<unique_pidl_absolute> pidlPtrs;
	std::vector<PCIDLIST_ABSOLUTE> pidls;
	int iItem = -1;

	while ((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
	{
		auto pidlPtr = m_pActiveShellBrowser->GetItemCompleteIdl(iItem);

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
		tab->GetShellBrowser()->OnDeviceChange(wParam,lParam);
	}

	/* Forward the message to the treeview, so that
	it can handle the message as well. */
	SendMessage(m_hTreeView,WM_DEVICECHANGE,wParam,lParam);

	switch(wParam)
	{
	case DBT_DEVICEARRIVAL:
		HardwareChangeNotifier::GetInstance().NotifyDeviceArrival(reinterpret_cast<DEV_BROADCAST_HDR *>(lParam));
		break;

	case DBT_DEVICEREMOVECOMPLETE:
		HardwareChangeNotifier::GetInstance().NotifyDeviceRemovalComplete(reinterpret_cast<DEV_BROADCAST_HDR *>(lParam));
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
	DirectoryAltered	*pDirectoryAltered = nullptr;
	Explorerplusplus			*pContainer = nullptr;

	pDirectoryAltered = (DirectoryAltered *)pData;
	pContainer = (Explorerplusplus *)pDirectoryAltered->pData;

	Tab *tab = pContainer->m_tabContainer->GetTabOptional(pDirectoryAltered->iIndex);

	if (tab)
	{
		std::wstring directory = tab->GetShellBrowser()->GetDirectory();
		LOG(debug) << _T("Directory change notification received for \"") << directory << _T("\", Action = ") << dwAction
			<< _T(", Filename = \"") << szFileName << _T("\"");

		tab->GetShellBrowser()->FilesModified(dwAction, szFileName,
			pDirectoryAltered->iIndex, pDirectoryAltered->iFolderIndex);
	}
}

void Explorerplusplus::FolderSizeCallbackStub(int nFolders,int nFiles,PULARGE_INTEGER lTotalFolderSize,LPVOID pData)
{
	auto *pfsei = reinterpret_cast<Explorerplusplus::FolderSizeExtraInfo *>(pData);
	reinterpret_cast<Explorerplusplus *>(pfsei->pContainer)->FolderSizeCallback(pfsei,nFolders,nFiles,lTotalFolderSize);
	free(pfsei);
}

void Explorerplusplus::FolderSizeCallback(FolderSizeExtraInfo *pfsei,
int nFolders,int nFiles,PULARGE_INTEGER lTotalFolderSize)
{
	UNREFERENCED_PARAMETER(nFolders);
	UNREFERENCED_PARAMETER(nFiles);

	DWFolderSizeCompletion *pDWFolderSizeCompletion = nullptr;

	pDWFolderSizeCompletion = (DWFolderSizeCompletion *)malloc(sizeof(DWFolderSizeCompletion));

	pDWFolderSizeCompletion->liFolderSize = *lTotalFolderSize;
	pDWFolderSizeCompletion->uId = pfsei->uId;

	/* Queue the result back to the main thread, so that
	the folder size can be displayed. It is up to the main
	thread to determine whether the folder size should actually
	be shown. */
	PostMessage(m_hContainer,WM_APP_FOLDERSIZECOMPLETED,
		(WPARAM)pDWFolderSizeCompletion,0);
}

void Explorerplusplus::OnSelectColumns()
{
	SelectColumnsDialog selectColumnsDialog(m_hLanguageModule, m_hContainer,
		m_tabContainer->GetSelectedTab().GetShellBrowser(), m_iconResourceLoader.get());
	selectColumnsDialog.ShowModalDialog();
}

StatusBar *Explorerplusplus::GetStatusBar()
{
	return m_pStatusBar;
}