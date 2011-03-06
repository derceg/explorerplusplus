/******************************************************************
 *
 * Project: ShellBrowser
 * File: ColumnManager.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the columns in details view.
 *
 * Notes:
 *  - Column widths need to save when:
 *     - Switching to a different folder type
 *     - Swapping columns (i.e. checking/unchecking columns)
 *     - Exiting the program
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <gdiplus.h>
#include <list>
#include "IShellView.h"
#include "iShellBrowser_internal.h"
#include "../Helper/Helper.h"
#include "../Helper/Buffer.h"
#include "../Helper/FolderSize.h"


using namespace std;

#define COLUMN_TIME_MODIFIED	0
#define COLUMN_TIME_CREATED		1
#define COLUMN_TIME_ACCESSED	2

#define MEDIAMETADATA_TYPE_BITRATE				0
#define MEDIAMETADATA_TYPE_COPYRIGHT			1
#define MEDIAMETADATA_TYPE_DURATION				2
#define MEDIAMETADATA_TYPE_PROTECTED			3
#define MEDIAMETADATA_TYPE_RATING				4
#define MEDIAMETADATA_TYPE_ALBUMARTIST			5
#define MEDIAMETADATA_TYPE_ALBUM				6
#define MEDIAMETADATA_TYPE_BEATSPERMINUTE		7
#define MEDIAMETADATA_TYPE_COMPOSER				8
#define MEDIAMETADATA_TYPE_CONDUCTOR			9
#define MEDIAMETADATA_TYPE_DIRECTOR				10
#define MEDIAMETADATA_TYPE_GENRE				11
#define MEDIAMETADATA_TYPE_LANGUAGE				12
#define MEDIAMETADATA_TYPE_BROADCASTDATE		13
#define MEDIAMETADATA_TYPE_CHANNEL				14
#define MEDIAMETADATA_TYPE_STATIONNAME			15
#define MEDIAMETADATA_TYPE_MOOD					16
#define MEDIAMETADATA_TYPE_PARENTALRATING		17
#define MEDIAMETADATA_TYPE_PARENTALRATINGREASON	18
#define MEDIAMETADATA_TYPE_PERIOD				19
#define MEDIAMETADATA_TYPE_PRODUCER				20
#define MEDIAMETADATA_TYPE_PUBLISHER			21
#define MEDIAMETADATA_TYPE_WRITER				22
#define MEDIAMETADATA_TYPE_YEAR					23

void CFolderView::SetColumnData(unsigned int ColumnId,int iItem,int iColumnIndex)
{
	switch(ColumnId)
	{
		case CM_NAME:
			SetNameColumnData(m_hListView,iItem,iColumnIndex);
			break;

		case CM_TYPE:
			SetTypeColumnData(m_hListView,iItem,iColumnIndex);
			break;
		case CM_SIZE:
			SetSizeColumnData(m_hListView,iItem,iColumnIndex);
			break;

		case CM_DATEMODIFIED:
			SetTimeColumnData(m_hListView,iItem,iColumnIndex,COLUMN_TIME_MODIFIED);
			break;
		case CM_CREATED:
			SetTimeColumnData(m_hListView,iItem,iColumnIndex,COLUMN_TIME_CREATED);
			break;
		case CM_ACCESSED:
			SetTimeColumnData(m_hListView,iItem,iColumnIndex,COLUMN_TIME_ACCESSED);
			break;

		case CM_ATTRIBUTES:
			SetAttributeColumnData(m_hListView,iItem,iColumnIndex);
			break;
		case CM_REALSIZE:
			SetRealSizeColumnData(m_hListView,iItem,iColumnIndex);
			break;
		case CM_SHORTNAME:
			SetShortNameColumnData(m_hListView,iItem,iColumnIndex);
			break;
		case CM_OWNER:
			SetOwnerColumnData(m_hListView,iItem,iColumnIndex);
			break;

		case CM_PRODUCTNAME:
			SetVersionColumnData(m_hListView,iItem,iColumnIndex,_T("ProductName"));
			break;
		case CM_COMPANY:
			SetVersionColumnData(m_hListView,iItem,iColumnIndex,_T("CompanyName"));
			break;
		case CM_DESCRIPTION:
			SetVersionColumnData(m_hListView,iItem,iColumnIndex,_T("FileDescription"));
			break;
		case CM_FILEVERSION:
			SetVersionColumnData(m_hListView,iItem,iColumnIndex,_T("FileVersion"));
			break;
		case CM_PRODUCTVERSION:
			SetVersionColumnData(m_hListView,iItem,iColumnIndex,_T("ProductVersion"));
			break;

		case CM_SHORTCUTTO:
			SetShortcutColumnData(m_hListView,iItem,iColumnIndex);
			break;
		case CM_HARDLINKS:
			SetHardLinksColumnData(m_hListView,iItem,iColumnIndex);
			break;
		case CM_EXTENSION:
			SetExtensionColumnData(m_hListView,iItem,iColumnIndex);
			break;

		case CM_TITLE:
			SetSummaryColumnData(m_hListView,iItem,iColumnIndex,PROPERTY_ID_TITLE);
			break;
		case CM_SUBJECT:
			SetSummaryColumnData(m_hListView,iItem,iColumnIndex,PROPERTY_ID_SUBJECT);
			break;
		case CM_AUTHOR:
			SetSummaryColumnData(m_hListView,iItem,iColumnIndex,PROPERTY_ID_AUTHOR);
			break;
		case CM_KEYWORDS:
			SetSummaryColumnData(m_hListView,iItem,iColumnIndex,PROPERTY_ID_KEYWORDS);
			break;
		case CM_COMMENT:
			SetSummaryColumnData(m_hListView,iItem,iColumnIndex,PROPERTY_ID_COMMENT);
			break;

		case CM_CAMERAMODEL:
			SetImageColumnData(m_hListView,iItem,iColumnIndex,PropertyTagEquipModel);
			break;
		case CM_DATETAKEN:
			SetImageColumnData(m_hListView,iItem,iColumnIndex,PropertyTagExifDTOrig);
			break;
		case CM_WIDTH:
			SetImageColumnData(m_hListView,iItem,iColumnIndex,PropertyTagImageWidth);
			break;
		case CM_HEIGHT:
			SetImageColumnData(m_hListView,iItem,iColumnIndex,PropertyTagImageHeight);
			break;

		case CM_VIRTUALCOMMENTS:
			SetControlPanelComments(iItem,iColumnIndex);
			break;

		case CM_VIRTUALTYPE:
			SetVirtualTypeColumnData(iItem,iColumnIndex);
			break;

		case CM_TOTALSIZE:
			SetTotalSizeColumnData(iItem,iColumnIndex,TRUE);
			break;

		case CM_FREESPACE:
			SetTotalSizeColumnData(iItem,iColumnIndex,FALSE);
			break;

		case CM_FILESYSTEM:
			SetFileSystemColumnData(iItem,iColumnIndex);
			break;

		case CM_NUMPRINTERDOCUMENTS:
			SetNumPrinterDocumentsColumnData(iItem,iColumnIndex);
			break;

		case CM_PRINTERSTATUS:
			SetPrinterStatusColumnData(iItem,iColumnIndex);
			break;

		case CM_PRINTERCOMMENTS:
			SetPrinterCommentsColumnData(iItem,iColumnIndex);
			break;

		case CM_PRINTERLOCATION:
			SetPrinterLocationColumnData(iItem,iColumnIndex);
			break;

		case CM_PRINTERMODEL:
			SetPrinterModelColumnData(iItem,iColumnIndex);
			break;

		case CM_NETWORKADAPTER_STATUS:
			SetNetworkAdapterStatusColumnData(iItem,iColumnIndex);
			break;

		case CM_MEDIA_BITRATE:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_BITRATE);
			break;

		case CM_MEDIA_COPYRIGHT:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_COPYRIGHT);
			break;

		case CM_MEDIA_DURATION:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_DURATION);
			break;

		case CM_MEDIA_PROTECTED:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_PROTECTED);
			break;

		case CM_MEDIA_RATING:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_RATING);
			break;

		case CM_MEDIA_ALBUMARTIST:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_ALBUMARTIST);
			break;

		case CM_MEDIA_ALBUM:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_ALBUM);
			break;

		case CM_MEDIA_BEATSPERMINUTE:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_BEATSPERMINUTE);
			break;

		case CM_MEDIA_COMPOSER:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_COMPOSER);
			break;

		case CM_MEDIA_CONDUCTOR:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_CONDUCTOR);
			break;

		case CM_MEDIA_DIRECTOR:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_DIRECTOR);
			break;

		case CM_MEDIA_GENRE:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_GENRE);
			break;

		case CM_MEDIA_LANGUAGE:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_LANGUAGE);
			break;

		case CM_MEDIA_BROADCASTDATE:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_BROADCASTDATE);
			break;

		case CM_MEDIA_CHANNEL:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_CHANNEL);
			break;

		case CM_MEDIA_STATIONNAME:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_STATIONNAME);
			break;

		case CM_MEDIA_MOOD:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_MOOD);
			break;

		case CM_MEDIA_PARENTALRATING:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_PARENTALRATING);
			break;

		case CM_MEDIA_PARENTALRATINGREASON:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_PARENTALRATINGREASON);
			break;

		case CM_MEDIA_PERIOD:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_PERIOD);
			break;

		case CM_MEDIA_PRODUCER:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_PRODUCER);
			break;

		case CM_MEDIA_PUBLISHER:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_PUBLISHER);
			break;

		case CM_MEDIA_WRITER:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_WRITER);
			break;

		case CM_MEDIA_YEAR:
			SetMediaStatusColumnData(iItem,iColumnIndex,MEDIAMETADATA_TYPE_YEAR);
			break;
	}
}

void CALLBACK SetAllColumnDataAPC(ULONG_PTR dwParam)
{
	CFolderView *pFolderView;

	pFolderView = (CFolderView *)dwParam;

	pFolderView->SetAllColumnData();
}

/* Queueing model:
When first browsing into a folder, all items in queue
are cleared.
Then, all new items are added.
Finally, the APC is queued to the worker thread.

The worker thread will process items, removing items
from the queue as it does. Interlocking is required
between queue removal and emptying the queue.

Folder sizes are NOT calculated here. They are done
within a separate thread called from the main thread. */
void CFolderView::AddToColumnQueue(int iItem)
{
	m_pColumnInfoList.push_back(iItem);
}

