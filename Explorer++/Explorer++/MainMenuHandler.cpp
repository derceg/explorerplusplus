// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AboutDialog.h"
#include "ColorRuleModelFactory.h"
#include "Config.h"
#include "CustomizeColorsDialog.h"
#include "DestroyFilesDialog.h"
#include "DisplayColoursDialog.h"
#include "Explorer++_internal.h"
#include "FileProgressSink.h"
#include "FilterDialog.h"
#include "MainResource.h"
#include "MergeFilesDialog.h"
#include "ModelessDialogs.h"
#include "OptionsDialog.h"
#include "ScriptingDialog.h"
#include "SearchDialog.h"
#include "SearchTabsDialog.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "SplitFileDialog.h"
#include "TabContainer.h"
#include "UpdateCheckDialog.h"
#include "WildcardSelectDialog.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
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
	if (g_hwndSearch == nullptr)
	{
		Tab &selectedTab = m_tabContainer->GetSelectedTab();
		std::wstring currentDirectory = selectedTab.GetShellBrowser()->GetDirectory();

		auto *searchDialog = new SearchDialog(m_resourceInstance, m_hContainer, currentDirectory,
			this, this, m_tabContainer);
		g_hwndSearch = searchDialog->ShowModelessDialog([]() { g_hwndSearch = nullptr; });
	}
	else
	{
		SetFocus(g_hwndSearch);
	}
}

void Explorerplusplus::OnCustomizeColors()
{
	CustomizeColorsDialog customizeColorsDialog(m_resourceInstance, m_hContainer, this,
		ColorRuleModelFactory::GetInstance()->GetColorRuleModel());
	customizeColorsDialog.ShowModalDialog();
}

void Explorerplusplus::OnRunScript()
{
	if (g_hwndRunScript == nullptr)
	{
		auto *scriptingDialog = new ScriptingDialog(m_resourceInstance, m_hContainer, this);
		g_hwndRunScript = scriptingDialog->ShowModelessDialog([]() { g_hwndRunScript = nullptr; });
	}
	else
	{
		SetFocus(g_hwndRunScript);
	}
}

void Explorerplusplus::OnShowOptions()
{
	if (g_hwndOptions == nullptr)
	{
		auto *optionsDialog = new OptionsDialog(m_resourceInstance, m_hContainer, m_config, this);
		g_hwndOptions = optionsDialog->ShowModelessDialog([]() { g_hwndOptions = nullptr; });
	}
	else
	{
		SetFocus(g_hwndOptions);
	}
}

void Explorerplusplus::OnSearchTabs()
{
	if (g_hwndSearchTabs == nullptr)
	{
		auto *searchTabsDialog = SearchTabsDialog::Create(m_resourceInstance, m_hContainer, this);
		g_hwndSearchTabs =
			searchTabsDialog->ShowModelessDialog([]() { g_hwndSearchTabs = nullptr; });
	}
	else
	{
		SetFocus(g_hwndSearchTabs);
	}
}

