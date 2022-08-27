// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DriveEnumeratorImpl.h"
#include <windows.h>
#include <memory>

// DriveEnumerator
outcome::std_result<std::set<std::wstring>> DriveEnumeratorImpl::GetDrives()
{
	DWORD size = GetLogicalDriveStrings(0, nullptr);

	if (size == 0)
	{
		return { GetLastError(), std::system_category() };
	}

	auto driveStrings = std::make_unique<TCHAR[]>(size);
	size = GetLogicalDriveStrings(size, driveStrings.get());

	if (size == 0)
	{
		return { GetLastError(), std::system_category() };
	}

	std::set<std::wstring> drives;
	TCHAR *currentDrive = driveStrings.get();

	while (*currentDrive != '\0')
	{
		drives.emplace(currentDrive);

		currentDrive += (lstrlen(currentDrive) + 1);
	}

	return drives;
}
