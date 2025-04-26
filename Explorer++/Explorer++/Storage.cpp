// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Storage.h"
#include "../Helper/Helper.h"
#include "../Helper/ProcessHelper.h"
#include <filesystem>

namespace Storage
{

std::wstring GetConfigFilePath()
{
	auto configPath = GetExpandedEnvironmentVariable(CONFIG_FILE_ENV_VAR_NAME);
	if (configPath)
	{
		return configPath->c_str();
	}

	wchar_t currentProcessPath[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), currentProcessPath, std::size(currentProcessPath));

	std::filesystem::path configFilePath(currentProcessPath);
	configFilePath.replace_filename(CONFIG_FILE_FILENAME);

	return configFilePath.c_str();
}

}
