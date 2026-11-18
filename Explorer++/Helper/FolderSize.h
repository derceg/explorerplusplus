// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct FolderInfo
{
	std::uintmax_t size;
	int numFolders;
	int numFiles;
};

FolderInfo GetFolderInfo(const std::wstring &path);
