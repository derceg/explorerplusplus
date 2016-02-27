/******************************************************************
*
* Project: Explorer++
* File: MainMenuHandler.cpp
* License: GPL - See LICENSE in the top level directory
*
* Handles some of the main menu entries (which might also be
* invoked through other means, such as via keyboard accelerators).
*
* Written by David Erceg
* www.explorerplusplus.com
*
******************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "CustomizeColorsDialog.h"
#include "HelpFileMissingDialog.h"
#include "IModelessDialogNotification.h"
#include "ModelessDialogs.h"
#include "SearchDialog.h"
#include "WildcardSelectDialog.h"
#include "MainResource.h"
#include "../Helper/ProcessHelper.h"
#include "../Helper/ShellHelper.h"


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

		CSearchDialog *SearchDialog = new CSearchDialog(m_hLanguageModule, IDD_SEARCH, m_hContainer, szCurrentDirectory, this);
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
	}

	if(!bOpenedHelpFile)
	{
		CHelpFileMissingDialog HelpFileMissingDialog(m_hLanguageModule, IDD_HELPFILEMISSING, m_hContainer);
		HelpFileMissingDialog.ShowModalDialog();
	}
}