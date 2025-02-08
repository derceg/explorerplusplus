// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ResourceTestHelper.h"
#include "../Helper/ProcessHelper.h"
#include <windows.h>
#include <filesystem>

namespace
{

constexpr wchar_t RESOURCES_DIRECTORY_NAME[] = L"Resources";

}

std::filesystem::path GetResourcesDirectoryPath()
{
	wchar_t processImageName[MAX_PATH];
	GetProcessImageName(GetCurrentProcessId(), processImageName, std::size(processImageName));

	std::filesystem::path path(processImageName);
	return path.parent_path() / RESOURCES_DIRECTORY_NAME;
}

std::filesystem::path GetResourcePath(const std::wstring &filename)
{
	return GetResourcesDirectoryPath() / filename;
}
