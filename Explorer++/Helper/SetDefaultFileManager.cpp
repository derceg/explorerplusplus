/******************************************************************
 *
 * Project: Helper
 * File: SetDefaultFileManager.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Sets/removes an application as the default file manager.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

/*
Notes:

- To replace Explorer for filesystem folders only,
  add a key at:
  HKEY_CLASSES_ROOT\Directory
  (Default value is 'none')

- To replace Explorer for all folders, add a key at:
  HKEY_CLASSES_ROOT\Folder
  (Default value is empty)

The value of the "command" sub-key will be of the form:
  "C:\Application.exe" "%1"
  where "%1" is the path passed from the shell,
  encapsulated within quotes.
*/

#include "stdafx.h"
#include "Helper.h"
#include "SetDefaultFileManager.h"


namespace NDefaultFileManagerInternal
{
	const TCHAR *KEY_DIRECTORY_SHELL	= _T("Directory\\shell");
	const TCHAR *KEY_FOLDER_SHELL		= _T("Folder\\shell");
	const TCHAR *SHELL_DEFAULT_VALUE	= _T("none");

	BOOL SetAsDefaultFileManagerInternal(NDefaultFileManager::ReplaceExplorerModes_t ReplacementType,
		TCHAR *szInternalCommand,TCHAR *szMenuText);
	BOOL RemoveAsDefaultFileManagerInternal(NDefaultFileManager::ReplaceExplorerModes_t ReplacementType,
		TCHAR *szInternalCommand);
}

BOOL NDefaultFileManager::SetAsDefaultFileManagerFileSystem(TCHAR *szInternalCommand,TCHAR *szMenuText)
{
	return NDefaultFileManagerInternal::SetAsDefaultFileManagerInternal(REPLACEEXPLORER_FILESYSTEM,
		szInternalCommand,szMenuText);
}

BOOL NDefaultFileManager::SetAsDefaultFileManagerAll(TCHAR *szInternalCommand,TCHAR *szMenuText)
{
	return NDefaultFileManagerInternal::SetAsDefaultFileManagerInternal(REPLACEEXPLORER_ALL,
		szInternalCommand,szMenuText);
}

BOOL NDefaultFileManagerInternal::SetAsDefaultFileManagerInternal(NDefaultFileManager::ReplaceExplorerModes_t ReplacementType,
	TCHAR *szInternalCommand,TCHAR *szMenuText)
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);

	GetVersionEx(&osvi);

	if(osvi.dwMajorVersion == WINDOWS_XP_MAJORVERSION &&
		ReplacementType == NDefaultFileManager::REPLACEEXPLORER_ALL)
	{
		return FALSE;
	}

	const TCHAR *pszSubKey = NULL;

	switch(ReplacementType)
	{
	case NDefaultFileManager::REPLACEEXPLORER_FILESYSTEM:
		pszSubKey = KEY_DIRECTORY_SHELL;
		break;

	case NDefaultFileManager::REPLACEEXPLORER_ALL:
		pszSubKey = KEY_FOLDER_SHELL;
		break;

	default:
		pszSubKey = KEY_DIRECTORY_SHELL;
		break;
	}

	HKEY hKeyShell;
	LONG lRes;
	BOOL bSuccess = FALSE;

	lRes = RegOpenKeyEx(HKEY_CLASSES_ROOT,
		pszSubKey,0,KEY_WRITE,&hKeyShell);

	if(lRes == ERROR_SUCCESS)
	{
		HKEY hKeyApp;
		DWORD Disposition;

		lRes = RegCreateKeyEx(hKeyShell,szInternalCommand,
			0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,
			NULL,&hKeyApp,&Disposition);

		if(lRes == ERROR_SUCCESS)
		{
			HKEY hKeyCommand;

			/* Now, set the defaault value for the key. This
			default value will be the text that is shown on the
			context menu for folders. */
			RegSetValueEx(hKeyApp,NULL,0,REG_SZ,reinterpret_cast<LPBYTE>(szMenuText),
				(lstrlen(szMenuText) + 1) * sizeof(TCHAR));

			/* Now, create the "command" sub-key. */
			lRes = RegCreateKeyEx(hKeyApp,_T("command"),
				0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,
				NULL,&hKeyCommand,&Disposition);

			if(lRes == ERROR_SUCCESS)
			{
				TCHAR szCommand[512];
				TCHAR szExecutable[MAX_PATH];

				/* Get the current location of the program, and use
				it as part of the command. */
				GetCurrentProcessImageName(szExecutable,
					SIZEOF_ARRAY(szExecutable));

				StringCchPrintf(szCommand,SIZEOF_ARRAY(szCommand),
					_T("\"%s\" \"%%1\""),szExecutable);

				/* ...and write the command out. */
				lRes = RegSetValueEx(hKeyCommand,NULL,0,REG_SZ,
					reinterpret_cast<LPBYTE>(szCommand),
					(lstrlen(szCommand) + 1) * sizeof(TCHAR));

				if(lRes == ERROR_SUCCESS)
				{
					/* Set the current entry as the default. */
					lRes = RegSetValueEx(hKeyShell,NULL,0,REG_SZ,
						reinterpret_cast<LPBYTE>(szInternalCommand),
						(lstrlen(szInternalCommand) + 1) * sizeof(TCHAR));

					if(lRes == ERROR_SUCCESS)
					{
						bSuccess = TRUE;
					}
				}

				RegCloseKey(hKeyCommand);
			}

			RegCloseKey(hKeyApp);
		}

		RegCloseKey(hKeyShell);
	}

	return bSuccess;
}

