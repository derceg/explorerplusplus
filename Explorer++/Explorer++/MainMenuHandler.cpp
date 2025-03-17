// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AboutDialog.h"
#include "App.h"
#include "Config.h"
#include "CustomizeColorsDialog.h"
#include "DestroyFilesDialog.h"
#include "DisplayColoursDialog.h"
#include "DisplayWindow/DisplayWindow.h"
#include "FileProgressSink.h"
#include "FilterDialog.h"
#include "MainResource.h"
#include "MergeFilesDialog.h"
#include "ModelessDialogHelper.h"
#include "OptionsDialog.h"
#include "ResourceHelper.h"
#include "ScriptingDialog.h"
#include "SearchDialog.h"
#include "SearchTabsDialog.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "SplitFileDialog.h"
#include "TabContainerImpl.h"
#include "UpdateCheckDialog.h"
#include "WildcardSelectDialog.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"
#include <wil/com.h>

void Explorerplusplus::OnChangeDisplayColors()
{
	DisplayColoursDialog displayColoursDialog(m_app->GetResourceInstance(), m_hContainer,
		m_app->GetThemeManager(), m_config);
	displayColoursDialog.ShowModalDialog();
}

void Explorerplusplus::OnFilterResults()
{
	FilterDialog filterDialog(m_app->GetResourceInstance(), m_hContainer, m_app->GetThemeManager(),
		this, m_app->GetIconResourceLoader());
	filterDialog.ShowModalDialog();
}

