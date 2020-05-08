// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "ResourceHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/ProcessHelper.h"
#include <tchar.h>
#include <filesystem>

const TCHAR g_resourcesFileName[] = L"Resources";

std::wstring GetResourcePath(const std::wstring &filename)
{
	TCHAR processImageName[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), processImageName, SIZEOF_ARRAY(processImageName));

	std::filesystem::path resourcePath(processImageName);
	resourcePath = resourcePath.parent_path() / g_resourcesFileName / filename;

	return resourcePath;
}