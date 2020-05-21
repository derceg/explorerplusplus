// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

namespace NDefaultFileManager
{
enum class ReplaceExplorerMode
{
	None = 1,
	FileSystem = 2,
	All = 3
};

BOOL SetAsDefaultFileManagerFileSystem(const TCHAR *szInternalCommand, const TCHAR *szMenuText);
BOOL SetAsDefaultFileManagerAll(const TCHAR *szInternalCommand, const TCHAR *szMenuText);
BOOL RemoveAsDefaultFileManagerFileSystem(const TCHAR *szInternalCommand);
BOOL RemoveAsDefaultFileManagerAll(const TCHAR *szInternalCommand);
}