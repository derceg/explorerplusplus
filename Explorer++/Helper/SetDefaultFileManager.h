// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

namespace DefaultFileManager
{
enum class ReplaceExplorerMode
{
	None = 1,
	FileSystem = 2,
	All = 3
};

LSTATUS SetAsDefaultFileManagerFileSystem(
	const std::wstring &applicationKeyName, const std::wstring &menuText);
LSTATUS SetAsDefaultFileManagerAll(
	const std::wstring &applicationKeyName, const std::wstring &menuText);
LSTATUS RemoveAsDefaultFileManagerFileSystem(const std::wstring &applicationKeyName);
LSTATUS RemoveAsDefaultFileManagerAll(const std::wstring &applicationKeyName);
}