BOOL NDefaultFileManager::RemoveAsDefaultFileManagerFileSystem(TCHAR *szInternalCommand)
{
	return NDefaultFileManagerInternal::RemoveAsDefaultFileManagerInternal(REPLACEEXPLORER_FILESYSTEM,
		szInternalCommand);
}

BOOL NDefaultFileManager::RemoveAsDefaultFileManagerAll(TCHAR *szInternalCommand)
{
	return NDefaultFileManagerInternal::RemoveAsDefaultFileManagerInternal(REPLACEEXPLORER_ALL,
		szInternalCommand);
}

BOOL NDefaultFileManagerInternal::RemoveAsDefaultFileManagerInternal(NDefaultFileManager::ReplaceExplorerModes_t ReplacementType,
	TCHAR *szInternalCommand)
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	GetVersionEx(&osvi);

	if(osvi.dwMajorVersion == WINDOWS_XP_MAJORVERSION &&
		ReplacementType == NDefaultFileManager::REPLACEEXPLORER_ALL)
	{
		return FALSE;
	}

	const TCHAR *pszSubKey = NULL;
	const TCHAR *pszDefaultValue = NULL;

	switch(ReplacementType)
	{
	case NDefaultFileManager::REPLACEEXPLORER_FILESYSTEM:
		pszSubKey = KEY_DIRECTORY_SHELL;
		pszDefaultValue = SHELL_DEFAULT_VALUE;
		break;

	case NDefaultFileManager::REPLACEEXPLORER_ALL:
		pszSubKey = KEY_FOLDER_SHELL;
		pszDefaultValue = EMPTY_STRING;
		break;

	default:
		pszSubKey = KEY_DIRECTORY_SHELL;
		pszDefaultValue = SHELL_DEFAULT_VALUE;
		break;
	}

	HKEY hKeyShell;
	LONG lRes1;
	LSTATUS lRes2 = 1;

	/* Remove the shell default value. */
	lRes1 = RegOpenKeyEx(HKEY_CLASSES_ROOT,
		pszSubKey,0,KEY_WRITE,&hKeyShell);

	if(lRes1 == ERROR_SUCCESS)
	{
		lRes1 = RegSetValueEx(hKeyShell,NULL,0,REG_SZ,
			reinterpret_cast<const BYTE *>(pszDefaultValue),
			(lstrlen(pszDefaultValue) + 1) * sizeof(TCHAR));

		if(lRes1 == ERROR_SUCCESS)
		{
			TCHAR szDeleteSubKey[512];

			StringCchPrintf(szDeleteSubKey,SIZEOF_ARRAY(szDeleteSubKey),_T("%s\\%s"),
				pszSubKey,szInternalCommand);

			/* Remove the main command key. */
			lRes2 = SHDeleteKey(HKEY_CLASSES_ROOT,szDeleteSubKey);
		}

		RegCloseKey(hKeyShell);
	}

	return ((lRes1 == ERROR_SUCCESS) && (lRes2 == ERROR_SUCCESS));
}