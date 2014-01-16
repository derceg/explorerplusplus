#pragma once

#include <Windows.h>

LONG	GetClusterSize(const TCHAR *Drive);
LONG	GetSectorSize(const TCHAR *Drive);
TCHAR	GetDriveLetterFromMask(ULONG unitmask);
LONG	GetFileSectorSize(const TCHAR *FileName);