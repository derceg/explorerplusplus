/******************************************************************
 *
 * Project: ShellBrowser
 * File: SortManager.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the sorting of items
 * within the listview.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include <gdiplus.h>
#include "IShellView.h"
#include "iShellBrowser_internal.h"
#include "../Helper/Controls.h"
#include "../Helper/Helper.h"
#include "../Helper/FileOperations.h"
#include "../Helper/FolderSize.h"


#define DATE_CREATED			0
#define DATE_MODIFIED			1
#define DATE_ACCESSED			2

#define VERSION_PRODUCTNAME		0
#define VERSION_COMPANY			1
#define VERSION_DESCRIPTION		2
#define VERSION_FILEVERSION		3
#define VERSION_PRODUCTVERSION	4

int CALLBACK SortStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);

int CALLBACK SortByNameStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;
	
	return pFolderView->SortByName(lParam1,lParam2);
}

/*
Sorting order:
Drives
Folders
Normal items
*/
int CALLBACK CFolderView::SortByName(LPARAM lParam1,LPARAM lParam2)
{
	int ReturnValue;

	if(m_bVirtualFolder)
	{
		LPITEMIDLIST	pidlComplete = NULL;
		LPITEMIDLIST	pidlRelative = NULL;
		IShellFolder	*pShellFolder = NULL;
		STRRET			str;
		TCHAR			szItem1[MAX_PATH];
		TCHAR			szItem2[MAX_PATH];
		BOOL			bRoot1;
		BOOL			bRoot2;

		pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[lParam1].pridl);
		SHBindToParent(pidlComplete,IID_IShellFolder,
		(void **)&pShellFolder,(LPCITEMIDLIST *)&pidlRelative);

		pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
		StrRetToBuf(&str,pidlRelative,szItem1,SIZEOF_ARRAY(szItem1));

		CoTaskMemFree(pidlComplete);
		pShellFolder->Release();
		pShellFolder = NULL;

		pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[lParam2].pridl);
		SHBindToParent(pidlComplete,IID_IShellFolder,
		(void **)&pShellFolder,(LPCITEMIDLIST *)&pidlRelative);

		pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
		StrRetToBuf(&str,pidlRelative,szItem2,SIZEOF_ARRAY(szItem2));

		CoTaskMemFree(pidlComplete);
		pShellFolder->Release();
		pShellFolder = NULL;

		bRoot1 = PathIsRoot(szItem1);
		bRoot2 = PathIsRoot(szItem2);

		if(bRoot1 && !bRoot2)
			return -1;
		else if(!bRoot1 && bRoot2)
			return 1;
		else if(bRoot1 && bRoot2)
		{
			/* Sort by drive letter. */
			PathStripToRoot(szItem1);
			PathStripToRoot(szItem2);

			return lstrcmp(szItem1,szItem2);
		}
	}

	if(!CompareVirtualFolders(CSIDL_BITBUCKET))
	{
		if(((m_pwfdFiles[lParam1].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)&&
			((m_pwfdFiles[lParam2].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY))
		{
			return -1;
		}

		if(((m_pwfdFiles[lParam1].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)&&
			((m_pwfdFiles[lParam2].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
		{
			return 1;
		}
	}

	ReturnValue = StrCmpLogicalW(m_pExtraItemInfo[lParam1].szDisplayName,m_pExtraItemInfo[lParam2].szDisplayName);

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortBySizeStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortBySize(lParam1,lParam2);
}

int CALLBACK CFolderView::SortBySize(LPARAM lParam1,LPARAM lParam2)
{
	WIN32_FIND_DATA	*File1 = NULL;
	WIN32_FIND_DATA	*File2 = NULL;
	BOOL			IsFolder1;
	BOOL			IsFolder2;
	int				ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		if(m_pExtraItemInfo[(int)lParam1].bFolderSizeRetrieved
			&& !m_pExtraItemInfo[(int)lParam2].bFolderSizeRetrieved)
			ReturnValue = -1;
		else if(!m_pExtraItemInfo[(int)lParam1].bFolderSizeRetrieved
			&& m_pExtraItemInfo[(int)lParam2].bFolderSizeRetrieved)
			ReturnValue = 1;
		else
		{
			double FileSize1 = File1->nFileSizeLow + (File1->nFileSizeHigh * pow(2.0,32.0));
			double FileSize2 = File2->nFileSizeLow + (File2->nFileSizeHigh * pow(2.0,32.0));

			double fRes;
			fRes = FileSize1 - FileSize2;

			if(fRes > 0)
				ReturnValue = 1;
			else if(fRes < 0)
				ReturnValue = -1;
			else
				ReturnValue = 0;

			if(!IsSortAscending())
				ReturnValue = -ReturnValue;
		}

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}

	double FileSize1 = File1->nFileSizeLow + (File1->nFileSizeHigh * pow(2.0,32.0));
	double FileSize2 = File2->nFileSizeLow + (File2->nFileSizeHigh * pow(2.0,32.0));

	double fRes;
	fRes = FileSize1 - FileSize2;

	if(fRes > 0)
		ReturnValue = 1;
	else if(fRes < 0)
		ReturnValue = -1;
	else
		ReturnValue = 0;

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByTypeStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByType(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByType(LPARAM lParam1,LPARAM lParam2)
{
	IShellFolder	*pShellFolder1 = NULL;
	IShellFolder	*pShellFolder2 = NULL;
	LPITEMIDLIST	pidlComplete1 = NULL;
	LPITEMIDLIST	pidlComplete2 = NULL;
	LPITEMIDLIST	pidlRelative1 = NULL;
	LPITEMIDLIST	pidlRelative2 = NULL;
	WIN32_FIND_DATA	*File1 = NULL;
	WIN32_FIND_DATA	*File2 = NULL;
	SHFILEINFO		shfi1;
	SHFILEINFO		shfi2;
	STRRET			str;
	TCHAR			szItem1[MAX_PATH];
	TCHAR			szItem2[MAX_PATH];
	BOOL			IsFolder1;
	BOOL			IsFolder2;
	BOOL			bRoot1;
	BOOL			bRoot2;
	int				ReturnValue;	

	if(m_bVirtualFolder)
	{
		LPITEMIDLIST	pidlComplete = NULL;
		IShellFolder	*pShellFolder = NULL;
		LPITEMIDLIST	pidlRelative = NULL;
		STRRET			str;
		TCHAR			szItem1[MAX_PATH];
		TCHAR			szItem2[MAX_PATH];
		BOOL			bRoot1;
		BOOL			bRoot2;

		pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[lParam1].pridl);
		SHBindToParent(pidlComplete,IID_IShellFolder,
		(void **)&pShellFolder,(LPCITEMIDLIST *)&pidlRelative);

		pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
		StrRetToBuf(&str,pidlRelative,szItem1,SIZEOF_ARRAY(szItem1));

		SHGetFileInfo((LPTSTR)pidlComplete,0,&shfi1,sizeof(shfi1),SHGFI_PIDL|SHGFI_TYPENAME);

		CoTaskMemFree(pidlComplete);
		pShellFolder->Release();
		pShellFolder = NULL;

		pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[lParam2].pridl);
		SHBindToParent(pidlComplete,IID_IShellFolder,
		(void **)&pShellFolder,(LPCITEMIDLIST *)&pidlRelative);

		pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
		StrRetToBuf(&str,pidlRelative,szItem2,SIZEOF_ARRAY(szItem2));

		SHGetFileInfo((LPTSTR)pidlComplete,0,&shfi2,sizeof(shfi2),SHGFI_PIDL|SHGFI_TYPENAME);

		CoTaskMemFree(pidlComplete);
		pShellFolder->Release();
		pShellFolder = NULL;

		bRoot1 = PathIsRoot(szItem1);
		bRoot2 = PathIsRoot(szItem2);

		if(bRoot1 && !bRoot2)
			return -1;
		else if(!bRoot1 && bRoot2)
			return 1;
		else if(bRoot1 && bRoot2)
			return lstrcmp(shfi1.szTypeName,shfi2.szTypeName);
	}

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;

	if(IsFolder1 && !IsFolder2)
	{
		return -1;
	}

	if(IsFolder2 && !IsFolder1)
	{
		return 1;
	}

	pidlComplete1 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[lParam1].pridl);
	pidlComplete2 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[lParam2].pridl);

	SHGetFileInfo((LPTSTR)pidlComplete1,0,&shfi1,sizeof(shfi1),SHGFI_PIDL|SHGFI_TYPENAME);
	SHGetFileInfo((LPTSTR)pidlComplete2,0,&shfi2,sizeof(shfi2),SHGFI_PIDL|SHGFI_TYPENAME);

	SHBindToParent(pidlComplete1,IID_IShellFolder,
	(void **)&pShellFolder1,(LPCITEMIDLIST *)&pidlRelative1);

	pShellFolder1->GetDisplayNameOf(pidlRelative1,SHGDN_FORPARSING,&str);
	StrRetToBuf(&str,pidlRelative1,szItem1,SIZEOF_ARRAY(szItem1));

	SHBindToParent(pidlComplete2,IID_IShellFolder,
	(void **)&pShellFolder2,(LPCITEMIDLIST *)&pidlRelative2);

	pShellFolder2->GetDisplayNameOf(pidlRelative2,SHGDN_FORPARSING,&str);
	StrRetToBuf(&str,pidlRelative2,szItem2,SIZEOF_ARRAY(szItem2));

	bRoot1 = PathIsRoot(szItem1);
	bRoot2 = PathIsRoot(szItem2);

	pShellFolder2->Release();
	pShellFolder1->Release();
	CoTaskMemFree(pidlComplete2);
	CoTaskMemFree(pidlComplete1);

	ReturnValue = lstrcmp(shfi1.szTypeName,shfi2.szTypeName);

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByDateModifiedStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByDateModified(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByDateModified(LPARAM lParam1,LPARAM lParam2)
{
	return SortByDate(lParam1,lParam2,DATE_MODIFIED);
}

int CALLBACK SortByDateDeletedStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByDateDeleted(lParam1,lParam2);
}

/* TODO: Implement. */
int CFolderView::SortByDateDeleted(LPARAM lParam1,LPARAM lParam2)
{
	int			ReturnValue;

	ReturnValue = 0;

	if(!IsSortAscending())
		ReturnValue = -ReturnValue;

	return ReturnValue;
}

/*
DateType:
0 - Created.
1 - Modified.
2 - Accessed;
*/
int CFolderView::SortByDate(LPARAM lParam1,LPARAM lParam2,int DateType)
{
	BOOL	IsFolder1;
	BOOL	IsFolder2;
	int		ReturnValue = 0;

	IsFolder1 = (m_pwfdFiles[lParam1].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (m_pwfdFiles[lParam2].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		switch(DateType)
		{
			case DATE_CREATED:
				ReturnValue = CompareFileTime(&m_pwfdFiles[lParam1].ftCreationTime,&m_pwfdFiles[lParam2].ftCreationTime);
				break;

			case DATE_MODIFIED:
				ReturnValue = CompareFileTime(&m_pwfdFiles[lParam1].ftLastWriteTime,&m_pwfdFiles[lParam2].ftLastWriteTime);
				break;

			case DATE_ACCESSED:
				ReturnValue = CompareFileTime(&m_pwfdFiles[lParam1].ftLastAccessTime,&m_pwfdFiles[lParam2].ftLastAccessTime);
				break;

			default:
				ReturnValue = 0;
				break;
		}

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		return -1;
	}

	if(IsFolder2)
	{
		return 1;
	}

	switch(DateType)
	{
		case 0:
			ReturnValue = CompareFileTime(&m_pwfdFiles[lParam1].ftCreationTime,&m_pwfdFiles[lParam2].ftCreationTime);
			break;

		case 1:
			ReturnValue = CompareFileTime(&m_pwfdFiles[lParam1].ftLastWriteTime,&m_pwfdFiles[lParam2].ftLastWriteTime);
			break;

		case 2:
			ReturnValue = CompareFileTime(&m_pwfdFiles[lParam1].ftLastAccessTime,&m_pwfdFiles[lParam2].ftLastAccessTime);
			break;
	}

	if(!IsSortAscending())
		ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByTotalSizeStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByTotalSize(lParam1,lParam2,TRUE);
}

int CALLBACK CFolderView::SortByTotalSize(LPARAM lParam1,LPARAM lParam2,BOOL bTotalSize)
{
	IShellFolder	*pShellFolder1 = NULL;
	IShellFolder	*pShellFolder2 = NULL;
	LPITEMIDLIST	pidlComplete1 = NULL;
	LPITEMIDLIST	pidlComplete2 = NULL;
	LPITEMIDLIST	pidlRelative1 = NULL;
	LPITEMIDLIST	pidlRelative2 = NULL;
	STRRET			str;
	TCHAR			szItem1[MAX_PATH];
	TCHAR			szItem2[MAX_PATH];
	ULARGE_INTEGER	nTotalBytes1;
	ULARGE_INTEGER	nTotalBytes2;
	ULARGE_INTEGER	nFreeBytes1;
	ULARGE_INTEGER	nFreeBytes2;
	BOOL			bRoot1;
	BOOL			bRoot2;
	int				ReturnValue;

	pidlComplete1 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[lParam1].pridl);
	SHBindToParent(pidlComplete1,IID_IShellFolder,
	(void **)&pShellFolder1,(LPCITEMIDLIST *)&pidlRelative1);

	pShellFolder1->GetDisplayNameOf(pidlRelative1,SHGDN_FORPARSING,&str);
	StrRetToBuf(&str,pidlRelative1,szItem1,SIZEOF_ARRAY(szItem1));

	pidlComplete2 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[lParam2].pridl);
	SHBindToParent(pidlComplete2,IID_IShellFolder,
	(void **)&pShellFolder2,(LPCITEMIDLIST *)&pidlRelative2);

	pShellFolder2->GetDisplayNameOf(pidlRelative2,SHGDN_FORPARSING,&str);
	StrRetToBuf(&str,pidlRelative2,szItem2,SIZEOF_ARRAY(szItem2));

	bRoot1 = PathIsRoot(szItem1);
	bRoot2 = PathIsRoot(szItem2);

	if(bRoot1 && !bRoot2)
		return -1;
	else if(!bRoot1 && bRoot2)
		return 1;
	else if(!bRoot1 && !bRoot2)
		return 0;

	GetDiskFreeSpaceEx(szItem1,NULL,&nTotalBytes1,&nFreeBytes1);

	GetDiskFreeSpaceEx(szItem2,NULL,&nTotalBytes2,&nFreeBytes2);

	CoTaskMemFree(pidlComplete2);
	CoTaskMemFree(pidlComplete1);
	pShellFolder2->Release();
	pShellFolder1->Release();

	if(!bTotalSize)
	{
		if(nFreeBytes1.QuadPart == nFreeBytes2.QuadPart)
			ReturnValue = 0;
		else
			ReturnValue = nFreeBytes1.QuadPart > nFreeBytes2.QuadPart ? 1 : -1;
	}
	else
	{
		if(nTotalBytes1.QuadPart == nTotalBytes2.QuadPart)
			ReturnValue = 0;
		else
			ReturnValue = nTotalBytes1.QuadPart > nTotalBytes2.QuadPart ? 1 : -1;
	}

	return ReturnValue;
}

int CALLBACK SortByFreeSpaceStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByTotalSize(lParam1,lParam2,FALSE);
}

int CALLBACK SortByOriginalLocationStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByOriginalLocation(lParam1,lParam2);
}

