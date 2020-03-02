// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Navigation.h"
#include "TabContainer.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"

HRESULT Explorerplusplus::ExpandAndBrowsePath(const TCHAR *szPath)
{
	return ExpandAndBrowsePath(szPath, FALSE, FALSE);
}

/* Browses to the specified path. The path may
have any environment variables expanded (if
necessary). */
HRESULT Explorerplusplus::ExpandAndBrowsePath(
	const TCHAR *szPath, BOOL bOpenInNewTab, BOOL bSwitchToNewTab)
{
	TCHAR szExpandedPath[MAX_PATH];

	MyExpandEnvironmentStrings(szPath, szExpandedPath, SIZEOF_ARRAY(szExpandedPath));

	if (bOpenInNewTab)
	{
		TabSettings tabSettings;

		if (bSwitchToNewTab)
		{
			tabSettings.selected = true;
		}

		return m_tabContainer->CreateNewTab(szExpandedPath, tabSettings);
	}

	return m_navigation->BrowseFolderInCurrentTab(szExpandedPath);
}