void CFolderView::EmptyColumnQueue(void)
{
	EnterCriticalSection(&m_column_cs);

	m_pColumnInfoList.clear();

	LeaveCriticalSection(&m_column_cs);
}

BOOL CFolderView::RemoveFromColumnQueue(int *iItem)
{
	BOOL bQueueNotEmpty;

	EnterCriticalSection(&m_column_cs);

	if(m_pColumnInfoList.empty() == TRUE)
	{
		SetEvent(m_hColumnQueueEvent);
		bQueueNotEmpty = FALSE;
	}
	else
	{
		list<int>::iterator itr;

		itr = m_pColumnInfoList.begin();

		*iItem = *itr;

		ResetEvent(m_hColumnQueueEvent);

		m_pColumnInfoList.erase(itr);

		bQueueNotEmpty = TRUE;
	}

	LeaveCriticalSection(&m_column_cs);

	return bQueueNotEmpty;
}

int CFolderView::SetAllColumnData(void)
{
	list<Column_t>				pActiveColumnList;
	list<Column_t>::iterator	itr;
	BOOL						bQueueNotEmpty;
	int							iItem;
	int							iColumnIndex = 0;

	pActiveColumnList = *m_pActiveColumnList;

	bQueueNotEmpty = RemoveFromColumnQueue(&iItem);

	while(bQueueNotEmpty)
	{
		if(m_bBrowsing)
		{
			goto end;
			break;
		}

		for(itr = pActiveColumnList.begin();itr != pActiveColumnList.end();itr++)
		{
			if(m_bBrowsing)
			{
				goto end;
				break;
			}

			if(itr->bChecked)
			{
				SetColumnData(itr->id,iItem,iColumnIndex++);
			}
		}
		iColumnIndex = 0;

		bQueueNotEmpty = RemoveFromColumnQueue(&iItem);
	}

	ApplyHeaderSortArrow();

end:

	return 1;
}

void CFolderView::AddToFolderQueue(int iItem)
{
	m_pFolderInfoList.push_back(iItem);
}

void CFolderView::EmptyFolderQueue(void)
{
	EnterCriticalSection(&m_folder_cs);

	m_pFolderInfoList.clear();

	LeaveCriticalSection(&m_folder_cs);
}

BOOL CFolderView::RemoveFromFolderQueue(int *iItem)
{
	BOOL bQueueNotEmpty;

	EnterCriticalSection(&m_folder_cs);

	SetEvent(m_hFolderQueueEvent);

	if(m_pFolderInfoList.empty() == TRUE)
	{
		bQueueNotEmpty = FALSE;
	}
	else
	{
		list<int>::iterator itr;

		itr = m_pFolderInfoList.end();

		itr--;

		*iItem = *itr;

		ResetEvent(m_hFolderQueueEvent);

		m_pFolderInfoList.erase(itr);

		bQueueNotEmpty = TRUE;
	}

	LeaveCriticalSection(&m_folder_cs);

	return bQueueNotEmpty;
}

void CALLBACK SetAllFolderSizeColumnDataAPC(ULONG_PTR dwParam)
{
	CFolderView *pFolderView;

	pFolderView = (CFolderView *)dwParam;

	pFolderView->SetAllFolderSizeColumnData();
}

