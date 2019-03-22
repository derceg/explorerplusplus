// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AboutDialog.h"
#include "CustomizeColorsDialog.h"
#include "DestroyFilesDialog.h"
#include "DisplayColoursDialog.h"
#include "FileProgressSink.h"
#include "FilterDialog.h"
#include "HelpFileMissingDialog.h"
#include "IModelessDialogNotification.h"
#include "MergeFilesDialog.h"
#include "ModelessDialogs.h"
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

#pragma warning(disable:4459) // declaration of 'boost_scope_exit_aux_args' hides global declaration

void Explorerplusplus::OnChangeDisplayColors()
{
	CDisplayColoursDialog DisplayColoursDialog(m_hLanguageModule, IDD_DISPLAYCOLOURS, m_hContainer,
		m_hDisplayWindow, m_DisplayWindowCentreColor.ToCOLORREF(), m_DisplayWindowSurroundColor.ToCOLORREF());
	DisplayColoursDialog.ShowModalDialog();
}

void Explorerplusplus::OnFilterResults()
{
	CFilterDialog FilterDialog(m_hLanguageModule, IDD_FILTER, m_hContainer, this);
	FilterDialog.ShowModalDialog();
}

void Explorerplusplus::OnMergeFiles()
{
	TCHAR szCurrentDirectory[MAX_PATH];
	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(szCurrentDirectory), szCurrentDirectory);

	std::list<std::wstring>	FullFilenameList;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
	{
		TCHAR szFullFilename[MAX_PATH];
		m_pActiveShellBrowser->QueryFullItemName(iItem, szFullFilename, SIZEOF_ARRAY(szFullFilename));
		FullFilenameList.push_back(szFullFilename);
	}

	CMergeFilesDialog CMergeFilesDialog(m_hLanguageModule, IDD_MERGEFILES,
		m_hContainer, szCurrentDirectory, FullFilenameList, m_config->globalFolderSettings.showFriendlyDates);
	CMergeFilesDialog.ShowModalDialog();
}

void Explorerplusplus::OnSplitFile()
{
	int iSelected = ListView_GetNextItem(m_hActiveListView, -1, LVNI_SELECTED);

	if(iSelected != -1)
	{
		TCHAR szFullFilename[MAX_PATH];
		m_pActiveShellBrowser->QueryFullItemName(iSelected, szFullFilename, SIZEOF_ARRAY(szFullFilename));

		CSplitFileDialog SplitFileDialog(m_hLanguageModule, IDD_SPLITFILE, m_hContainer, szFullFilename);
		SplitFileDialog.ShowModalDialog();
	}
}

void Explorerplusplus::OnDestroyFiles()
{
	std::list<std::wstring>	FullFilenameList;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView, iItem, LVNI_SELECTED)) != -1)
	{
		TCHAR szFullFilename[MAX_PATH];
		m_pActiveShellBrowser->QueryFullItemName(iItem, szFullFilename, SIZEOF_ARRAY(szFullFilename));
		FullFilenameList.push_back(szFullFilename);
	}

	CDestroyFilesDialog CDestroyFilesDialog(m_hLanguageModule, IDD_DESTROYFILES,
		m_hContainer, FullFilenameList, m_config->globalFolderSettings.showFriendlyDates);
	CDestroyFilesDialog.ShowModalDialog();
}

void Explorerplusplus::OnWildcardSelect(BOOL bSelect)
{
	CWildcardSelectDialog WilcardSelectDialog(m_hLanguageModule, IDD_WILDCARDSELECT, m_hContainer, bSelect, this);
	WilcardSelectDialog.ShowModalDialog();
}

void Explorerplusplus::OnSearch()
{
	if(g_hwndSearch == NULL)
	{
		TCHAR szCurrentDirectory[MAX_PATH];
		m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(szCurrentDirectory), szCurrentDirectory);

		CSearchDialog *SearchDialog = new CSearchDialog(m_hLanguageModule, IDD_SEARCH, m_hContainer, szCurrentDirectory, this, this);
		g_hwndSearch = SearchDialog->ShowModelessDialog(new CModelessDialogNotification());
	}
	else
	{
		SetFocus(g_hwndSearch);
	}
}

void Explorerplusplus::OnCustomizeColors()
{
	CCustomizeColorsDialog CustomizeColorsDialog(m_hLanguageModule, IDD_CUSTOMIZECOLORS, m_hContainer, &m_ColorRules);
	CustomizeColorsDialog.ShowModalDialog();

	/* Causes the active listview to redraw (therefore
	applying any updated color schemes). */
	InvalidateRect(m_hActiveListView, NULL, FALSE);
}

