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
#include "ResourceLoader.h"
#include "SelectColumnsDialog.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellTreeView/ShellTreeView.h"
#include "TabContainer.h"
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
	SelectColumnsDialog selectColumnsDialog(m_app->GetResourceLoader(), m_hContainer,
		GetActivePane()->GetTabContainer()->GetSelectedTab().GetShellBrowserImpl());
	selectColumnsDialog.ShowModalDialog();
}
