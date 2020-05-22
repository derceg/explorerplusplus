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
#include "SetDefaultFileManager.h"
#include "Helper.h"
#include "Macros.h"
#include "ProcessHelper.h"
#include "RegistrySettings.h"
#include <wil/resource.h>

using namespace NRegistrySettings;

namespace NDefaultFileManagerInternal
{
const TCHAR KEY_DIRECTORY_SHELL[] = _T("Directory\\shell");
const TCHAR KEY_FOLDER_SHELL[] = _T("Folder\\shell");
const TCHAR SHELL_DEFAULT_VALUE[] = _T("none");

LSTATUS SetAsDefaultFileManagerInternal(NDefaultFileManager::ReplaceExplorerMode replacementType,
	const std::wstring &applicationKeyName, const std::wstring &menuText);
LSTATUS RemoveAsDefaultFileManagerInternal(NDefaultFileManager::ReplaceExplorerMode replacementType,
	const std::wstring &applicationKeyName);
}

LSTATUS NDefaultFileManager::SetAsDefaultFileManagerFileSystem(
	const std::wstring &applicationKeyName, const std::wstring &menuText)
{
	return NDefaultFileManagerInternal::SetAsDefaultFileManagerInternal(
		ReplaceExplorerMode::FileSystem, applicationKeyName, menuText);
}

LSTATUS NDefaultFileManager::SetAsDefaultFileManagerAll(
	const std::wstring &applicationKeyName, const std::wstring &menuText)
{
	return NDefaultFileManagerInternal::SetAsDefaultFileManagerInternal(
		ReplaceExplorerMode::All, applicationKeyName, menuText);
}

LSTATUS NDefaultFileManagerInternal::SetAsDefaultFileManagerInternal(
	NDefaultFileManager::ReplaceExplorerMode replacementType,
	const std::wstring &applicationKeyName, const std::wstring &menuText)
{
	const TCHAR *shellKeyPath = nullptr;

	switch (replacementType)
	{
	case NDefaultFileManager::ReplaceExplorerMode::All:
		shellKeyPath = KEY_FOLDER_SHELL;
		break;

	case NDefaultFileManager::ReplaceExplorerMode::FileSystem:
	default:
		shellKeyPath = KEY_DIRECTORY_SHELL;
		break;
	}

	wil::unique_hkey shellKey;
	LSTATUS res = RegOpenKeyEx(HKEY_CLASSES_ROOT, shellKeyPath, 0, KEY_WRITE, &shellKey);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	wil::unique_hkey appKey;
	res = RegCreateKeyEx(shellKey.get(), applicationKeyName.c_str(), 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &appKey, NULL);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	// Now, set the default value for the key. This default value will be the text that is shown on
	// the context menu for folders.
	res = SaveStringToRegistry(appKey.get(), NULL, menuText.c_str());

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	// Now, create the "command" sub-key.
	wil::unique_hkey commandKey;
	res = RegCreateKeyEx(appKey.get(), _T("command"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE,
		NULL, &commandKey, NULL);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	TCHAR command[512];
	TCHAR executable[MAX_PATH];

	GetProcessImageName(GetCurrentProcessId(), executable, SIZEOF_ARRAY(executable));
	StringCchPrintf(command, SIZEOF_ARRAY(command), _T("\"%s\" \"%%1\""), executable);

	res = SaveStringToRegistry(commandKey.get(), NULL, command);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	// Set the current entry as the default.
	res = SaveStringToRegistry(shellKey.get(), NULL, applicationKeyName.c_str());

	return res;
}

LSTATUS NDefaultFileManager::RemoveAsDefaultFileManagerFileSystem(
	const std::wstring &applicationKeyName)
{
	return NDefaultFileManagerInternal::RemoveAsDefaultFileManagerInternal(
		ReplaceExplorerMode::FileSystem, applicationKeyName);
}

LSTATUS NDefaultFileManager::RemoveAsDefaultFileManagerAll(const std::wstring &applicationKeyName)
{
	return NDefaultFileManagerInternal::RemoveAsDefaultFileManagerInternal(
		ReplaceExplorerMode::All, applicationKeyName);
}

LSTATUS NDefaultFileManagerInternal::RemoveAsDefaultFileManagerInternal(
	NDefaultFileManager::ReplaceExplorerMode replacementType,
	const std::wstring &applicationKeyName)
{
	const TCHAR *shellKeyPath = nullptr;
	const TCHAR *defaultValue = nullptr;

	switch (replacementType)
	{
	case NDefaultFileManager::ReplaceExplorerMode::All:
		shellKeyPath = KEY_FOLDER_SHELL;
		defaultValue = EMPTY_STRING;
		break;

	case NDefaultFileManager::ReplaceExplorerMode::FileSystem:
	default:
		shellKeyPath = KEY_DIRECTORY_SHELL;
		defaultValue = SHELL_DEFAULT_VALUE;
		break;
	}

	// Remove the shell default value.
	wil::unique_hkey shellKey;
	LSTATUS res = RegOpenKeyEx(HKEY_CLASSES_ROOT, shellKeyPath, 0, KEY_WRITE, &shellKey);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	res = SaveStringToRegistry(shellKey.get(), NULL, defaultValue);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	// Remove the main application key.
	std::wstring commandKeyPath = std::wstring(shellKeyPath) + L"\\" + applicationKeyName;
	res = SHDeleteKey(HKEY_CLASSES_ROOT, commandKeyPath.c_str());

	return res;
}