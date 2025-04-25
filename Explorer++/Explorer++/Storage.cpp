// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Storage.h"
#include "../Helper/ProcessHelper.h"
#include <wil/win32_helpers.h>
#include <filesystem>

namespace Storage
{

std::wstring GetConfigFilePath()
{
	wil::unique_hglobal_string configPath;
	HRESULT hr = wil::GetEnvironmentVariableW(CONFIG_FILE_ENV_VAR_NAME, configPath);
	if (SUCCEEDED(hr))
	{
		return std::wstring(configPath.get());
	}

	wchar_t currentProcessPath[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), currentProcessPath, std::size(currentProcessPath));

	std::filesystem::path configFilePath(currentProcessPath);
	configFilePath.replace_filename(CONFIG_FILE_FILENAME);

	return configFilePath.c_str();
}

}