int CFolderView::SetAllFolderSizeColumnData(void)
{
	list<Column_t>::iterator itr;
	LVITEM lvItem;
	BOOL bQueueNotEmpty;
	int iItem;
	int iColumnIndex = 0;

	bQueueNotEmpty = RemoveFromFolderQueue(&iItem);

	while(bQueueNotEmpty)
	{
		lvItem.mask		= LVIF_PARAM;
		lvItem.iItem	= iItem;
		lvItem.iSubItem	= 0;
		ListView_GetItem(m_hListView,&lvItem);

		if((m_pwfdFiles[(int)lvItem.lParam].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
			FILE_ATTRIBUTE_DIRECTORY)
		{
			for(itr = m_pActiveColumnList->begin();itr != m_pActiveColumnList->end();itr++)
			{
				if(itr->bChecked && itr->id == CM_SIZE)
				{
					TCHAR			FullItemPath[MAX_PATH];
					TCHAR			lpszFileSize[32];
					ULARGE_INTEGER	lTotalFolderSize;
					int				nFolders;
					int				nFiles;

					LVITEM lvItem;
					int iItemInternal;
					BOOL bRes;

					lvItem.mask		= LVIF_PARAM;
					lvItem.iItem	= iItem;
					lvItem.iSubItem	= 0;
					bRes = ListView_GetItem(m_hListView,&lvItem);

					if(bRes)
					{
						iItemInternal = (int)lvItem.lParam;

						QueryFullItemName(iItem,FullItemPath);

						CalculateFolderSize(FullItemPath,&nFolders,&nFiles,&lTotalFolderSize);

						/* Does the item still exist? */
						/* TODO: Need to lock this against the main thread. */
						/* TODO: Discard the result if the folder was deleted. */
						if(m_pItemMap[(int)lvItem.lParam] == 1)
						{
							m_pwfdFiles[(int)lvItem.lParam].nFileSizeLow = lTotalFolderSize.LowPart;
							m_pwfdFiles[(int)lvItem.lParam].nFileSizeHigh = lTotalFolderSize.HighPart;
							m_pExtraItemInfo[(int)lvItem.lParam].bFolderSizeRetrieved = TRUE;

							FormatSizeString(lTotalFolderSize,lpszFileSize,SIZEOF_ARRAY(lpszFileSize),
								m_bForceSize,m_SizeDisplayFormat);

							ListView_SetItemText(m_hListView,iItem,iColumnIndex,lpszFileSize);
						}
					}
				}

				if(itr->bChecked)
				{
					iColumnIndex++;
				}
			}
		}

		iColumnIndex = 0;

		bQueueNotEmpty = RemoveFromFolderQueue(&iItem);
	}

	ApplyHeaderSortArrow();

	return 0;
}

int CFolderView::SetNameColumnData(HWND hListView,int iItem,int iColumn)
{
	LVITEM	File;
	BOOL	bItem;

	File.mask		= LVIF_PARAM;
	File.iSubItem	= 0;
	File.iItem		= iItem;
	bItem = ListView_GetItem(hListView,&File);

	if(bItem)
	{
		ListView_SetItemText(hListView,iItem,iColumn,
			ProcessItemFileName((int)File.lParam));
	}

	return m_nTotalItems;
}

int CFolderView::SetSizeColumnData(HWND hListView,int iItem,int iColumn)
{
	LVITEM			lvItem;
	TCHAR			lpszFileSize[32];
	ULARGE_INTEGER	lFileSize;
	BOOL			bItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iSubItem	= 0;
	lvItem.iItem	= iItem;
	bItem = ListView_GetItem(hListView,&lvItem);

	if(bItem)
	{
		/* Is this item a file or a folder? */
		if((m_pwfdFiles[(int)lvItem.lParam].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
			FILE_ATTRIBUTE_DIRECTORY)
		{
			lFileSize.LowPart = m_pwfdFiles[(int)lvItem.lParam].nFileSizeLow;
			lFileSize.HighPart = m_pwfdFiles[(int)lvItem.lParam].nFileSizeHigh;

			FormatSizeString(lFileSize,lpszFileSize,SIZEOF_ARRAY(lpszFileSize),
				m_bForceSize,m_SizeDisplayFormat);

			ListView_SetItemText(hListView,iItem,iColumn,lpszFileSize);
		}
	}

	return m_nTotalItems;
}

int CFolderView::SetRealSizeColumnData(HWND hListView,int iItem,int iColumn)
{
	LVITEM File;
	ULARGE_INTEGER lRealFileSize;
	DWORD ClusterSize;
	DWORD RealFileSize;
	TCHAR Root[MAX_PATH];
	TCHAR lpszFileSize[32];
	BOOL bItem;

	File.mask		= LVIF_PARAM;
	File.iItem		= iItem;
	File.iSubItem	= 0;
	bItem = ListView_GetItem(hListView,&File);

	if(bItem)
	{
		StringCchCopy(Root,SIZEOF_ARRAY(Root),m_CurDir);
		PathStripToRoot(Root);

		if((m_pwfdFiles[(int)File.lParam].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
			FILE_ATTRIBUTE_DIRECTORY)
		{
			StringCchCopy(lpszFileSize,SIZEOF_ARRAY(lpszFileSize),EMPTY_STRING);
		}
		else
		{
			ClusterSize = GetClusterSize(Root);

			RealFileSize = m_pwfdFiles[(int)File.lParam].nFileSizeLow;

			if(RealFileSize != 0 && (RealFileSize % ClusterSize) != 0)
				RealFileSize += ClusterSize - (RealFileSize % ClusterSize);

			lRealFileSize.LowPart = RealFileSize;
			lRealFileSize.HighPart = m_pwfdFiles[(int)File.lParam].nFileSizeHigh;

			FormatSizeString(lRealFileSize,lpszFileSize,SIZEOF_ARRAY(lpszFileSize),
				m_bForceSize,m_SizeDisplayFormat);
		}

		ListView_SetItemText(hListView,iItem,iColumn,lpszFileSize);
	}

	return m_nTotalItems;
}

int CFolderView::SetTypeColumnData(HWND hListView,int iItem,int iColumn)
{
	SHFILEINFO shfi;
	TCHAR FullFileName[MAX_PATH];

	QueryFullItemName(iItem,FullFileName);

	SHGetFileInfo(FullFileName,0,
	&shfi,sizeof(SHFILEINFO),SHGFI_TYPENAME);

	ListView_SetItemText(m_hListView,iItem,iColumn,shfi.szTypeName);

	return m_nTotalItems;
}

void CFolderView::SetVirtualTypeColumnData(int iItem,int iColumn)
{
	LPITEMIDLIST pidlComplete	= NULL;
	LVITEM lvItem;
	SHFILEINFO shfi;
	BOOL bItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	bItem = ListView_GetItem(m_hListView,&lvItem);

	if(bItem)
	{
		pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)lvItem.lParam].pridl);

		SHGetFileInfo((LPTSTR)pidlComplete,0,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_TYPENAME);

		ListView_SetItemText(m_hListView,iItem,iColumn,shfi.szTypeName);

		CoTaskMemFree(pidlComplete);
	}
}

void CFolderView::SetTotalSizeColumnData(int iItem,int iColumn,BOOL bTotalSize)
{
	LPITEMIDLIST pidlComplete	= NULL;
	IShellFolder *pShellFolder	= NULL;
	LPITEMIDLIST pidlRelative	= NULL;
	ULARGE_INTEGER nTotalBytes;
	ULARGE_INTEGER nFreeBytes;
	LVITEM File;
	TCHAR szItem[MAX_PATH];
	TCHAR szSizeBuf[32];
	STRRET str;
	BOOL bRoot;
	BOOL bItem;

	File.mask		= LVIF_PARAM;
	File.iItem		= iItem;
	File.iSubItem	= 0;
	bItem = ListView_GetItem(m_hListView,&File);

	if(bItem)
	{
		pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)File.lParam].pridl);

		SHBindToParent(pidlComplete,IID_IShellFolder,
			(void **)&pShellFolder,(LPCITEMIDLIST *)&pidlRelative);

		pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
		StrRetToBuf(&str,pidlRelative,szItem,SIZEOF_ARRAY(szItem));

		bRoot = PathIsRoot(szItem);

		if(bRoot)
		{
			BOOL bRes;

			bRes = GetDiskFreeSpaceEx(szItem,NULL,&nTotalBytes,&nFreeBytes);

			if(bRes)
			{
				if(bTotalSize)
				{
					FormatSizeString(nTotalBytes,szSizeBuf,
						SIZEOF_ARRAY(szSizeBuf),m_bForceSize,m_SizeDisplayFormat);
				}
				else
				{
					FormatSizeString(nFreeBytes,szSizeBuf,
						SIZEOF_ARRAY(szSizeBuf),m_bForceSize,m_SizeDisplayFormat);
				}
			}
			else
			{
				StringCchCopy(szSizeBuf,SIZEOF_ARRAY(szSizeBuf),EMPTY_STRING);
			}

			ListView_SetItemText(m_hListView,iItem,iColumn,szSizeBuf);
		}
		else
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,EMPTY_STRING);
		}

		pShellFolder->Release();
		CoTaskMemFree(pidlComplete);
	}
}

