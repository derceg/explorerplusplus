/******************************************************************
 *
 * Project: Explorer++
 * File: PhysicalDriveProperties.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the 'Show Drive Information' dialog.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "Misc.h"
#include "Explorer++.h"
#include "../Helper/FileOperations.h"
#include "../Helper/Helper.h"
#include "../Helper/Controls.h"
#include "../Helper/Bookmark.h"
#include "MainResource.h"


INT_PTR CALLBACK PhysicalDrivePropertiesProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static CContainer *pContainer = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pContainer = (CContainer *)lParam;
		}
		break;
	}

	return pContainer->PhysicalDrivePropertiesProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK CContainer::PhysicalDrivePropertiesProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch(Msg)
	{
		case WM_INITDIALOG:
			OnDrivePropertiesInit(hDlg);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDSAVE:
					OnDrivePropertiesSave(hDlg);
					break;

				case IDCLOSE:
					DrivePropertiesSaveState(hDlg);
					EndDialog(hDlg,1);
					break;
			}
			break;

		case WM_CLOSE:
			DrivePropertiesSaveState(hDlg);
			EndDialog(hDlg,0);
			break;
	}

	return 0;
}

void CContainer::OnDrivePropertiesInit(HWND hDlg)
{
	HWND hRichEdit;
	CHARRANGE cr;

	hRichEdit = GetDlgItem(hDlg,IDC_RICHEDIT_DRIVEINFO);

	FlushDriveInfoToRichEdit(hRichEdit,_T("\\\\.\\PhysicalDrive0"));

	if(m_bDrivePropertiesDlgStateSaved)
	{
		SetWindowPos(hDlg,NULL,m_ptDriveProperties.x,
			m_ptDriveProperties.y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
	else
	{
		CenterWindow(m_hContainer,hDlg);
	}

	SetFocus(hRichEdit);

	/* Deselect all text in the richedit control. */
	cr.cpMin = 0;
	cr.cpMax = 0;
	SendMessage(hRichEdit,EM_EXSETSEL,0,(LPARAM)&cr);
}

