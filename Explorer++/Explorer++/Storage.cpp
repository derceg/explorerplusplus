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

std::optional<std::wstring> GetExpandedEnvironmentVariable(const std::wstring &name)
{
	wil::unique_hglobal_string value;
	HRESULT hr = wil::GetEnvironmentVariableW(name.c_str(), value);
	if (FAILED(hr))
	{
		return std::nullopt;
	}

	wil::unique_hglobal_string expandedValue;
	hr = wil::ExpandEnvironmentStringsW(value.get(), expandedValue);
	if (FAILED(hr))
	{
		return std::nullopt;
	}
	return std::wstring(expandedValue.get());
}

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