void Explorerplusplus::OnRunScript()
{
	if (g_hwndRunScript == NULL)
	{
		ScriptingDialog *scriptingDialog = new ScriptingDialog(m_hLanguageModule, IDD_SCRIPTING, m_hContainer, this);
		g_hwndRunScript = scriptingDialog->ShowModelessDialog(new CModelessDialogNotification());
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
		ShowOptions();
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

	LPITEMIDLIST pidl = NULL;
	HRESULT hr = GetIdlFromParsingName(szHelpFile, &pidl);

	bool bOpenedHelpFile = false;

	if(SUCCEEDED(hr))
	{
		BOOL bRes = ExecuteFileAction(m_hContainer, NULL, NULL, NULL, pidl);

		if(bRes)
		{
			bOpenedHelpFile = true;
		}

		CoTaskMemFree(pidl);
	}

	if(!bOpenedHelpFile)
	{
		CHelpFileMissingDialog HelpFileMissingDialog(m_hLanguageModule, IDD_HELPFILEMISSING, m_hContainer);
		HelpFileMissingDialog.ShowModalDialog();
	}
}

void Explorerplusplus::OnCheckForUpdates()
{
	CUpdateCheckDialog UpdateCheckDialog(m_hLanguageModule, IDD_UPDATECHECK, m_hContainer);
	UpdateCheckDialog.ShowModalDialog();
}

void Explorerplusplus::OnAbout()
{
	CAboutDialog AboutDialog(m_hLanguageModule, IDD_ABOUT, m_hContainer);
	AboutDialog.ShowModalDialog();
}

void Explorerplusplus::OnSaveDirectoryListing() const
{
	TCHAR FileName[MAX_PATH];
	LoadString(m_hLanguageModule, IDS_GENERAL_DIRECTORY_LISTING_FILENAME, FileName, SIZEOF_ARRAY(FileName));
	StringCchCat(FileName, SIZEOF_ARRAY(FileName), _T(".txt"));
	BOOL bSaveNameRetrieved = GetFileNameFromUser(m_hContainer, FileName, SIZEOF_ARRAY(FileName), m_CurrentDirectory);

	if(bSaveNameRetrieved)
	{
		NFileOperations::SaveDirectoryListing(m_CurrentDirectory, FileName);
	}
}

void Explorerplusplus::OnCreateNewFolder()
{
	PIDLPointer pidlDirectory(m_pActiveShellBrowser->QueryCurrentDirectoryIdl());

	IShellItem *directoryShellItem = nullptr;
	HRESULT hr = SHCreateItemFromIDList(pidlDirectory.get(), IID_PPV_ARGS(&directoryShellItem));

	if (FAILED(hr))
	{
		return;
	}

	BOOST_SCOPE_EXIT(directoryShellItem) {
		directoryShellItem->Release();
	} BOOST_SCOPE_EXIT_END

	FileProgressSink *sink = FileProgressSink::CreateNew();
	sink->SetPostNewItemObserver([this] (PIDLIST_ABSOLUTE pidl) {
		m_bCountingDown = TRUE;
		NListView::ListView_SelectAllItems(m_hActiveListView, FALSE);
		SetFocus(m_hActiveListView);

		m_pActiveShellBrowser->QueueRename(pidl);
	});

	TCHAR newFolderName[128];
	LoadString(m_hLanguageModule, IDS_NEW_FOLDER_NAME, newFolderName, SIZEOF_ARRAY(newFolderName));
	hr = NFileOperations::CreateNewFolder(directoryShellItem, newFolderName, sink);
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
		m_pActiveShellBrowser->QueryFullItemName(iItem, ShortcutFileName, SIZEOF_ARRAY(ShortcutFileName));

		hr = NFileOperations::ResolveLink(m_hContainer, 0, ShortcutFileName, szFullFileName, SIZEOF_ARRAY(szFullFileName));

		if(hr == S_OK)
		{
			/* Strip the filename, just leaving the path component. */
			StringCchCopy(szPath, SIZEOF_ARRAY(szPath), szFullFileName);
			PathRemoveFileSpec(szPath);

			hr = CreateNewTab(szPath, nullptr, TabSettings(_selected = true), nullptr);

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