void CContainer::OnDrivePropertiesSave(HWND hDlg)
{
	HWND hRichEdit;
	TCHAR FullFileName[MAX_PATH] = EMPTY_STRING;
	DWORD NumBytesWritten;
	BOOL bSaveNameRetrieved;

	hRichEdit = GetDlgItem(hDlg,IDC_RICHEDIT_DRIVEINFO);

	bSaveNameRetrieved = GetFileNameFromUser(hDlg,FullFileName,NULL);	

	if(bSaveNameRetrieved)
	{
		HANDLE hFile;

		hFile = CreateFile(FullFileName,FILE_WRITE_DATA,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

		if(hFile != INVALID_HANDLE_VALUE)
		{
			TCHAR *lpszDriveInfo;
			GETTEXTLENGTHEX GetTextLengthEx;
			GETTEXTEX GetTextEx;
			UINT BufSize;
			UINT nCharsCopied;

			hRichEdit = GetDlgItem(hDlg,IDC_RICHEDIT_DRIVEINFO);

			GetTextLengthEx.flags = GTL_DEFAULT;
			BufSize = (UINT)SendMessage(hRichEdit,EM_GETTEXTLENGTHEX,(WPARAM)&GetTextLengthEx,0);

			if(BufSize > 0)
			{
				lpszDriveInfo = (TCHAR *)malloc((BufSize + 1) * sizeof(TCHAR));

				if(lpszDriveInfo != NULL)
				{
					GetTextEx.cb			= BufSize * sizeof(TCHAR);
					GetTextEx.flags			= GT_DEFAULT;
					GetTextEx.codepage		= CP_ACP;
					GetTextEx.lpDefaultChar	= NULL;
					GetTextEx.lpUsedDefChar	= NULL;
					nCharsCopied = (UINT)SendMessage(hRichEdit,EM_GETTEXTEX,(WPARAM)&GetTextEx,(LPARAM)lpszDriveInfo);

					WriteFile(hFile,lpszDriveInfo,nCharsCopied * sizeof(char),&NumBytesWritten,NULL);

					free(lpszDriveInfo);
				}
			}

			CloseHandle(hFile);
		}
	}
}

/*
Whole drive:
 - Drive capacity
 - Number of cylinders
 - Tracks per cylinder
 - Sectors per track
 - Bytes per sector
*/
void CContainer::FlushDriveInfoToRichEdit(HWND hRichEdit,TCHAR *lpszDrive)
{
	DISK_GEOMETRY_EX			*pDiskGeometryEx = NULL;
	DISK_GEOMETRY				*pDiskGeometry = NULL;
	DRIVE_LAYOUT_INFORMATION_EX	*pDriveLayout = NULL;
	PARTITION_INFORMATION_EX	*pPartitionInfo = NULL;
	NTFS_VOLUME_DATA_BUFFER		*pNtfsVolumeInfo = NULL;
	LARGE_INTEGER				DriveSize;
	TCHAR						PartitionName[512];
	TCHAR						PrettySize[512];
	BOOL						bResult;
	int							nPartitions;
	int							i = 0;

	pDiskGeometryEx = (DISK_GEOMETRY_EX *)malloc(1024);
	GetDriveGeometryEx(lpszDrive,pDiskGeometryEx,1024);

	DISK_PARTITION_INFO *pdpi;

	pdpi = DiskGeometryGetPartition(pDiskGeometryEx);

	pDiskGeometry = &pDiskGeometryEx->Geometry;

	WriteTextToRichEdit(hRichEdit,_T("Hard Disk %d\t\n"),0);

	DriveSize = GetDriveLength(lpszDrive);
	FormatSizeString(DriveSize.LowPart,DriveSize.HighPart,PrettySize,SIZEOF_ARRAY(PrettySize),FALSE);
	WriteTextToRichEdit(hRichEdit,_T("Drive capacity:\t\t%s bytes (%s)\n\n"),PrintCommaLargeNum(DriveSize),PrettySize);

	WriteTextToRichEdit(hRichEdit,_T("Number of cylinders:\t%s\n"),PrintCommaLargeNum(pDiskGeometry->Cylinders));
	WriteTextToRichEdit(hRichEdit,_T("Tracks per cylinder:\t\t%s\n"),PrintComma(pDiskGeometry->TracksPerCylinder));
	WriteTextToRichEdit(hRichEdit,_T("Sectors per track:\t\t%s\n"),PrintComma(pDiskGeometry->SectorsPerTrack));
	WriteTextToRichEdit(hRichEdit,_T("Bytes per sector:\t\t%s\n"),PrintComma(pDiskGeometry->BytesPerSector));
	WriteTextToRichEdit(hRichEdit,_T("\n"));
	
	DWORD dwLayoutSize;
	dwLayoutSize = 2048;

	pDriveLayout = (DRIVE_LAYOUT_INFORMATION_EX *)malloc(dwLayoutSize);
	GetDriveLayoutEx(lpszDrive,&pDriveLayout,dwLayoutSize);

	nPartitions = GetNumberOfUsedPartitions(pDriveLayout);
	WriteTextToRichEdit(hRichEdit,_T("Number of partitions:\t%s\n\n"),PrintComma(nPartitions));

	/* Only show information on real partitions. Extended/container
	partitions will simply be shown as parents of other partitions. */
	for(i = 0;(unsigned int)i < pDriveLayout->PartitionCount;i++)
	{
		pPartitionInfo = &pDriveLayout->PartitionEntry[i];

		if(pPartitionInfo->Mbr.PartitionType != PARTITION_ENTRY_UNUSED)
		{
			/* TODO: Potentially not enough space for partition name. */
			StringCchPrintf(PartitionName,SIZEOF_ARRAY(PartitionName),
				_T("\\\\.\\%s"),GetPartitionName(pPartitionInfo->StartingOffset));
			PartitionName[lstrlen(PartitionName) - 1] = '\0';

			if(pPartitionInfo->PartitionNumber != 0)
			{
				WriteTextToRichEdit(hRichEdit,_T("Partition %s\n"),PrintComma(pPartitionInfo->PartitionNumber));
			}
			else
			{
				/* Partitions are 1-based, so add one to i. */
				//WriteTextToRichEdit(hRichEdit,_T("Partition %s\n"),PrintComma(i + 1));
			}

			pNtfsVolumeInfo = (NTFS_VOLUME_DATA_BUFFER *)malloc(512);

			/* Serial numbers are different for each drive (and each partition).
			Therefore, this function needs a drive path, not a physical hard
			drive path. */
			bResult = GetNtfsVolumeInfo(PartitionName,pNtfsVolumeInfo,512);

			if(bResult)
				WriteTextToRichEdit(hRichEdit,_T("Volume serial number:\t%X\n"),pNtfsVolumeInfo->VolumeSerialNumber);
			else
				WriteTextToRichEdit(hRichEdit,_T("Volume serial number:\tUnknown\n"));

			WriteTextToRichEdit(hRichEdit,_T("Offset:\t\t\t%s\n"),
				PrintCommaLargeNum(pPartitionInfo->StartingOffset));

			FormatSizeString(pPartitionInfo->PartitionLength.LowPart,pPartitionInfo->PartitionLength.HighPart,
				PrettySize,SIZEOF_ARRAY(PrettySize),FALSE);
			WriteTextToRichEdit(hRichEdit,_T("Partition capacity:\t\t%s bytes (%s)\n"),
				PrintCommaLargeNum(pPartitionInfo->PartitionLength),PrettySize);

			FILESYSTEM_STATISTICS *pStatistics;
			NTFS_STATISTICS *pNtfsStatistics;
			LPBYTE ptr;

			pStatistics = (FILESYSTEM_STATISTICS *)malloc(1024);

			GetFileSystemInfo(PartitionName,pStatistics,1024);
			GetNtfsVolumeInfo(PartitionName,pNtfsVolumeInfo,512);

			WriteTextToRichEdit(hRichEdit,_T("File system:\t\t"));
			if(pStatistics->FileSystemType == FILESYSTEM_STATISTICS_TYPE_NTFS)
				WriteTextToRichEdit(hRichEdit,_T("NTFS\n"));
			else if(pStatistics->FileSystemType == FILESYSTEM_STATISTICS_TYPE_FAT)
				WriteTextToRichEdit(hRichEdit,_T("FAT\n"));
			else
				WriteTextToRichEdit(hRichEdit,_T("Unknown\n"));

			ptr = (LPBYTE)pStatistics + sizeof(FILESYSTEM_STATISTICS);
			pNtfsStatistics = (NTFS_STATISTICS *)(LPBYTE)ptr;
			//WriteTextToRichEdit(hRichEdit,_T("Size Of MFT(bytes):\t%ld\n"),pNtfsStatistics->MftWriteBytes);
			//WriteTextToRichEdit(hRichEdit,_T("Size Of Bitmap(bytes):\t%ld\n"),pNtfsStatistics->BitmapWriteBytes);
			//WriteTextToRichEdit(hRichEdit,_T("Number of clusters allocated:\t%ld\n"),pNtfsStatistics->Allocate.Clusters);

			if(pStatistics->FileSystemType == FILESYSTEM_STATISTICS_TYPE_NTFS ||
				pStatistics->FileSystemType == FILESYSTEM_STATISTICS_TYPE_FAT)
			{
				WriteTextToRichEdit(hRichEdit,_T("Reserved Clusters:\t\t%s\n"),PrintCommaLargeNum(pNtfsVolumeInfo->TotalReserved));
				WriteTextToRichEdit(hRichEdit,_T("MFT Starting Cluster:\t%s\n"),PrintCommaLargeNum(pNtfsVolumeInfo->MftStartLcn));

				FormatSizeString(pNtfsVolumeInfo->MftValidDataLength.LowPart,pNtfsVolumeInfo->MftValidDataLength.HighPart,
					PrettySize,SIZEOF_ARRAY(PrettySize),FALSE);
				WriteTextToRichEdit(hRichEdit,_T("Size Of MFT:\t\t%s bytes (%s)\n"),
					PrintCommaLargeNum(pNtfsVolumeInfo->MftValidDataLength),PrettySize);

				WriteTextToRichEdit(hRichEdit,_T("Bytes Per Cluster:\t\t%s\n"),PrintComma(pNtfsVolumeInfo->BytesPerCluster));
			}

			WriteTextToRichEdit(hRichEdit,_T("\n"));
		}
	}

	//WriteTextToRichEdit(hRichEdit,_T("\n"));
}

void CContainer::DrivePropertiesSaveState(HWND hDlg)
{
	RECT rcTemp;

	GetWindowRect(hDlg,&rcTemp);
	m_ptDriveProperties.x = rcTemp.left;
	m_ptDriveProperties.y = rcTemp.top;

	m_bDrivePropertiesDlgStateSaved = TRUE;
}