void CFolderView::SetFileSystemColumnData(int iItem,int iColumn)
{
	LPITEMIDLIST pidlComplete	= NULL;
	IShellFolder *pShellFolder	= NULL;
	LPITEMIDLIST pidlRelative	= NULL;
	LVITEM File;
	TCHAR szItem[MAX_PATH];
	STRRET str;
	BOOL bRoot;
	BOOL bItem;

	File.mask		= LVIF_PARAM;
	File.iItem		= iItem;
	File.iSubItem	= 0;
	bItem = ListView_GetItem(m_hListView,&File);

	if(bItem)
	{
		pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[(int)File.lParam].pridl);

		SHBindToParent(pidlComplete,IID_IShellFolder,
			(void **)&pShellFolder,(LPCITEMIDLIST *)&pidlRelative);

		pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
		StrRetToBuf(&str,pidlRelative,szItem,SIZEOF_ARRAY(szItem));

		bRoot = PathIsRoot(szItem);

		if(bRoot)
		{
			TCHAR szFileSystemName[MAX_PATH];
			BOOL bRes;

			bRes = GetVolumeInformation(szItem,NULL,0,NULL,NULL,NULL,szFileSystemName,
				SIZEOF_ARRAY(szFileSystemName));

			if(!bRes)
			{
				StringCchCopy(szFileSystemName,SIZEOF_ARRAY(szFileSystemName),EMPTY_STRING);
			}

			ListView_SetItemText(m_hListView,iItem,iColumn,szFileSystemName);
		}
		else
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,EMPTY_STRING);
		}

		pShellFolder->Release();
		CoTaskMemFree(pidlComplete);
	}
}

int CFolderView::SetTimeColumnData(HWND hListView,int iItem,int iColumn,int TimeType)
{
	LVITEM File;
	TCHAR lpszFileTime[64];
	BOOL bItem;
	int iReturn = -1;

	File.mask		= LVIF_PARAM;
	File.iItem		= iItem;
	File.iSubItem	= 0;
	bItem = ListView_GetItem(hListView,&File);

	if(bItem)
	{
		switch(TimeType)
		{
		case COLUMN_TIME_MODIFIED:
			iReturn = CreateFileTimeString(&m_pwfdFiles[(int)File.lParam].ftLastWriteTime,
				lpszFileTime,SIZEOF_ARRAY(lpszFileTime),m_bShowFriendlyDates);
			break;

		case COLUMN_TIME_CREATED:
			iReturn = CreateFileTimeString(&m_pwfdFiles[(int)File.lParam].ftCreationTime,
				lpszFileTime,SIZEOF_ARRAY(lpszFileTime),m_bShowFriendlyDates);
			break;

		case COLUMN_TIME_ACCESSED:
			iReturn = CreateFileTimeString(&m_pwfdFiles[(int)File.lParam].ftLastAccessTime,
				lpszFileTime,SIZEOF_ARRAY(lpszFileTime),m_bShowFriendlyDates);
			break;
		}

		if(iReturn != -1)
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,lpszFileTime);
		}
		else
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,EMPTY_STRING);
		}
	}

	return m_nTotalItems;
}

int CFolderView::SetAttributeColumnData(HWND hListView,int iItem,int iColumn)
{
	TCHAR lpszAttributes[32];
	TCHAR FullFileName[MAX_PATH];
	HRESULT hr;

	hr = QueryFullItemName(iItem,FullFileName);

	if(SUCCEEDED(hr))
	{
		BuildFileAttributeString(FullFileName,lpszAttributes,
			SIZEOF_ARRAY(lpszAttributes));

		ListView_SetItemText(m_hListView,iItem,iColumn,lpszAttributes);
	}

	return m_nTotalItems;
}

int CFolderView::SetShortNameColumnData(HWND hListView,int iItem,int iColumn)
{
	LVITEM File;
	BOOL bItem;

	File.mask		= LVIF_PARAM;
	File.iItem		= iItem;
	File.iSubItem	= 0;
	bItem = ListView_GetItem(hListView,&File);

	if(bItem)
	{
		if(lstrlen(m_pwfdFiles[(int)File.lParam].cAlternateFileName) > 0)
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,m_pwfdFiles[(int)File.lParam].cAlternateFileName);
		}
		else
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,m_pwfdFiles[(int)File.lParam].cFileName);
		}
	}

	return m_nTotalItems;
}

int CFolderView::SetOwnerColumnData(HWND hListView,int iItem,int iColumn)
{
	TCHAR FullFileName[MAX_PATH];
	TCHAR szOwner[512] = EMPTY_STRING;
	HRESULT hr;

	hr = QueryFullItemName(iItem,FullFileName);

	if(SUCCEEDED(hr))
	{
		GetFileOwner(FullFileName,szOwner,SIZEOF_ARRAY(szOwner));

		ListView_SetItemText(m_hListView,iItem,iColumn,szOwner);
	}

	return m_nTotalItems;
}

int CFolderView::SetVersionColumnData(HWND hListView,int iItem,int iColumn,TCHAR *lpszVersion)
{
	LVITEM File;
	TCHAR FullFileName[MAX_PATH];
	TCHAR szVersionBuf[512];
	BOOL bVersionInfoObtained = FALSE;
	BOOL bItem;

	File.mask		= LVIF_PARAM;
	File.iItem		= iItem;
	File.iSubItem	= 0;
	bItem = ListView_GetItem(hListView,&File);

	if(bItem)
	{
		StringCchCopy(FullFileName,SIZEOF_ARRAY(FullFileName),m_CurDir);
		PathAppend(FullFileName,m_pwfdFiles[(int)File.lParam].cFileName);

		bVersionInfoObtained = GetVersionInfoString(FullFileName,
			lpszVersion,szVersionBuf,SIZEOF_ARRAY(szVersionBuf));

		if(bVersionInfoObtained)
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,szVersionBuf);
		}
		else
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,EMPTY_STRING);
		}
	}

	return m_nTotalItems;
}

int CFolderView::SetShortcutColumnData(HWND hListView,int iItem,int iColumn)
{
	LVITEM File;
	TCHAR FullFileName[MAX_PATH];
	TCHAR szResolvedLinkPath[MAX_PATH];
	HRESULT hr;
	BOOL bItem;

	File.mask		= LVIF_PARAM;
	File.iItem		= iItem;
	File.iSubItem	= 0;
	bItem = ListView_GetItem(hListView,&File);

	if(bItem)
	{
		StringCchCopy(FullFileName,SIZEOF_ARRAY(FullFileName),m_CurDir);
		PathAppend(FullFileName,m_pwfdFiles[(int)File.lParam].cFileName);
		hr = NFileOperations::ResolveLink(NULL,SLR_NO_UI,FullFileName,szResolvedLinkPath,SIZEOF_ARRAY(szResolvedLinkPath));

		if(SUCCEEDED(hr))
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,szResolvedLinkPath);
		}
		else
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,EMPTY_STRING);
		}
	}

	return m_nTotalItems;
}

