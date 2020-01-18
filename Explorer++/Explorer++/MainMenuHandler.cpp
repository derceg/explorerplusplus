// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AboutDialog.h"
#include "Config.h"
#include "CustomizeColorsDialog.h"
#include "DestroyFilesDialog.h"
#include "DisplayColoursDialog.h"
#include "FileProgressSink.h"
#include "FilterDialog.h"
#include "HelpFileMissingDialog.h"
#include "IModelessDialogNotification.h"
#include "MergeFilesDialog.h"
#include "ModelessDialogs.h"
#include "OptionsDialog.h"
#include "ScriptingDialog.h"
#include "SearchDialog.h"
#include "SplitFileDialog.h"
#include "UpdateCheckDialog.h"
#include "WildcardSelectDialog.h"
#include "MainResource.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"
#include <boost/scope_exit.hpp>
#include <wil/com.h>

#pragma warning(disable:4459) // declaration of 'boost_scope_exit_aux_args' hides global declaration

void Explorerplusplus::OnChangeDisplayColors()
{
	DisplayColoursDialog displayColoursDialog(m_hLanguageModule, m_hContainer,
		m_hDisplayWindow, m_config->displayWindowCentreColor.ToCOLORREF(),
		m_config->displayWindowSurroundColor.ToCOLORREF());
	displayColoursDialog.ShowModalDialog();
}

void Explorerplusplus::OnFilterResults()
{
	FilterDialog filterDialog(m_hLanguageModule, m_hContainer, this);
	filterDialog.ShowModalDialog();
}

void Explorerplusplus::OnMergeFiles()
{
	std::wstring currentDirectory = m_pActiveShellBrowser->GetDirectory();

	std::list<std::wstring>	FullFilenameList;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
	{
		TCHAR szFullFilename[MAX_PATH];
		m_pActiveShellBrowser->GetItemFullName(iItem, szFullFilename, SIZEOF_ARRAY(szFullFilename));
		FullFilenameList.push_back(szFullFilename);
	}

	MergeFilesDialog mergeFilesDialog(m_hLanguageModule, m_hContainer, this, currentDirectory,
		FullFilenameList, m_config->globalFolderSettings.showFriendlyDates);
	mergeFilesDialog.ShowModalDialog();
}

void Explorerplusplus::OnSplitFile()
{
	int iSelected = ListView_GetNextItem(m_hActiveListView, -1, LVNI_SELECTED);

	if(iSelected != -1)
	{
		TCHAR szFullFilename[MAX_PATH];
		m_pActiveShellBrowser->GetItemFullName(iSelected, szFullFilename, SIZEOF_ARRAY(szFullFilename));

		SplitFileDialog splitFileDialog(m_hLanguageModule, m_hContainer, this, szFullFilename);
		splitFileDialog.ShowModalDialog();
	}
}

void Explorerplusplus::OnDestroyFiles()
{
	std::list<std::wstring>	FullFilenameList;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
	{
		TCHAR szFullFilename[MAX_PATH];
		m_pActiveShellBrowser->GetItemFullName(iItem, szFullFilename, SIZEOF_ARRAY(szFullFilename));
		FullFilenameList.push_back(szFullFilename);
	}

	DestroyFilesDialog destroyFilesDialog(m_hLanguageModule, m_hContainer,
		FullFilenameList, m_config->globalFolderSettings.showFriendlyDates);
	destroyFilesDialog.ShowModalDialog();
}

void Explorerplusplus::OnWildcardSelect(BOOL bSelect)
{
	WildcardSelectDialog wilcardSelectDialog(m_hLanguageModule, m_hContainer, bSelect, this);
	wilcardSelectDialog.ShowModalDialog();
}

