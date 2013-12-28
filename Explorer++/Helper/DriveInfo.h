#pragma once

#include <Windows.h>

LONG	GetClusterSize(TCHAR *Drive);
LONG	GetSectorSize(TCHAR *Drive);
TCHAR	GetDriveNameFromMask(ULONG unitmask);
LONG	GetFileSectorSize(TCHAR *FileName);