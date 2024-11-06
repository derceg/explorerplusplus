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
#include "Explorer++_internal.h"
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
#include "TabContainer.h"
#include "UpdateCheckDialog.h"
#include "WildcardSelectDialog.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"
#include <wil/com.h>

void Explorerplusplus::OnChangeDisplayColors()
{
	DisplayColoursDialog displayColoursDialog(m_resourceInstance, m_hContainer, m_hDisplayWindow,
		m_config->displayWindowCentreColor.ToCOLORREF(),
		m_config->displayWindowSurroundColor.ToCOLORREF());
	displayColoursDialog.ShowModalDialog();
}

void Explorerplusplus::OnFilterResults()
{
	FilterDialog filterDialog(m_resourceInstance, m_hContainer, this);
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

	MergeFilesDialog mergeFilesDialog(m_resourceInstance, m_hContainer, this, currentDirectory,
		fullFilenameList, m_config->globalFolderSettings.showFriendlyDates);
	mergeFilesDialog.ShowModalDialog();
}

void Explorerplusplus::OnSplitFile()
{
	int iSelected = ListView_GetNextItem(m_hActiveListView, -1, LVNI_SELECTED);

	if (iSelected != -1)
	{
		std::wstring fullFilename = m_pActiveShellBrowser->GetItemFullName(iSelected);

		SplitFileDialog splitFileDialog(m_resourceInstance, m_hContainer, this, fullFilename);
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

	DestroyFilesDialog destroyFilesDialog(m_resourceInstance, m_hContainer, fullFilenameList,
		m_config->globalFolderSettings.showFriendlyDates);
	destroyFilesDialog.ShowModalDialog();
}

void Explorerplusplus::OnWildcardSelect(BOOL bSelect)
{
	WildcardSelectDialog wilcardSelectDialog(m_resourceInstance, m_hContainer, bSelect, this);
	wilcardSelectDialog.ShowModalDialog();
}

void Explorerplusplus::OnSearch()
{
	CreateOrSwitchToModelessDialog(m_app->GetModelessDialogList(), L"SearchDialog",
		[this]
		{
			Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
			std::wstring currentDirectory = selectedTab.GetShellBrowser()->GetDirectory();

			return new SearchDialog(m_resourceInstance, m_hContainer, currentDirectory, this, this,
				GetActivePane()->GetTabContainer());
		});
}

void Explorerplusplus::OnCustomizeColors()
{
	CustomizeColorsDialog customizeColorsDialog(m_resourceInstance, m_hContainer, this,
		m_app->GetColorRuleModel());
	customizeColorsDialog.ShowModalDialog();
}

void Explorerplusplus::OnRunScript()
{
	CreateOrSwitchToModelessDialog(m_app->GetModelessDialogList(), L"ScriptingDialog",
		[this] { return new ScriptingDialog(m_resourceInstance, m_hContainer, this); });
}

void Explorerplusplus::OnShowOptions()
{
	CreateOrSwitchToModelessDialog(m_app->GetModelessDialogList(), L"OptionsDialog",
		[this] { return new OptionsDialog(m_resourceInstance, m_hContainer, m_config, this); });
}

void Explorerplusplus::OnSearchTabs()
{
	CreateOrSwitchToModelessDialog(m_app->GetModelessDialogList(), L"SearchTabsDialog",
		[this] { return SearchTabsDialog::Create(m_resourceInstance, m_hContainer, this); });
}

void Explorerplusplus::OnOpenOnlineDocumentation()
{
	ShellExecute(nullptr, L"open", NExplorerplusplus::DOCUMENTATION_LINK, nullptr, nullptr,
		SW_SHOWNORMAL);
}

void Explorerplusplus::OnCheckForUpdates()
{
	UpdateCheckDialog updateCheckDialog(m_resourceInstance, m_hContainer);
	updateCheckDialog.ShowModalDialog();
}

void Explorerplusplus::OnAbout()
{
	AboutDialog aboutDialog(m_resourceInstance, m_hContainer);
	aboutDialog.ShowModalDialog();
}

void Explorerplusplus::OnSaveDirectoryListing() const
{
	TCHAR fileName[MAX_PATH];
	LoadString(m_resourceInstance, IDS_GENERAL_DIRECTORY_LISTING_FILENAME, fileName,
		SIZEOF_ARRAY(fileName));
	StringCchCat(fileName, SIZEOF_ARRAY(fileName), _T(".txt"));

	std::wstring directory = m_pActiveShellBrowser->GetDirectory();

	BOOL bSaveNameRetrieved =
		GetFileNameFromUser(m_hContainer, fileName, SIZEOF_ARRAY(fileName), directory.c_str());

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
			ListViewHelper::SelectAllItems(m_hActiveListView, FALSE);
			SetFocus(m_hActiveListView);

			m_pActiveShellBrowser->QueueRename(pidl);
		});

	auto newFolderName = ResourceHelper::LoadString(m_resourceInstance, IDS_NEW_FOLDER_NAME);
	hr = FileOperations::CreateNewFolder(directoryShellItem.get(), newFolderName, sink.get());

	if (FAILED(hr))
	{
		auto errorMessage = ResourceHelper::LoadString(m_resourceInstance, IDS_NEWFOLDERERROR);
		MessageBox(m_hContainer, errorMessage.c_str(), NExplorerplusplus::APP_NAME,
			MB_ICONERROR | MB_OK);
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
			SIZEOF_ARRAY(szFullFileName));

		if (hr == S_OK)
		{
			/* Strip the filename, just leaving the path component. */
			StringCchCopy(szPath, std::size(szPath), szFullFileName);
			PathRemoveFileSpec(szPath);

			Tab &newTab = GetActivePane()->GetTabContainer()->CreateNewTab(szPath,
				TabSettings(_selected = true));

			if (newTab.GetShellBrowser()->GetDirectory() == szPath)
			{
				wil::com_ptr_nothrow<IShellFolder> parent;
				hr = SHBindToObject(nullptr, newTab.GetShellBrowser()->GetDirectoryIdl().get(),
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

HRESULT Explorerplusplus::OnGoToOffset(int offset)
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	return selectedTab.GetShellBrowser()->GetNavigationController()->GoToOffset(offset);
}

HRESULT Explorerplusplus::OnGoHome()
{
	Tab &selectedTab = GetActivePane()->GetTabContainer()->GetSelectedTab();
	HRESULT hr = selectedTab.GetShellBrowser()->GetNavigationController()->Navigate(
		m_config->defaultTabDirectory);

	if (FAILED(hr))
	{
		hr = selectedTab.GetShellBrowser()->GetNavigationController()->Navigate(
			m_config->defaultTabDirectoryStatic);
	}

	return hr;
}
