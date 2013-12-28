#pragma once

#include <Windows.h>

LONG	GetClusterSize(const TCHAR *Drive);
LONG	GetSectorSize(const TCHAR *Drive);
TCHAR	GetDriveNameFromMask(ULONG unitmask);
LONG	GetFileSectorSize(const TCHAR *FileName);