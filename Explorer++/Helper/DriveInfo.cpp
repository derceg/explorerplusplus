/******************************************************************
 *
 * Project: Helper
 * File: DriveInfo.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides a set of drive information functions.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Helper.h"
#include "FileOperations.h"
#include "Macros.h"


LONG GetClusterSize(const TCHAR *Drive)
{
	DWORD SectorsPerCluster;
	DWORD BytesPerSector;

	GetDiskFreeSpace(Drive,&SectorsPerCluster,&BytesPerSector,NULL,NULL);

	return BytesPerSector * SectorsPerCluster;
}

TCHAR GetDriveLetterFromMask(ULONG unitmask)
{
	int BitNum = 0;

	while(!(unitmask & 0x1))
	{
		unitmask >>= 1;
		BitNum++;
	}

	return (TCHAR)BitNum + 'A';
}