int CFolderView::SetHardLinksColumnData(HWND hListView,int iItem,int iColumn)
{
	LVITEM File;
	TCHAR FullFileName[MAX_PATH];
	TCHAR szNumHardLinks[32];
	DWORD dwNumHardLinks;
	BOOL bItem;

	File.mask		= LVIF_PARAM;
	File.iItem		= iItem;
	File.iSubItem	= 0;
	bItem = ListView_GetItem(hListView,&File);

	if(bItem)
	{
		StringCchCopy(FullFileName,SIZEOF_ARRAY(FullFileName),m_CurDir);
		PathAppend(FullFileName,m_pwfdFiles[(int)File.lParam].cFileName);
		dwNumHardLinks = GetNumFileHardLinks(FullFileName);

		if(dwNumHardLinks == -1)
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,EMPTY_STRING);
		}
		else
		{
			StringCchPrintf(szNumHardLinks,SIZEOF_ARRAY(szNumHardLinks),_T("%ld"),dwNumHardLinks);
			ListView_SetItemText(m_hListView,iItem,iColumn,szNumHardLinks);
		}
	}

	return m_nTotalItems;
}

int CFolderView::SetExtensionColumnData(HWND hListView,int iItem,int iColumn)
{
	LVITEM File;
	TCHAR *pExt = NULL;
	BOOL bItem;

	File.mask		= LVIF_PARAM;
	File.iItem		= iItem;
	File.iSubItem	= 0;
	bItem = ListView_GetItem(hListView,&File);

	if(bItem)
	{
		pExt = PathFindExtension(m_pwfdFiles[(int)File.lParam].cFileName);

		if(*pExt != '.')
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,EMPTY_STRING);
		}
		else
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,pExt + 1);
		}
	}

	return m_nTotalItems;
}

int CFolderView::SetSummaryColumnData(HWND hListView,int iItem,int iColumn,DWORD dwPropertyType)
{
	LVITEM File;
	TCHAR szFullFileName[MAX_PATH];
	TCHAR szPropertyBuf[512];
	int iRes;
	BOOL bItem;

	File.mask		= LVIF_PARAM;
	File.iItem		= iItem;
	File.iSubItem	= 0;
	bItem = ListView_GetItem(hListView,&File);

	if(bItem)
	{
		StringCchCopy(szFullFileName,SIZEOF_ARRAY(szFullFileName),m_CurDir);
		PathAppend(szFullFileName,m_pwfdFiles[(int)File.lParam].cFileName);

		iRes = ReadFileProperty(szFullFileName,dwPropertyType,szPropertyBuf,
			SIZEOF_ARRAY(szPropertyBuf));

		if(iRes == -1)
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,EMPTY_STRING);
		}
		else
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,szPropertyBuf);
		}
	}

	return m_nTotalItems;
}

int CFolderView::SetImageColumnData(HWND hListView,int iItem,int iColumn,PROPID PropertyId)
{
	LVITEM	File;
	TCHAR	szFullFileName[MAX_PATH];
	TCHAR	szPropertyBuf[512];
	BOOL	bRes;
	BOOL	bItem;

	File.mask		= LVIF_PARAM;
	File.iItem		= iItem;
	File.iSubItem	= 0;
	bItem = ListView_GetItem(hListView,&File);

	if(bItem)
	{
		StringCchCopy(szFullFileName,SIZEOF_ARRAY(szFullFileName),m_CurDir);
		PathAppend(szFullFileName,m_pwfdFiles[(int)File.lParam].cFileName);

		bRes = ReadImageProperty(szFullFileName,PropertyId,szPropertyBuf,
			SIZEOF_ARRAY(szPropertyBuf));

		if(bRes)
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,szPropertyBuf);
		}
		else
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,EMPTY_STRING);
		}
	}

	return m_nTotalItems;
}

void CFolderView::SetControlPanelComments(int iItem,int iColumn)
{
	TCHAR	szInfoTip[512];
	HRESULT	hr;

	hr = RetrieveItemInfoTip(iItem,szInfoTip,SIZEOF_ARRAY(szInfoTip));

	if(SUCCEEDED(hr))
	{
		ReplaceCharacters(szInfoTip,'\n',' ');
		ListView_SetItemText(m_hListView,iItem,iColumn,szInfoTip);
	}
	else
	{
		ListView_SetItemText(m_hListView,iItem,iColumn,EMPTY_STRING);
	}
}

void CFolderView::SetNumPrinterDocumentsColumnData(int iItem,int iColumn)
{
	HANDLE hPrinter;
	LVITEM lvItem;
	PRINTER_INFO_2 *pPrinterInfo2;
	TCHAR szNumJobs[32]	= EMPTY_STRING;
	DWORD cbNeeded;
	DWORD cbSize;
	BOOL bRes;
	BOOL bItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	bItem = ListView_GetItem(m_hListView,&lvItem);

	if(bItem)
	{
		bRes = OpenPrinter(m_pExtraItemInfo[(int)lvItem.lParam].szDisplayName,&hPrinter,NULL);

		if(bRes)
		{
			GetPrinter(hPrinter,2,NULL,0,&cbNeeded);

			pPrinterInfo2 = (PRINTER_INFO_2 *)malloc(cbNeeded);

			cbSize = cbNeeded;

			bRes = GetPrinter(hPrinter,2,(LPBYTE)pPrinterInfo2,cbSize,&cbNeeded);

			if(bRes)
			{
				StringCchPrintf(szNumJobs,SIZEOF_ARRAY(szNumJobs),
					_T("%d"),pPrinterInfo2->cJobs);
			}

			free(pPrinterInfo2);
		}

		ListView_SetItemText(m_hListView,iItem,iColumn,szNumJobs);
	}
}

void CFolderView::SetPrinterStatusColumnData(int iItem,int iColumn)
{
	HANDLE hPrinter;
	LVITEM lvItem;
	PRINTER_INFO_2 *pPrinterInfo2;
	TCHAR szNumJobs[32]	= EMPTY_STRING;
	DWORD cbNeeded;
	DWORD cbSize;
	BOOL bRes;
	BOOL bItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	bItem = ListView_GetItem(m_hListView,&lvItem);

	if(bItem)
	{
		bRes = OpenPrinter(m_pExtraItemInfo[(int)lvItem.lParam].szDisplayName,&hPrinter,NULL);

		if(bRes)
		{
			GetPrinter(hPrinter,2,NULL,0,&cbNeeded);

			pPrinterInfo2 = (PRINTER_INFO_2 *)malloc(cbNeeded);

			cbSize = cbNeeded;

			bRes = GetPrinter(hPrinter,2,(LPBYTE)pPrinterInfo2,cbSize,&cbNeeded);

			if(bRes)
			{
				StringCchCopyEx(szNumJobs,SIZEOF_ARRAY(szNumJobs),
					DecodePrinterStatus(pPrinterInfo2->Status),NULL,NULL,STRSAFE_IGNORE_NULLS);
			}

			free(pPrinterInfo2);
			ClosePrinter(hPrinter);
		}

		ListView_SetItemText(m_hListView,iItem,iColumn,szNumJobs);
	}
}

void CFolderView::SetPrinterCommentsColumnData(int iItem,int iColumn)
{
	HANDLE hPrinter;
	LVITEM lvItem;
	PRINTER_INFO_2 *pPrinterInfo2;
	TCHAR szNumJobs[32]	= EMPTY_STRING;
	DWORD cbNeeded;
	DWORD cbSize;
	BOOL bRes;
	BOOL bItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	bItem = ListView_GetItem(m_hListView,&lvItem);

	if(bItem)
	{
		bRes = OpenPrinter(m_pExtraItemInfo[(int)lvItem.lParam].szDisplayName,&hPrinter,NULL);

		if(bRes)
		{
			GetPrinter(hPrinter,2,NULL,0,&cbNeeded);

			pPrinterInfo2 = (PRINTER_INFO_2 *)malloc(cbNeeded);

			cbSize = cbNeeded;

			bRes = GetPrinter(hPrinter,2,(LPBYTE)pPrinterInfo2,cbSize,&cbNeeded);

			if(bRes)
			{
				StringCchCopyEx(szNumJobs,SIZEOF_ARRAY(szNumJobs),
					pPrinterInfo2->pComment,NULL,NULL,STRSAFE_IGNORE_NULLS);
			}

			free(pPrinterInfo2);
		}

		ListView_SetItemText(m_hListView,iItem,iColumn,szNumJobs);
	}
}

