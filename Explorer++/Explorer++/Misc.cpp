// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "DisplayWindow/DisplayWindow.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "SelectColumnsDialog.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellTreeView/ShellTreeView.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Logging.h"
#include "../Helper/Macros.h"
#include "../Helper/WindowHelper.h"
#include <boost/range/adaptor/map.hpp>

void Explorerplusplus::ValidateLoadedSettings()
{
	if (m_config->displayWindowWidth < DISPLAY_WINDOW_MINIMUM_WIDTH)
	{
		m_config->displayWindowWidth = Config::DEFAULT_DISPLAYWINDOW_WIDTH;
	}

	if (m_config->displayWindowHeight < DISPLAY_WINDOW_MINIMUM_HEIGHT)
	{
		m_config->displayWindowHeight = Config::DEFAULT_DISPLAYWINDOW_HEIGHT;
	}

	ValidateColumns(m_config->globalFolderSettings.folderColumns);
}

void Explorerplusplus::ValidateColumns(FolderColumns &folderColumns)
{
	ValidateSingleColumnSet(VALIDATE_REALFOLDER_COLUMNS, folderColumns.realFolderColumns);
	ValidateSingleColumnSet(VALIDATE_CONTROLPANEL_COLUMNS, folderColumns.controlPanelColumns);
	ValidateSingleColumnSet(VALIDATE_MYCOMPUTER_COLUMNS, folderColumns.myComputerColumns);
	ValidateSingleColumnSet(VALIDATE_RECYCLEBIN_COLUMNS, folderColumns.recycleBinColumns);
	ValidateSingleColumnSet(VALIDATE_PRINTERS_COLUMNS, folderColumns.printersColumns);
	ValidateSingleColumnSet(VALIDATE_NETWORKCONNECTIONS_COLUMNS,
		folderColumns.networkConnectionsColumns);
	ValidateSingleColumnSet(VALIDATE_MYNETWORKPLACES_COLUMNS, folderColumns.myNetworkPlacesColumns);
}

void Explorerplusplus::ValidateSingleColumnSet(int iColumnSet, std::vector<Column_t> &columns)
{
	Column_t column;
	BOOL bFound = FALSE;
	const Column_t *pColumns = nullptr;
	unsigned int iTotalColumnSize = 0;
	unsigned int i = 0;

	switch (iColumnSet)
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

	/* Check that every column that is supposed to appear
	is in the column list. */
	for (i = 0; i < iTotalColumnSize; i++)
	{
		bFound = FALSE;

		for (auto itr = columns.begin(); itr != columns.end(); itr++)
		{
			if (itr->type == pColumns[i].type)
			{
				bFound = TRUE;
				break;
			}
		}

		/* The column is not currently in the set. Add it in. */
		if (!bFound)
		{
			column.type = pColumns[i].type;
			column.bChecked = pColumns[i].bChecked;
			column.iWidth = DEFAULT_COLUMN_WIDTH;
			columns.push_back(column);
		}
	}

	/* Check that no unknown column types appear in the column list. */
	for (auto itr = columns.cbegin(); itr != columns.cend();)
	{
		bFound = FALSE;

		for (i = 0; i < iTotalColumnSize; i++)
		{
			if (itr->type == pColumns[i].type)
			{
				bFound = TRUE;
				break;
			}
		}

		if (!bFound)
		{
			/* The column is not recognized in the set. Remove it. */
			itr = columns.erase(itr);
		}
		else
		{
			++itr;
		}
	}
}

void Explorerplusplus::ApplyDisplayWindowPosition()
{
	SendMessage(m_hDisplayWindow, WM_USER_DISPLAYWINDOWMOVED, m_config->displayWindowVertical,
		NULL);
}

