/******************************************************************
 *
 * Project: Explorer++
 * File: AddressBarHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the address bar.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++.h"
#include "Explorer++_internal.h"


/* Called when the user presses 'Enter' while
the address bar has focus, or when the 'Go'
toolbar button to the right of the address
bar is pressed.

The path entered may be relative to the current
directory, or absolute.
Basic procedure:
1. Path is expanded (if possible)
2. Any special character sequences ("..", ".") are removed
3. If the path is a URL, pass it straight out, else
4. If the path is relative, add it onto onto the current directory
*/
void CContainer::OnAddressBarGo(void)
{
	TCHAR			szPath[MAX_PATH];
	TCHAR			szFullFilePath[MAX_PATH];
	TCHAR			szExpandedPath[MAX_PATH];
	TCHAR			szCanonicalPath[MAX_PATH];
	BOOL			bRelative;
	BOOL			bRet;

	/* Retrieve the combobox text, and determine if it is a
	valid path. */
	SendMessage(m_hAddressBar,WM_GETTEXT,SIZEOF_ARRAY(szPath),(LPARAM)szPath);

	/* Attempt to expand the path (in the event that
	it contains embedded environment variables). */
	bRet = MyExpandEnvironmentStrings(szPath,
		szExpandedPath,SIZEOF_ARRAY(szExpandedPath));

	if(!bRet)
	{
		StringCchCopy(szExpandedPath,
			SIZEOF_ARRAY(szExpandedPath),szPath);
	}

	/* Canonicalizing the path will remove any "." and
	".." components. */
	PathCanonicalize(szCanonicalPath,szExpandedPath);

	if(PathIsURL(szCanonicalPath))
	{
		StringCchCopy(szFullFilePath,SIZEOF_ARRAY(szFullFilePath),szCanonicalPath);
	}
	else
	{
		bRelative = PathIsRelative(szCanonicalPath);

		/* If the path is relative, prepend it
		with the current directory. */
		if(bRelative)
		{
			m_pActiveShellBrowser->QueryCurrentDirectory(MAX_PATH,szFullFilePath);
			PathAppend(szFullFilePath,szCanonicalPath);
		}
		else
		{
			StringCchCopy(szFullFilePath,SIZEOF_ARRAY(szFullFilePath),szCanonicalPath);
		}
	}

	OpenItem(szFullFilePath,FALSE);
}