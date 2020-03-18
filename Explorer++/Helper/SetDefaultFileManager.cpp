// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

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
#include "ProcessHelper.h"
#include "RegistrySettings.h"
#include "SetDefaultFileManager.h"
#include "Macros.h"


using namespace NRegistrySettings;

namespace NDefaultFileManagerInternal
{
	const TCHAR KEY_DIRECTORY_SHELL[]	= _T("Directory\\shell");
	const TCHAR KEY_FOLDER_SHELL[]		= _T("Folder\\shell");
	const TCHAR SHELL_DEFAULT_VALUE[]	= _T("none");

	BOOL SetAsDefaultFileManagerInternal(NDefaultFileManager::ReplaceExplorerModes_t replacementType,
		const TCHAR *szInternalCommand, const TCHAR *szMenuText);
	BOOL RemoveAsDefaultFileManagerInternal(NDefaultFileManager::ReplaceExplorerModes_t replacementType,
		const TCHAR *szInternalCommand);
}

BOOL NDefaultFileManager::SetAsDefaultFileManagerFileSystem(const TCHAR *szInternalCommand, const TCHAR *szMenuText)
{
	return NDefaultFileManagerInternal::SetAsDefaultFileManagerInternal(REPLACEEXPLORER_FILESYSTEM,
		szInternalCommand,szMenuText);
}

BOOL NDefaultFileManager::SetAsDefaultFileManagerAll(const TCHAR *szInternalCommand, const TCHAR *szMenuText)
{
	return NDefaultFileManagerInternal::SetAsDefaultFileManagerInternal(REPLACEEXPLORER_ALL,
		szInternalCommand,szMenuText);
}

BOOL NDefaultFileManagerInternal::SetAsDefaultFileManagerInternal(NDefaultFileManager::ReplaceExplorerModes_t replacementType,
	const TCHAR *szInternalCommand, const TCHAR *szMenuText)
{
	const TCHAR *pszSubKey = NULL;

	switch(replacementType)
	{
	case NDefaultFileManager::REPLACEEXPLORER_ALL:
		pszSubKey = KEY_FOLDER_SHELL;
		break;

	case NDefaultFileManager::REPLACEEXPLORER_FILESYSTEM:
	default:
		pszSubKey = KEY_DIRECTORY_SHELL;
		break;
	}

	BOOL bSuccess = FALSE;

	HKEY hKeyShell;
	LONG lRes = RegOpenKeyEx(HKEY_CLASSES_ROOT,
		pszSubKey,0,KEY_WRITE,&hKeyShell);

	if(lRes == ERROR_SUCCESS)
	{
		HKEY hKeyApp;
		lRes = RegCreateKeyEx(hKeyShell,szInternalCommand,
			0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,
			NULL,&hKeyApp,NULL);

		if(lRes == ERROR_SUCCESS)
		{
			/* Now, set the defaault value for the key. This
			default value will be the text that is shown on the
			context menu for folders. */
			lRes = SaveStringToRegistry(hKeyApp, NULL, szMenuText);

			if(lRes == ERROR_SUCCESS)
			{
				/* Now, create the "command" sub-key. */
				HKEY hKeyCommand;
				lRes = RegCreateKeyEx(hKeyApp, _T("command"),
					0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE,
					NULL, &hKeyCommand, NULL);

				if(lRes == ERROR_SUCCESS)
				{
					TCHAR szCommand[512];
					TCHAR szExecutable[MAX_PATH];

					/* Get the current location of the program, and use
					it as part of the command. */
					GetProcessImageName(GetCurrentProcessId(), szExecutable,
						SIZEOF_ARRAY(szExecutable));

					StringCchPrintf(szCommand, SIZEOF_ARRAY(szCommand),
						_T("\"%s\" \"%%1\""), szExecutable);

					/* ...and write the command out. */
					lRes = SaveStringToRegistry(hKeyCommand, NULL, szCommand);

					if(lRes == ERROR_SUCCESS)
					{
						/* Set the current entry as the default. */
						lRes = SaveStringToRegistry(hKeyShell, NULL, szInternalCommand);

						if(lRes == ERROR_SUCCESS)
						{
							bSuccess = TRUE;
						}
					}

					RegCloseKey(hKeyCommand);
				}
			}

			RegCloseKey(hKeyApp);
		}

		RegCloseKey(hKeyShell);
	}

	return bSuccess;
}

BOOL NDefaultFileManager::RemoveAsDefaultFileManagerFileSystem(const TCHAR *szInternalCommand)
{
	return NDefaultFileManagerInternal::RemoveAsDefaultFileManagerInternal(REPLACEEXPLORER_FILESYSTEM,
		szInternalCommand);
}

BOOL NDefaultFileManager::RemoveAsDefaultFileManagerAll(const TCHAR *szInternalCommand)
{
	return NDefaultFileManagerInternal::RemoveAsDefaultFileManagerInternal(REPLACEEXPLORER_ALL,
		szInternalCommand);
}

BOOL NDefaultFileManagerInternal::RemoveAsDefaultFileManagerInternal(NDefaultFileManager::ReplaceExplorerModes_t replacementType,
	const TCHAR *szInternalCommand)
{
	const TCHAR *pszSubKey = NULL;
	const TCHAR *pszDefaultValue = NULL;

	switch(replacementType)
	{
	case NDefaultFileManager::REPLACEEXPLORER_ALL:
		pszSubKey = KEY_FOLDER_SHELL;
		pszDefaultValue = EMPTY_STRING;
		break;

	case NDefaultFileManager::REPLACEEXPLORER_FILESYSTEM:
	default:
		pszSubKey = KEY_DIRECTORY_SHELL;
		pszDefaultValue = SHELL_DEFAULT_VALUE;
		break;
	}

	BOOL bSuccess = FALSE;

	/* Remove the shell default value. */
	HKEY hKeyShell;
	LONG lRes = RegOpenKeyEx(HKEY_CLASSES_ROOT,
		pszSubKey,0,KEY_WRITE,&hKeyShell);

	if(lRes == ERROR_SUCCESS)
	{
		lRes = SaveStringToRegistry(hKeyShell, NULL, pszDefaultValue);

		if(lRes == ERROR_SUCCESS)
		{
			TCHAR szDeleteSubKey[512];
			HRESULT hr = StringCchPrintf(szDeleteSubKey,SIZEOF_ARRAY(szDeleteSubKey),_T("%s\\%s"),
				pszSubKey,szInternalCommand);

			if(SUCCEEDED(hr))
			{
				/* Remove the main command key. */
				lRes = SHDeleteKey(HKEY_CLASSES_ROOT, szDeleteSubKey);

				if(lRes == ERROR_SUCCESS)
				{
					bSuccess = TRUE;
				}
			}
		}

		RegCloseKey(hKeyShell);
	}

	return bSuccess;
}