/* TODO: Implement. */
int CALLBACK CFolderView::SortByOriginalLocation(LPARAM lParam1,LPARAM lParam2)
{
	int			ReturnValue;

	ReturnValue = 0;

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByAttributesStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByAttributes(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByAttributes(LPARAM lParam1,LPARAM lParam2)
{
	WIN32_FIND_DATA	*File1 = NULL;
	WIN32_FIND_DATA	*File2 = NULL;
	TCHAR			szAttributes1[32];
	TCHAR			szAttributes2[32];
	TCHAR			FullFileName1[MAX_PATH];
	TCHAR			FullFileName2[MAX_PATH];
	BOOL			IsFolder1;
	BOOL			IsFolder2;
	int				ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		ReturnValue = StrCmpI(File1->cFileName,File2->cFileName);

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}

	PathCombine(FullFileName1,m_CurDir,File1->cFileName);
	PathCombine(FullFileName2,m_CurDir,File2->cFileName);

	/* Build the attribute string for the first item. */
	BuildFileAttributeString(FullFileName1,szAttributes1,
	SIZEOF_ARRAY(szAttributes1));

	/* Build the attribute string for the second item. */
	BuildFileAttributeString(FullFileName2,szAttributes2,
	SIZEOF_ARRAY(szAttributes2));

	ReturnValue = lstrcmp(szAttributes1,szAttributes2);

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByRealSizeStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByRealSize(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByRealSize(LPARAM lParam1,LPARAM lParam2)
{
	WIN32_FIND_DATA	*File1 = NULL;
	WIN32_FIND_DATA	*File2 = NULL;
	TCHAR			szRoot[MAX_PATH];
	long long		fRes;
	DWORD			ClusterSize;
	BOOL			IsFolder1;
	BOOL			IsFolder2;
	int				ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		ReturnValue = StrCmpI(File1->cFileName,File2->cFileName);

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}

	long long FileSize1 = (long long)(File1->nFileSizeLow + (File1->nFileSizeHigh * pow(2.0,32.0)));
	long long FileSize2 = (long long)(File2->nFileSizeLow + (File2->nFileSizeHigh * pow(2.0,32.0)));

	/* Determine the root of the current directory. */
	StringCchCopy(szRoot,SIZEOF_ARRAY(szRoot),m_CurDir);
	PathStripToRoot(szRoot);

	ClusterSize = GetClusterSize(szRoot);

	if(FileSize1 != 0)
		FileSize1 += ClusterSize - (FileSize1 % ClusterSize);

	if(FileSize2 != 0)
		FileSize2 += ClusterSize - (FileSize2 % ClusterSize);

	fRes = FileSize1 - FileSize2;

	if(fRes > 0)
		ReturnValue = 1;
	else if(fRes < 0)
		ReturnValue = -1;
	else
		ReturnValue = 0;

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByShortNameStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByShortName(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByShortName(LPARAM lParam1,LPARAM lParam2)
{
	WIN32_FIND_DATA	*File1 = NULL;
	WIN32_FIND_DATA	*File2 = NULL;
	BOOL			IsFolder1;
	BOOL			IsFolder2;
	int				ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		ReturnValue = StrCmpI(File1->cFileName,File2->cFileName);

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}

	ReturnValue = lstrcmp(m_pwfdFiles[lParam1].cAlternateFileName,m_pwfdFiles[lParam2].cAlternateFileName);

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByOwnerStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByOwner(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByOwner(LPARAM lParam1,LPARAM lParam2)
{
	WIN32_FIND_DATA	*File1 = NULL;
	WIN32_FIND_DATA	*File2 = NULL;
	TCHAR			FullFileName1[MAX_PATH];
	TCHAR			FullFileName2[MAX_PATH];
	TCHAR			szOwner1[512];
	TCHAR			szOwner2[512];
	BOOL			IsFolder1;
	BOOL			IsFolder2;
	int				ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		ReturnValue = StrCmpI(File1->cFileName,File2->cFileName);

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}

	PathCombine(FullFileName1,m_CurDir,File1->cFileName);
	PathCombine(FullFileName2,m_CurDir,File2->cFileName);

	GetFileOwner(FullFileName1,szOwner1,SIZEOF_ARRAY(szOwner1));
	GetFileOwner(FullFileName2,szOwner1,SIZEOF_ARRAY(szOwner2));

	ReturnValue = lstrcmp(szOwner1,szOwner2);

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByProductNameStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByProductName(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByProductName(LPARAM lParam1,LPARAM lParam2)
{
	return SortByVersionInfo(lParam1,lParam2,VERSION_PRODUCTNAME);
}

int CALLBACK SortByCompanyStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByCompany(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByCompany(LPARAM lParam1,LPARAM lParam2)
{
	return SortByVersionInfo(lParam1,lParam2,VERSION_COMPANY);
}

int CALLBACK SortByDescriptionStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByDescription(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByDescription(LPARAM lParam1,LPARAM lParam2)
{
	return SortByVersionInfo(lParam1,lParam2,VERSION_DESCRIPTION);
}

int CALLBACK SortByFileVersionStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByFileVersion(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByFileVersion(LPARAM lParam1,LPARAM lParam2)
{
	return SortByVersionInfo(lParam1,lParam2,VERSION_FILEVERSION);
}

int CALLBACK SortByProductVersionStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByProductVersion(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByProductVersion(LPARAM lParam1,LPARAM lParam2)
{
	return SortByVersionInfo(lParam1,lParam2,VERSION_PRODUCTVERSION);
}

int CALLBACK CFolderView::SortByVersionInfo(LPARAM lParam1,LPARAM lParam2,int VersionProperty)
{
	WIN32_FIND_DATA	*File1 = NULL;
	WIN32_FIND_DATA	*File2 = NULL;
	TCHAR			FullFileName1[MAX_PATH];
	TCHAR			FullFileName2[MAX_PATH];
	TCHAR			szVersionBuf1[512];
	TCHAR			szVersionBuf2[512];
	BOOL			IsFolder1;
	BOOL			IsFolder2;
	BOOL			bVersionInfoObtained;
	int				ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		ReturnValue = StrCmpI(File1->cFileName,File2->cFileName);

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}

	PathCombine(FullFileName1,m_CurDir,File1->cFileName);
	PathCombine(FullFileName2,m_CurDir,File2->cFileName);

	bVersionInfoObtained = GetVersionInfoString(FullFileName1,
	_T("ProductName"),szVersionBuf1,SIZEOF_ARRAY(szVersionBuf1));

	bVersionInfoObtained = GetVersionInfoString(FullFileName2,
	_T("ProductName"),szVersionBuf2,SIZEOF_ARRAY(szVersionBuf2));

	ReturnValue = lstrcmp(szVersionBuf1,szVersionBuf2);

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByShortcutToStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByShortcutTo(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByShortcutTo(LPARAM lParam1,LPARAM lParam2)
{
	WIN32_FIND_DATA	*File1 = NULL;
	WIN32_FIND_DATA	*File2 = NULL;
	TCHAR			FullFileName1[MAX_PATH];
	TCHAR			FullFileName2[MAX_PATH];
	TCHAR			szResolvedLinkPath1[MAX_PATH];
	TCHAR			szResolvedLinkPath2[MAX_PATH];
	BOOL			IsFolder1;
	BOOL			IsFolder2;
	HRESULT			hr;
	int				ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		ReturnValue = StrCmpI(File1->cFileName,File2->cFileName);

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}

	PathCombine(FullFileName1,m_CurDir,File1->cFileName);
	PathCombine(FullFileName2,m_CurDir,File2->cFileName);

	hr = NFileOperations::ResolveLink(NULL,SLR_NO_UI,FullFileName1,szResolvedLinkPath1,
	SIZEOF_ARRAY(szResolvedLinkPath1));

	hr = NFileOperations::ResolveLink(NULL,SLR_NO_UI,FullFileName2,szResolvedLinkPath2,
	SIZEOF_ARRAY(szResolvedLinkPath2));

	ReturnValue = lstrcmp(szResolvedLinkPath1,szResolvedLinkPath2);

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByHardlinksStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByHardlinks(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByHardlinks(LPARAM lParam1,LPARAM lParam2)
{
	WIN32_FIND_DATA	*File1 = NULL;
	WIN32_FIND_DATA	*File2 = NULL;
	TCHAR			FullFileName1[MAX_PATH];
	TCHAR			FullFileName2[MAX_PATH];
	BOOL			IsFolder1;
	BOOL			IsFolder2;
	DWORD			dwNumHardLinks1;
	DWORD			dwNumHardLinks2;
	int				ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		ReturnValue = StrCmpI(File1->cFileName,File2->cFileName);

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}

	PathCombine(FullFileName1,m_CurDir,File1->cFileName);
	PathCombine(FullFileName2,m_CurDir,File2->cFileName);

	dwNumHardLinks1 = GetNumFileHardLinks(FullFileName1);
	dwNumHardLinks2 = GetNumFileHardLinks(FullFileName2);

	ReturnValue = dwNumHardLinks1 - dwNumHardLinks2;

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByExtensionStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByExtension(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByExtension(LPARAM lParam1,LPARAM lParam2)
{
	WIN32_FIND_DATA	*File1 = NULL;
	WIN32_FIND_DATA	*File2 = NULL;
	TCHAR			FullFileName1[MAX_PATH];
	TCHAR			FullFileName2[MAX_PATH];
	TCHAR			*pExt1 = NULL;
	TCHAR			*pExt2 = NULL;
	BOOL			IsFolder1;
	BOOL			IsFolder2;
	int				ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		ReturnValue = StrCmpI(File1->cFileName,File2->cFileName);

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}

	PathCombine(FullFileName1,m_CurDir,File1->cFileName);
	PathCombine(FullFileName2,m_CurDir,File2->cFileName);

	pExt1 = PathFindExtension(File1->cFileName);
	pExt2 = PathFindExtension(File2->cFileName);

	/* TODO: Need to check if pExt == NULL. */
	ReturnValue = lstrcmp(pExt1,pExt2);

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByDateCreatedStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByDateCreated(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByDateCreated(LPARAM lParam1,LPARAM lParam2)
{
	return SortByDate(lParam1,lParam2,DATE_CREATED);
}

int CALLBACK SortByDateAccessedStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByDateAccessed(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByDateAccessed(LPARAM lParam1,LPARAM lParam2)
{
	return SortByDate(lParam1,lParam2,DATE_ACCESSED);
}

int CALLBACK SortByTitleStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByTitle(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByTitle(LPARAM lParam1,LPARAM lParam2)
{
	return SortBySummaryProperty(lParam1,lParam2,PROPERTY_ID_TITLE);
}

int CALLBACK SortBySubjectStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortBySubject(lParam1,lParam2);
}

int CALLBACK CFolderView::SortBySubject(LPARAM lParam1,LPARAM lParam2)
{
	return SortBySummaryProperty(lParam1,lParam2,PROPERTY_ID_SUBJECT);
}

int CALLBACK SortByAuthorStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByAuthor(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByAuthor(LPARAM lParam1,LPARAM lParam2)
{
	return SortBySummaryProperty(lParam1,lParam2,PROPERTY_ID_AUTHOR);
}

int CALLBACK SortByKeywordsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByKeywords(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByKeywords(LPARAM lParam1,LPARAM lParam2)
{
	return SortBySummaryProperty(lParam1,lParam2,PROPERTY_ID_KEYWORDS);
}

int CALLBACK SortByCommentsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByComments(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByComments(LPARAM lParam1,LPARAM lParam2)
{
	return SortBySummaryProperty(lParam1,lParam2,PROPERTY_ID_COMMENT);
}

int CALLBACK CFolderView::SortBySummaryProperty(LPARAM lParam1,LPARAM lParam2,DWORD dwPropertyType)
{
	WIN32_FIND_DATA	*File1 = NULL;
	WIN32_FIND_DATA	*File2 = NULL;
	TCHAR			FullFileName1[MAX_PATH];
	TCHAR			FullFileName2[MAX_PATH];
	TCHAR			szPropertyBuf1[512];
	TCHAR			szPropertyBuf2[512];
	BOOL			IsFolder1;
	BOOL			IsFolder2;
	int				iRes;
	int				ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		ReturnValue = StrCmpI(File1->cFileName,File2->cFileName);

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}

	PathCombine(FullFileName1,m_CurDir,File1->cFileName);
	PathCombine(FullFileName2,m_CurDir,File2->cFileName);

	iRes = ReadFileProperty(FullFileName1,dwPropertyType,szPropertyBuf1,
	SIZEOF_ARRAY(szPropertyBuf1));

	iRes = ReadFileProperty(FullFileName2,dwPropertyType,szPropertyBuf2,
	SIZEOF_ARRAY(szPropertyBuf2));

	ReturnValue = lstrcmp(szPropertyBuf1,szPropertyBuf2);

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByCameraModelStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByCameraModel(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByCameraModel(LPARAM lParam1,LPARAM lParam2)
{
	return SortByImageProperty(lParam1,lParam2,PropertyTagEquipModel);
}

int CALLBACK SortByDateTakenStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByDateTaken(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByDateTaken(LPARAM lParam1,LPARAM lParam2)
{
	return SortByImageProperty(lParam1,lParam2,PropertyTagDateTime);
}

int CALLBACK SortByWidthStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByWidth(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByWidth(LPARAM lParam1,LPARAM lParam2)
{
	return SortByImageProperty(lParam1,lParam2,PropertyTagImageWidth);
}

int CALLBACK SortByHeightStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByHeight(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByHeight(LPARAM lParam1,LPARAM lParam2)
{
	return SortByImageProperty(lParam1,lParam2,PropertyTagImageHeight);
}

int CALLBACK CFolderView::SortByImageProperty(LPARAM lParam1,LPARAM lParam2,PROPID PropertyId)
{
	WIN32_FIND_DATA	*File1 = NULL;
	WIN32_FIND_DATA	*File2 = NULL;
	TCHAR			FullFileName1[MAX_PATH];
	TCHAR			FullFileName2[MAX_PATH];
	TCHAR			szPropertyBuf1[512];
	TCHAR			szPropertyBuf2[512];
	BOOL			IsFolder1;
	BOOL			IsFolder2;
	BOOL			bRes;
	int				ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		ReturnValue = StrCmpI(File1->cFileName,File2->cFileName);

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}

	PathCombine(FullFileName1,m_CurDir,File1->cFileName);
	PathCombine(FullFileName2,m_CurDir,File2->cFileName);

	bRes = ReadImageProperty(FullFileName1,PropertyId,szPropertyBuf1,
	SIZEOF_ARRAY(szPropertyBuf1));

	bRes = ReadImageProperty(FullFileName2,PropertyId,szPropertyBuf2,
	SIZEOF_ARRAY(szPropertyBuf2));

	ReturnValue = lstrcmp(szPropertyBuf1,szPropertyBuf2);

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByVirtualCommentsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByVirtualComments(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByVirtualComments(LPARAM lParam1,LPARAM lParam2)
{
	WIN32_FIND_DATA	*File1 = NULL;
	WIN32_FIND_DATA	*File2 = NULL;
	TCHAR			szInfoTip1[512];
	TCHAR			szInfoTip2[512];
	BOOL			IsFolder1;
	BOOL			IsFolder2;
	HRESULT			hr;
	int				ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		ReturnValue = StrCmpI(File1->cFileName,File2->cFileName);

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}

	hr = GetFileInfoTip(m_hOwner,m_pidlDirectory,const_cast<LPCITEMIDLIST *>(&m_pExtraItemInfo[(int)lParam1].pridl),
		szInfoTip1,SIZEOF_ARRAY(szInfoTip1));
	hr = GetFileInfoTip(m_hOwner,m_pidlDirectory,const_cast<LPCITEMIDLIST *>(&m_pExtraItemInfo[(int)lParam2].pridl),
		szInfoTip2,SIZEOF_ARRAY(szInfoTip2));

	ReturnValue = lstrcmp(szInfoTip1,szInfoTip2);

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByFileSystemStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByFileSystem(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByFileSystem(LPARAM lParam1,LPARAM lParam2)
{
	int				ReturnValue = 0;
	TCHAR			szFullFileName1[MAX_PATH];
	TCHAR			szFullFileName2[MAX_PATH];
	BOOL			bRoot1;
	BOOL			bRoot2;
	BOOL			bRes1;
	BOOL			bRes2;
	TCHAR			szFileSystemName1[32];
	TCHAR			szFileSystemName2[32];

	LPITEMIDLIST	pidlComplete = NULL;
	LPITEMIDLIST	pidlRelative = NULL;
	IShellFolder	*pShellFolder = NULL;
	STRRET			str;

	pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[lParam1].pridl);
	SHBindToParent(pidlComplete,IID_IShellFolder,
		(void **)&pShellFolder,(LPCITEMIDLIST *)&pidlRelative);

	pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
	StrRetToBuf(&str,pidlRelative,szFullFileName1,SIZEOF_ARRAY(szFullFileName1));

	CoTaskMemFree(pidlComplete);
	pShellFolder->Release();
	pShellFolder = NULL;

	pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[lParam2].pridl);
	SHBindToParent(pidlComplete,IID_IShellFolder,
		(void **)&pShellFolder,(LPCITEMIDLIST *)&pidlRelative);

	pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
	StrRetToBuf(&str,pidlRelative,szFullFileName2,SIZEOF_ARRAY(szFullFileName2));

	CoTaskMemFree(pidlComplete);
	pShellFolder->Release();
	pShellFolder = NULL;

	bRoot1 = PathIsRoot(szFullFileName1);
	bRoot2 = PathIsRoot(szFullFileName2);

	if(bRoot1 && !bRoot2)
		ReturnValue = -1;
	else if(!bRoot1 && bRoot2)
		ReturnValue = 1;
	else if(bRoot1 && bRoot2)
	{
		bRes1 = GetVolumeInformation(szFullFileName1,NULL,0,NULL,
			NULL,NULL,szFileSystemName1,SIZEOF_ARRAY(szFileSystemName1));

		bRes2 = GetVolumeInformation(szFullFileName2,NULL,0,NULL,
			NULL,NULL,szFileSystemName2,SIZEOF_ARRAY(szFileSystemName2));

		if(bRes1 && !bRes2)
			ReturnValue = -1;
		else if(!bRes1 && bRes2)
			ReturnValue = 1;
		else if(bRes1 && bRes2)
			ReturnValue = lstrcmp(szFileSystemName1,szFileSystemName2);
	}

	if(!IsSortAscending())
		ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByVirtualTypeStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByVirtualType(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByVirtualType(LPARAM lParam1,LPARAM lParam2)
{
	WIN32_FIND_DATA	*File1 = NULL;
	WIN32_FIND_DATA	*File2 = NULL;
	LPITEMIDLIST	pidlComplete1 = NULL;
	LPITEMIDLIST	pidlComplete2 = NULL;
	LPITEMIDLIST	pidlComplete = NULL;
	IShellFolder	*pShellFolder = NULL;
	LPITEMIDLIST	pidlRelative = NULL;
	SHFILEINFO		shfi1;
	SHFILEINFO		shfi2;
	STRRET			str;
	TCHAR			szItem1[MAX_PATH];
	TCHAR			szItem2[MAX_PATH];
	BOOL			bRoot1;
	BOOL			bRoot2;
	int				ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[lParam1].pridl);
	SHBindToParent(pidlComplete,IID_IShellFolder,
		(void **)&pShellFolder,(LPCITEMIDLIST *)&pidlRelative);

	pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
	StrRetToBuf(&str,pidlRelative,szItem1,SIZEOF_ARRAY(szItem1));

	CoTaskMemFree(pidlComplete);
	pShellFolder->Release();
	pShellFolder = NULL;

	pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[lParam2].pridl);
	SHBindToParent(pidlComplete,IID_IShellFolder,
		(void **)&pShellFolder,(LPCITEMIDLIST *)&pidlRelative);

	pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
	StrRetToBuf(&str,pidlRelative,szItem2,SIZEOF_ARRAY(szItem2));

	CoTaskMemFree(pidlComplete);
	pShellFolder->Release();
	pShellFolder = NULL;

	bRoot1 = PathIsRoot(szItem1);
	bRoot2 = PathIsRoot(szItem2);

	if(bRoot1 && !bRoot2)
		return -1;
	else if(!bRoot1 && bRoot2)
		return 1;
	else
	{
		pidlComplete1 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)lParam1].pridl);
		pidlComplete2 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)lParam2].pridl);

		SHGetFileInfo((LPTSTR)pidlComplete1,0,&shfi1,sizeof(shfi1),SHGFI_PIDL|SHGFI_TYPENAME);
		SHGetFileInfo((LPTSTR)pidlComplete2,0,&shfi2,sizeof(shfi2),SHGFI_PIDL|SHGFI_TYPENAME);

		ReturnValue = lstrcmp(shfi1.szTypeName,shfi2.szTypeName);

		CoTaskMemFree(pidlComplete1);
		CoTaskMemFree(pidlComplete2);
	}

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByNumPrinterDocumentsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByNumPrinterDocuments(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByNumPrinterDocuments(LPARAM lParam1,LPARAM lParam2)
{
	HANDLE hPrinter1;
	HANDLE hPrinter2;
	PRINTER_INFO_2 *pPrinterInfo21 = NULL;
	PRINTER_INFO_2 *pPrinterInfo22 = NULL;
	DWORD cbNeeded;
	DWORD cbSize;
	BOOL bRes1;
	BOOL bRes2;
	int ReturnValue = 0;

	bRes1 = OpenPrinter(m_pExtraItemInfo[(int)lParam1].szDisplayName,&hPrinter1,NULL);
	bRes2 = OpenPrinter(m_pExtraItemInfo[(int)lParam2].szDisplayName,&hPrinter2,NULL);

	if(bRes1 && !bRes2)
		ReturnValue = -1;
	else if(!bRes1 && bRes2)
		ReturnValue = 1;
	else if(!bRes1 && !bRes2)
		ReturnValue = 0;
	else if(bRes1 && bRes2)
	{
		GetPrinter(hPrinter1,2,NULL,0,&cbNeeded);

		pPrinterInfo21 = (PRINTER_INFO_2 *)malloc(cbNeeded);

		cbSize = cbNeeded;

		bRes1 = GetPrinter(hPrinter1,2,(LPBYTE)pPrinterInfo21,cbSize,&cbNeeded);

		GetPrinter(hPrinter2,2,NULL,0,&cbNeeded);

		pPrinterInfo22 = (PRINTER_INFO_2 *)malloc(cbNeeded);

		cbSize = cbNeeded;

		bRes2 = GetPrinter(hPrinter2,2,(LPBYTE)pPrinterInfo22,cbSize,&cbNeeded);

		ReturnValue = pPrinterInfo21->cJobs < pPrinterInfo22->cJobs;

		free(pPrinterInfo21);
		free(pPrinterInfo22);
	}


	if(!IsSortAscending())
		ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByPrinterStatusStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByPrinterStatus(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByPrinterStatus(LPARAM lParam1,LPARAM lParam2)
{
	WIN32_FIND_DATA *File1		= NULL;
	WIN32_FIND_DATA *File2		= NULL;
	BOOL IsFolder1;
	BOOL IsFolder2;
	int ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		ReturnValue = StrCmpI(File1->cFileName,File2->cFileName);

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}



	/* TODO: Fix. */
	/*HANDLE hPrinter;
	PRINTER_INFO_2 *pPrinterInfo1;
	TCHAR szNumJobs[32]	= EMPTY_STRING;
	DWORD cbNeeded;
	DWORD cbSize;
	BOOL bRes;

	bRes = OpenPrinter(m_pExtraItemInfo[(int)lParam1].szDisplayName,&hPrinter,NULL);

	if(bRes)
	{
		GetPrinter(hPrinter,2,NULL,0,&cbNeeded);

		pPrinterInfo1 = (PRINTER_INFO_2 *)malloc(cbNeeded);

		cbSize = cbNeeded;

		bRes = GetPrinter(hPrinter,2,(LPBYTE)pPrinterInfo1,cbSize,&cbNeeded);

		if(bRes)
		{
			StringCchCopyEx(szNumJobs,SIZEOF_ARRAY(szNumJobs),
			DecodePrinterStatus(pPrinterInfo1->Status),NULL,NULL,STRSAFE_IGNORE_NULLS);
		}

		free(pPrinterInfo1);
		ClosePrinter(hPrinter);
	}*/



	ReturnValue = 0;



	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByPrinterCommentsStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByPrinterComments(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByPrinterComments(LPARAM lParam1,LPARAM lParam2)
{
	WIN32_FIND_DATA *File1		= NULL;
	WIN32_FIND_DATA *File2		= NULL;
	LPITEMIDLIST pidlComplete1	= NULL;
	LPITEMIDLIST pidlComplete2	= NULL;
	BOOL IsFolder1;
	BOOL IsFolder2;
	SHFILEINFO shfi1;
	SHFILEINFO shfi2;
	int ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		ReturnValue = StrCmpI(File1->cFileName,File2->cFileName);

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}

	pidlComplete1 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)lParam1].pridl);
	pidlComplete2 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)lParam2].pridl);

	SHGetFileInfo((LPTSTR)pidlComplete1,0,&shfi1,sizeof(shfi1),SHGFI_PIDL | SHGFI_TYPENAME);
	SHGetFileInfo((LPTSTR)pidlComplete2,0,&shfi2,sizeof(shfi2),SHGFI_PIDL | SHGFI_TYPENAME);

	ReturnValue = lstrcmp(shfi1.szTypeName,shfi2.szTypeName);

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByPrinterLocationStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByPrinterLocation(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByPrinterLocation(LPARAM lParam1,LPARAM lParam2)
{
	WIN32_FIND_DATA *File1		= NULL;
	WIN32_FIND_DATA *File2		= NULL;
	LPITEMIDLIST pidlComplete1	= NULL;
	LPITEMIDLIST pidlComplete2	= NULL;
	BOOL IsFolder1;
	BOOL IsFolder2;
	SHFILEINFO shfi1;
	SHFILEINFO shfi2;
	int ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	IsFolder1 = (File1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	IsFolder2 = (File2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	
	if(IsFolder1 && IsFolder2)
	{
		ReturnValue = StrCmpI(File1->cFileName,File2->cFileName);

		if(!IsSortAscending())
			ReturnValue = -ReturnValue;

		return ReturnValue;
	}

	if(IsFolder1)
	{
		ReturnValue = -1;

		return ReturnValue;
	}

	if(IsFolder2)
	{
		ReturnValue = 1;

		return ReturnValue;
	}

	pidlComplete1 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)lParam1].pridl);
	pidlComplete2 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)lParam2].pridl);

	SHGetFileInfo((LPTSTR)pidlComplete1,0,&shfi1,sizeof(shfi1),SHGFI_PIDL | SHGFI_TYPENAME);
	SHGetFileInfo((LPTSTR)pidlComplete2,0,&shfi2,sizeof(shfi2),SHGFI_PIDL | SHGFI_TYPENAME);

	ReturnValue = lstrcmp(shfi1.szTypeName,shfi2.szTypeName);

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortByNetworkAdapterStatusStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->SortByNetworkAdapterStatus(lParam1,lParam2);
}

int CALLBACK CFolderView::SortByNetworkAdapterStatus(LPARAM lParam1,LPARAM lParam2)
{
	WIN32_FIND_DATA *File1		= NULL;
	WIN32_FIND_DATA *File2		= NULL;
	LPITEMIDLIST pidlComplete1	= NULL;
	LPITEMIDLIST pidlComplete2	= NULL;
	int ReturnValue;

	File1 = &m_pwfdFiles[(int)lParam1];
	File2 = &m_pwfdFiles[(int)lParam2];

	pidlComplete1 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)lParam1].pridl);
	pidlComplete2 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)lParam2].pridl);


	/*TCHAR szStatus[32]						= EMPTY_STRING;
	IP_ADAPTER_ADDRESSES *pAdapterAddresses	= NULL;
	UINT uStatusID;
	ULONG ulOutBufLen						= 0;

	GetAdaptersAddresses(AF_UNSPEC,0,NULL,NULL,&ulOutBufLen);

	pAdapterAddresses = (IP_ADAPTER_ADDRESSES *)malloc(ulOutBufLen);

	GetAdaptersAddresses(AF_UNSPEC,0,NULL,pAdapterAddresses,&ulOutBufLen);

	switch(pAdapterAddresses->OperStatus)
	{
		case IfOperStatusUp:
			uStatusID = IDS_NETWORKADAPTER_CONNECTED;
			break;

		case IfOperStatusDown:
			uStatusID = IDS_NETWORKADAPTER_DISCONNECTED;
			break;

		case IfOperStatusTesting:
			uStatusID = IDS_NETWORKADAPTER_TESTING;
			break;

		case IfOperStatusUnknown:
			uStatusID = IDS_NETWORKADAPTER_UNKNOWN;
			break;

		case IfOperStatusDormant:
			uStatusID = IDS_NETWORKADAPTER_DORMANT;
			break;

		case IfOperStatusNotPresent:
			uStatusID = IDS_NETWORKADAPTER_NOTPRESENT;
			break;

		case IfOperStatusLowerLayerDown:
			uStatusID = IDS_NETWORKADAPTER_LOWLAYER;
			break;
	}

	LoadString(m_hResourceModule,uStatusID,
		szStatus,SIZEOF_ARRAY(szStatus));*/


	ReturnValue = 0;

	if(!IsSortAscending())
			ReturnValue = -ReturnValue;

	return ReturnValue;
}

int CALLBACK SortStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CFolderView	*pFolderView = NULL;

	pFolderView = (CFolderView *)lParamSort;

	return pFolderView->Sort(lParam1,lParam2);
}

int CALLBACK CFolderView::Sort(LPARAM lParam1,LPARAM lParam2)
{
	return SortItemsRelative(lParam1,lParam2);
}

HRESULT CFolderView::SortFolder(UINT SortMode)
{
	m_SortMode = SortMode;

	if(m_bShowInGroups)
	{
		ListView_EnableGroupView(m_hListView,FALSE);
		ListView_RemoveAllGroups(m_hListView);
		ListView_EnableGroupView(m_hListView,TRUE);

		SetGrouping(TRUE);
	}

	SendMessage(m_hListView,LVM_SORTITEMS,(WPARAM)this,(LPARAM)SortStub);

	/* If in details view, the column sort
	arrow will need to be changed to reflect
	the new sorting mode. */
	if(m_ViewMode == VM_DETAILS)
		ApplyHeaderSortArrow();

	return S_OK;
}