void CFolderView::SetPrinterLocationColumnData(int iItem,int iColumn)
{
	HANDLE hPrinter;
	LVITEM lvItem;
	PRINTER_INFO_2 *pPrinterInfo2;
	TCHAR szNumJobs[32]	= EMPTY_STRING;
	DWORD cbNeeded;
	DWORD cbSize;
	BOOL bRes;
	BOOL bItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	bItem = ListView_GetItem(m_hListView,&lvItem);

	if(bItem)
	{
		bRes = OpenPrinter(m_pExtraItemInfo[(int)lvItem.lParam].szDisplayName,&hPrinter,NULL);

		if(bRes)
		{
			GetPrinter(hPrinter,2,NULL,0,&cbNeeded);

			pPrinterInfo2 = (PRINTER_INFO_2 *)malloc(cbNeeded);

			cbSize = cbNeeded;

			bRes = GetPrinter(hPrinter,2,(LPBYTE)pPrinterInfo2,cbSize,&cbNeeded);

			if(bRes)
			{
				StringCchCopyEx(szNumJobs,SIZEOF_ARRAY(szNumJobs),
					pPrinterInfo2->pLocation,NULL,NULL,STRSAFE_IGNORE_NULLS);
			}

			free(pPrinterInfo2);
		}

		ListView_SetItemText(m_hListView,iItem,iColumn,szNumJobs);
	}
}

void CFolderView::SetPrinterModelColumnData(int iItem,int iColumn)
{
	HANDLE hPrinter;
	LVITEM lvItem;
	PRINTER_INFO_2 *pPrinterInfo2;
	TCHAR szNumJobs[32]	= EMPTY_STRING;
	DWORD cbNeeded;
	DWORD cbSize;
	BOOL bRes;
	BOOL bItem;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	bItem = ListView_GetItem(m_hListView,&lvItem);

	if(bItem)
	{
		bRes = OpenPrinter(m_pExtraItemInfo[(int)lvItem.lParam].szDisplayName,&hPrinter,NULL);

		if(bRes)
		{
			GetPrinter(hPrinter,2,NULL,0,&cbNeeded);

			pPrinterInfo2 = (PRINTER_INFO_2 *)malloc(cbNeeded);

			cbSize = cbNeeded;

			bRes = GetPrinter(hPrinter,2,(LPBYTE)pPrinterInfo2,cbSize,&cbNeeded);

			if(bRes)
			{
				StringCchCopyEx(szNumJobs,SIZEOF_ARRAY(szNumJobs),
					pPrinterInfo2->pDriverName,NULL,NULL,STRSAFE_IGNORE_NULLS);
			}

			free(pPrinterInfo2);
		}

		ListView_SetItemText(m_hListView,iItem,iColumn,szNumJobs);
	}
}

void CFolderView::SetNetworkAdapterStatusColumnData(int iItem,int iColumn)
{
	TCHAR szStatus[32] = EMPTY_STRING;
	IP_ADAPTER_ADDRESSES *pAdapterAddresses = NULL;
	UINT uStatusID = 0;
	ULONG ulOutBufLen = 0;

	GetAdaptersAddresses(AF_UNSPEC,0,NULL,NULL,&ulOutBufLen);

	pAdapterAddresses = (IP_ADAPTER_ADDRESSES *)malloc(ulOutBufLen);

	GetAdaptersAddresses(AF_UNSPEC,0,NULL,pAdapterAddresses,&ulOutBufLen);

	/* TODO: These strings need to be setup correctly. */
	/*switch(pAdapterAddresses->OperStatus)
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
	}*/

	free(pAdapterAddresses);

	LoadString(m_hResourceModule,uStatusID,
		szStatus,SIZEOF_ARRAY(szStatus));

	/* TODO: Fix. */
	/*LVITEM	lvItem;
	ULONG	uAdapterIndex;
	DWORD	dwRet;

	lvItem.mask		= LVIF_PARAM;
	lvItem.iItem	= iItem;
	lvItem.iSubItem	= 0;
	ListView_GetItem(m_hListView,&lvItem);

	dwRet = GetAdapterIndex(m_pwfdFiles[(int)lvItem.lParam].cFileName,&uAdapterIndex);*/

	ListView_SetItemText(m_hListView,iItem,iColumn,szStatus);
}