void Explorerplusplus::ApplyToolbarSettings()
{
	BOOL bVisible = FALSE;
	int i = 0;

	/* Set the state of the toolbars contained within
	the main rebar. */
	for (i = 0; i < NUM_MAIN_TOOLBARS; i++)
	{
		switch (m_ToolbarInformation[i].wID)
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

	if (m_config->lockToolbars)
	{
		for (i = 0; i < NUM_MAIN_TOOLBARS; i++)
		{
			AddStyleToToolbar(&m_ToolbarInformation[i].fStyle, RBBS_NOGRIPPER);
		}
	}
}

void Explorerplusplus::CopyToFolder(bool move)
{
	if (ListView_GetSelectedCount(m_hActiveListView) == 0)
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
	LoadString(m_resourceInstance, IDS_GENERAL_COPY_TO_FOLDER_TITLE, szTemp, SIZEOF_ARRAY(szTemp));
	NFileOperations::CopyFilesToFolder(m_hContainer, szTemp, pidls, move);
}

void Explorerplusplus::OnDeviceChange(WPARAM wParam, LPARAM lParam)
{
	m_deviceChangeSignal(static_cast<UINT>(wParam), lParam);
}

boost::signals2::connection Explorerplusplus::AddDeviceChangeObserver(
	const DeviceChangeSignal::slot_type &observer)
{
	return m_deviceChangeSignal.connect(observer);
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
void Explorerplusplus::DirectoryAlteredCallback(const TCHAR *szFileName, DWORD dwAction,
	void *pData)
{
	DirectoryAltered *pDirectoryAltered = nullptr;
	Explorerplusplus *pContainer = nullptr;

	pDirectoryAltered = (DirectoryAltered *) pData;
	pContainer = (Explorerplusplus *) pDirectoryAltered->pData;

	Tab *tab =
		pContainer->GetActivePane()->GetTabContainer()->GetTabOptional(pDirectoryAltered->iIndex);

	if (tab)
	{
		std::wstring directory = tab->GetShellBrowser()->GetDirectory();
		LOG(debug) << _T("Directory change notification received for \"") << directory
				   << _T("\", Action = ") << dwAction << _T(", Filename = \"") << szFileName
				   << _T("\"");

		tab->GetShellBrowser()->FilesModified(dwAction, szFileName, pDirectoryAltered->iIndex,
			pDirectoryAltered->iFolderIndex);
	}
}

void Explorerplusplus::FolderSizeCallbackStub(int nFolders, int nFiles,
	PULARGE_INTEGER lTotalFolderSize, LPVOID pData)
{
	auto *pfsei = reinterpret_cast<Explorerplusplus::FolderSizeExtraInfo *>(pData);
	reinterpret_cast<Explorerplusplus *>(pfsei->pContainer)
		->FolderSizeCallback(pfsei, nFolders, nFiles, lTotalFolderSize);
	free(pfsei);
}

void Explorerplusplus::FolderSizeCallback(FolderSizeExtraInfo *pfsei, int nFolders, int nFiles,
	PULARGE_INTEGER lTotalFolderSize)
{
	UNREFERENCED_PARAMETER(nFolders);
	UNREFERENCED_PARAMETER(nFiles);

	DWFolderSizeCompletion *pDWFolderSizeCompletion = nullptr;

	pDWFolderSizeCompletion = (DWFolderSizeCompletion *) malloc(sizeof(DWFolderSizeCompletion));

	pDWFolderSizeCompletion->liFolderSize = *lTotalFolderSize;
	pDWFolderSizeCompletion->uId = pfsei->uId;

	/* Queue the result back to the main thread, so that
	the folder size can be displayed. It is up to the main
	thread to determine whether the folder size should actually
	be shown. */
	PostMessage(m_hContainer, WM_APP_FOLDERSIZECOMPLETED, (WPARAM) pDWFolderSizeCompletion, 0);
}

void Explorerplusplus::OnSelectColumns()
{
	SelectColumnsDialog selectColumnsDialog(m_resourceInstance, m_hContainer,
		GetActivePane()->GetTabContainer()->GetSelectedTab().GetShellBrowser(),
		m_iconResourceLoader.get());
	selectColumnsDialog.ShowModalDialog();
}

StatusBar *Explorerplusplus::GetStatusBar()
{
	return m_pStatusBar;
}
