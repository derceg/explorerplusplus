// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

namespace Storage
{

enum class OperationType
{
	Load,
	Save
};

// Registry keys used to store application settings.
inline const wchar_t REGISTRY_APPLICATION_KEY_PATH[] = L"Software\\Explorer++";
inline const wchar_t REGISTRY_SETTINGS_KEY_NAME[] = L"Settings";

// The name of the config file that settings are stored in (ignored if
// CONFIG_FILE_ENV_VAR_NAME is set).
inline const wchar_t CONFIG_FILE_FILENAME[] = L"config.xml";
inline const wchar_t CONFIG_FILE_ROOT_NODE_NAME[] = L"ExplorerPlusPlus";
inline const wchar_t CONFIG_FILE_SETTINGS_NODE_NAME[] = L"Settings";
inline const wchar_t CONFIG_FILE_ENV_VAR_NAME[] = L"EXPLORERPP_CONFIG";

std::wstring GetConfigFilePath();

}
