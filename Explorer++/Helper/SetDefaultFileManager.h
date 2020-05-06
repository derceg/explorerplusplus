// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

namespace NDefaultFileManager
{
enum ReplaceExplorerModes_t
{
	REPLACEEXPLORER_NONE = 1,
	REPLACEEXPLORER_FILESYSTEM = 2,
	REPLACEEXPLORER_ALL = 3
};

BOOL SetAsDefaultFileManagerFileSystem(const TCHAR *szInternalCommand, const TCHAR *szMenuText);
BOOL SetAsDefaultFileManagerAll(const TCHAR *szInternalCommand, const TCHAR *szMenuText);
BOOL RemoveAsDefaultFileManagerFileSystem(const TCHAR *szInternalCommand);
BOOL RemoveAsDefaultFileManagerAll(const TCHAR *szInternalCommand);
}