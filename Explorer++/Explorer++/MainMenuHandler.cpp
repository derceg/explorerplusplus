// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AppServices.h"
#include "Config.h"
#include "DestroyFilesDialog.h"
#include "ModelessDialogHelper.h"
#include "OptionsDialog.h"
#include "SearchDialog.h"
#include "SelectColumnsDialog.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainer.h"
#include "../Helper/PidlHelper.h"
#include "../Helper/ShellHelper.h"
#include <wil/com.h>
#include <Shlwapi.h>

void Explorerplusplus::OnDestroyFiles()
{
	std::list<std::wstring> fullFilenameList;
	int iItem = -1;

	while ((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
	{
		std::wstring fullFilename = m_pActiveShellBrowser->GetItemFullName(iItem);
		fullFilenameList.push_back(fullFilename);
	}

	auto *destroyFilesDialog = DestroyFilesDialog::Create(m_resourceLoader, m_hwnd,
		fullFilenameList, m_config->globalFolderSettings.showFriendlyDates);
	destroyFilesDialog->ShowModalDialog();
}

void Explorerplusplus::OnSearch()
{
	CreateOrSwitchToModelessDialog(m_appServices->GetModelessDialogList(), L"SearchDialog",
		[this]
		{
			Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
			std::wstring currentDirectory = selectedTab.GetShellBrowser()->GetDirectoryPath();

			return SearchDialog::Create(m_resourceLoader, m_hwnd, currentDirectory, m_browserList);
		});
}

void Explorerplusplus::OnShowOptions()
{
	CreateOrSwitchToModelessDialog(m_appServices->GetModelessDialogList(), L"OptionsDialog",
		[this]
		{
			return OptionsDialog::Create(m_resourceLoader, m_hwnd,
				m_appServices->GetAppController(), m_config, m_appServices->GetDarkModeManager(),
				m_appServices->GetThemeManager(), this);
		});
}

void Explorerplusplus::OnResolveLink()
{
	TCHAR szFullFileName[MAX_PATH];
	TCHAR szPath[MAX_PATH];
	HRESULT hr;
	int iItem;

	iItem = ListView_GetNextItem(m_hActiveListView, -1, LVNI_FOCUSED);

	if (iItem != -1)
	{
		std::wstring shortcutFileName = m_pActiveShellBrowser->GetItemFullName(iItem);

		hr = FileOperations::ResolveLink(m_hwnd, 0, shortcutFileName.c_str(), szFullFileName,
			std::size(szFullFileName));

		if (hr == S_OK)
		{
			/* Strip the filename, just leaving the path component. */
			StringCchCopy(szPath, std::size(szPath), szFullFileName);
			PathRemoveFileSpec(szPath);

			Tab &newTab =
				GetActivePane()->GetTabContainer()->CreateNewTab(szPath, { .selected = true });

			if (newTab.GetShellBrowser()->GetDirectoryPath() == szPath)
			{
				wil::com_ptr_nothrow<IShellFolder> parent;
				hr = SHBindToObject(nullptr, newTab.GetShellBrowser()->GetDirectory().Raw(),
					nullptr, IID_PPV_ARGS(&parent));

				if (hr == S_OK)
				{
					auto *filename = PathFindFileName(szFullFileName);
					assert(filename != szFullFileName);

					PidlAbsolute pidl;
					hr = CreateSimplePidl(filename, pidl, parent.get());

					if (SUCCEEDED(hr))
					{
						m_pActiveShellBrowser->SelectItems({ pidl.Raw() });
					}
				}
			}

			SetFocus(m_hActiveListView);
		}
	}
}

void Explorerplusplus::OnGoToOffset(int offset)
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	selectedTab.GetShellBrowserImpl()->GetNavigationController()->GoToOffset(offset);
}

void Explorerplusplus::OnSelectColumns()
{
	auto *selectColumnsDialog = SelectColumnsDialog::Create(m_resourceLoader, m_hwnd,
		GetActivePane()->GetTabContainer()->GetSelectedTab().GetShellBrowserImpl());
	selectColumnsDialog->ShowModalDialog();
}
