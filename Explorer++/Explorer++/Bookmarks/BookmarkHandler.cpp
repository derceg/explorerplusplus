// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Navigation.h"
#include "TabContainer.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"

void Explorerplusplus::ExpandAndBrowsePath(const TCHAR *szPath)
{
	ExpandAndBrowsePath(szPath, FALSE, FALSE);
}

/* Browses to the specified path. The path may
have any environment variables expanded (if
necessary). */
void Explorerplusplus::ExpandAndBrowsePath(
	const TCHAR *szPath, BOOL bOpenInNewTab, BOOL bSwitchToNewTab)
{
	TCHAR szExpandedPath[MAX_PATH];
	BOOL res = MyExpandEnvironmentStrings(szPath, szExpandedPath, SIZEOF_ARRAY(szExpandedPath));

	if (!res)
	{
		StringCchCopy(szExpandedPath, std::size(szExpandedPath), szPath);
	}

	if (bOpenInNewTab)
	{
		TabSettings tabSettings;

		if (bSwitchToNewTab)
		{
			tabSettings.selected = true;
		}

		m_tabContainer->CreateNewTab(szExpandedPath, tabSettings);
		return;
	}

	m_navigation->BrowseFolderInCurrentTab(szExpandedPath);
}