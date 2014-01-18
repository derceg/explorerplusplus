#pragma once

#include <Windows.h>

BOOL	GetClusterSize(const TCHAR *Drive, DWORD *pdwClusterSize);
TCHAR	GetDriveLetterFromMask(ULONG unitmask);