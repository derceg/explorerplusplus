// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FolderSize.h"
#include "Macros.h"
#include <list>

HRESULT CalculateFolderSize(
	const TCHAR *szPath, int *nFolders, int *nFiles, PULARGE_INTEGER lTotalFolderSize)
{
	HANDLE hFirstFile;
	WIN32_FIND_DATA wfd;
	TCHAR initialPath[MAX_PATH + 2];
	TCHAR tempPath[MAX_PATH + 2];
	ULARGE_INTEGER l_TotalFolderSize;
	ULARGE_INTEGER r_TotalFolderSize;
	ULARGE_INTEGER lFileSize;
	int l_NumFiles = 0;
	int l_NumFolders = 0;
	int r_NumFiles = 0;
	int r_NumFolders = 0;

	if (!szPath || !nFolders || !nFiles || !lTotalFolderSize)
	{
		return E_INVALIDARG;
	}

	l_TotalFolderSize.QuadPart = 0;
	r_TotalFolderSize.QuadPart = 0;

	StringCchCopy(initialPath, SIZEOF_ARRAY(initialPath), szPath);
	StringCchCat(initialPath, SIZEOF_ARRAY(initialPath), _T("\\*"));

	hFirstFile = FindFirstFile(initialPath, &wfd);

	if (hFirstFile == INVALID_HANDLE_VALUE)
	{
		return S_OK;
	}

	if (StrCmp(wfd.cFileName, _T(".")) != 0)
	{
		if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		{
			l_NumFolders++;
		}
		else
		{
			l_NumFiles++;
		}
	}

	while (FindNextFile(hFirstFile, &wfd) != 0)
	{
		if (StrCmp(wfd.cFileName, _T("..")) != 0)
		{
			if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
			{
				l_NumFolders++;

				StringCchCopy(tempPath, SIZEOF_ARRAY(tempPath), szPath);
				PathAppend(tempPath, wfd.cFileName);

				CalculateFolderSize(tempPath, &r_NumFolders, &r_NumFiles, &r_TotalFolderSize);

				l_NumFolders += r_NumFolders;
				l_NumFiles += r_NumFiles;
				l_TotalFolderSize.QuadPart += r_TotalFolderSize.QuadPart;
			}
			else
			{
				l_NumFiles++;
				lFileSize.LowPart = wfd.nFileSizeLow;
				lFileSize.HighPart = wfd.nFileSizeHigh;

				l_TotalFolderSize.QuadPart += lFileSize.QuadPart;
			}
		}
	}

	*nFolders = l_NumFolders;
	*nFiles = l_NumFiles;
	lTotalFolderSize->QuadPart = l_TotalFolderSize.QuadPart;

	FindClose(hFirstFile);

	return S_OK;
}

DWORD WINAPI Thread_CalculateFolderSize(LPVOID lpParameter)
{
	FolderSize_t *pFolderSize = nullptr;
	static int nFiles;
	static int nFolders;
	ULARGE_INTEGER lTotalDirSize;
	HRESULT hr;

	if (lpParameter == nullptr)
	{
		return 0;
	}

	pFolderSize = (FolderSize_t *) lpParameter;

	hr = CalculateFolderSize(pFolderSize->szPath, &nFolders, &nFiles, &lTotalDirSize);

	pFolderSize->pfnCallback(nFolders, nFiles, &lTotalDirSize, pFolderSize->pData);

	free(pFolderSize);

	return 1;
}