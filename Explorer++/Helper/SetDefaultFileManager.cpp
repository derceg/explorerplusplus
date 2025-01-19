// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SetDefaultFileManager.h"
#include "Helper.h"
#include "Macros.h"
#include "ProcessHelper.h"
#include "RegistrySettings.h"
#include <wil/resource.h>

/*
Notes:

- To replace Explorer for filesystem folders only, add a key at:

  HKEY_CURRENT_USER\Software\Classes\Directory

  (Default value is 'none')

- To replace Explorer for all folders, add a key at:

  HKEY_CURRENT_USER\Software\Classes\Folder

  (Default value is empty)

- The value of the "command" sub-key should be of the form:

  "C:\Application.exe" "%1"

  where "%1" is the path passed from the shell, encapsulated within quotes.
*/

namespace DefaultFileManagerInternal
{
const TCHAR KEY_DIRECTORY_SHELL[] = _T("Software\\Classes\\Directory\\shell");
const TCHAR KEY_FOLDER_SHELL[] = _T("Software\\Classes\\Folder\\shell");
const TCHAR SHELL_DEFAULT_VALUE[] = _T("none");

LSTATUS SetAsDefaultFileManagerInternal(DefaultFileManager::ReplaceExplorerMode replacementType,
	const std::wstring &applicationKeyName, const std::wstring &menuText);
LSTATUS RemoveAsDefaultFileManagerInternal(DefaultFileManager::ReplaceExplorerMode replacementType,
	const std::wstring &applicationKeyName);
}

LSTATUS DefaultFileManager::SetAsDefaultFileManagerFileSystem(
	const std::wstring &applicationKeyName, const std::wstring &menuText)
{
	return DefaultFileManagerInternal::SetAsDefaultFileManagerInternal(
		ReplaceExplorerMode::FileSystem, applicationKeyName, menuText);
}

LSTATUS DefaultFileManager::SetAsDefaultFileManagerAll(const std::wstring &applicationKeyName,
	const std::wstring &menuText)
{
	return DefaultFileManagerInternal::SetAsDefaultFileManagerInternal(ReplaceExplorerMode::All,
		applicationKeyName, menuText);
}

LSTATUS DefaultFileManagerInternal::SetAsDefaultFileManagerInternal(
	DefaultFileManager::ReplaceExplorerMode replacementType, const std::wstring &applicationKeyName,
	const std::wstring &menuText)
{
	const TCHAR *shellKeyPath = nullptr;

	switch (replacementType)
	{
	case DefaultFileManager::ReplaceExplorerMode::All:
		shellKeyPath = KEY_FOLDER_SHELL;
		break;

	case DefaultFileManager::ReplaceExplorerMode::FileSystem:
	default:
		shellKeyPath = KEY_DIRECTORY_SHELL;
		break;
	}

	wil::unique_hkey shellKey;
	LSTATUS res = RegCreateKeyEx(HKEY_CURRENT_USER, shellKeyPath, 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &shellKey, nullptr);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	wil::unique_hkey appKey;
	res = RegCreateKeyEx(shellKey.get(), applicationKeyName.c_str(), 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &appKey, nullptr);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	// Now, set the default value for the key. This default value will be the text that is shown on
	// the context menu for folders.
	res = RegistrySettings::SaveString(appKey.get(), L"", menuText);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	// Now, create the "command" sub-key.
	wil::unique_hkey commandKey;
	res = RegCreateKeyEx(appKey.get(), _T("command"), 0, nullptr, REG_OPTION_NON_VOLATILE,
		KEY_WRITE, nullptr, &commandKey, nullptr);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	TCHAR command[512];
	TCHAR executable[MAX_PATH];

	GetProcessImageName(GetCurrentProcessId(), executable, std::size(executable));
	StringCchPrintf(command, std::size(command), _T("\"%s\" \"%%1\""), executable);

	res = RegistrySettings::SaveString(commandKey.get(), L"", command);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	// Set the current entry as the default.
	res = RegistrySettings::SaveString(shellKey.get(), L"", applicationKeyName);

	return res;
}

LSTATUS DefaultFileManager::RemoveAsDefaultFileManagerFileSystem(
	const std::wstring &applicationKeyName)
{
	return DefaultFileManagerInternal::RemoveAsDefaultFileManagerInternal(
		ReplaceExplorerMode::FileSystem, applicationKeyName);
}

LSTATUS DefaultFileManager::RemoveAsDefaultFileManagerAll(const std::wstring &applicationKeyName)
{
	return DefaultFileManagerInternal::RemoveAsDefaultFileManagerInternal(ReplaceExplorerMode::All,
		applicationKeyName);
}

LSTATUS DefaultFileManagerInternal::RemoveAsDefaultFileManagerInternal(
	DefaultFileManager::ReplaceExplorerMode replacementType, const std::wstring &applicationKeyName)
{
	const TCHAR *shellKeyPath = nullptr;
	const TCHAR *defaultValue = nullptr;

	switch (replacementType)
	{
	case DefaultFileManager::ReplaceExplorerMode::All:
		shellKeyPath = KEY_FOLDER_SHELL;
		defaultValue = EMPTY_STRING;
		break;

	case DefaultFileManager::ReplaceExplorerMode::FileSystem:
	default:
		shellKeyPath = KEY_DIRECTORY_SHELL;
		defaultValue = SHELL_DEFAULT_VALUE;
		break;
	}

	// Remove the shell default value.
	wil::unique_hkey shellKey;
	LSTATUS res = RegOpenKeyEx(HKEY_CURRENT_USER, shellKeyPath, 0, KEY_WRITE, &shellKey);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	res = RegistrySettings::SaveString(shellKey.get(), L"", defaultValue);

	if (res != ERROR_SUCCESS)
	{
		return res;
	}

	// Remove the main application key.
	std::wstring commandKeyPath = std::wstring(shellKeyPath) + L"\\" + applicationKeyName;
	res = SHDeleteKey(HKEY_CURRENT_USER, commandKeyPath.c_str());

	return res;
}
