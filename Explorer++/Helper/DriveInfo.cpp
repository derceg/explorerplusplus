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

LONG GetSectorSize(const TCHAR *Drive)
{
	DWORD BytesPerSector;

	GetDiskFreeSpace(Drive,NULL,&BytesPerSector,NULL,NULL);

	return BytesPerSector;
}

TCHAR GetDriveNameFromMask(ULONG unitmask)
{
	int BitNum = 0;

	while(!(unitmask & 0x1))
	{
		unitmask >>= 1;
		BitNum++;
	}

	return (TCHAR)BitNum + 'A';
}

LONG GetFileSectorSize(const TCHAR *FileName)
{
	LONG SectorSize;
	LONG FileSize;
	LONG SectorFileSize;
	HANDLE hFile;
	int SectorCount = 0;
	TCHAR Root[MAX_PATH];

	if(FileName == NULL)
		return -1;

	/* Get a handle to the file. */
	hFile = CreateFile(FileName, GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return -1;

	/* Get the files size (count of number of actual
	number of bytes in file). */
	FileSize = GetFileSize(hFile, NULL);

	StringCchCopy(Root, SIZEOF_ARRAY(Root), FileName);
	PathStripToRoot(Root);

	/* Get the sector size of the drive the file resides on. */
	SectorSize = GetSectorSize(Root);

	SectorFileSize = 0;
	while(SectorFileSize < FileSize)
	{
		SectorFileSize += SectorSize;
		SectorCount++;
	}

	CloseHandle(hFile);

	return SectorCount;
}