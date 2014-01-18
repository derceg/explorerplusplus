#pragma once

#include <Windows.h>

LONG	GetClusterSize(const TCHAR *Drive);
TCHAR	GetDriveLetterFromMask(ULONG unitmask);