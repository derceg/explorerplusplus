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
#include "Buffer.h"

BOOL GetDriveGeometryEx(TCHAR *lpszDrive,DISK_GEOMETRY_EX *pDiskGeometry,DWORD dwSize)
{
	HANDLE hDevice;
	DWORD NumBytesReturned;
	BOOL bResult;

	hDevice = CreateFile(lpszDrive,0,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,
	0,NULL);

	if(hDevice == INVALID_HANDLE_VALUE)
		return FALSE;

	bResult = DeviceIoControl(hDevice,IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,NULL,0,
	pDiskGeometry,dwSize,&NumBytesReturned,NULL);

	CloseHandle(hDevice);

	return bResult;
}

BOOL GetDriveLayoutEx(TCHAR *lpszDrive,
DRIVE_LAYOUT_INFORMATION_EX **pDriveLayout,DWORD BufSize)
{
	HANDLE hDevice;
	DWORD NumBytesReturned;
	int TotalAlloc = 0;
	BOOL bResult = 0;

	hDevice = CreateFile(lpszDrive,0,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,
	0,NULL);

	if(hDevice == INVALID_HANDLE_VALUE)
		return FALSE;

	*pDriveLayout = NULL;

	//while(bResult == 0)
	{
		//TotalAlloc += 5;
		TotalAlloc = 2048;
		*pDriveLayout = (DRIVE_LAYOUT_INFORMATION_EX *)malloc(TotalAlloc * sizeof(DRIVE_LAYOUT_INFORMATION_EX));

		bResult = DeviceIoControl(hDevice,IOCTL_DISK_GET_DRIVE_LAYOUT_EX,NULL,0,(LPVOID)*pDriveLayout,
		TotalAlloc * sizeof(DRIVE_LAYOUT_INFORMATION_EX),&NumBytesReturned,NULL);
	}

	CloseHandle(hDevice);

	return bResult;
}

BOOL GetDrivePerformanceInfo(TCHAR *lpszDrive,DISK_PERFORMANCE *pDiskPerformance)
{
	HANDLE hDevice;
	DWORD NumBytesReturned;
	BOOL bResult;

	hDevice = CreateFile(_T("\\\\.\\PhysicalDrive0"),0,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,
	0,NULL);

	if(hDevice == INVALID_HANDLE_VALUE)
		return FALSE;

	bResult = DeviceIoControl(hDevice,IOCTL_DISK_PERFORMANCE,NULL,0,pDiskPerformance,
	sizeof(*pDiskPerformance),&NumBytesReturned,NULL);

	CloseHandle(hDevice);

	return bResult;
}

BOOL GetFileSystemInfo(TCHAR *lpszDrive,FILESYSTEM_STATISTICS *pStatistics,DWORD BufSize)
{
	HANDLE hDevice;
	DWORD NumBytesReturned;
	BOOL bResult;

	hDevice = CreateFile(lpszDrive,FILE_READ_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,
	0,NULL);

	if(hDevice == INVALID_HANDLE_VALUE)
		return FALSE;

	bResult = DeviceIoControl(hDevice,FSCTL_FILESYSTEM_GET_STATISTICS,NULL,0,pStatistics,
	BufSize,&NumBytesReturned,NULL);

	CloseHandle(hDevice);

	return bResult;
}

BOOL GetNtfsVolumeInfo(TCHAR *lpszDrive,NTFS_VOLUME_DATA_BUFFER *pNtfsVolumeInfo,DWORD BufSize)
{
	HANDLE hDevice;
	DWORD NumBytesReturned;
	BOOL bResult;

	/* MUST have FILE_READ_DATA access. */
	hDevice = CreateFile(lpszDrive,FILE_READ_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,
	0,NULL);

	if(hDevice == INVALID_HANDLE_VALUE)
		return FALSE;

	bResult = DeviceIoControl(hDevice,FSCTL_GET_NTFS_VOLUME_DATA,NULL,0,pNtfsVolumeInfo,
	BufSize,&NumBytesReturned,NULL);

	CloseHandle(hDevice);

	return bResult;
}

