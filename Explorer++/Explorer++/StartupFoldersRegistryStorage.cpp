// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "StartupFoldersRegistryStorage.h"
#include "../Helper/RegistrySettings.h"

namespace
{

constexpr wchar_t SETTING_STARTUP_FOLDER_PATH[] = L"Path";

std::optional<std::wstring> LoadStartupFolder(HKEY key)
{
	std::wstring path;
	auto res = RegistrySettings::ReadString(key, SETTING_STARTUP_FOLDER_PATH, path);

	if (res != ERROR_SUCCESS || path.empty())
	{
		return std::nullopt;
	}

	return path;
}

void SaveStartupFolder(HKEY key, const std::wstring &startupFolder)
{
	RegistrySettings::SaveString(key, SETTING_STARTUP_FOLDER_PATH, startupFolder);
}

}

namespace StartupFoldersRegistryStorage
{

std::vector<std::wstring> Load(HKEY startupFoldersKey)
{
	return RegistrySettings::ReadItemList<std::wstring>(startupFoldersKey, LoadStartupFolder);
}

void Save(HKEY startupFoldersKey, const std::vector<std::wstring> &startupFolders)
{
	RegistrySettings::SaveItemList<std::wstring>(startupFoldersKey, startupFolders,
		SaveStartupFolder);
}

}
