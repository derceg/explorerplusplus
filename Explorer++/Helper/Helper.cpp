/******************************************************************
 *
 * Project: Helper
 * File: Helper.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Contains various helper functions.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <sstream>
#include "Helper.h"
#include "FileOperations.h"
#include "Buffer.h"
#include "ShellHelper.h"


using namespace Gdiplus;

/* Local helpers. */
void	EnterAttributeIntoString(BOOL bEnter,TCHAR *String,int Pos,TCHAR chAttribute);
BOOL	GetFileAllocationInfo(TCHAR *lpszFileName,STARTING_VCN_INPUT_BUFFER *pStartingVcn,
							  RETRIEVAL_POINTERS_BUFFER *pRetrievalPointers,DWORD BufSize);
BOOL	GetNtfsVolumeInfo(TCHAR *lpszDrive,NTFS_VOLUME_DATA_BUFFER *pNtfsVolumeInfo,DWORD BufSize);
TCHAR	*GetPartitionName(LARGE_INTEGER StartingOffset);

void FormatSizeString(ULARGE_INTEGER lFileSize,TCHAR *pszFileSize,
size_t cchBuf)
{
	FormatSizeString(lFileSize,pszFileSize,cchBuf,FALSE,SIZE_FORMAT_NONE);
}

void FormatSizeString(ULARGE_INTEGER lFileSize,TCHAR *pszFileSize,
size_t cchBuf,BOOL bForceSize,SizeDisplayFormat_t sdf)
{
	TCHAR *pszSizeTypes[] = {_T("bytes"),_T("KB"),_T("MB"),_T("GB"),_T("TB"),_T("PB")};

	double fFileSize = static_cast<double>(lFileSize.QuadPart);
	int iSizeIndex = 0;

	if(bForceSize)
	{
		switch(sdf)
		{
		case SIZE_FORMAT_BYTES:
			iSizeIndex = 0;
			break;

		case SIZE_FORMAT_KBYTES:
			iSizeIndex = 1;
			break;

		case SIZE_FORMAT_MBYTES:
			iSizeIndex = 2;
			break;

		case SIZE_FORMAT_GBYTES:
			iSizeIndex = 3;
			break;

		case SIZE_FORMAT_TBYTES:
			iSizeIndex = 4;
			break;

		case SIZE_FORMAT_PBYTES:
			iSizeIndex = 5;
			break;
		}

		for(int i = 0;i < iSizeIndex;i++)
		{
			fFileSize /= 1024;
		}
	}
	else
	{
		while((fFileSize / 1024) > 1)
		{
			fFileSize /= 1024;

			iSizeIndex++;
		}

		if(iSizeIndex > SIZEOF_ARRAY(pszSizeTypes))
		{
			StringCchCopy(pszFileSize,cchBuf,EMPTY_STRING);
			return;
		}
	}

	int iPrecision;

	if(iSizeIndex == 0 ||
		lFileSize.QuadPart % (1024 * iSizeIndex) == 0)
	{
		iPrecision = 0;
	}
	else
	{
		if(fFileSize < 10)
		{
			iPrecision = 2;
		}
		else if(fFileSize < 100)
		{
			iPrecision = 1;
		}
		else
		{
			iPrecision = 0;
		}
	}

	int iLeast = static_cast<int>((fFileSize - static_cast<int>(fFileSize)) *
		pow(10.0,iPrecision + 1));

	/* Setting the precision will cause automatic rounding. Therefore,
	if the least significant digit to be dropped is greater than 0.5,
	reduce it to below 0.5. */
	if(iLeast >= 5)
	{
		fFileSize -= 5.0 * pow(10.0,-(iPrecision + 1));
	}

	std::wstringstream ss;
	ss.imbue(std::locale(""));
	ss.precision(iPrecision);

	ss << std::fixed << fFileSize << _T(" ") << pszSizeTypes[iSizeIndex];
	wstring str = ss.str();
	StringCchCopy(pszFileSize,cchBuf,str.c_str());
}

int CreateFileTimeString(FILETIME *FileTime,
TCHAR *Buffer,int MaxCharacters,BOOL bFriendlyDate)
{
	SYSTEMTIME SystemTime;
	FILETIME LocalFileTime;
	TCHAR TempBuffer[MAX_STRING_LENGTH];
	TCHAR DateBuffer[MAX_STRING_LENGTH];
	TCHAR TimeBuffer[MAX_STRING_LENGTH];
	SYSTEMTIME CurrentTime;
	int iReturn1 = 0;
	int iReturn2 = 0;

	if(FileTime == NULL)
	{
		Buffer = NULL;
		return -1;
	}

	FileTimeToLocalFileTime(FileTime,&LocalFileTime);
	FileTimeToSystemTime(&LocalFileTime,&SystemTime);
	
	GetLocalTime(&CurrentTime);

	if(bFriendlyDate)
	{
		if((CurrentTime.wYear == SystemTime.wYear) &&
		(CurrentTime.wMonth == SystemTime.wMonth))
		{
			if(CurrentTime.wDay == SystemTime.wDay)
			{
				StringCchCopy(DateBuffer,SIZEOF_ARRAY(DateBuffer),_T("Today"));

				iReturn1 = 1;
			}
			else if(CurrentTime.wDay == (SystemTime.wDay + 1))
			{
				StringCchCopy(DateBuffer,SIZEOF_ARRAY(DateBuffer),_T("Yesterday"));

				iReturn1 = 1;
			}
			else
			{
				iReturn1 = GetDateFormat(LOCALE_USER_DEFAULT,LOCALE_USE_CP_ACP,&SystemTime,
				NULL,DateBuffer,MAX_STRING_LENGTH);
			}
		}
		else
		{
			iReturn1 = GetDateFormat(LOCALE_USER_DEFAULT,LOCALE_USE_CP_ACP,&SystemTime,
			NULL,DateBuffer,MAX_STRING_LENGTH);
		}
	}
	else
	{
		iReturn1 = GetDateFormat(LOCALE_USER_DEFAULT,LOCALE_USE_CP_ACP,&SystemTime,
		NULL,DateBuffer,MAX_STRING_LENGTH);
	}

	iReturn2 = GetTimeFormat(LOCALE_USER_DEFAULT,LOCALE_USE_CP_ACP,&SystemTime,
	NULL,TimeBuffer,MAX_STRING_LENGTH);
	
	if(iReturn1 && iReturn2)
	{
		StringCchPrintf(TempBuffer,SIZEOF_ARRAY(TempBuffer),
			_T("%s, %s"),DateBuffer,TimeBuffer);

		if(MaxCharacters < (lstrlen(TempBuffer) + 1))
		{
			Buffer = NULL;

			return lstrlen(TempBuffer) + 1;
		}
		else
		{
			StringCchCopy(Buffer,MaxCharacters,TempBuffer);

			return lstrlen(TempBuffer) + 1;
		}
	}

	Buffer = NULL;

	return -1;
}

int ListView_SelectAllItems(HWND hListView)
{
	if(hListView == NULL)
		return -1;

	SendMessage(hListView,WM_SETREDRAW,FALSE,0);
	ListView_SetItemState(hListView,-1,LVIS_SELECTED,LVIS_SELECTED);
	SendMessage(hListView,WM_SETREDRAW,TRUE,0);

	return 0;
}

int ListView_DeselectAllItems(HWND hListView)
{
	if(hListView == NULL)
		return -1;

	SendMessage(hListView,WM_SETREDRAW,FALSE,0);
	ListView_SetItemState(hListView,-1,0,LVIS_SELECTED);
	SendMessage(hListView,WM_SETREDRAW,TRUE,0);

	return 0;
}

int ListView_InvertSelection(HWND hListView)
{
	int i = 0;
	int TotalNumberOfItems;
	int nSelected = 0;

	if(hListView == NULL)
		return -1;

	TotalNumberOfItems = ListView_GetItemCount(hListView);

	for(i = 0;i < TotalNumberOfItems;i++)
	{
		if(ListView_GetItemState(hListView,i,LVIS_SELECTED) == LVIS_SELECTED)
		{
			/* Deselect the item. */
			ListView_SelectItem(hListView,i,FALSE);
		}
		else
		{
			/* Select the item. */
			ListView_SelectItem(hListView,i,TRUE);
			nSelected++;
		}
	}

	return nSelected;
}

BOOL ListView_SelectItem(HWND hListView,int nItem,BOOL bSelect)
{
	LVITEM lvItem;

	lvItem.stateMask	= LVIS_SELECTED;

	if(bSelect)
		lvItem.state	= LVIS_SELECTED;
	else
		lvItem.state	= 0;

	return (BOOL)SendMessage(hListView,LVM_SETITEMSTATE,(WPARAM)nItem,(LPARAM)&lvItem);
}

BOOL ListView_FocusItem(HWND hListView,int nItem,BOOL bFocus)
{
	LVITEM lvItem;

	lvItem.stateMask	= LVIS_FOCUSED;

	if(bFocus)
		lvItem.state	= LVIS_FOCUSED;
	else
		lvItem.state	= 0;

	return (BOOL)SendMessage(hListView,LVM_SETITEMSTATE,(WPARAM)nItem,(LPARAM)&lvItem);
}

void ListView_HandleInsertionMark(HWND hListView,int iItemFocus,POINT *ppt)
{
	LVFINDINFO lvfi;
	LVINSERTMARK lvim;
	LV_HITTESTINFO item;
	RECT ItemRect;
	DWORD dwFlags = 0;
	int iNext;
	int iItem;
	int nItems;

	/* Remove the insertion mark. */
	if(ppt == NULL)
	{
		lvim.cbSize		= sizeof(LVINSERTMARK);
		lvim.dwFlags	= 0;
		lvim.iItem		= -1;
		ListView_SetInsertMark(hListView,&lvim);

		return;
	}

	item.pt = *ppt;
	iItem = ListView_HitTest(hListView,&item);

	if(iItem != -1 && item.flags & LVHT_ONITEM)
	{
		ListView_GetItemRect(hListView,item.iItem,&ItemRect,LVIR_BOUNDS);

		/* If the cursor is on the left side
		of this item, set the insertion before
		this item; if it is on the right side
		of this item, set the insertion mark
		after this item. */
		if((ppt->x - ItemRect.left) >
			((ItemRect.right - ItemRect.left)/2))
		{
			iNext = iItem;
			dwFlags = LVIM_AFTER;
		}
		else
		{
			iNext = iItem;
			dwFlags = 0;
		}
	}
	else
	{
		dwFlags = 0;

		/* VK_UP finds whichever item is "above" the cursor.
		This item may be in the same row as the cursor is in
		(e.g. an item in the next row won't be found until the
		cursor passes into that row). Appears to find the item
		on the right side. */
		lvfi.flags			= LVFI_NEARESTXY;
		lvfi.pt				= *ppt;
		lvfi.vkDirection	= VK_UP;
		iNext = ListView_FindItem(hListView,-1,&lvfi);

		if(iNext == -1)
		{
			lvfi.flags			= LVFI_NEARESTXY;
			lvfi.pt				= *ppt;
			lvfi.vkDirection	= VK_LEFT;
			iNext = ListView_FindItem(hListView,-1,&lvfi);
		}

		ListView_GetItemRect(hListView,iNext,&ItemRect,LVIR_BOUNDS);

		/* This situation only occurs at the
		end of the row. Prior to this, it is
		always the item on the right side that
		is found, with the insertion mark been
		inserted before that item.
		Once the end of the row is reached, the
		item found will be on the left side of
		the cursor. */
		if(ppt->x > ItemRect.left +
			((ItemRect.right - ItemRect.left)/2))
		{
			/* At the end of a row, VK_UP appears to
			find the next item up. Therefore, if we're
			at the end of a row, and the cursor is
			completely below the "next" item, find the
			one under it instead (if there is an item
			under it), and anchor the insertion mark
			there. */
			if(ppt->y > ItemRect.bottom)
			{
				int iBelow;

				iBelow = ListView_GetNextItem(hListView,iNext,LVNI_BELOW);

				if(iBelow != -1)
					iNext = iBelow;
			}

			dwFlags = LVIM_AFTER;
		}

		nItems = ListView_GetItemCount(hListView);

		/* Last item is at position nItems - 1. */
		ListView_GetItemRect(hListView,nItems - 1,&ItemRect,LVIR_BOUNDS);

		/* Special case needed for very last item. If cursor is within 0.5 to 1.5 width
		of last item, and is greater than it's y coordinate, snap the insertion mark to
		this item. */
		if((ppt->x > ItemRect.left + ((ItemRect.right - ItemRect.left)/2)) &&
			ppt->x < ItemRect.right + ((ItemRect.right - ItemRect.left)/2) + 2 &&
			ppt->y > ItemRect.top)
		{
			iNext = nItems - 1;
			dwFlags = LVIM_AFTER;
		}
	}

	lvim.cbSize			= sizeof(LVINSERTMARK);
	lvim.dwFlags		= dwFlags;
	lvim.iItem			= iNext;
	ListView_SetInsertMark(hListView,&lvim);
}

