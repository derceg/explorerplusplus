// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <windows.h>
#include <optional>

typedef void (*OnDirectoryAltered)(const TCHAR *szFileName, DWORD dwAction, void *pData);

/* Main exported interface. */
__interface IDirectoryMonitor : IUnknown
{
	std::optional<int> WatchDirectory(const TCHAR *Directory, UINT WatchFlags,
		OnDirectoryAltered onDirectoryAltered, BOOL bWatchSubTree, void *pData);
	std::optional<int> WatchDirectory(HANDLE hDirectory, const TCHAR *Directory, UINT WatchFlags,
		OnDirectoryAltered onDirectoryAltered, BOOL bWatchSubTree, void *pData);
	BOOL StopDirectoryMonitor(int iStopIndex);
};

HRESULT CreateDirectoryMonitor(IDirectoryMonitor **pDirectoryMonitor);