void CFolderView::SetMediaStatusColumnData(int iItem,int iColumn,int iType)
{
	LVITEM	File;
	TCHAR	szFullFileName[MAX_PATH];
	TCHAR	szOutput[256];
	TCHAR	*pszTemp = NULL;
	QWORD	*pqwTemp = NULL;
	DWORD	*pdwTemp = NULL;
	HRESULT	hr = E_FAIL;
	BOOL	bItem;

	File.mask		= LVIF_PARAM;
	File.iItem		= iItem;
	File.iSubItem	= 0;
	bItem = ListView_GetItem(m_hListView,&File);

	if(bItem)
	{
		StringCchCopy(szFullFileName,SIZEOF_ARRAY(szFullFileName),m_CurDir);
		PathAppend(szFullFileName,m_pwfdFiles[(int)File.lParam].cFileName);

		switch(iType)
		{
		case MEDIAMETADATA_TYPE_BITRATE:
			hr = GetMediaMetadata(szFullFileName,g_wszWMBitrate,(BYTE **)&pdwTemp);

			if(SUCCEEDED(hr))
			{
				/* TODO: Fix (bps->Kbps). */
				StringCchPrintf(szOutput,SIZEOF_ARRAY(szOutput),_T("%d bps"),*pdwTemp);

				free(pdwTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_COPYRIGHT:
			hr = GetMediaMetadata(szFullFileName,g_wszWMCopyright,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_DURATION:
			hr = GetMediaMetadata(szFullFileName,g_wszWMDuration,(BYTE **)&pqwTemp);

			if(SUCCEEDED(hr))
			{
				/* TODO: Fix (display actual time). */
				StringCchPrintf(szOutput,SIZEOF_ARRAY(szOutput),_T("%d"),*pqwTemp);

				free(pqwTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_PROTECTED:
			/* TODO: Implement. */
			break;

		case MEDIAMETADATA_TYPE_RATING:
			hr = GetMediaMetadata(szFullFileName,g_wszWMRating,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_ALBUMARTIST:
			hr = GetMediaMetadata(szFullFileName,g_wszWMAlbumArtist,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_ALBUM:
			hr = GetMediaMetadata(szFullFileName,g_wszWMAlbumTitle,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_BEATSPERMINUTE:
			hr = GetMediaMetadata(szFullFileName,g_wszWMBeatsPerMinute,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_COMPOSER:
			hr = GetMediaMetadata(szFullFileName,g_wszWMComposer,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_CONDUCTOR:
			hr = GetMediaMetadata(szFullFileName,g_wszWMConductor,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_DIRECTOR:
			hr = GetMediaMetadata(szFullFileName,g_wszWMDirector,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_GENRE:
			hr = GetMediaMetadata(szFullFileName,g_wszWMGenre,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_LANGUAGE:
			hr = GetMediaMetadata(szFullFileName,g_wszWMLanguage,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_BROADCASTDATE:
			hr = GetMediaMetadata(szFullFileName,g_wszWMMediaOriginalBroadcastDateTime,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_CHANNEL:
			hr = GetMediaMetadata(szFullFileName,g_wszWMMediaOriginalChannel,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_STATIONNAME:
			hr = GetMediaMetadata(szFullFileName,g_wszWMMediaStationName,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_MOOD:
			hr = GetMediaMetadata(szFullFileName,g_wszWMMood,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_PARENTALRATING:
			hr = GetMediaMetadata(szFullFileName,g_wszWMParentalRating,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_PARENTALRATINGREASON:
			hr = GetMediaMetadata(szFullFileName,g_wszWMParentalRatingReason,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_PERIOD:
			hr = GetMediaMetadata(szFullFileName,g_wszWMPeriod,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_PRODUCER:
			hr = GetMediaMetadata(szFullFileName,g_wszWMProducer,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_PUBLISHER:
			hr = GetMediaMetadata(szFullFileName,g_wszWMPublisher,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_WRITER:
			hr = GetMediaMetadata(szFullFileName,g_wszWMWriter,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;

		case MEDIAMETADATA_TYPE_YEAR:
			hr = GetMediaMetadata(szFullFileName,g_wszWMYear,(BYTE **)&pszTemp);

			if(SUCCEEDED(hr))
			{
				StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),pszTemp);

				free(pszTemp);
			}
			break;
		}

		if(SUCCEEDED(hr))
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,szOutput);
		}
		else
		{
			ListView_SetItemText(m_hListView,iItem,iColumn,EMPTY_STRING);
		}
	}
}

void CFolderView::PlaceColumns(void)
{
	list<Column_t>::iterator	itr;
	int							iColumnIndex = 0;
	int							i = 0;

	m_nActiveColumns = 0;

	if(m_pActiveColumnList != NULL)
	{
		for(itr = m_pActiveColumnList->begin();itr != m_pActiveColumnList->end();itr++)
		{
			if(itr->bChecked)
			{
				InsertColumn(itr->id,iColumnIndex,itr->iWidth);

				/* Do NOT set column widths here. For some reason, this causes list mode to
				break. (If this code is active, and the listview starts of in details mode
				and is then switched to list mode, no items will be shown; they appear to
				be placed off the left edge of the listview). */
				//ListView_SetColumnWidth(m_hListView,iColumnIndex,LVSCW_AUTOSIZE_USEHEADER);

				iColumnIndex++;
				m_nActiveColumns++;
			}
		}

		for(i = m_nCurrentColumns + m_nActiveColumns;i >= m_nActiveColumns;i--)
		{
			ListView_DeleteColumn(m_hListView,i);
		}

		m_nCurrentColumns = m_nActiveColumns;
	}
}

void CFolderView::InsertColumn(unsigned int ColumnId,int iColumnIndex,int iWidth)
{
	HWND		hHeader;
	HDITEM		hdItem;
	LV_COLUMN	lvColumn;
	TCHAR		szText[64];
	int			iActualColumnIndex;
	int			iStringIndex;

	iStringIndex = (int)SendMessage(m_hOwner,WM_USER_GETCOLUMNNAMEINDEX,ColumnId,0);

	LoadString(m_hResourceModule,iStringIndex,
		szText,SIZEOF_ARRAY(szText));

	lvColumn.mask		= LVCF_TEXT|LVCF_WIDTH;
	lvColumn.pszText	= szText;
	lvColumn.cx			= iWidth;

	if(ColumnId == CM_SIZE)
	{
		lvColumn.mask	|= LVCF_FMT;
		lvColumn.fmt	= LVCFMT_RIGHT;
	}

	iActualColumnIndex = ListView_InsertColumn(m_hListView,iColumnIndex,&lvColumn);

	hHeader = ListView_GetHeader(m_hListView);

	/* Store the column's ID with the column itself. */
	hdItem.mask		= HDI_LPARAM;
	hdItem.lParam	= ColumnId;

	Header_SetItem(hHeader,iActualColumnIndex,&hdItem);
}

void CFolderView::SetActiveColumnSet(void)
{
	list<Column_t> *pActiveColumnList = NULL;

	if(CompareVirtualFolders(CSIDL_CONTROLS))
	{
		/* Control panel. */
		pActiveColumnList = &m_ControlPanelColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_DRIVES))
	{
		/* My Computer. */
		pActiveColumnList = &m_MyComputerColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_BITBUCKET))
	{
		/* Recycle Bin. */
		pActiveColumnList = &m_RecycleBinColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_PRINTERS))
	{
		/* Printers virtual folder. */
		pActiveColumnList = &m_PrintersColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_CONNECTIONS))
	{
		/* Network connections virtual folder. */
		pActiveColumnList = &m_NetworkConnectionsColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_NETWORK))
	{
		/* My Network Places (Network on Vista) virtual folder. */
		pActiveColumnList = &m_MyNetworkPlacesColumnList;
	}
	else
	{
		/* Real folder. */
		pActiveColumnList = &m_RealFolderColumnList;
	}

	/* If the current set of columns are different
	from the previous set of columns (i.e. the
	current folder and previous folder are of a
	different 'type'), set the new columns, and
	place them (else do nothing). */
	if(m_pActiveColumnList != pActiveColumnList)
	{
		m_pActiveColumnList = pActiveColumnList;
		m_bColumnsPlaced = FALSE;
	}
}

unsigned int CFolderView::DetermineColumnSortMode(int iColumnId)
{
	switch(iColumnId)
	{
		case CM_NAME:
			return FSM_NAME;
			break;

		case CM_TYPE:
			return FSM_TYPE;
			break;

		case CM_SIZE:
			return FSM_SIZE;
			break;

		case CM_DATEMODIFIED:
			return FSM_DATEMODIFIED;
			break;

		case CM_ATTRIBUTES:
			return FSM_ATTRIBUTES;
			break;

		case CM_REALSIZE:
			return FSM_REALSIZE;
			break;

		case CM_SHORTNAME:
			return FSM_SHORTNAME;
			break;

		case CM_OWNER:
			return FSM_OWNER;
			break;

		case CM_PRODUCTNAME:
			return FSM_PRODUCTNAME;
			break;

		case CM_COMPANY:
			return FSM_COMPANY;
			break;

		case CM_DESCRIPTION:
			return FSM_DESCRIPTION;
			break;

		case CM_FILEVERSION:
			return FSM_FILEVERSION;
			break;

		case CM_PRODUCTVERSION:
			return FSM_PRODUCTVERSION;
			break;

		case CM_SHORTCUTTO:
			return FSM_SHORTCUTTO;
			break;

		case CM_HARDLINKS:
			return FSM_HARDLINKS;
			break;

		case CM_EXTENSION:
			return FSM_EXTENSION;
			break;

		case CM_CREATED:
			return FSM_CREATED;
			break;

		case CM_ACCESSED:
			return FSM_ACCESSED;
			break;

		case CM_TITLE:
			return FSM_TITLE;
			break;

		case CM_SUBJECT:
			return FSM_SUBJECT;
			break;

		case CM_AUTHOR:
			return FSM_AUTHOR;
			break;

		case CM_KEYWORDS:
			return FSM_KEYWORDS;
			break;

		case CM_COMMENT:
			return FSM_COMMENTS;
			break;

		case CM_CAMERAMODEL:
			return FSM_CAMERAMODEL;
			break;

		case CM_DATETAKEN:
			return FSM_DATETAKEN;
			break;

		case CM_WIDTH:
			return FSM_WIDTH;
			break;

		case CM_HEIGHT:
			return FSM_HEIGHT;
			break;

		case CM_VIRTUALCOMMENTS:
			return FSM_VIRTUALCOMMENTS;
			break;

		case CM_TOTALSIZE:
			return FSM_TOTALSIZE;
			break;

		case CM_FREESPACE:
			return FSM_FREESPACE;
			break;

		case CM_FILESYSTEM:
			return FSM_FILESYSTEM;
			break;

		case CM_VIRTUALTYPE:
			return FSM_VIRTUALTYPE;
			break;

		case CM_ORIGINALLOCATION:
			return FSM_ORIGINALLOCATION;
			break;

		case CM_DATEDELETED:
			return FSM_DATEDELETED;
			break;

		case CM_NUMPRINTERDOCUMENTS:
			return FSM_NUMPRINTERDOCUMENTS;
			break;

		case CM_PRINTERSTATUS:
			return FSM_PRINTERSTATUS;
			break;

		case CM_PRINTERCOMMENTS:
			return FSM_PRINTERCOMMENTS;
			break;

		case CM_PRINTERLOCATION:
			return FSM_PRINTERLOCATION;
			break;

		case CM_NETWORKADAPTER_STATUS:
			return FSM_NETWORKADAPTER_STATUS;
			break;
	}

	return 0;
}

void CFolderView::ColumnClicked(int iClickedColumn)
{
	list<Column_t>::iterator itr;
	int iCurrentColumn = 0;
	UINT SortMode = 0;
	UINT iColumnId = 0;

	for(itr = m_pActiveColumnList->begin();itr != m_pActiveColumnList->end();itr++)
	{
		/* Only increnment if this column is actually been shown. */
		if(itr->bChecked)
		{
			if(iCurrentColumn == iClickedColumn)
			{
				SortMode = DetermineColumnSortMode(itr->id);
				iColumnId = itr->id;
				break;
			}

			iCurrentColumn++;
		}
	}

	/* Same column was clicked. Toggle the
	ascending/descending sort state. Use unique
	column ID, not index, as columns may be
	inserted/deleted. */
	if(m_iPreviousSortedColumnId == iColumnId)
	{
		ToggleSortAscending();
	}

	SortFolder(SortMode);
}

void CFolderView::ApplyHeaderSortArrow(void)
{
	HWND hHeader;
	HDITEM hdItem;
	list<Column_t>::iterator itr;
	BOOL bPreviousColumnExists = FALSE;
	int iColumn = 0;
	int iPreviousSortedColumn = 0;
	int iColumnId = -1;

	hHeader = ListView_GetHeader(m_hListView);

	/* Search through the currently active columns to find the column that previously
	had the up/down arrow. */
	for(itr = m_pActiveColumnList->begin();itr != m_pActiveColumnList->end();itr++)
	{
		/* Only increnment if this column is actually been shown. */
		if(itr->bChecked)
		{
			if(m_iPreviousSortedColumnId == itr->id)
			{
				bPreviousColumnExists = TRUE;
				break;
			}

			iPreviousSortedColumn++;
		}
	}

	if(bPreviousColumnExists)
	{
		hdItem.mask = HDI_FORMAT;
		Header_GetItem(hHeader,iPreviousSortedColumn,&hdItem);

		if(hdItem.fmt & HDF_SORTUP)
		{
			hdItem.fmt &= ~HDF_SORTUP;
		}
		else if(hdItem.fmt & HDF_SORTDOWN)
		{
			hdItem.fmt &= ~HDF_SORTDOWN;
		}

		/* Remove the up/down arrow from the column by which
		results were previously sorted. */
		Header_SetItem(hHeader,iPreviousSortedColumn,&hdItem);
	}

	/* Find the index of the column representing the current sort mode. */
	for(itr = m_pActiveColumnList->begin();itr != m_pActiveColumnList->end();itr++)
	{
		if(itr->bChecked)
		{
			if(DetermineColumnSortMode(itr->id) == m_SortMode)
			{
				iColumnId = itr->id;
				break;
			}

			iColumn++;
		}
	}

	hdItem.mask = HDI_FORMAT;
	Header_GetItem(hHeader,iColumn,&hdItem);

	if(!m_bSortAscending)
		hdItem.fmt |= HDF_SORTDOWN;
	else
		hdItem.fmt |= HDF_SORTUP;

	/* Add the up/down arrow to the column by which
	items are now sorted. */
	Header_SetItem(hHeader,iColumn,&hdItem);

	m_iPreviousSortedColumnId = iColumnId;
}

size_t CFolderView::QueryNumActiveColumns(void)
{
	return m_pActiveColumnList->size();
}

void CFolderView::ImportAllColumns(ColumnExport_t *pce)
{
	CopyColumnsInternal(&m_ControlPanelColumnList,&pce->ControlPanelColumnList);
	CopyColumnsInternal(&m_MyComputerColumnList,&pce->MyComputerColumnList);
	CopyColumnsInternal(&m_MyNetworkPlacesColumnList,&pce->MyNetworkPlacesColumnList);
	CopyColumnsInternal(&m_NetworkConnectionsColumnList,&pce->NetworkConnectionsColumnList);
	CopyColumnsInternal(&m_PrintersColumnList,&pce->PrintersColumnList);
	CopyColumnsInternal(&m_RealFolderColumnList,&pce->RealFolderColumnList);
	CopyColumnsInternal(&m_RecycleBinColumnList,&pce->RecycleBinColumnList);
}

void CFolderView::ExportAllColumns(ColumnExport_t *pce)
{
	SaveColumnWidths();

	pce->ControlPanelColumnList			= m_ControlPanelColumnList;
	pce->MyComputerColumnList			= m_MyComputerColumnList;
	pce->MyNetworkPlacesColumnList		= m_MyNetworkPlacesColumnList;
	pce->NetworkConnectionsColumnList	= m_NetworkConnectionsColumnList;
	pce->PrintersColumnList				= m_PrintersColumnList;
	pce->RealFolderColumnList			= m_RealFolderColumnList;
	pce->RecycleBinColumnList			= m_RecycleBinColumnList;
}

void CFolderView::SaveColumnWidths(void)
{
	list<Column_t> *pActiveColumnList = NULL;
	list<Column_t>::iterator itr;
	int iColumn = 0;

	if(CompareVirtualFolders(CSIDL_CONTROLS))
	{
		/* Control panel. */
		pActiveColumnList = &m_ControlPanelColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_DRIVES))
	{
		/* My Computer. */
		pActiveColumnList = &m_MyComputerColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_BITBUCKET))
	{
		/* Recycle Bin. */
		pActiveColumnList = &m_RecycleBinColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_PRINTERS))
	{
		/* Printers virtual folder. */
		pActiveColumnList = &m_PrintersColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_CONNECTIONS))
	{
		/* Network connections virtual folder. */
		pActiveColumnList = &m_NetworkConnectionsColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_NETWORK))
	{
		/* My Network Places (Network on Vista) virtual folder. */
		pActiveColumnList = &m_MyNetworkPlacesColumnList;
	}
	else
	{
		/* Real folder. */
		pActiveColumnList = &m_RealFolderColumnList;
	}

	/* Only save column widths if the listview is currently in
	details view. If it's not currently in details view, then
	column widths have already been saved when the view changed. */
	if(m_ViewMode == VM_DETAILS)
	{
		for(itr = pActiveColumnList->begin();itr != pActiveColumnList->end();itr++)
		{
			if(itr->bChecked)
			{
				itr->iWidth = ListView_GetColumnWidth(m_hListView,iColumn);

				iColumn++;
			}
		}
	}
}