void Explorerplusplus::OnSearch()
{
	if(g_hwndSearch == NULL)
	{
		Tab &selectedTab = m_tabContainer->GetSelectedTab();
		std::wstring currentDirectory = selectedTab.GetShellBrowser()->GetDirectory();

		SearchDialog *searchDialog = new SearchDialog(m_hLanguageModule, m_hContainer,
			currentDirectory, this, m_tabContainer);
		g_hwndSearch = searchDialog->ShowModelessDialog(new ModelessDialogNotification());
	}
	else
	{
		SetFocus(g_hwndSearch);
	}
}

void Explorerplusplus::OnCustomizeColors()
{
	CustomizeColorsDialog customizeColorsDialog(m_hLanguageModule, m_hContainer, this, &m_ColorRules);
	customizeColorsDialog.ShowModalDialog();

	/* Causes the active listview to redraw (therefore
	applying any updated color schemes). */
	InvalidateRect(m_hActiveListView, NULL, FALSE);
}

void Explorerplusplus::OnRunScript()
{
	if (g_hwndRunScript == NULL)
	{
		ScriptingDialog *scriptingDialog = new ScriptingDialog(m_hLanguageModule, m_hContainer, this);
		g_hwndRunScript = scriptingDialog->ShowModelessDialog(new ModelessDialogNotification());
	}
	else
	{
		SetFocus(g_hwndRunScript);
	}
}

void Explorerplusplus::OnShowOptions()
{
	if(g_hwndOptions == NULL)
	{
		OptionsDialog *optionsDialog = OptionsDialog::Create(m_config, m_hLanguageModule, this, m_tabContainer);
		g_hwndOptions = optionsDialog->Show(m_hContainer);
	}
	else
	{
		SetFocus(g_hwndOptions);
	}
}

void Explorerplusplus::OnShowHelp()
{
	TCHAR szHelpFile[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), szHelpFile, SIZEOF_ARRAY(szHelpFile));
	PathRemoveFileSpec(szHelpFile);
	PathAppend(szHelpFile, NExplorerplusplus::HELP_FILE_NAME);

	unique_pidl_absolute pidl;
	HRESULT hr = SHParseDisplayName(szHelpFile, nullptr, wil::out_param(pidl), 0, nullptr);

	bool bOpenedHelpFile = false;

	if(SUCCEEDED(hr))
	{
		BOOL bRes = ExecuteFileAction(m_hContainer, NULL, NULL, NULL, pidl.get());

		if(bRes)
		{
			bOpenedHelpFile = true;
		}
	}

	if(!bOpenedHelpFile)
	{
		HelpFileMissingDialog helpFileMissingDialog(m_hLanguageModule, m_hContainer);
		helpFileMissingDialog.ShowModalDialog();
	}
}

void Explorerplusplus::OnCheckForUpdates()
{
	UpdateCheckDialog updateCheckDialog(m_hLanguageModule, m_hContainer);
	updateCheckDialog.ShowModalDialog();
}

void Explorerplusplus::OnAbout()
{
	AboutDialog aboutDialog(m_hLanguageModule, m_hContainer);
	aboutDialog.ShowModalDialog();
}

void Explorerplusplus::OnSaveDirectoryListing() const
{
	TCHAR FileName[MAX_PATH];
	LoadString(m_hLanguageModule, IDS_GENERAL_DIRECTORY_LISTING_FILENAME, FileName, SIZEOF_ARRAY(FileName));
	StringCchCat(FileName, SIZEOF_ARRAY(FileName), _T(".txt"));
	BOOL bSaveNameRetrieved = GetFileNameFromUser(m_hContainer, FileName, SIZEOF_ARRAY(FileName), m_CurrentDirectory.c_str());

	if(bSaveNameRetrieved)
	{
		NFileOperations::SaveDirectoryListing(m_CurrentDirectory, FileName);
	}
}

