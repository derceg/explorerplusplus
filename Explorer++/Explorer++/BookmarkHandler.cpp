/******************************************************************
 *
 * Project: Explorer++
 * File: BookmarkHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles tasks associated with bookmarks,
 * such as creating a bookmarks menu, and
 * adding bookmarks to a toolbar.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "AddBookmarkDialog.h"
#include "NewBookmarkFolderDialog.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/Macros.h"


HRESULT Explorerplusplus::ExpandAndBrowsePath(TCHAR *szPath)
{
	return ExpandAndBrowsePath(szPath,FALSE,FALSE);
}

/* Browses to the specified path. The path may
have any environment variables expanded (if
necessary). */
HRESULT Explorerplusplus::ExpandAndBrowsePath(TCHAR *szPath,BOOL bOpenInNewTab,BOOL bSwitchToNewTab)
{
	TCHAR szExpandedPath[MAX_PATH];

	MyExpandEnvironmentStrings(szPath,
		szExpandedPath,SIZEOF_ARRAY(szExpandedPath));

	return BrowseFolder(szExpandedPath,SBSP_ABSOLUTE,bOpenInNewTab,bSwitchToNewTab,FALSE);
}