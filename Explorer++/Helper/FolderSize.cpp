// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FolderSize.h"
#include <filesystem>

FolderInfo GetFolderInfo(const std::wstring &path)
{
	FolderInfo folderInfo = {};
	std::error_code error;

	for (const auto &entry : std::filesystem::directory_iterator(path, error))
	{
		if (std::filesystem::is_directory(entry.status()))
		{
			folderInfo.numFolders++;

			FolderInfo subFolderInfo = GetFolderInfo(entry.path());

			folderInfo.size += subFolderInfo.size;
			folderInfo.numFolders += subFolderInfo.numFolders;
			folderInfo.numFiles += subFolderInfo.numFiles;
		}
		else
		{
			std::error_code sizeErrorCode;
			const auto size = std::filesystem::file_size(entry.path(), sizeErrorCode);

			// If the size can't be retrieved, the error will be ignored and the current file will
			// effectively be skipped over.
			if (!sizeErrorCode)
			{
				folderInfo.size += size;
				folderInfo.numFiles++;
			}
		}
	}

	return folderInfo;
}

DWORD WINAPI Thread_CalculateFolderSize(LPVOID lpParameter)
{
	FolderSize_t *pFolderSize = reinterpret_cast<FolderSize_t *>(lpParameter);

	auto folderInfo = GetFolderInfo(pFolderSize->szPath);

	ULARGE_INTEGER size;
	size.QuadPart = folderInfo.size;

	pFolderSize->pfnCallback(folderInfo.numFolders, folderInfo.numFiles, &size, pFolderSize->pData);

	free(pFolderSize);

	return 1;
}