HRESULT GetBitmapDimensions(TCHAR *FileName,SIZE *BitmapSize)
{
	HANDLE hFile;
	HANDLE hMappedFile;
	BITMAPFILEHEADER *BitmapFileHeader;
	BITMAPINFOHEADER *BitmapInfoHeader;
	LPVOID Address;

	if((FileName == NULL) || (BitmapSize == NULL))
		return E_INVALIDARG;

	hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return E_FAIL;

	hMappedFile = CreateFileMapping(hFile,NULL,PAGE_READONLY,0,0,_T("MappedFile"));

	if(hMappedFile == NULL)
	{
		CloseHandle(hFile);
		return E_FAIL;
	}

	Address = MapViewOfFile(hMappedFile,FILE_MAP_READ,0,0,0);

	if(Address == NULL)
	{
		CloseHandle(hFile);
		CloseHandle(hMappedFile);
		return E_FAIL;
	}

	BitmapInfoHeader = (BITMAPINFOHEADER *)(LPBYTE)((TCHAR *)Address + sizeof(BITMAPFILEHEADER));

	BitmapFileHeader = (BITMAPFILEHEADER *)(LPBYTE)Address;

	BitmapSize->cx = BitmapInfoHeader->biWidth;
	BitmapSize->cy = BitmapInfoHeader->biHeight;

	CloseHandle(hFile);
	UnmapViewOfFile(Address);
	CloseHandle(hMappedFile);

	return S_OK;
}

HINSTANCE StartCommandPrompt(TCHAR *Directory)
{
	TCHAR SystemPath[MAX_PATH];
	TCHAR FullPath[MAX_PATH];
	TCHAR Prompt[]			= _T("cmd.exe");
	HINSTANCE hNewInstance	= NULL;
	BOOL bRes;

	bRes = SHGetSpecialFolderPath(NULL,SystemPath,CSIDL_SYSTEM,0);

	if(bRes)
	{
		PathCombine(FullPath,SystemPath,Prompt);

		hNewInstance = ShellExecute(NULL,_T("open"),FullPath,Directory,Directory,
		SW_SHOW);
	}

	return hNewInstance;
}

HANDLE CopyTextToClipboard(TCHAR *szTextData)
{
	HGLOBAL hGlobal		= NULL;
	LPVOID pMem			= NULL;
	HANDLE DataHandle	= NULL;
	UINT uFormat;
	BOOL bOpened;

	if(szTextData == NULL)
		return NULL;

	bOpened = OpenClipboard(NULL);

	if(!bOpened)
		return NULL;

	EmptyClipboard();

	hGlobal = GlobalAlloc(GMEM_MOVEABLE,(lstrlen(szTextData) + 1) * sizeof(TCHAR));

	if(hGlobal != NULL)
	{
		pMem = GlobalLock(hGlobal);
		memcpy(pMem,szTextData,(lstrlen(szTextData) + 1) * sizeof(TCHAR));
		GlobalUnlock(hGlobal);

		#ifndef UNICODE
			uFormat = CF_TEXT;
		#else
			uFormat = CF_UNICODETEXT;
		#endif

		DataHandle = SetClipboardData(uFormat,hGlobal);
	}

	CloseClipboard();

	return DataHandle;
}

BOOL lCheckMenuItem(HMENU hMenu,UINT ItemID,BOOL bCheck)
{
	if(bCheck)
	{
		CheckMenuItem(hMenu,ItemID,MF_CHECKED);
		return TRUE;
	}
	else
	{
		CheckMenuItem(hMenu,ItemID,MF_UNCHECKED);
		return FALSE;
	}
}

BOOL lEnableMenuItem(HMENU hMenu,UINT ItemID,BOOL bEnable)
{
	if(bEnable)
	{
		EnableMenuItem(hMenu,ItemID,MF_ENABLED);
		return TRUE;
	}
	else
	{
		EnableMenuItem(hMenu,ItemID,MF_GRAYED);
		return FALSE;
	}
}

BOOL GetRealFileSize(TCHAR *FileName,PLARGE_INTEGER lpRealFileSize)
{
	LARGE_INTEGER lFileSize;
	LONG ClusterSize;
	HANDLE hFile;
	TCHAR szRoot[MAX_PATH];

	if(FileName == NULL)
		return FALSE;

	/* Get a handle to the file. */
	hFile = CreateFile(FileName,GENERIC_READ,
	FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,NULL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	/* Get the files size (count of number of actual
	number of bytes in file). */
	GetFileSizeEx(hFile,&lFileSize);

	*lpRealFileSize = lFileSize;

	if(lFileSize.QuadPart != 0)
	{
		StringCchCopy(szRoot,SIZEOF_ARRAY(szRoot),FileName);
		PathStripToRoot(szRoot);

		/* Get the cluster size of the drive the file resides on. */
		ClusterSize = GetClusterSize(szRoot);

		if((lpRealFileSize->QuadPart % ClusterSize) != 0)
		{
			/* The real size is the logical file size rounded up to the end of the
			nearest cluster. */
			lpRealFileSize->QuadPart += ClusterSize - (lpRealFileSize->QuadPart % ClusterSize);
		}
	}

	CloseHandle(hFile);

	return TRUE;
}

LONG GetFileSectorSize(TCHAR *FileName)
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
	hFile = CreateFile(FileName,GENERIC_READ,
	FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,NULL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return -1;

	/* Get the files size (count of number of actual
	number of bytes in file). */
	FileSize = GetFileSize(hFile,NULL);

	StringCchCopy(Root,SIZEOF_ARRAY(Root),FileName);
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

BOOL FileTimeToLocalSystemTime(LPFILETIME lpFileTime,LPSYSTEMTIME lpLocalTime)
{
	SYSTEMTIME SystemTime;

	FileTimeToSystemTime(lpFileTime,&SystemTime);

	return SystemTimeToTzSpecificLocalTime(NULL,&SystemTime,lpLocalTime);
}

BOOL LocalSystemTimeToFileTime(LPSYSTEMTIME lpLocalTime,LPFILETIME lpFileTime)
{
	SYSTEMTIME SystemTime;

	TzSpecificLocalTimeToSystemTime(NULL,lpLocalTime,&SystemTime);

	return SystemTimeToFileTime(&SystemTime,lpFileTime);
}

BOOL SetProcessTokenPrivilege(DWORD ProcessId,TCHAR *PrivilegeName,BOOL bEnablePrivilege)
{
	HANDLE hProcess;
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	LUID luid;

	hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,ProcessId);

	if(hProcess == NULL)
		return FALSE;

	OpenProcessToken(hProcess,TOKEN_ALL_ACCESS,&hToken);

	LookupPrivilegeValue(NULL,PrivilegeName,&luid);

	tp.PrivilegeCount				= 1;
	tp.Privileges[0].Luid			= luid;

	if(bEnablePrivilege)
		tp.Privileges[0].Attributes	= SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes	= 0;

	CloseHandle(hProcess);

	return AdjustTokenPrivileges(hToken,FALSE,&tp,0,NULL,NULL);
}

BOOL CompareFileTypes(TCHAR *pszFile1,TCHAR *pszFile2)
{
	SHFILEINFO shfi1;
	SHFILEINFO shfi2;

	SHGetFileInfo(pszFile1,NULL,&shfi1,sizeof(shfi1),SHGFI_TYPENAME);
	SHGetFileInfo(pszFile2,NULL,&shfi2,sizeof(shfi2),SHGFI_TYPENAME);

	if(StrCmp(shfi1.szTypeName,shfi2.szTypeName) == 0)
		return TRUE;

	return FALSE;
}

BOOL SetFileSparse(TCHAR *szFileName)
{
	HANDLE hFile;
	DWORD NumBytesReturned;
	BOOL res;

	hFile = CreateFile(szFileName,FILE_WRITE_DATA,0,
	NULL,OPEN_EXISTING,NULL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	res = DeviceIoControl(hFile,FSCTL_SET_SPARSE,NULL,0,
	NULL,0,&NumBytesReturned,NULL);

	CloseHandle(hFile);

	return res;
}

void CheckItem(HWND hwnd,BOOL bCheck)
{
	DWORD CheckState;

	if(bCheck)
		CheckState = BST_CHECKED;
	else
		CheckState = BST_UNCHECKED;

	SendMessage(hwnd,BM_SETCHECK,(WPARAM)CheckState,(LPARAM)0);
}

BOOL IsItemChecked(HWND hwnd)
{
	LRESULT CheckState;

	CheckState = SendMessage(hwnd,BM_GETCHECK,0,0);

	return (CheckState == BST_CHECKED) ? TRUE : FALSE;
}

TCHAR *PrintComma(unsigned long nPrint)
{
	LARGE_INTEGER lPrint;

	lPrint.LowPart = nPrint;
	lPrint.HighPart = 0;

	return PrintCommaLargeNum(lPrint);
}

TCHAR *PrintCommaLargeNum(LARGE_INTEGER lPrint)
{
	static TCHAR szBuffer[14];
	TCHAR *p = &szBuffer[SIZEOF_ARRAY(szBuffer) - 1];
	static TCHAR chComma = ',';
	unsigned long long nTemp = (unsigned long long)(lPrint.LowPart + (lPrint.HighPart * pow(2.0,32.0)));
	int i = 0;

	if(nTemp == 0)
	{
		StringCchPrintf(szBuffer,SIZEOF_ARRAY(szBuffer),_T("%d"),0);
		return szBuffer;
	}

	*p = (TCHAR)'\0';

	while(nTemp != 0)
	{
		if(i%3 == 0 && i != 0)
			*--p = chComma;

		*--p = '0' + (TCHAR)(nTemp % 10);

		nTemp /= 10;

		i++;
	}

	return p;
}

int WriteTextToRichEdit(HWND hRichEdit,TCHAR *fmt,...)
{
	va_list		varg;
	SETTEXTEX	TextEx;
	TCHAR		pszBuf[1024];
	int			iResult;

	va_start(varg,fmt);

	/* Can't dynamically allocate pszBuf properly, because
	the string may be expanded. */
	StringCchVPrintf(pszBuf,SIZEOF_ARRAY(pszBuf),fmt,varg);

	/* Replace current selection. */
	TextEx.flags	= ST_SELECTION;

	/* Unicode page set. */
	TextEx.codepage	= CP_UNICODE;

	iResult = (int)SendMessage(hRichEdit,EM_SETTEXTEX,(WPARAM)&TextEx,(LPARAM)pszBuf);

	va_end(varg);

	return iResult;
}

BOOL lShowWindow(HWND hwnd,BOOL bShowWindow)
{
	int WindowShowState;

	if(bShowWindow)
		WindowShowState = SW_SHOW;
	else
		WindowShowState = SW_HIDE;

	return ShowWindow(hwnd,WindowShowState);
}

int GetRectHeight(RECT *rc)
{
	return rc->bottom - rc->top;
}

int GetRectWidth(RECT *rc)
{
	return rc->right - rc->left;
}

DWORD ListView_SetGridlines(HWND hListView,BOOL bEnableGridlines)
{
	DWORD OldExtendedStyle;

	OldExtendedStyle = ListView_GetExtendedListViewStyle(hListView);

	if(bEnableGridlines)
	{
		OldExtendedStyle |= LVS_EX_GRIDLINES;
	}
	else
	{
		if(OldExtendedStyle & LVS_EX_GRIDLINES)
		{
			OldExtendedStyle &= ~LVS_EX_GRIDLINES;
		}
	}

	return ListView_SetExtendedListViewStyle(hListView,OldExtendedStyle);
}

BOOL GetMonitorDeviceName(TCHAR *lpszMonitorName,DWORD BufSize)
{
	HMONITOR hMonitor;
	MONITORINFOEX mfex;
	POINT pt = {0,0};
	BOOL bRes;

	hMonitor = MonitorFromPoint(pt,MONITOR_DEFAULTTOPRIMARY);

	mfex.cbSize	= sizeof(mfex);
	bRes = GetMonitorInfo(hMonitor,&mfex);

	StringCchCopy(lpszMonitorName,BufSize,mfex.szDevice);

	return bRes;
}

DWORD BuildFileAttributeString(TCHAR *lpszFileName,TCHAR *Buffer,DWORD BufSize)
{
	HANDLE hFindFile;
	WIN32_FIND_DATA wfd;

	/* FindFirstFile is used instead of GetFileAttributes() or
	GetFileAttributesEx() because of its behaviour
	in relation to system files that normally
	won't have their attributes given (such as the
	pagefile, which neither of the two functions
	above can retrieve the attributes of). */
	hFindFile = FindFirstFile(lpszFileName,&wfd);

	if(hFindFile == INVALID_HANDLE_VALUE)
	{
		StringCchCopy(Buffer,BufSize,EMPTY_STRING);
		return 0;
	}

	BuildFileAttributeStringInternal(wfd.dwFileAttributes,Buffer,BufSize);

	FindClose(hFindFile);

	return wfd.dwFileAttributes;
}

void BuildFileAttributeStringInternal(DWORD dwFileAttributes,TCHAR *szOutput,DWORD cchMax)
{
	TCHAR szAttributes[8];
	int i = 0;

	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE,szAttributes,i++,'A');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_HIDDEN,szAttributes,i++,'H');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_READONLY,szAttributes,i++,'R');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_SYSTEM,szAttributes,i++,'S');
	EnterAttributeIntoString((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY,
		szAttributes,i++,'D');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED,szAttributes,i++,'C');
	EnterAttributeIntoString(dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED,szAttributes,i++,'E');

	szAttributes[i] = '\0';

	StringCchCopy(szOutput,cchMax,szAttributes);
}

