// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>
#include <vector>

namespace FileDialogs
{

struct FileType
{
	std::wstring description;
	std::wstring pattern;
};

HRESULT ShowSelectFolderDialog(HWND owner, const std::wstring &defaultFolder,
	std::wstring &chosenFolderPath);
HRESULT ShowSaveAsDialog(HWND owner, const std::wstring &defaultFolder,
	const std::wstring &defaultFileName, const std::vector<FileType> &fileTypes, UINT fileTypeIndex,
	std::wstring &chosenFilePath);

}