void Explorerplusplus::OnMergeFiles()
{
	std::wstring currentDirectory = m_pActiveShellBrowser->GetDirectory();

	std::list<std::wstring> fullFilenameList;
	int iItem = -1;

	while ((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
	{
		std::wstring fullFilename = m_pActiveShellBrowser->GetItemFullName(iItem);
		fullFilenameList.push_back(fullFilename);
	}

	MergeFilesDialog mergeFilesDialog(m_app->GetResourceInstance(), m_hContainer,
		m_app->GetThemeManager(), m_app->GetIconResourceLoader(), currentDirectory,
		fullFilenameList, m_config->globalFolderSettings.showFriendlyDates);
	mergeFilesDialog.ShowModalDialog();
}

void Explorerplusplus::OnSplitFile()
{
	int iSelected = ListView_GetNextItem(m_hActiveListView, -1, LVNI_SELECTED);

	if (iSelected != -1)
	{
		std::wstring fullFilename = m_pActiveShellBrowser->GetItemFullName(iSelected);

		SplitFileDialog splitFileDialog(m_app->GetResourceInstance(), m_hContainer,
			m_app->GetThemeManager(), m_app->GetIconResourceLoader(), fullFilename);
		splitFileDialog.ShowModalDialog();
	}
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

	DestroyFilesDialog destroyFilesDialog(m_app->GetResourceInstance(), m_hContainer,
		m_app->GetThemeManager(), fullFilenameList,
		m_config->globalFolderSettings.showFriendlyDates);
	destroyFilesDialog.ShowModalDialog();
}

void Explorerplusplus::OnWildcardSelect(BOOL bSelect)
{
	WildcardSelectDialog wilcardSelectDialog(m_app->GetResourceInstance(), m_hContainer,
		m_app->GetThemeManager(), bSelect, this);
	wilcardSelectDialog.ShowModalDialog();
}

void Explorerplusplus::OnSearch()
{
	CreateOrSwitchToModelessDialog(m_app->GetModelessDialogList(), L"SearchDialog",
		[this]
		{
			Tab &selectedTab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();
			std::wstring currentDirectory = selectedTab.GetShellBrowserImpl()->GetDirectory();

			return new SearchDialog(m_app->GetResourceInstance(), m_hContainer,
				m_app->GetThemeManager(), currentDirectory, this, this,
				GetActivePane()->GetTabContainerImpl(), m_app->GetIconResourceLoader());
		});
}

void Explorerplusplus::OnCustomizeColors()
{
	CustomizeColorsDialog customizeColorsDialog(m_app->GetResourceInstance(), m_hContainer,
		m_app->GetThemeManager(), m_app->GetColorRuleModel(), m_app->GetIconResourceLoader());
	customizeColorsDialog.ShowModalDialog();
}

void Explorerplusplus::OnRunScript()
{
	CreateOrSwitchToModelessDialog(m_app->GetModelessDialogList(), L"ScriptingDialog",
		[this]
		{
			return new ScriptingDialog(m_app->GetResourceInstance(), m_hContainer,
				m_app->GetThemeManager(), this);
		});
}

void Explorerplusplus::OnShowOptions()
{
	CreateOrSwitchToModelessDialog(m_app->GetModelessDialogList(), L"OptionsDialog",
		[this]
		{
			return new OptionsDialog(m_app->GetResourceInstance(), m_hContainer, m_app, m_config,
				this);
		});
}

void Explorerplusplus::OnSearchTabs()
{
	CreateOrSwitchToModelessDialog(m_app->GetModelessDialogList(), L"SearchTabsDialog",
		[this] { return SearchTabsDialog::Create(m_app, m_hContainer, this); });
}

void Explorerplusplus::OnOpenOnlineDocumentation()
{
	ShellExecute(nullptr, L"open", App::DOCUMENTATION_URL, nullptr, nullptr, SW_SHOWNORMAL);
}

void Explorerplusplus::OnCheckForUpdates()
{
	UpdateCheckDialog updateCheckDialog(m_app->GetResourceInstance(), m_hContainer,
		m_app->GetThemeManager());
	updateCheckDialog.ShowModalDialog();
}

void Explorerplusplus::OnAbout()
{
	AboutDialog aboutDialog(m_app->GetResourceInstance(), m_hContainer, m_app->GetThemeManager());
	aboutDialog.ShowModalDialog();
}

void Explorerplusplus::OnSaveDirectoryListing() const
{
	TCHAR fileName[MAX_PATH];
	LoadString(m_app->GetResourceInstance(), IDS_GENERAL_DIRECTORY_LISTING_FILENAME, fileName,
		std::size(fileName));
	StringCchCat(fileName, std::size(fileName), _T(".txt"));

	std::wstring directory = m_pActiveShellBrowser->GetDirectory();

	BOOL bSaveNameRetrieved =
		GetFileNameFromUser(m_hContainer, fileName, std::size(fileName), directory.c_str());

	if (bSaveNameRetrieved)
	{
		FileOperations::SaveDirectoryListing(directory, fileName);
	}
}

void Explorerplusplus::OnCreateNewFolder()
{
	auto pidlDirectory = m_pActiveShellBrowser->GetDirectoryIdl();

	wil::com_ptr_nothrow<IShellItem> directoryShellItem;
	HRESULT hr = SHCreateItemFromIDList(pidlDirectory.get(), IID_PPV_ARGS(&directoryShellItem));

	if (FAILED(hr))
	{
		return;
	}

	auto sink = winrt::make_self<FileProgressSink>();
	sink->SetPostNewItemObserver(
		[this](PIDLIST_ABSOLUTE pidl)
		{
			ListViewHelper::SelectAllItems(m_hActiveListView, false);
			SetFocus(m_hActiveListView);

			m_pActiveShellBrowser->QueueRename(pidl);
		});

	auto newFolderName =
		ResourceHelper::LoadString(m_app->GetResourceInstance(), IDS_NEW_FOLDER_NAME);
	hr = FileOperations::CreateNewFolder(directoryShellItem.get(), newFolderName, sink.get());

	if (FAILED(hr))
	{
		auto errorMessage =
			ResourceHelper::LoadString(m_app->GetResourceInstance(), IDS_NEWFOLDERERROR);
		MessageBox(m_hContainer, errorMessage.c_str(), App::APP_NAME, MB_ICONERROR | MB_OK);
	}
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

			if (newTab.GetShellBrowserImpl()->GetDirectory() == szPath)
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

void Explorerplusplus::OnGoHome()
{
	Tab &selectedTab = GetActivePane()->GetTabContainerImpl()->GetSelectedTab();
	selectedTab.GetShellBrowserImpl()->GetNavigationController()->Navigate(
		m_config->defaultTabDirectory);
}