void Explorerplusplus::OnOpenOnlineDocumentation()
{
	ShellExecute(nullptr, L"open", NExplorerplusplus::DOCUMENTATION_LINK, nullptr, nullptr,
		SW_SHOW);
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
		NFileOperations::SaveDirectoryListing(directory, fileName);
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

	FileProgressSink *sink = FileProgressSink::CreateNew();
	sink->SetPostNewItemObserver(
		[this](PIDLIST_ABSOLUTE pidl)
		{
			ListViewHelper::SelectAllItems(m_hActiveListView, FALSE);
			SetFocus(m_hActiveListView);

			m_pActiveShellBrowser->QueueRename(pidl);
		});

	TCHAR newFolderName[128];
	LoadString(m_resourceInstance, IDS_NEW_FOLDER_NAME, newFolderName, SIZEOF_ARRAY(newFolderName));
	hr = NFileOperations::CreateNewFolder(directoryShellItem.get(), newFolderName, sink);
	sink->Release();

	if (FAILED(hr))
	{
		TCHAR szTemp[512];

		LoadString(m_resourceInstance, IDS_NEWFOLDERERROR, szTemp, SIZEOF_ARRAY(szTemp));

		MessageBox(m_hContainer, szTemp, NExplorerplusplus::APP_NAME, MB_ICONERROR | MB_OK);
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

		hr = NFileOperations::ResolveLink(m_hContainer, 0, shortcutFileName.c_str(), szFullFileName,
			SIZEOF_ARRAY(szFullFileName));

		if (hr == S_OK)
		{
			/* Strip the filename, just leaving the path component. */
			StringCchCopy(szPath, SIZEOF_ARRAY(szPath), szFullFileName);
			PathRemoveFileSpec(szPath);

			Tab &newTab = m_tabContainer->CreateNewTab(szPath, TabSettings(_selected = true));

			if (newTab.GetShellBrowser()->GetDirectory() == szPath)
			{
				wil::com_ptr_nothrow<IShellFolder> parent;
				hr = SHBindToObject(nullptr, newTab.GetShellBrowser()->GetDirectoryIdl().get(),
					nullptr, IID_PPV_ARGS(&parent));

				if (hr == S_OK)
				{
					auto *filename = PathFindFileName(szFullFileName);
					assert(filename != szFullFileName);

					unique_pidl_absolute pidl;
					hr = CreateSimplePidl(filename, wil::out_param(pidl), parent.get());

					if (SUCCEEDED(hr))
					{
						m_pActiveShellBrowser->SelectItems({ pidl.get() });
					}
				}
			}

			SetFocus(m_hActiveListView);
		}
	}
}

HRESULT Explorerplusplus::OnGoBack()
{
	Tab &selectedTab = m_tabContainer->GetSelectedTab();
	return selectedTab.GetShellBrowser()->GetNavigationController()->GoBack();
}

HRESULT Explorerplusplus::OnGoForward()
{
	Tab &selectedTab = m_tabContainer->GetSelectedTab();
	return selectedTab.GetShellBrowser()->GetNavigationController()->GoForward();
}

HRESULT Explorerplusplus::OnNavigateUp()
{
	Tab &selectedTab = m_tabContainer->GetSelectedTab();
	return selectedTab.GetShellBrowser()->GetNavigationController()->GoUp();
}

HRESULT Explorerplusplus::OnGoToOffset(int offset)
{
	Tab &selectedTab = m_tabContainer->GetSelectedTab();
	return selectedTab.GetShellBrowser()->GetNavigationController()->GoToOffset(offset);
}

HRESULT Explorerplusplus::OnGoToKnownFolder(REFKNOWNFOLDERID knownFolderId)
{
	unique_pidl_absolute pidl;
	HRESULT hr =
		SHGetKnownFolderIDList(knownFolderId, KF_FLAG_DEFAULT, nullptr, wil::out_param(pidl));

	if (FAILED(hr))
	{
		return hr;
	}

	return GoToPidl(pidl.get());
}

HRESULT Explorerplusplus::OnGoToPath(const std::wstring &path)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(path.c_str(), nullptr, wil::out_param(pidl), 0, nullptr);

	if (FAILED(hr))
	{
		return hr;
	}

	return GoToPidl(pidl.get());
}

HRESULT Explorerplusplus::GoToPidl(PCIDLIST_ABSOLUTE pidl)
{
	Tab &selectedTab = m_tabContainer->GetSelectedTab();
	auto navigateParams = NavigateParams::Normal(pidl);
	return selectedTab.GetShellBrowser()->GetNavigationController()->Navigate(navigateParams);
}

HRESULT Explorerplusplus::OnGoHome()
{
	Tab &selectedTab = m_tabContainer->GetSelectedTab();
	HRESULT hr = selectedTab.GetShellBrowser()->GetNavigationController()->Navigate(
		m_config->defaultTabDirectory);

	if (FAILED(hr))
	{
		hr = selectedTab.GetShellBrowser()->GetNavigationController()->Navigate(
			m_config->defaultTabDirectoryStatic);
	}

	return hr;
}
