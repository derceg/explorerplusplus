// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BetterEnumsWrapper.h"

namespace DefaultFileManager
{

// clang-format off
BETTER_ENUM(ReplaceExplorerMode, int,
	None = 1,
	FileSystem = 2,
	All = 3
)
// clang-format on

LSTATUS SetAsDefaultFileManagerFileSystem(const std::wstring &applicationKeyName,
	const std::wstring &menuText);
LSTATUS SetAsDefaultFileManagerAll(const std::wstring &applicationKeyName,
	const std::wstring &menuText);
LSTATUS RemoveAsDefaultFileManagerFileSystem(const std::wstring &applicationKeyName);
LSTATUS RemoveAsDefaultFileManagerAll(const std::wstring &applicationKeyName);

}

// This is needed to be able to use the enum as a key in std::unordered_map.
BETTER_ENUMS_DECLARE_STD_HASH(DefaultFileManager::ReplaceExplorerMode);
