// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "App.h"
#include "Config.h"
#include "CustomizeColorsDialog.h"
#include "DestroyFilesDialog.h"
#include "DisplayColoursDialog.h"
#include "DisplayWindow/DisplayWindow.h"
#include "FileProgressSink.h"
#include "MainResource.h"
#include "ModelessDialogHelper.h"
#include "OptionsDialog.h"
#include "ResourceLoader.h"
#include "ScriptingDialog.h"
#include "SearchDialog.h"
#include "SearchTabsDialog.h"
#include "SearchTabsModel.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "TabContainerImpl.h"
#include "WildcardSelectDialog.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"
#include <wil/com.h>

void Explorerplusplus::OnChangeDisplayColors()
{
	DisplayColoursDialog displayColoursDialog(m_app->GetResourceLoader(), m_hContainer, m_config);
	displayColoursDialog.ShowModalDialog();
}

void Explorerplusplus::OnDestroyFiles()
{
	std::list<std::wstring> fullFilenameList;
	int iItem = -1;

	while ((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
	{
		std::wstring fullFilename = m_pActiveShellBrowser->GetItemFullName(iItem);
		fullFilenameList.push_back(fullFilename);
	}

	DestroyFilesDialog destroyFilesDialog(m_app->GetResourceLoader(), m_hContainer,
		fullFilenameList, m_config->globalFolderSettings.showFriendlyDates);
	destroyFilesDialog.ShowModalDialog();
}

void Explorerplusplus::OnWildcardSelect(SelectionType selectionType)
{
	WildcardSelectDialog wilcardSelectDialog(m_app->GetResourceLoader(), m_hContainer,
		m_pActiveShellBrowser, selectionType);
	wilcardSelectDialog.ShowModalDialog();
}

void Explorerplusplus::OnSearch()
{
	CreateOrSwitchToModelessDialog(m_app->GetModelessDialogList(), L"SearchDialog",
		[this]
		{
			Tab &selectedTab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();
			std::wstring currentDirectory = selectedTab.GetShellBrowserImpl()->GetDirectoryPath();

			return new SearchDialog(m_app->GetResourceLoader(), m_hContainer, currentDirectory,
				m_app->GetBrowserList());
		});
}

void Explorerplusplus::OnCustomizeColors()
{
	CustomizeColorsDialog customizeColorsDialog(m_app->GetResourceLoader(), m_hContainer,
		m_app->GetColorRuleModel());
	customizeColorsDialog.ShowModalDialog();
}

void Explorerplusplus::OnRunScript()
{
	CreateOrSwitchToModelessDialog(m_app->GetModelessDialogList(), L"ScriptingDialog", [this]
		{ return new ScriptingDialog(m_app->GetResourceLoader(), m_hContainer, this, m_config); });
}

void Explorerplusplus::OnShowOptions()
{
	CreateOrSwitchToModelessDialog(m_app->GetModelessDialogList(), L"OptionsDialog",
		[this] { return new OptionsDialog(m_hContainer, m_app, m_config, this); });
}

void Explorerplusplus::OnSearchTabs()
{
	CreateOrSwitchToModelessDialog(m_app->GetModelessDialogList(), L"SearchTabsDialog",
		[this]
		{
			return SearchTabsDialog::Create(m_hContainer,
				std::make_unique<SearchTabsModel>(m_app->GetTabList(), m_app->GetTabEvents(),
					m_app->GetShellBrowserEvents(), m_app->GetNavigationEvents()),
				m_app->GetResourceLoader());
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

		hr = FileOperations::ResolveLink(m_hContainer, 0, shortcutFileName.c_str(), szFullFileName,
			std::size(szFullFileName));

		if (hr == S_OK)
		{
			/* Strip the filename, just leaving the path component. */
			StringCchCopy(szPath, std::size(szPath), szFullFileName);
			PathRemoveFileSpec(szPath);

			Tab &newTab = GetActivePane()->GetTabContainerImpl()->CreateNewTab(szPath,
				TabSettings(_selected = true));

			if (newTab.GetShellBrowserImpl()->GetDirectoryPath() == szPath)
			{
				wil::com_ptr_nothrow<IShellFolder> parent;
				hr = SHBindToObject(nullptr, newTab.GetShellBrowserImpl()->GetDirectoryIdl().get(),
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
	Tab &selectedTab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();
	selectedTab.GetShellBrowserImpl()->GetNavigationController()->GoToOffset(offset);
}