void EnterAttributeIntoString(BOOL bEnter,TCHAR *String,int Pos,TCHAR chAttribute)
{
	if(bEnter)
		String[Pos] = chAttribute;
	else
		String[Pos] = '-';
}

size_t GetFileOwner(TCHAR *szFile,TCHAR *szOwner,DWORD BufSize)
{
	HANDLE hFile;
	PSID pSid;
	PSECURITY_DESCRIPTOR pSecurityDescriptor;
	DWORD dwRes;
	TCHAR szAccountName[512];
	DWORD dwAccountName = SIZEOF_ARRAY(szAccountName);
	TCHAR szDomainName[512];
	DWORD dwDomainName = SIZEOF_ARRAY(szDomainName);
	size_t ReturnLength = 0;
	SID_NAME_USE eUse;
	LPTSTR StringSid;
	BOOL bRes;

	/* The SE_SECURITY_NAME privilege is needed to call GetSecurityInfo on the given file. */
	bRes = SetProcessTokenPrivilege(GetCurrentProcessId(),SE_SECURITY_NAME,TRUE);

	if(!bRes)
		return 0;

	hFile = CreateFile(szFile,STANDARD_RIGHTS_READ|ACCESS_SYSTEM_SECURITY,FILE_SHARE_READ,NULL,OPEN_EXISTING,
	FILE_FLAG_BACKUP_SEMANTICS,NULL);

	if(hFile != INVALID_HANDLE_VALUE)
	{
		pSid = (PSID)GlobalAlloc(GMEM_FIXED,sizeof(PSID));

		pSecurityDescriptor = (PSECURITY_DESCRIPTOR)GlobalAlloc(GMEM_FIXED,sizeof(PSECURITY_DESCRIPTOR));

		dwRes = GetSecurityInfo(hFile,SE_FILE_OBJECT,OWNER_SECURITY_INFORMATION,&pSid,
			NULL,NULL,NULL,&pSecurityDescriptor);

		if(dwRes != ERROR_SUCCESS)
		{
			CloseHandle(hFile);
			return 0;
		}

		bRes = LookupAccountSid(NULL,pSid,szAccountName,&dwAccountName,
			szDomainName,&dwDomainName,&eUse);

		/* LookupAccountSid failed. */
		if(bRes == 0)
		{
			bRes = ConvertSidToStringSid(pSid,&StringSid);

			if(bRes != 0)
			{
				StringCchCopy(szOwner,BufSize,StringSid);

				LocalFree(StringSid);
				ReturnLength = lstrlen(StringSid);
			}
		}
		else
		{
			StringCchPrintf(szOwner,BufSize,_T("%s\\%s"),szDomainName,szAccountName);

			ReturnLength = lstrlen(szAccountName);
		}

		LocalFree(&pSecurityDescriptor);
		CloseHandle(hFile);
	}

	/* Reset the privilege. */
	SetProcessTokenPrivilege(GetCurrentProcessId(),SE_SECURITY_NAME,FALSE);

	return ReturnLength;
}

BOOL GetProcessOwner(TCHAR *szOwner,DWORD BufSize)
{
	HANDLE hProcess;
	HANDLE hToken;
	TOKEN_USER *pTokenUser = NULL;
	SID_NAME_USE eUse;
	LPTSTR StringSid;
	TCHAR szAccountName[512];
	DWORD dwAccountName = SIZEOF_ARRAY(szAccountName);
	TCHAR szDomainName[512];
	DWORD dwDomainName = SIZEOF_ARRAY(szDomainName);
	DWORD ReturnLength;
	DWORD dwSize = 0;
	BOOL bRes;
	BOOL bReturn = FALSE;

	hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,GetCurrentProcessId());

	if(hProcess != NULL)
	{
		bRes = OpenProcessToken(hProcess,TOKEN_ALL_ACCESS,&hToken);

		if(bRes)
		{
			GetTokenInformation(hToken,TokenUser,NULL,0,&dwSize);

			pTokenUser = (PTOKEN_USER)GlobalAlloc(GMEM_FIXED,dwSize);

			if(pTokenUser != NULL)
			{
				GetTokenInformation(hToken,TokenUser,(LPVOID)pTokenUser,dwSize,&ReturnLength);

				bRes = LookupAccountSid(NULL,pTokenUser->User.Sid,szAccountName,&dwAccountName,
					szDomainName,&dwDomainName,&eUse);

				/* LookupAccountSid failed. */
				if(bRes == 0)
				{
					bRes = ConvertSidToStringSid(pTokenUser->User.Sid,&StringSid);

					if(bRes != 0)
					{
						StringCchCopy(szOwner,BufSize,StringSid);

						LocalFree(StringSid);

						bReturn = TRUE;
					}
				}
				else
				{
					StringCchPrintf(szOwner,BufSize,_T("%s\\%s"),szDomainName,szAccountName);

					bReturn = TRUE;
				}

				GlobalFree(pTokenUser);
			}
		}
		CloseHandle(hProcess);
	}

	if(!bReturn)
		StringCchCopy(szOwner,BufSize,EMPTY_STRING);

	return bReturn;
}

BOOL CheckGroupMembership(GroupType_t GroupType)
{
	SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
	PSID psid;
	DWORD dwGroup = 0;
	BOOL bMember = FALSE;
	BOOL bRet;

	switch(GroupType)
	{
	case GROUP_ADMINISTRATORS:
		dwGroup = DOMAIN_ALIAS_RID_ADMINS;
		break;

	case GROUP_POWERUSERS:
		dwGroup = DOMAIN_ALIAS_RID_POWER_USERS;
		break;

	case GROUP_USERS:
		dwGroup = DOMAIN_ALIAS_RID_USERS;
		break;

	case GROUP_USERSRESTRICTED:
		dwGroup = DOMAIN_ALIAS_RID_GUESTS;
		break;
	}

	bRet = AllocateAndInitializeSid(&sia,2,SECURITY_BUILTIN_DOMAIN_RID,
		dwGroup,0,0,0,0,0,0,&psid);

	if(bRet)
	{
		CheckTokenMembership(NULL,psid,&bMember);

		FreeSid(psid);
	}

	return bMember;
}

struct LANGCODEPAGE
{
	WORD wLanguage;
	WORD wCodePage;
} *lpTranslate;

BOOL GetVersionInfoString(TCHAR *szFileName,TCHAR *szVersionInfo,TCHAR *szBuffer,UINT cbBufLen)
{
	LPVOID lpData;
	TCHAR szSubBlock[64];
	TCHAR *lpszLocalBuf = NULL;
	LANGID UserLangId;
	DWORD dwLen;
	DWORD dwHandle = NULL;
	UINT cbTranslate;
	UINT cbLen;
	BOOL bRet = FALSE;
	unsigned int i = 0;

	dwLen = GetFileVersionInfoSize(szFileName,&dwHandle);

	if(dwLen > 0)
	{
		lpData = malloc(dwLen);

		if(lpData != NULL)
		{
			if(GetFileVersionInfo(szFileName,0,dwLen,lpData) != 0)
			{
				UserLangId = GetUserDefaultLangID();

				VerQueryValue(lpData,_T("\\VarFileInfo\\Translation"),(LPVOID *)&lpTranslate,&cbTranslate);

				for(i = 0;i < (cbTranslate / sizeof(LANGCODEPAGE));i++)
				{
					/* If the bottom eight bits of the language id's match, use this
					version information (since this means that the version information
					and the users default language are the same). Also use this version
					information if the language is not specified (i.e. wLanguage is 0). */
					if((lpTranslate[i].wLanguage & 0xFF) == (UserLangId & 0xFF) ||
						lpTranslate[i].wLanguage == 0)
					{
						StringCchPrintf(szSubBlock,SIZEOF_ARRAY(szSubBlock),
							_T("\\StringFileInfo\\%04X%04X\\%s"),lpTranslate[i].wLanguage,
							lpTranslate[i].wCodePage,szVersionInfo);

						if(VerQueryValue(lpData,szSubBlock,(LPVOID *)&lpszLocalBuf,&cbLen) != 0)
						{
							/* The buffer may be NULL if the specified data was not found
							within the file. */
							if(lpszLocalBuf != NULL)
							{
								StringCchCopy(szBuffer,cbBufLen,lpszLocalBuf);

								bRet = TRUE;
								break;
							}
						}
					}
				}
			}
			free(lpData);
		}
	}

	return bRet;
}

