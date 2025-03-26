// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "ColumnStorage.h"
#include "Config.h"
#include "DisplayWindow/DisplayWindow.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "SelectColumnsDialog.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellTreeView/ShellTreeView.h"
#include "TabContainerImpl.h"
#include "TabStorage.h"
#include "../Helper/Controls.h"
#include "../Helper/FileOperations.h"
#include "../Helper/WindowHelper.h"
#include <boost/range/adaptor/map.hpp>
#include <glog/logging.h>

void Explorerplusplus::ApplyDisplayWindowPosition()
{
	SendMessage(m_displayWindow->GetHWND(), WM_USER_DISPLAYWINDOWMOVED,
		m_config->displayWindowVertical, NULL);
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

	auto title =
		ResourceHelper::LoadString(m_app->GetResourceInstance(), IDS_GENERAL_COPY_TO_FOLDER_TITLE);
	FileOperations::CopyFilesToFolder(m_hContainer, title, pidls, move);
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

	Tab *tab = pContainer->GetActivePane()->GetTabContainerImpl()->GetTabOptional(
		pDirectoryAltered->iIndex);

	if (tab)
	{
		std::wstring directory = tab->GetShellBrowserImpl()->GetDirectory();
		LOG(INFO) << "Directory change notification received for \"" << wstrToUtf8Str(directory)
				  << "\", Action = " << dwAction << ", Filename = \"" << wstrToUtf8Str(szFileName)
				  << "\"";

		tab->GetShellBrowserImpl()->FilesModified(dwAction, szFileName, pDirectoryAltered->iIndex,
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
	SelectColumnsDialog selectColumnsDialog(m_app->GetResourceInstance(), m_hContainer,
		m_app->GetThemeManager(),
		GetActivePane()->GetTabContainerImpl()->GetSelectedTab().GetShellBrowserImpl(),
		m_app->GetIconResourceLoader());
	selectColumnsDialog.ShowModalDialog();
}