void Explorerplusplus::OnCreateNewFolder()
{
	auto pidlDirectory = m_pActiveShellBrowser->GetDirectoryIdl();

	wil::com_ptr<IShellItem> directoryShellItem;
	HRESULT hr = SHCreateItemFromIDList(pidlDirectory.get(), IID_PPV_ARGS(&directoryShellItem));

	if (FAILED(hr))
	{
		return;
	}

	FileProgressSink *sink = FileProgressSink::CreateNew();
	sink->SetPostNewItemObserver([this] (PIDLIST_ABSOLUTE pidl) {
		NListView::ListView_SelectAllItems(m_hActiveListView, FALSE);
		SetFocus(m_hActiveListView);

		m_pActiveShellBrowser->QueueRename(pidl);
	});

	TCHAR newFolderName[128];
	LoadString(m_hLanguageModule, IDS_NEW_FOLDER_NAME, newFolderName, SIZEOF_ARRAY(newFolderName));
	hr = NFileOperations::CreateNewFolder(directoryShellItem.get(), newFolderName, sink);
	sink->Release();

	if(FAILED(hr))
	{
		TCHAR szTemp[512];

		LoadString(m_hLanguageModule, IDS_NEWFOLDERERROR, szTemp,
			SIZEOF_ARRAY(szTemp));

		MessageBox(m_hContainer, szTemp, NExplorerplusplus::APP_NAME,
			MB_ICONERROR | MB_OK);
	}
}

void Explorerplusplus::OnResolveLink()
{
	TCHAR	ShortcutFileName[MAX_PATH];
	TCHAR	szFullFileName[MAX_PATH];
	TCHAR	szPath[MAX_PATH];
	HRESULT	hr;
	int		iItem;

	iItem = ListView_GetNextItem(m_hActiveListView, -1, LVNI_FOCUSED);

	if(iItem != -1)
	{
		m_pActiveShellBrowser->GetItemFullName(iItem, ShortcutFileName, SIZEOF_ARRAY(ShortcutFileName));

		hr = NFileOperations::ResolveLink(m_hContainer, 0, ShortcutFileName, szFullFileName, SIZEOF_ARRAY(szFullFileName));

		if(hr == S_OK)
		{
			/* Strip the filename, just leaving the path component. */
			StringCchCopy(szPath, SIZEOF_ARRAY(szPath), szFullFileName);
			PathRemoveFileSpec(szPath);

			hr = m_tabContainer->CreateNewTab(szPath, TabSettings(_selected = true));

			if(SUCCEEDED(hr))
			{
				/* Strip off the path, and select the shortcut target
				in the listview. */
				PathStripPath(szFullFileName);
				m_pActiveShellBrowser->SelectFiles(szFullFileName);

				SetFocus(m_hActiveListView);
			}
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

HRESULT Explorerplusplus::OnGoToOffset(int offset)
{
	Tab &selectedTab = m_tabContainer->GetSelectedTab();
	return selectedTab.GetShellBrowser()->GetNavigationController()->GoToOffset(offset);
}

HRESULT Explorerplusplus::OnGoToKnownFolder(REFKNOWNFOLDERID knownFolderId)
{
	unique_pidl_absolute pidl;
	HRESULT hr = SHGetKnownFolderIDList(knownFolderId, KF_FLAG_DEFAULT, nullptr, wil::out_param(pidl));

	if (FAILED(hr))
	{
		return hr;
	}

	Tab &selectedTab = m_tabContainer->GetSelectedTab();
	return selectedTab.GetShellBrowser()->GetNavigationController()->BrowseFolder(pidl.get());
}

HRESULT Explorerplusplus::OnGoHome()
{
	Tab &selectedTab = m_tabContainer->GetSelectedTab();
	HRESULT hr = selectedTab.GetShellBrowser()->GetNavigationController()->BrowseFolder(m_config->defaultTabDirectory);

	if (FAILED(hr))
	{
		hr = selectedTab.GetShellBrowser()->GetNavigationController()->BrowseFolder(m_config->defaultTabDirectoryStatic);
	}

	return hr;
}