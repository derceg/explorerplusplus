// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Storage.h"
#include "../Helper/ProcessHelper.h"
#include <filesystem>

namespace Storage
{

std::optional<std::wstring> GetEnvironmentVariableWrapper(const std::wstring &name)
{
	auto length = GetEnvironmentVariable(name.c_str(), nullptr, 0);
	if (length == 0)
	{
		return std::nullopt;
	}

	std::wstring value;
	value.resize(length);
	length = GetEnvironmentVariable(name.c_str(), value.data(), length);
	if (length == 0)
	{
		return std::nullopt;
	}
	return value;
}

std::wstring GetConfigFilePath()
{
	auto configPath = GetEnvironmentVariableWrapper(CONFIG_FILE_ENV_VAR_NAME);
	if (configPath) {
		return configPath->c_str();
	}

	wchar_t currentProcessPath[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), currentProcessPath, std::size(currentProcessPath));

	std::filesystem::path configFilePath(currentProcessPath);
	configFilePath.replace_filename(CONFIG_FILE_FILENAME);

	return configFilePath.c_str();
}

}