DWORD GetNumFileHardLinks(TCHAR *lpszFileName)
{
	HANDLE hFile;
	BY_HANDLE_FILE_INFORMATION FileInfo;
	BOOL bRes;

	hFile = CreateFile(lpszFileName,FILE_READ_ATTRIBUTES,FILE_SHARE_READ,NULL,
	OPEN_EXISTING,NULL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return 0;

	bRes = GetFileInformationByHandle(hFile,&FileInfo);

	CloseHandle(hFile);

	if(bRes == 0)
	{
		return 0;
	}

	return FileInfo.nNumberOfLinks;
}

int ReadFileProperty(TCHAR *lpszFileName,DWORD dwPropertyType,TCHAR *lpszPropertyBuf,DWORD dwBufLen)
{
	HANDLE hFile;
	TCHAR szCommentStreamName[512];
	TCHAR *lpszProperty;
	DWORD dwNumBytesRead;
	DWORD dwSectionLength;
	DWORD dwPropertyCount;
	DWORD dwPropertyId;
	DWORD dwPropertyOffset = 0;
	DWORD dwPropertyLength;
	DWORD dwPropertyMarker;
	DWORD dwSectionOffset;
	BOOL bFound = FALSE;
	unsigned int i = 0;

	StringCchPrintf(szCommentStreamName,SIZEOF_ARRAY(szCommentStreamName),
	_T("%s:%cSummaryInformation"),lpszFileName,0x5);
	hFile = CreateFile(szCommentStreamName,GENERIC_READ,FILE_SHARE_READ,NULL,
	OPEN_EXISTING,NULL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return -1;

	/*Constant offset.*/
	SetFilePointer(hFile,0x2C,0,FILE_CURRENT);

	/*Read out the section offset (the SummaryInformation stream only has one offset anyway...).*/
	ReadFile(hFile,(LPBYTE)&dwSectionOffset,sizeof(dwSectionOffset),&dwNumBytesRead,NULL);

	/*The secion offset is from the start of the stream. Go back to the start of the stream,
	and then go to the start of the section.*/
	SetFilePointer(hFile,0,0,FILE_BEGIN);
	SetFilePointer(hFile,dwSectionOffset,0,FILE_BEGIN);

	/*Since this is the only section, the section length gives the length from the
	start of the section to the end of the stream. The property count gives the
	number of properties assocciated with this file (author, comments, etc).*/
	ReadFile(hFile,(LPBYTE)&dwSectionLength,sizeof(DWORD),&dwNumBytesRead,NULL);
	ReadFile(hFile,(LPBYTE)&dwPropertyCount,sizeof(DWORD),&dwNumBytesRead,NULL);

	/*Go through each property, try to find the one that was asked for.*/
	for(i = 0;i < dwPropertyCount;i++)
	{
		ReadFile(hFile,(LPBYTE)&dwPropertyId,sizeof(dwPropertyId),&dwNumBytesRead,NULL);

		if(dwPropertyId == dwPropertyType)
		{
			bFound = TRUE;

			/*The offset is from the start of this section.*/
			ReadFile(hFile,(LPBYTE)&dwPropertyOffset,sizeof(dwPropertyOffset),&dwNumBytesRead,NULL);
		}

		/*Skip past the offset, as this is a property that isn't needed.*/
		SetFilePointer(hFile,0x04,0,FILE_CURRENT);
	}

	if(!bFound)
	{
		CloseHandle(hFile);
		return -1;
	}

	/*Go back to the start of the offset, then to the property that is desired (property offsets
	are given from the start of the section).*/
	SetFilePointer(hFile,0,0,FILE_BEGIN);
	SetFilePointer(hFile,dwSectionOffset,0,FILE_BEGIN);
	SetFilePointer(hFile,dwPropertyOffset,0,FILE_CURRENT);

	/*Read the property marker. If this is not equal to 0x1E, then this is not a valid property
	(1E indicates a NULL terminated string prepended by dword string length).*/
	ReadFile(hFile,(LPBYTE)&dwPropertyMarker,sizeof(dwPropertyMarker),&dwNumBytesRead,NULL);

	//if(dwPropertyMarker != 0x1E)
		//return -1;

	/*Read the length of the property (if the property is a string, this length includes the
	NULL byte).*/
	ReadFile(hFile,(LPBYTE)&dwPropertyLength,sizeof(dwPropertyLength),&dwNumBytesRead,NULL);

	lpszProperty = (TCHAR *)malloc(dwPropertyLength * sizeof(TCHAR));

	/*Now finally read out the property of interest.*/
	ReadFile(hFile,(LPBYTE)lpszProperty,dwPropertyLength,&dwNumBytesRead,NULL);
	
	if(dwNumBytesRead != dwPropertyLength)
	{
		/*This stream is not valid, and has probably been altered or damaged.*/
		CloseHandle(hFile);
		return -1;
	}

	StringCchCopy(lpszPropertyBuf,dwBufLen,lpszProperty);

	CloseHandle(hFile);

	return dwPropertyLength;
}

typedef struct
{
	DWORD Id;
	DWORD Offset;
} PropertyDeclarations_t;

int SetFileProperty(TCHAR *lpszFileName,DWORD dwPropertyType,TCHAR *szNewValue)
{
	HANDLE hFile;
	PropertyDeclarations_t *pPropertyDeclarations = NULL;
	TCHAR szCommentStreamName[512];
	DWORD dwNumBytesRead;
	DWORD dwSectionLength;
	DWORD dwPropertyCount;
	DWORD dwPropertyMarker;
	DWORD dwSectionOffset;
	DWORD dwNumBytesWritten;
	BOOL bFound = FALSE;
	unsigned int i = 0;
	TCHAR szBuf[512];
	int iPropertyNumber;

	StringCchPrintf(szCommentStreamName,SIZEOF_ARRAY(szCommentStreamName),
	_T("%s:%cSummaryInformation"),lpszFileName,0x5);
	hFile = CreateFile(szCommentStreamName,GENERIC_READ,FILE_SHARE_READ,NULL,
	OPEN_EXISTING,NULL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return -1;

	/*Constant offset.*/
	SetFilePointer(hFile,0x2C,0,FILE_CURRENT);

	/*Read out the section offset (the SummaryInformation stream only has one offset anyway...).*/
	ReadFile(hFile,(LPBYTE)&dwSectionOffset,sizeof(dwSectionOffset),&dwNumBytesRead,NULL);

	/*The secion offset is from the start of the stream. Go back to the start of the stream,
	and then go to the start of the section.*/
	SetFilePointer(hFile,0,0,FILE_BEGIN);

	/* Read header and section declarations. */
	ReadFile(hFile,(LPBYTE)szBuf,512,&dwNumBytesRead,NULL);

	SetFilePointer(hFile,dwSectionOffset,0,FILE_BEGIN);

	/*Since this is the only section, the section length gives the length from the
	start of the section to the end of the stream. The property count gives the
	number of properties assocciated with this file (author, comments, etc).*/
	ReadFile(hFile,(LPBYTE)&dwSectionLength,sizeof(DWORD),&dwNumBytesRead,NULL);
	ReadFile(hFile,(LPBYTE)&dwPropertyCount,sizeof(DWORD),&dwNumBytesRead,NULL);

	pPropertyDeclarations = (PropertyDeclarations_t *)malloc(dwPropertyCount * sizeof(PropertyDeclarations_t));

	/*Go through each property, try to find the one that was asked for.
	This is in the property declarations part.*/
	for(i = 0;i < dwPropertyCount;i++)
	{
		ReadFile(hFile,(LPBYTE)&pPropertyDeclarations[i],sizeof(PropertyDeclarations_t),&dwNumBytesRead,NULL);

		if(pPropertyDeclarations[i].Id == dwPropertyType)
		{
			bFound = TRUE;
			iPropertyNumber = i;
		}
	}

	if(!bFound)
	{
		free(pPropertyDeclarations);
		CloseHandle(hFile);
		return -1;
	}

	TCHAR szPropertyBuf[512];
	int iCurrentSize;
	int iSizeDifferential;
	ReadFileProperty(lpszFileName,dwPropertyType,szPropertyBuf,512);
	iCurrentSize = lstrlen(szPropertyBuf);

	iSizeDifferential = lstrlen(szNewValue) - iCurrentSize;

	dwSectionLength += iSizeDifferential;

	/* New section length (offset 0x30). */
	SetFilePointer(hFile,dwSectionOffset,0,FILE_BEGIN);
	WriteFile(hFile,(LPCVOID)&dwSectionLength,sizeof(dwSectionLength),&dwNumBytesWritten,NULL);

	/* Beginning of the property declarations table. */
	SetFilePointer(hFile,0x38,0,FILE_BEGIN);

	/* Property declarations after specfied one. */
	SetFilePointer(hFile,(iPropertyNumber + 1) * sizeof(PropertyDeclarations_t),0,FILE_CURRENT);

	for(i = iPropertyNumber + 1;i < dwPropertyCount;i++)
	{
		WriteFile(hFile,(LPCVOID)&pPropertyDeclarations[i],sizeof(PropertyDeclarations_t),&dwNumBytesWritten,NULL);
		pPropertyDeclarations[i].Offset += iSizeDifferential; 
	}

	/*Go back to the start of the offset, then to the property that is desired (property offsets
	are given from the start of the section).*/
	/*SetFilePointer(hFile,0,0,FILE_BEGIN);
	SetFilePointer(hFile,dwSectionOffset,0,FILE_BEGIN);
	SetFilePointer(hFile,dwPropertyOffset,0,FILE_CURRENT);*/

	/*Read the property marker. If this is not equal to 0x1E, then this is not a valid property
	(1E indicates a NULL terminated string prepended by dword string length).*/
	ReadFile(hFile,(LPBYTE)&dwPropertyMarker,sizeof(dwPropertyMarker),&dwNumBytesRead,NULL);
	
	free(pPropertyDeclarations);
	CloseHandle(hFile);

	return 0;
}

BOOL ReadImageProperty(TCHAR *lpszImage,UINT PropertyId,void *pPropBuffer,DWORD dwBufLen)
{
	GdiplusStartupInput	StartupInput;
	WCHAR				wszImage[MAX_PATH];
	PropertyItem		*pPropItems = NULL;
	char				pTempBuffer[512];
	ULONG_PTR			Token;
	UINT				Size;
	UINT				NumProperties;
	Status				res;
	BOOL				bFound = FALSE;
	unsigned int		i = 0;

	GdiplusStartup(&Token,&StartupInput,NULL);

	#ifndef UNICODE
	MultiByteToWideChar(CP_ACP,0,lpszImage,
	-1,wszImage,SIZEOF_ARRAY(wszImage));
	#else
	StringCchCopy(wszImage,SIZEOF_ARRAY(wszImage),lpszImage);
	#endif

	Image *image = new Image(wszImage,FALSE);

	if(image->GetLastStatus() != Ok)
	{
		delete image;
		return FALSE;
	}

	if(PropertyId == PropertyTagImageWidth)
	{
		UINT uWidth;

		uWidth = image->GetWidth();

		bFound = TRUE;

		StringCchPrintf((LPWSTR)pPropBuffer,dwBufLen,L"%u pixels",uWidth);
	}
	else if(PropertyId == PropertyTagImageHeight)
	{
		UINT uHeight;

		uHeight = image->GetHeight();

		bFound = TRUE;

		StringCchPrintf((LPWSTR)pPropBuffer,dwBufLen,L"%u pixels",uHeight);
	}
	else
	{
		image->GetPropertySize(&Size,&NumProperties);

		pPropItems = (PropertyItem *)malloc(Size);
		res = image->GetAllPropertyItems(Size,NumProperties,pPropItems);

		if(res == Ok)
		{
			for(i = 0;i < NumProperties;i++)
			{
				if(pPropItems[i].id == PropertyId)
				{
					bFound = TRUE;
					break;
				}
			}
		}

		if(!bFound && (PropertyId == PropertyTagExifDTOrig))
		{
			/* If the specified tag is PropertyTagExifDTOrig, we'll
			transparently fall back on PropertyTagDateTime. */
			for(i = 0;i < NumProperties;i++)
			{
				if(pPropItems[i].id == PropertyTagDateTime)
				{
					bFound = TRUE;
					break;
				}
			}
		}

		if(bFound)
			memcpy(pTempBuffer,pPropItems[i].value,pPropItems[i].length);
		else
			pPropBuffer = NULL;

		/* All property strings are ANSI. */
		#ifndef UNICODE
		StringCchCopy((char *)pPropBuffer,dwBufLen,pTempBuffer);
		#else
		MultiByteToWideChar(CP_ACP,0,pTempBuffer,
			-1,(WCHAR *)pPropBuffer,dwBufLen);
		#endif

		free(pPropItems);
	}

	delete image;
	GdiplusShutdown(Token);

	return bFound;
}

int DumpSummaryInformationStream(TCHAR *lpszInputFile,TCHAR *lpszOutputFile)
{
	HANDLE hInputFile;
	HANDLE hOutputFile;
	TCHAR szCommentStreamName[512];
	DWORD dwNumBytesRead;
	DWORD NumBytesWritten;
	LPVOID Buffer[512];
	DWORD FileSize;

	StringCchPrintf(szCommentStreamName,SIZEOF_ARRAY(szCommentStreamName),
	_T("%s:%cDocumentSummaryInformation"),lpszInputFile,0x5);

	hInputFile = CreateFile(szCommentStreamName,GENERIC_READ,FILE_SHARE_READ,NULL,
	OPEN_EXISTING,NULL,NULL);

	if(hInputFile == INVALID_HANDLE_VALUE)
		return -1;

	FileSize = GetFileSize(hInputFile,NULL);

	ReadFile(hInputFile,Buffer,FileSize,&dwNumBytesRead,NULL);

	hOutputFile = CreateFile(lpszOutputFile,GENERIC_WRITE,0,NULL,OPEN_EXISTING,
	NULL,NULL);

	WriteFile(hOutputFile,Buffer,dwNumBytesRead,&NumBytesWritten,NULL);

	CloseHandle(hInputFile);
	CloseHandle(hOutputFile);

	return NumBytesWritten;
}

int EnumFileStreams(TCHAR *lpszFileName)
{
	HANDLE hFile;
	WIN32_STREAM_ID sid;
	WCHAR wszStreamName[MAX_PATH];
	LPVOID lpContext = NULL;
	DWORD dwNumBytesRead;
	DWORD dwNumBytesSeekedLow;
	DWORD dwNumBytesSeekedHigh;
	int iNumStreams = 0;

	hFile = CreateFile(lpszFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
	OPEN_EXISTING,NULL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return -1;

	BackupRead(hFile,(LPBYTE)&sid,20,&dwNumBytesRead,FALSE,FALSE,&lpContext);

	BackupRead(hFile,(LPBYTE)wszStreamName,sid.dwStreamNameSize,
	&dwNumBytesRead,FALSE,FALSE,&lpContext);

	/*Seek to the end of this stream (this is the default data
	stream for the file).*/
	BackupSeek(hFile,sid.Size.LowPart,sid.Size.HighPart,
	&dwNumBytesSeekedLow,&dwNumBytesSeekedHigh,&lpContext);

	BackupRead(hFile,(LPBYTE)&sid,20,&dwNumBytesRead,FALSE,FALSE,&lpContext);
	while(dwNumBytesRead != 0)
	{
		BackupRead(hFile,(LPBYTE)wszStreamName,sid.dwStreamNameSize,
		&dwNumBytesRead,FALSE,FALSE,&lpContext);

		BackupSeek(hFile,sid.Size.LowPart,sid.Size.HighPart,
		&dwNumBytesSeekedLow,&dwNumBytesSeekedHigh,&lpContext);

		if(sid.dwStreamId == BACKUP_ALTERNATE_DATA)
			iNumStreams++;

		BackupRead(hFile,(LPBYTE)&sid,20,&dwNumBytesRead,FALSE,FALSE,&lpContext);
	}
	
	BackupRead(hFile,NULL,0,NULL,TRUE,FALSE,&lpContext);

	CloseHandle(hFile);

	return 1;
}

void WriteFileSlack(TCHAR *szFileName,void *pData,int iDataSize)
{
	HANDLE hFile;
	DWORD FileSize;
	DWORD NumBytesWritten;
	DWORD SectorsPerCluster;
	DWORD BytesPerSector;
	LARGE_INTEGER lRealFileSize;
	TCHAR Root[MAX_PATH];
	LONG FileSectorSize;

	/*The SE_MANAGE_VOLUME_NAME privilege is needed to set
	the valid data length of a file.*/
	if(!SetProcessTokenPrivilege(GetCurrentProcessId(),SE_MANAGE_VOLUME_NAME,TRUE))
		return;

	hFile = CreateFile(szFileName,GENERIC_WRITE,
	FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,
	NULL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return;

	StringCchCopy(Root,SIZEOF_ARRAY(Root),szFileName);
	PathStripToRoot(Root);

	GetDiskFreeSpace(Root,&SectorsPerCluster,&BytesPerSector,NULL,NULL);

	/*Ask for the files logical size.*/
	FileSize = GetFileSize(hFile,NULL);

	/*Determine the files actual size (i.e. the
	size it physically takes up on disk).*/
	GetRealFileSize(szFileName,&lRealFileSize);

	if((FileSize % GetClusterSize(Root)) == 0)
	{
		/*This file takes up an exact number of clusters,
		thus there is no free space at the end of the file
		to write data.*/
		CloseHandle(hFile);
		return;
	}
	
	FileSectorSize = GetFileSectorSize(szFileName);
	if((FileSectorSize % SectorsPerCluster) == 0)
	{
		/*This file has data in all the sectors of its clusters.
		Data written to the end of a sector in use is wiped when
		the file is shrunk. Therefore, cannot write data to the end
		of this file.*/
		CloseHandle(hFile);
		return;
	}

	/* Extend the file to the physical end of file. */
	SetFilePointerEx(hFile,lRealFileSize,NULL,FILE_BEGIN);
	SetEndOfFile(hFile);
	SetFileValidData(hFile,lRealFileSize.QuadPart);

	/*Move back to the first spare sector.*/
	SetFilePointer(hFile,FileSectorSize * BytesPerSector,NULL,FILE_BEGIN);

	/*Write the data to be hidden into the file.*/
	WriteFile(hFile,pData,iDataSize,&NumBytesWritten,NULL);

	/*The data that was written must be flushed.
	If it is not, the os will not physically
	write the data to disk before the file is
	shrunk.*/
	FlushFileBuffers(hFile);

	/*Now shrink the file back to its original size.*/
	SetFilePointer(hFile,FileSize,NULL,FILE_BEGIN);
	SetEndOfFile(hFile);
	SetFileValidData(hFile,FileSize);

	CloseHandle(hFile);
}

int ReadFileSlack(TCHAR *FileName,TCHAR *pszSlack,int iBufferLen)
{
	HANDLE			hFile;
	DWORD			FileSize;
	DWORD			nBytesRead = 0;
	DWORD			FileSectorSize;
	TCHAR			*pszSlackTemp = NULL;
	DWORD			BytesPerSector;
	LARGE_INTEGER	lRealFileSize;
	TCHAR			Root[MAX_PATH];
	int				SpareSectors;
	BOOL			res;

	/* The SE_MANAGE_VOLUME_NAME privilege is needed to set
	the valid data length of a file. */
	SetProcessTokenPrivilege(GetCurrentProcessId(),SE_MANAGE_VOLUME_NAME,TRUE);

	if(GetLastError() != ERROR_SUCCESS)
		return -1;

	hFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,
	FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,
	NULL,NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return -1;

	StringCchCopy(Root,SIZEOF_ARRAY(Root),FileName);
	PathStripToRoot(Root);

	res = GetDiskFreeSpace(Root,NULL,&BytesPerSector,NULL,NULL);

	if(res)
	{
		/* Get the file's logical size. */
		FileSize = GetFileSize(hFile,NULL);

		/* Determine the files actual size (i.e. the
		size it physically takes up on disk). */
		GetRealFileSize(FileName,&lRealFileSize);

		if(lRealFileSize.QuadPart > FileSize)
		{
			FileSectorSize = GetFileSectorSize(FileName);

			/* Determine the number of sectors at the end of the file
			that are currently not in use. */
			SpareSectors = (int)((lRealFileSize.QuadPart / BytesPerSector) - FileSectorSize);

			/* Extend the file to the physical end of file. */
			SetFilePointerEx(hFile,lRealFileSize,NULL,FILE_BEGIN);
			SetEndOfFile(hFile);
			SetFileValidData(hFile,lRealFileSize.QuadPart);

			if((FileSectorSize * BytesPerSector) > FileSize)
			{
				/* Move the file pointer back to the logical end of file, so that all data
				after the logical end of file can be read. */
				SetFilePointer(hFile,FileSectorSize * BytesPerSector,NULL,FILE_BEGIN);

				pszSlackTemp = (TCHAR *)malloc(SpareSectors * BytesPerSector);

				if(pszSlackTemp != NULL)
				{
					/* Read out the data contained after the logical end of file. */
					ReadFile(hFile,(LPVOID)pszSlackTemp,SpareSectors * BytesPerSector,&nBytesRead,NULL);

					memcpy_s(pszSlack,iBufferLen,pszSlackTemp,nBytesRead);
				}
			}

			/* Now shrink the file back to its original size. */
			SetFilePointer(hFile,FileSize,NULL,FILE_BEGIN);
			SetEndOfFile(hFile);
			SetFileValidData(hFile,FileSize);
		}
	}

	CloseHandle(hFile);

	return nBytesRead;
}

HRESULT SetComboBoxExPath(HWND CbEx,ITEMIDLIST *PathIdl)
{
	IShellFolder *pDesktop		= NULL;
	COMBOBOXEXITEM cbItem;
	SHFILEINFO shfi;
	STRRET str;
	HRESULT hr;
	TCHAR PathDisplayName[MAX_PATH];

	hr = SHGetDesktopFolder(&pDesktop);

	if(SUCCEEDED(hr))
	{
		SHGetFileInfo((LPTSTR)PathIdl,NULL,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_SYSICONINDEX);

		hr = pDesktop->GetDisplayNameOf(PathIdl,SHGDN_NORMAL,&str);

		if(SUCCEEDED(hr))
		{
			StrRetToBuf(&str,PathIdl,PathDisplayName,SIZEOF_ARRAY(PathDisplayName));

			cbItem.mask				= CBEIF_TEXT|CBEIF_IMAGE|CBEIF_INDENT|CBEIF_SELECTEDIMAGE;
			cbItem.iItem			= -1;
			cbItem.iImage			= shfi.iIcon;
			cbItem.iSelectedImage	= shfi.iIcon;
			cbItem.iIndent			= 1;
			cbItem.iOverlay			= 1;
			cbItem.pszText			= PathDisplayName;
			cbItem.cchTextMax		= lstrlen(PathDisplayName);

			SendMessage(CbEx,CBEM_SETITEM,(WPARAM)-1,(LPARAM)&cbItem);
		}

		pDesktop->Release();
	}

	return hr;
}

BOOL GetFileNameFromUser(HWND hwnd,TCHAR *FullFileName,TCHAR *InitialDirectory)
{
	TCHAR *Filter = _T("Text Document (*.txt)\0*.txt\0All Files\0*.*\0\0");
	OPENFILENAME ofn;
	BOOL bRet;

	ofn.lStructSize			= sizeof(ofn);
	ofn.hwndOwner			= hwnd;
	ofn.lpstrFilter			= Filter;
	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 0;
	ofn.lpstrFile			= FullFileName;
	ofn.nMaxFile			= MAX_PATH;
	ofn.lpstrFileTitle		= NULL;
	ofn.nMaxFileTitle		= 0;
	ofn.lpstrInitialDir		= InitialDirectory;
	ofn.lpstrTitle			= NULL;
	ofn.Flags				= OFN_ENABLESIZING|OFN_OVERWRITEPROMPT|OFN_EXPLORER;
	ofn.lpstrDefExt			= _T("txt");
	ofn.lCustData			= NULL;
	ofn.lpfnHook			= NULL;
	ofn.pvReserved			= NULL;
	ofn.dwReserved			= NULL;
	ofn.FlagsEx				= NULL;

	bRet = GetSaveFileName(&ofn);

	return bRet;
}

void ListView_SetAutoArrange(HWND hListView,BOOL bAutoArrange)
{
	UINT uStyle;

	uStyle = GetWindowStyle(hListView);

	if(bAutoArrange)
	{
		if((uStyle & LVS_AUTOARRANGE) != LVS_AUTOARRANGE)
			uStyle |= LVS_AUTOARRANGE;
	}
	else
	{
		if((uStyle & LVS_AUTOARRANGE) == LVS_AUTOARRANGE)
			uStyle &= ~LVS_AUTOARRANGE;
	}

	SetWindowLongPtr(hListView,GWL_STYLE,uStyle);
}

HRESULT ListView_SetBackgroundImage(HWND hListView,UINT Image)
{
	LVBKIMAGE lvbki;
	TCHAR ModuleName[MAX_PATH];
	TCHAR Bitmap[MAX_PATH];
	BOOL res;

	if(hListView == NULL)
		return E_INVALIDARG;

	GetModuleFileName(NULL,ModuleName,MAX_PATH);

	lvbki.ulFlags	= LVBKIF_STYLE_NORMAL | LVBKIF_SOURCE_URL;

	/* 2 means an image resource, 3 would mean an icon. */
	StringCchPrintf(Bitmap,SIZEOF_ARRAY(Bitmap),
	_T("res://%s/#2/#%d"),ModuleName,Image);

	if(Image == NULL)
		lvbki.pszImage		= NULL;
	else
		lvbki.pszImage		= Bitmap;

	lvbki.xOffsetPercent	= 45;
	lvbki.yOffsetPercent	= 50;

	res = ListView_SetBkImage(hListView,&lvbki);

	if(!res)
		return E_FAIL;

	return S_OK;
}

void ListView_SwapItems(HWND hListView,int iItem1,int iItem2)
{
	LVITEM lvItem;
	LPARAM lParam1;
	LPARAM lParam2;
	TCHAR szText1[512];
	TCHAR szText2[512];
	BOOL bItem1Checked;
	BOOL bItem2Checked;
	UINT Item1StateMask;
	UINT Item2StateMask;
	BOOL res;

	lvItem.mask			= LVIF_TEXT | LVIF_PARAM;
	lvItem.iItem		= iItem1;
	lvItem.iSubItem		= 0;
	lvItem.pszText		= szText1;
	lvItem.cchTextMax	= SIZEOF_ARRAY(szText1);

	res = ListView_GetItem(hListView,&lvItem);

	if(!res)
		return;

	lParam1 = lvItem.lParam;

	lvItem.mask			= LVIF_TEXT | LVIF_PARAM;
	lvItem.iItem		= iItem2;
	lvItem.iSubItem		= 0;
	lvItem.pszText		= szText2;
	lvItem.cchTextMax	= SIZEOF_ARRAY(szText2);

	res = ListView_GetItem(hListView,&lvItem);

	if(!res)
		return;

	lParam2 = lvItem.lParam;

	lvItem.mask		= LVIF_TEXT | LVIF_PARAM;
	lvItem.iItem	= iItem1;
	lvItem.iSubItem	= 0;
	lvItem.pszText	= szText2;
	lvItem.lParam	= lParam2;
	ListView_SetItem(hListView,&lvItem);

	lvItem.mask		= LVIF_TEXT | LVIF_PARAM;
	lvItem.iItem	= iItem2;
	lvItem.iSubItem	= 0;
	lvItem.pszText	= szText1;
	lvItem.lParam	= lParam1;
	ListView_SetItem(hListView,&lvItem);

	/* Swap all sub-items. */
	HWND hHeader;
	TCHAR szBuffer1[512];
	TCHAR szBuffer2[512];
	int nColumns;
	int iSubItem = 1;
	int i = 0;

	hHeader = ListView_GetHeader(hListView);

	nColumns = Header_GetItemCount(hHeader);

	for(i = 1;i < nColumns;i++)
	{
		lvItem.mask			= LVIF_TEXT;
		lvItem.iItem		= iItem1;
		lvItem.iSubItem		= iSubItem;
		lvItem.pszText		= szBuffer1;
		lvItem.cchTextMax	= SIZEOF_ARRAY(szBuffer1);

		res = ListView_GetItem(hListView,&lvItem);

		lvItem.mask			= LVIF_TEXT;
		lvItem.iItem		= iItem2;
		lvItem.iSubItem		= iSubItem;
		lvItem.pszText		= szBuffer2;
		lvItem.cchTextMax	= SIZEOF_ARRAY(szBuffer2);

		res = ListView_GetItem(hListView,&lvItem);

		lvItem.mask		= LVIF_TEXT;
		lvItem.iItem	= iItem1;
		lvItem.iSubItem	= iSubItem;
		lvItem.pszText	= szBuffer2;
		ListView_SetItem(hListView,&lvItem);

		lvItem.mask		= LVIF_TEXT;
		lvItem.iItem	= iItem2;
		lvItem.iSubItem	= iSubItem;
		lvItem.pszText	= szBuffer1;
		ListView_SetItem(hListView,&lvItem);

		iSubItem++;
	}

	Item1StateMask = ListView_GetItemState(hListView,iItem1,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);
	Item2StateMask = ListView_GetItemState(hListView,iItem2,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);

	ListView_SetItemState(hListView,iItem1,Item2StateMask,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);
	ListView_SetItemState(hListView,iItem2,Item1StateMask,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);

	bItem1Checked = ListView_GetCheckState(hListView,iItem1);
	bItem2Checked = ListView_GetCheckState(hListView,iItem2);

	ListView_SetCheckState(hListView,iItem1,bItem2Checked);
	ListView_SetCheckState(hListView,iItem2,bItem1Checked);
}

void ListView_SwapItemsNolParam(HWND hListView,int iItem1,int iItem2)
{
	LVITEM lvItem;
	TCHAR szText1[512];
	TCHAR szText2[512];
	BOOL bItem1Checked;
	BOOL bItem2Checked;
	UINT Item1StateMask;
	UINT Item2StateMask;
	BOOL res;

	lvItem.mask			= LVIF_TEXT;
	lvItem.iItem		= iItem1;
	lvItem.iSubItem		= 0;
	lvItem.pszText		= szText1;
	lvItem.cchTextMax	= SIZEOF_ARRAY(szText1);

	res = ListView_GetItem(hListView,&lvItem);

	if(!res)
		return;

	lvItem.mask			= LVIF_TEXT;
	lvItem.iItem		= iItem2;
	lvItem.iSubItem		= 0;
	lvItem.pszText		= szText2;
	lvItem.cchTextMax	= SIZEOF_ARRAY(szText2);

	res = ListView_GetItem(hListView,&lvItem);

	if(!res)
		return;

	lvItem.mask		= LVIF_TEXT;
	lvItem.iItem	= iItem1;
	lvItem.iSubItem	= 0;
	lvItem.pszText	= szText2;
	ListView_SetItem(hListView,&lvItem);

	lvItem.mask		= LVIF_TEXT;
	lvItem.iItem	= iItem2;
	lvItem.iSubItem	= 0;
	lvItem.pszText	= szText1;
	ListView_SetItem(hListView,&lvItem);

	/* Swap all sub-items. */
	HWND hHeader;
	TCHAR szBuffer1[512];
	TCHAR szBuffer2[512];
	int nColumns;
	int iSubItem = 1;
	int i = 0;

	hHeader = ListView_GetHeader(hListView);

	nColumns = Header_GetItemCount(hHeader);

	for(i = 0;i < nColumns;i++)
	{
		lvItem.mask			= LVIF_TEXT;
		lvItem.iItem		= iItem1;
		lvItem.iSubItem		= iSubItem;
		lvItem.pszText		= szBuffer1;
		lvItem.cchTextMax	= SIZEOF_ARRAY(szBuffer1);

		res = ListView_GetItem(hListView,&lvItem);

		lvItem.mask			= LVIF_TEXT;
		lvItem.iItem		= iItem2;
		lvItem.iSubItem		= iSubItem;
		lvItem.pszText		= szBuffer2;
		lvItem.cchTextMax	= SIZEOF_ARRAY(szBuffer2);

		res = ListView_GetItem(hListView,&lvItem);

		lvItem.mask		= LVIF_TEXT;
		lvItem.iItem	= iItem1;
		lvItem.iSubItem	= iSubItem;
		lvItem.pszText	= szBuffer2;
		ListView_SetItem(hListView,&lvItem);

		lvItem.mask		= LVIF_TEXT;
		lvItem.iItem	= iItem2;
		lvItem.iSubItem	= iSubItem;
		lvItem.pszText	= szBuffer1;
		ListView_SetItem(hListView,&lvItem);

		iSubItem++;
	}

	Item1StateMask = ListView_GetItemState(hListView,iItem1,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);
	Item2StateMask = ListView_GetItemState(hListView,iItem2,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);

	ListView_SetItemState(hListView,iItem1,Item2StateMask,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);
	ListView_SetItemState(hListView,iItem2,Item1StateMask,LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED);

	bItem1Checked = ListView_GetCheckState(hListView,iItem1);
	bItem2Checked = ListView_GetCheckState(hListView,iItem2);

	ListView_SetCheckState(hListView,iItem1,bItem2Checked);
	ListView_SetCheckState(hListView,iItem2,bItem1Checked);
}

void TabCtrl_SwapItems(HWND hTabCtrl,int iItem1,int iItem2)
{
	TCITEM tcItem;
	LPARAM lParam1;
	LPARAM lParam2;
	TCHAR szText1[512];
	TCHAR szText2[512];
	int iImage1;
	int iImage2;
	BOOL res;

	tcItem.mask			= TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
	tcItem.pszText		= szText1;
	tcItem.cchTextMax	= SIZEOF_ARRAY(szText1);

	res = TabCtrl_GetItem(hTabCtrl,iItem1,&tcItem);

	if(!res)
		return;

	lParam1 = tcItem.lParam;
	iImage1 = tcItem.iImage;

	tcItem.mask			= TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
	tcItem.pszText		= szText2;
	tcItem.cchTextMax	= SIZEOF_ARRAY(szText2);

	res = TabCtrl_GetItem(hTabCtrl,iItem2,&tcItem);

	if(!res)
		return;

	lParam2 = tcItem.lParam;
	iImage2 = tcItem.iImage;

	tcItem.mask		= TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
	tcItem.pszText	= szText1;
	tcItem.lParam	= lParam1;
	tcItem.iImage	= iImage1;

	TabCtrl_SetItem(hTabCtrl,iItem2,&tcItem);

	tcItem.mask		= TCIF_TEXT|TCIF_PARAM|TCIF_IMAGE;
	tcItem.pszText	= szText2;
	tcItem.lParam	= lParam2;
	tcItem.iImage	= iImage2;

	TabCtrl_SetItem(hTabCtrl,iItem1,&tcItem);
}

void TabCtrl_SetItemText(HWND Tab,int iTab,TCHAR *Text)
{
	TCITEM tcItem;

	if(Text == NULL)
		return;

	tcItem.mask			= TCIF_TEXT;
	tcItem.pszText		= Text;

	SendMessage(Tab,TCM_SETITEM,(WPARAM)(int)iTab,(LPARAM)&tcItem);
}

BOOL CheckWildcardMatch(const TCHAR *szWildcard,const TCHAR *szString,BOOL bCaseSensitive)
{
	/* Handles multiple wildcard patterns. If the wildcard pattern contains ':', 
	split the pattern into multiple subpatterns.
	For example "*.h: *.cpp" would match against "*.h" and "*.cpp" */
	BOOL bMultiplePattern = FALSE;

	for(int i = 0; i < lstrlen(szWildcard); i++)
	{
		if(szWildcard[i] == ':')
		{
			bMultiplePattern = TRUE;
			break;
		}
	}

	if(!bMultiplePattern)
	{
		return CheckWildcardMatchInternal(szWildcard,szString,bCaseSensitive);
	}
	else
	{
		TCHAR szWildcardPattern[512];
		TCHAR *szSinglePattern = NULL;
		TCHAR *szSearchPattern = NULL;
		TCHAR *szRemainingPattern = NULL;

		StringCchCopy(szWildcardPattern,SIZEOF_ARRAY(szWildcardPattern),szWildcard);

		szSinglePattern = cstrtok_s(szWildcardPattern,_T(":"),&szRemainingPattern);
		PathRemoveBlanks(szSinglePattern);

		while(szSinglePattern != NULL)
		{
			if(CheckWildcardMatchInternal(szSinglePattern,szString,bCaseSensitive))
			{
				return TRUE;
			}

			szSearchPattern = szRemainingPattern;
			szSinglePattern = cstrtok_s(szSearchPattern,_T(":"),&szRemainingPattern);
			PathRemoveBlanks(szSinglePattern);
		}
	}

	return FALSE;
}

BOOL CheckWildcardMatchInternal(const TCHAR *szWildcard,const TCHAR *szString,BOOL bCaseSensitive)
{
	BOOL bMatched;
	BOOL bCurrentMatch = TRUE;

	while(*szWildcard != '\0' && *szString != '\0' && bCurrentMatch)
	{
		switch(*szWildcard)
		{
		/* Match against the next part of the wildcard string.
		If there is a match, then return true, else consume
		the next character, and check again. */
		case '*':
			bMatched = FALSE;

			if(*(szWildcard + 1) != '\0')
			{
				bMatched = CheckWildcardMatch(++szWildcard,szString,bCaseSensitive);
			}

			while(*szWildcard != '\0' && *szString != '\0' && !bMatched)
			{
				/* Consume one more character on the input string,
				and keep (recursively) trying to match. */
				bMatched = CheckWildcardMatch(szWildcard,++szString,bCaseSensitive);
			}

			if(bMatched)
			{
				while(*szWildcard != '\0')
					szWildcard++;

				szWildcard--;

				while(*szString != '\0')
					szString++;
			}

			bCurrentMatch = bMatched;
			break;

		case '?':
			szString++;
			break;

		default:
			if(bCaseSensitive)
			{
				bCurrentMatch = (*szWildcard == *szString);
			}
			else
			{
				TCHAR ch1;
				TCHAR ch2;

				ch1 = LOWORD(CharLower((LPTSTR)MAKEWORD(*szWildcard,0)));
				ch2 = LOWORD(CharLower((LPTSTR)MAKEWORD(*szString,0)));

				bCurrentMatch = (ch1 == ch2);
			}

			szString++;
			break;
		}

		szWildcard++;
	}

	/* Skip past any trailing wildcards. */
	while(*szWildcard == '*')
		szWildcard++;

	if(*szWildcard == '\0' && *szString == '\0' && bCurrentMatch)
		return TRUE;

	return FALSE;
}

TCHAR *DecodePrinterStatus(DWORD dwStatus)
{
	if(dwStatus == 0)
		return _T("Ready");
	else if(dwStatus & PRINTER_STATUS_BUSY)
		return _T("Busy");
	else if(dwStatus & PRINTER_STATUS_ERROR)
		return _T("Error");
	else if(dwStatus & PRINTER_STATUS_INITIALIZING)
		return _T("Initializing");
	else if(dwStatus & PRINTER_STATUS_IO_ACTIVE)
		return _T("Active");
	else if(dwStatus & PRINTER_STATUS_NOT_AVAILABLE)
		return _T("Unavailable");
	else if(dwStatus & PRINTER_STATUS_OFFLINE)
		return _T("Offline");
	else if(dwStatus & PRINTER_STATUS_OUT_OF_MEMORY)
		return _T("Out of memory");
	else if(dwStatus & PRINTER_STATUS_NO_TONER)
		return _T("Out of toner");

	return NULL;
}

void RetrieveAdapterInfo(void)
{
	IP_ADAPTER_ADDRESSES *pAdapterAddresses	= NULL;
	ULONG ulOutBufLen						= 0;

	GetAdaptersAddresses(AF_UNSPEC,0,NULL,NULL,&ulOutBufLen);

	pAdapterAddresses = (IP_ADAPTER_ADDRESSES *)malloc(ulOutBufLen);

	if(pAdapterAddresses != NULL)
	{
		GetAdaptersAddresses(AF_UNSPEC,0,NULL,pAdapterAddresses,&ulOutBufLen);

		free(pAdapterAddresses);
	}
}

BOOL IsImage(TCHAR *szFileName)
{
	static TCHAR *ImageExts[10] = {_T("bmp"),_T("ico"),
	_T("gif"),_T("jpg"),_T("exf"),_T("png"),_T("tif"),_T("wmf"),_T("emf"),_T("tiff")};
	TCHAR *ext;
	int i = 0;

	if(szFileName != NULL)
	{
		ext = PathFindExtension(szFileName);

		if(ext == NULL || (ext + 1) == NULL)
			return FALSE;

		ext++;

		for(i = 0;i < 10;i++)
		{
			if(lstrcmpi(ext,ImageExts[i]) == 0)
				return TRUE;
		}
	}

	return FALSE;
}

void ReplaceCharacters(TCHAR *str,char ch,char replacement)
{
	int  i = 0;

	for(i = 0;i < lstrlen(str);i++)
	{
		if(str[i] == ch)
			str[i] = replacement;
	}
}

void ShowLastError(void)
{
	LPVOID ErrorMessage;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|
	FORMAT_MESSAGE_FROM_SYSTEM,NULL,GetLastError(),
	MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),(LPTSTR)&ErrorMessage,
	0,NULL);

	MessageBox(NULL,(LPTSTR)ErrorMessage,EMPTY_STRING,MB_OK|MB_ICONQUESTION);
}

TCHAR *GetToken(TCHAR *ptr,TCHAR *Buffer,TCHAR *BufferLength)
{
	TCHAR *p;
	int i = 0;

	if(ptr == NULL || *ptr == '\0')
	{
		*Buffer = NULL;
		return NULL;
	}

	p = ptr;

	while(*p == ' ' || *p == '\t')
		p++;

	if(*p == '\"')
	{
		p++;
		while(*p != '\0' && *p != '\"')
		{
			Buffer[i++] = *p;
			p++;
		}
		p++;
	}
	else
	{
		while(*p != '\0' && *p != ' ' && *p != '\t')
		{
			Buffer[i++] = *p;
			p++;
		}
	}

	Buffer[i] = '\0';

	while(*p == ' ' || *p == '\t')
		p++;

	return p;
}

HRESULT GetItemInfoTip(TCHAR *szItemPath,TCHAR *szInfoTip,int cchMax)
{
	LPITEMIDLIST	pidlItem = NULL;
	HRESULT			hr;

	hr = GetIdlFromParsingName(szItemPath,&pidlItem);

	if(SUCCEEDED(hr))
	{
		hr = GetItemInfoTip(pidlItem,szInfoTip,cchMax);

		CoTaskMemFree(pidlItem);
	}

	return hr;
}

HRESULT GetItemInfoTip(LPITEMIDLIST pidlComplete,TCHAR *szInfoTip,int cchMax)
{
	IShellFolder	*pShellFolder = NULL;
	IQueryInfo		*pQueryInfo = NULL;
	LPITEMIDLIST	pidlRelative = NULL;
	LPCWSTR			ppwszTip = NULL;
	HRESULT			hr;

	hr = SHBindToParent(pidlComplete,IID_IShellFolder,(void **)&pShellFolder,(LPCITEMIDLIST *)&pidlRelative);

	if(SUCCEEDED(hr))
	{
		hr = pShellFolder->GetUIObjectOf(NULL,1,(LPCITEMIDLIST *)&pidlRelative,
			IID_IQueryInfo,0,(void **)&pQueryInfo);

		if(SUCCEEDED(hr))
		{
			hr = pQueryInfo->GetInfoTip(QITIPF_USENAME,(WCHAR **)&ppwszTip);

			if(SUCCEEDED(hr))
			{
				#ifndef UNICODE
				WideCharToMultiByte(CP_ACP,0,ppwszTip,-1,szInfoTip,
					cchMax,NULL,NULL);
				#else
				StringCchCopy(szInfoTip,cchMax,ppwszTip);
				#endif
			}

			pQueryInfo->Release();
		}
		pShellFolder->Release();
	}

	return hr;
}

void AddGripperStyle(UINT *fStyle,BOOL bAddGripper)
{
	if(bAddGripper)
	{
		/* Remove the no-gripper style (if present). */
		if((*fStyle & RBBS_NOGRIPPER) == RBBS_NOGRIPPER)
			*fStyle &= ~RBBS_NOGRIPPER;

		/* Only add the gripper style if it isn't already present. */
		if((*fStyle & RBBS_GRIPPERALWAYS) != RBBS_GRIPPERALWAYS)
			*fStyle |= RBBS_GRIPPERALWAYS;
	}
	else
	{
		/* Remove the gripper style (if present). */
		if((*fStyle & RBBS_GRIPPERALWAYS) == RBBS_GRIPPERALWAYS)
			*fStyle &= ~RBBS_GRIPPERALWAYS;

		/* Only add the gripper style if it isn't already present. */
		if((*fStyle & RBBS_NOGRIPPER) != RBBS_NOGRIPPER)
			*fStyle |= RBBS_NOGRIPPER;
	}
}

/* Adds or removes the specified window
style from a window. */
void AddWindowStyle(HWND hwnd,UINT fStyle,BOOL bAdd)
{
	LONG_PTR fCurrentStyle;

	fCurrentStyle = GetWindowLongPtr(hwnd,GWL_STYLE);

	if(bAdd)
	{
		/* Only add the style if it isn't already present. */
		if((fCurrentStyle & fStyle) != fStyle)
			fCurrentStyle |= fStyle;
	}
	else
	{
		/* Only remove the style if it is present. */
		if((fCurrentStyle & fStyle) == fStyle)
			fCurrentStyle &= ~fStyle;
	}

	SetWindowLongPtr(hwnd,GWL_STYLE,fCurrentStyle);
}

DWORD GetCurrentProcessImageName(TCHAR *szImageName,DWORD nSize)
{
	HANDLE	hProcess;
	DWORD	dwProcessId;
	DWORD	dwRet = 0;

	dwProcessId = GetCurrentProcessId();
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,dwProcessId);

	if(hProcess != NULL)
	{
		dwRet = GetModuleFileNameEx(hProcess,NULL,szImageName,nSize);
		CloseHandle(hProcess);
	}

	return dwRet;
}

WORD GetFileLanguage(TCHAR *szFullFileName)
{
	LANGANDCODEPAGE	*plcp = NULL;
	DWORD			dwLen;
	DWORD			dwHandle;
	WORD			wLanguage = 0;
	UINT			uLen;
	void			*pTranslateInfo = NULL;

	dwLen = GetFileVersionInfoSize(szFullFileName,&dwHandle);

	if(dwLen > 0)
	{
		pTranslateInfo = malloc(dwLen);

		if(pTranslateInfo != NULL)
		{
			GetFileVersionInfo(szFullFileName,NULL,dwLen,pTranslateInfo);
			VerQueryValue(pTranslateInfo,_T("\\VarFileInfo\\Translation"),
				(LPVOID *)&plcp,&uLen);

			if(uLen >= sizeof(LANGANDCODEPAGE))
				wLanguage = PRIMARYLANGID(plcp[0].wLanguage);

			free(pTranslateInfo);
		}
	}

	return wLanguage;
}

BOOL GetFileProductVersion(TCHAR *szFullFileName,
DWORD *pdwProductVersionLS,DWORD *pdwProductVersionMS)
{
	VS_FIXEDFILEINFO	*pvsffi = NULL;
	DWORD			dwLen;
	DWORD			dwHandle;
	UINT			uLen;
	BOOL			bSuccess = FALSE;
	void			*pData = NULL;

	*pdwProductVersionLS = 0;
	*pdwProductVersionMS = 0;

	dwLen = GetFileVersionInfoSize(szFullFileName,&dwHandle);

	if(dwLen > 0)
	{
		pData = malloc(dwLen);

		if(pData != NULL)
		{
			GetFileVersionInfo(szFullFileName,NULL,dwLen,pData);
			VerQueryValue(pData,_T("\\"),
				(LPVOID *)&pvsffi,&uLen);

			/* To retrieve the product version numbers:
			HIWORD(pvsffi->dwProductVersionMS);
			LOWORD(pvsffi->dwProductVersionMS);
			HIWORD(pvsffi->dwProductVersionLS);
			LOWORD(pvsffi->dwProductVersionLS); */

			if(uLen > 0)
			{
				*pdwProductVersionLS = pvsffi->dwProductVersionMS;
				*pdwProductVersionMS = pvsffi->dwProductVersionLS;

				bSuccess = TRUE;
			}

			free(pData);
		}
	}

	return bSuccess;
}

void GetCPUBrandString(char *pszCPUBrand,UINT cchBuf)
{
	int CPUInfo[4] = {-1};
	char szCPUBrand[64];

	/* Refer to cpuid documentation at:
	ms-help://MS.VSCC.v90/MS.MSDNQTR.v90.en/dv_vclang/html/f8c344d3-91bf-405f-8622-cb0e337a6bdc.htm */
	__cpuid(CPUInfo,0x80000002);
	memcpy(szCPUBrand,CPUInfo,sizeof(CPUInfo));
	__cpuid(CPUInfo,0x80000003);
	memcpy(szCPUBrand + 16,CPUInfo,sizeof(CPUInfo));
	__cpuid(CPUInfo,0x80000004);
	memcpy(szCPUBrand + 32,CPUInfo,sizeof(CPUInfo));

	StringCchCopyA(pszCPUBrand,cchBuf,szCPUBrand);
}

void ReplaceCharacterWithString(TCHAR *szBaseString,TCHAR *szOutput,
UINT cchMax,TCHAR chToReplace,TCHAR *szReplacement)
{
	TCHAR szNewString[1024];
	int iBase = 0;
	int i = 0;

	szNewString[0] = '\0';
	for(i = 0;i < lstrlen(szBaseString);i++)
	{
		if(szBaseString[i] == '&')
		{
			StringCchCatN(szNewString,SIZEOF_ARRAY(szNewString),
				&szBaseString[iBase],i - iBase);
			StringCchCat(szNewString,SIZEOF_ARRAY(szNewString),szReplacement);

			iBase = i + 1;
		}
	}

	StringCchCatN(szNewString,SIZEOF_ARRAY(szNewString),
		&szBaseString[iBase],i - iBase);

	StringCchCopy(szOutput,cchMax,szNewString);
}

/* Centers one window (hChild) with respect to
another (hParent), as per the Windows UX
Guidelines (2009).
This means placing the child window 45% of the
way from the top of the parent window (with 55%
of the space left between the bottom of the
child window and the bottom of the parent window).*/
void CenterWindow(HWND hParent,HWND hChild)
{
	RECT rcParent;
	RECT rcChild;
	POINT ptOrigin;

	GetClientRect(hParent,&rcParent);
	GetClientRect(hChild,&rcChild);

	/* Take the offset between the two windows, and map it back to the
	desktop. */
	ptOrigin.x = (GetRectWidth(&rcParent) - GetRectWidth(&rcChild)) / 2;
	ptOrigin.y = (LONG)((GetRectHeight(&rcParent) - GetRectHeight(&rcChild)) * 0.45);
	MapWindowPoints(hParent,HWND_DESKTOP,&ptOrigin,1);

	SetWindowPos(hChild,NULL,ptOrigin.x,ptOrigin.y,
		0,0,SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOZORDER);
}

TCHAR *ReplaceSubString(TCHAR *szString,TCHAR *szSubString,TCHAR *szReplacement)
{
	static TCHAR szDest[1024];
	TCHAR szTemp[1024];
	TCHAR *pSub = NULL;

	StringCchCopy(szDest,SIZEOF_ARRAY(szDest),szString);

	while((pSub = StrStr(szDest,szSubString)) != NULL)
	{
		StringCchCopy(szTemp,SIZEOF_ARRAY(szTemp),szDest);

		StringCchCopyN(szDest,SIZEOF_ARRAY(szDest),szTemp,
			pSub - szDest);
		StringCchCat(szDest,SIZEOF_ARRAY(szDest),szReplacement);
		StringCchCat(szDest,SIZEOF_ARRAY(szDest),&szTemp[pSub - szDest + lstrlen(szSubString)]);
	}

	return szDest;
}

HRESULT GetMediaMetadata(TCHAR *szFileName,LPCWSTR szAttribute,BYTE **pszOutput)
{
	typedef HRESULT (WINAPI *WMCREATEEDITOR_PROC)(IWMMetadataEditor **);
	WMCREATEEDITOR_PROC pWMCreateEditor = NULL;
	HMODULE hWMVCore;
	IWMMetadataEditor *pEditor = NULL;
	IWMHeaderInfo *pWMHeaderInfo = NULL;
	HRESULT hr = E_FAIL;

	hWMVCore = LoadLibrary(_T("wmvcore.dll"));

	if(hWMVCore != NULL)
	{
		pWMCreateEditor = (WMCREATEEDITOR_PROC)GetProcAddress(hWMVCore,"WMCreateEditor");

		if(pWMCreateEditor != NULL)
		{
			hr = pWMCreateEditor(&pEditor);

			if(SUCCEEDED(hr))
			{
				hr = pEditor->Open(szFileName);

				if(SUCCEEDED(hr))
				{
					hr = pEditor->QueryInterface(IID_IWMHeaderInfo,(void **)&pWMHeaderInfo);

					if(SUCCEEDED(hr))
					{
						WORD wStreamNum;
						WMT_ATTR_DATATYPE Type;
						WORD cbLength;

						/* Any stream. Should be zero for MP3 files. */
						wStreamNum = 0;

						hr = pWMHeaderInfo->GetAttributeByName(&wStreamNum,szAttribute,&Type,NULL,&cbLength);

						if(SUCCEEDED(hr))
						{
							*pszOutput = (BYTE *)malloc(cbLength * sizeof(TCHAR));

							if(*pszOutput != NULL)
							{
								hr = pWMHeaderInfo->GetAttributeByName(&wStreamNum,szAttribute,&Type,
									*pszOutput,&cbLength);
							}
						}

						pWMHeaderInfo->Release();
					}
				}

				pEditor->Release();
			}
		}

		FreeLibrary(hWMVCore);
	}

	return hr;
}

void ListView_ActivateOneClickSelect(HWND hListView,BOOL bActivate,UINT HoverTime)
{
	DWORD dwExtendedStyle;
	UINT dwStyle;

	/* These three styles are used to control one-click
	selection. */
	dwStyle = 0;

	dwExtendedStyle = ListView_GetExtendedListViewStyle(hListView);

	if(bActivate)
	{
		if(!(dwExtendedStyle & LVS_EX_TRACKSELECT))
			dwStyle |= LVS_EX_TRACKSELECT;

		if(!(dwExtendedStyle & LVS_EX_ONECLICKACTIVATE))
			dwStyle |= LVS_EX_ONECLICKACTIVATE;

		if(!(dwExtendedStyle & LVS_EX_UNDERLINEHOT))
			dwStyle |= LVS_EX_UNDERLINEHOT;

		ListView_SetExtendedListViewStyle(hListView,
			dwExtendedStyle|dwStyle);

		ListView_SetHoverTime(hListView,HoverTime);
	}
	else
	{
		if(dwExtendedStyle & LVS_EX_TRACKSELECT)
			dwStyle |= LVS_EX_TRACKSELECT;

		if(dwExtendedStyle & LVS_EX_ONECLICKACTIVATE)
			dwStyle |= LVS_EX_ONECLICKACTIVATE;

		if(dwExtendedStyle & LVS_EX_UNDERLINEHOT)
			dwStyle |= LVS_EX_UNDERLINEHOT;

		ListView_SetExtendedListViewStyle(hListView,
			dwExtendedStyle&~dwStyle);
	}
}

void UpdateToolbarBandSizing(HWND hRebar,HWND hToolbar)
{
	REBARBANDINFO rbbi;
	SIZE sz;
	int nBands;
	int iBand = -1;
	int i = 0;

	nBands = (int)SendMessage(hRebar,RB_GETBANDCOUNT,0,0);

	for(i = 0;i < nBands;i++)
	{
		rbbi.cbSize	= sizeof(rbbi);
		rbbi.fMask	= RBBIM_CHILD;
		SendMessage(hRebar,RB_GETBANDINFO,i,(LPARAM)&rbbi);

		if(rbbi.hwndChild == hToolbar)
		{
			iBand = i;
			break;
		}
	}

	if(iBand != -1)
	{
		SendMessage(hToolbar,TB_GETMAXSIZE,0,(LPARAM)&sz);

		rbbi.cbSize		= sizeof(rbbi);
		rbbi.fMask		= RBBIM_SIZE|RBBIM_IDEALSIZE;
		rbbi.cx			= sz.cx;
		rbbi.cxIdeal	= sz.cx;
		SendMessage(hRebar,RB_SETBANDINFO,iBand,(LPARAM)&rbbi);
	}
}

void MergeDateTime(SYSTEMTIME *pstOutput,SYSTEMTIME *pstDate,SYSTEMTIME *pstTime)
{
	/* Date fields. */
	pstOutput->wYear		= pstDate->wYear;
	pstOutput->wMonth		= pstDate->wMonth;
	pstOutput->wDayOfWeek	= pstDate->wDayOfWeek;
	pstOutput->wDay			= pstDate->wDay;

	/* Time fields. */
	pstOutput->wHour			= pstTime->wHour;
	pstOutput->wMinute			= pstTime->wMinute;
	pstOutput->wSecond			= pstTime->wSecond;
	pstOutput->wMilliseconds	= pstTime->wMilliseconds;
}

void GetWindowString(HWND hwnd,std::wstring &str)
{
	int iLen = GetWindowTextLength(hwnd);

	TCHAR *szTemp = new TCHAR[iLen + 1];
	GetWindowText(hwnd,szTemp,iLen + 1);

	str = szTemp;

	delete[] szTemp;
}