TCHAR *GetPartitionName(LARGE_INTEGER StartingOffset)
{
	TCHAR	*lpszDrives = NULL;
	TCHAR	*p = NULL;
	DWORD	dwLen;
	BOOL	Break = FALSE;

	dwLen = GetLogicalDriveStrings(0,NULL);

	lpszDrives = (TCHAR *)malloc((dwLen + 1) * sizeof(TCHAR));
	GetLogicalDriveStrings(dwLen,lpszDrives);

	p = lpszDrives;

	while(Break == FALSE)
	{
		/* String list containing drive list is double NULL terminated.
		Detect if the character after the end of the current string is
		another NULL byte. */
		if(*p == '\0')
		{
			/* Break out of the loop at the next iteration (double NULL byte
			set has being found). */
			Break = TRUE;
		}
		else
		{
			HANDLE hDevice;
			DWORD NumBytesReturned;
			TCHAR lpszDriveName[32];

			VOLUME_DISK_EXTENTS *pVolumeDiskExtents;
			DISK_EXTENT *pExtents;

			pVolumeDiskExtents = (VOLUME_DISK_EXTENTS *)malloc(1024);

			StringCchPrintf(lpszDriveName,SIZEOF_ARRAY(lpszDriveName),_T("\\\\.\\%s"),p);

			lpszDriveName[lstrlen(lpszDriveName) - 1] = '\0';

			hDevice = CreateFile(lpszDriveName,FILE_READ_DATA,
			FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);

			DeviceIoControl(hDevice,IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
			NULL,0,pVolumeDiskExtents,1024,&NumBytesReturned,NULL);

			pExtents = pVolumeDiskExtents->Extents;

			if((pExtents->StartingOffset.LowPart == StartingOffset.LowPart) &&
			(pExtents->StartingOffset.HighPart == StartingOffset.HighPart))
				return p;
		}

		p += lstrlen(p) + 1;
	}

	return NULL;
}

LARGE_INTEGER GetDriveLength(TCHAR *lpszDrive)
{
	HANDLE hDevice;
	GET_LENGTH_INFORMATION LengthInfo;
	DWORD NumBytesReturned;

	hDevice = CreateFile(lpszDrive,FILE_READ_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,
	0,NULL);

	DeviceIoControl(hDevice,IOCTL_DISK_GET_LENGTH_INFO,NULL,0,&LengthInfo,
	sizeof(LengthInfo),&NumBytesReturned,NULL);

	CloseHandle(hDevice);

	return LengthInfo.Length;
}

BOOL GetFileAllocationInfo(TCHAR *lpszFileName,
STARTING_VCN_INPUT_BUFFER *pStartingVcn,
RETRIEVAL_POINTERS_BUFFER *pRetrievalPointers,DWORD BufSize)
{
	HANDLE hDevice;
	DWORD NumBytesReturned;
	BOOL bResult;

	hDevice = CreateFile(lpszFileName,FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,
	0,NULL);

	if(hDevice == INVALID_HANDLE_VALUE)
		return FALSE;

	bResult = DeviceIoControl(hDevice,FSCTL_GET_RETRIEVAL_POINTERS,pStartingVcn,sizeof(*pStartingVcn),
	pRetrievalPointers,BufSize,&NumBytesReturned,NULL);
	CloseHandle(hDevice);

	return bResult;
}

LONG GetClusterSize(TCHAR *Drive)
{
	DWORD SectorsPerCluster;
	DWORD BytesPerSector;

	GetDiskFreeSpace(Drive,&SectorsPerCluster,&BytesPerSector,NULL,NULL);

	return BytesPerSector * SectorsPerCluster;
}

LONG GetSectorSize(TCHAR *Drive)
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

int GetNumberOfUsedPartitions(DRIVE_LAYOUT_INFORMATION_EX *pDriveLayout)
{
	PARTITION_INFORMATION_EX *pPartitionInfo;
	PARTITION_INFORMATION_MBR *pInfoMbr;
	int nUsedPartitions = 0;
	unsigned int i = 0;

	for(i = 0;i < pDriveLayout->PartitionCount;i++)
	{
		pPartitionInfo = &pDriveLayout->PartitionEntry[i];
		pInfoMbr = &pPartitionInfo->Mbr;

		if(pInfoMbr->PartitionType != PARTITION_ENTRY_UNUSED &&
			!IsContainerPartition(pInfoMbr->PartitionType))
			nUsedPartitions++;
	}

	return nUsedPartitions;
}