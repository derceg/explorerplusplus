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
#include "../Helper/ShellHelper.h"


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
void Explorerplusplus::OnAddressBarGo(void)
{
	TCHAR szPath[MAX_PATH];
	TCHAR szFullFilePath[MAX_PATH];
	TCHAR szCurrentDirectory[MAX_PATH];

	/* Retrieve the combobox text, and determine if it is a
	valid path. */
	SendMessage(m_hAddressBar,WM_GETTEXT,SIZEOF_ARRAY(szPath),(LPARAM)szPath);

	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(szCurrentDirectory),szCurrentDirectory);
	DecodePath(szPath,szCurrentDirectory,szFullFilePath,SIZEOF_ARRAY(szFullFilePath));

	OpenItem(szFullFilePath,FALSE,FALSE);
}