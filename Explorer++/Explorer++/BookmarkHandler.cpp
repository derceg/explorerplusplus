// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AddBookmarkDialog.h"
#include "NewBookmarkFolderDialog.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/Macros.h"


HRESULT Explorerplusplus::ExpandAndBrowsePath(const TCHAR *szPath)
{
	return ExpandAndBrowsePath(szPath,FALSE,FALSE);
}

/* Browses to the specified path. The path may
have any environment variables expanded (if
necessary). */
HRESULT Explorerplusplus::ExpandAndBrowsePath(const TCHAR *szPath, BOOL bOpenInNewTab, BOOL bSwitchToNewTab)
{
	TCHAR szExpandedPath[MAX_PATH];

	MyExpandEnvironmentStrings(szPath,
		szExpandedPath,SIZEOF_ARRAY(szExpandedPath));

	if (bOpenInNewTab)
	{
		return CreateNewTab(szExpandedPath, nullptr, {}, bSwitchToNewTab, nullptr);
	}

	return BrowseFolderInCurrentTab(szExpandedPath,SBSP_ABSOLUTE);
}