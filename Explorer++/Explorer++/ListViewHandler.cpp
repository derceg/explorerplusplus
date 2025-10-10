// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Config.h"
#include "FolderView.h"
#include "IDropFilesCallback.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "ResourceHelper.h"
#include "ServiceProvider.h"
#include "ShellBrowser/Columns.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "ShellBrowser/ViewModes.h"
#include "SortMenuBuilder.h"
#include "TabContainer.h"
#include "ViewModeHelper.h"
#include "../Helper/ClipboardHelper.h"
#include "../Helper/DropHandler.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WinRTBaseWrapper.h"
#include <wil/com.h>

LRESULT Explorerplusplus::OnListViewKeyDown(LPARAM lParam)
{
	LV_KEYDOWN *keyDown = reinterpret_cast<LV_KEYDOWN *>(lParam);

	switch (keyDown->wVKey)
	{
	case 'V':
		if (IsKeyDown(VK_CONTROL) && !IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			OnListViewPaste();
		}
		break;

	case VK_INSERT:
		if (!IsKeyDown(VK_CONTROL) && IsKeyDown(VK_SHIFT) && !IsKeyDown(VK_MENU))
		{
			OnListViewPaste();
		}
		break;
	}

	return 0;
}

void Explorerplusplus::OnListViewPaste()
{
	auto clipboardObject = m_app->GetPlatformContext()->GetClipboardStore()->GetDataObject();

	if (!clipboardObject)
	{
		return;
	}

	const auto &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	auto directory = selectedTab.GetShellBrowserImpl()->GetDirectoryIdl();

	if (CanShellPasteDataObject(directory.get(), clipboardObject.get(), PasteType::Normal))
	{
		auto serviceProvider = winrt::make_self<ServiceProvider>();
		serviceProvider->RegisterService(IID_IFolderView,
			winrt::make<FolderView>(selectedTab.GetShellBrowserImpl()->GetWeakPtr()));

		ExecuteActionFromContextMenu(directory.get(), {},
			selectedTab.GetShellBrowserImpl()->GetListView(), L"paste", 0, serviceProvider.get());
	}
	else
	{
		TCHAR szDestination[MAX_PATH + 1];

		/* DO NOT use the internal current directory string.
		 Files are copied asynchronously, so a change of directory
		 will cause the destination directory to change in the
		 middle of the copy operation. */
		StringCchCopy(szDestination, std::size(szDestination),
			selectedTab.GetShellBrowserImpl()->GetDirectoryPath().c_str());

		/* Also, the string must be double NULL terminated. */
		szDestination[lstrlen(szDestination) + 1] = '\0';

		DropHandler *pDropHandler = DropHandler::CreateNew();
		DropFilesCallback dropFilesCallback{ this };
		pDropHandler->CopyClipboardData(clipboardObject.get(), m_hContainer, szDestination,
			&dropFilesCallback);
		pDropHandler->Release();
	}
}

int Explorerplusplus::HighlightSimilarFiles(HWND ListView) const
{
	BOOL bSimilarTypes;
	int iSelected;
	int nItems;
	int nSimilar = 0;
	int i = 0;

	iSelected = ListView_GetNextItem(ListView, -1, LVNI_SELECTED);

	if (iSelected == -1)
		return -1;

	std::wstring testFile = m_pActiveShellBrowser->GetItemFullName(iSelected);

	nItems = ListView_GetItemCount(ListView);

	for (i = 0; i < nItems; i++)
	{
		std::wstring fullFileName = m_pActiveShellBrowser->GetItemFullName(i);

		bSimilarTypes = CompareFileTypes(fullFileName.c_str(), testFile.c_str());

		if (bSimilarTypes)
		{
			ListViewHelper::SelectItem(ListView, i, true);
			nSimilar++;
		}
		else
		{
			ListViewHelper::SelectItem(ListView, i, false);
		}
	}

	return nSimilar;
}
