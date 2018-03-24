/******************************************************************
 *
 * Project: ShellBrowser
 * File: ColumnManager.cpp
 * License: GPL - See LICENSE in the top level directory
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
#include <list>
#include <cassert>

/* According to http://www.boost.org/doc/libs/release/more/getting_started/windows.html#header-only-libraries,
Boost.DateTime has a binary
component that's only required
if, for example, you're using
to_string/from_string. Those
functions aren't been used in
this case, so explicitly exclude
the library (also see
http://stackoverflow.com/questions/2364774/why-do-i-need-to-compile-datetime-in-boost-if-i-am-not-using-to-string-or-from-s). */
#define BOOST_DATE_TIME_NO_LIB

#pragma warning(push)
#pragma warning(disable:4995)
#include <boost\date_time\posix_time\posix_time.hpp>
#pragma warning(pop)

#include "IShellView.h"
#include "iShellBrowser_internal.h"
#include "../Helper/Helper.h"
#include "../Helper/DriveInfo.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/FolderSize.h"
#include "../Helper/Macros.h"


BOOL GetPrinterStatusDescription(DWORD dwStatus, TCHAR *szStatus, size_t cchMax);

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
void CShellBrowser::AddToColumnQueue(int iItem)
{
	m_pColumnInfoList.push_back(iItem);
}

void CShellBrowser::EmptyColumnQueue(void)
{
	EnterCriticalSection(&m_column_cs);

	m_pColumnInfoList.clear();

	LeaveCriticalSection(&m_column_cs);
}

BOOL CShellBrowser::RemoveFromColumnQueue(int *iItem)
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
		std::list<int>::iterator itr;

		itr = m_pColumnInfoList.begin();

		*iItem = *itr;

		ResetEvent(m_hColumnQueueEvent);

		m_pColumnInfoList.erase(itr);

		bQueueNotEmpty = TRUE;
	}

	LeaveCriticalSection(&m_column_cs);

	return bQueueNotEmpty;
}

void CShellBrowser::AddToFolderQueue(int iItem)
{
	m_pFolderInfoList.push_back(iItem);
}

void CShellBrowser::EmptyFolderQueue(void)
{
	EnterCriticalSection(&m_folder_cs);

	m_pFolderInfoList.clear();

	LeaveCriticalSection(&m_folder_cs);
}

BOOL CShellBrowser::RemoveFromFolderQueue(int *iItem)
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
		std::list<int>::iterator itr;

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
	CShellBrowser *pShellBrowser = reinterpret_cast<CShellBrowser *>(dwParam);
	pShellBrowser->SetAllFolderSizeColumnData();
}

int CShellBrowser::SetAllFolderSizeColumnData(void)
{
	std::list<Column_t>::iterator itr;
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

					LVITEM lvChildItem;
					int iItemInternal;
					BOOL bRes;

					lvChildItem.mask		= LVIF_PARAM;
					lvChildItem.iItem		= iItem;
					lvChildItem.iSubItem	= 0;
					bRes = ListView_GetItem(m_hListView,&lvChildItem);

					if(bRes)
					{
						iItemInternal = (int)lvChildItem.lParam;

						QueryFullItemName(iItem,FullItemPath,SIZEOF_ARRAY(FullItemPath));

						CalculateFolderSize(FullItemPath,&nFolders,&nFiles,&lTotalFolderSize);

						/* Does the item still exist? */
						/* TODO: Need to lock this against the main thread. */
						/* TODO: Discard the result if the folder was deleted. */
						if(m_pItemMap[(int)lvChildItem.lParam] == 1)
						{
							m_pwfdFiles[(int)lvChildItem.lParam].nFileSizeLow = lTotalFolderSize.LowPart;
							m_pwfdFiles[(int)lvChildItem.lParam].nFileSizeHigh = lTotalFolderSize.HighPart;
							m_pExtraItemInfo[(int)lvChildItem.lParam].bFolderSizeRetrieved = TRUE;

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

void CALLBACK SetAllColumnDataAPC(ULONG_PTR dwParam)
{
	CShellBrowser *pShellBrowser = reinterpret_cast<CShellBrowser *>(dwParam);
	pShellBrowser->SetAllColumnText();
}

void CShellBrowser::SetAllColumnText(void)
{
	int ItemIndex;
	BOOL QueueNotEmpty = RemoveFromColumnQueue(&ItemIndex);

	while(QueueNotEmpty)
	{
		int iColumnIndex = 0;

		for(auto itr = m_pActiveColumnList->begin();itr != m_pActiveColumnList->end();itr++)
		{
			if(itr->bChecked)
			{
				SetColumnText(itr->id,ItemIndex,iColumnIndex++);
			}
		}

		QueueNotEmpty = RemoveFromColumnQueue(&ItemIndex);
	}

	ApplyHeaderSortArrow();
}

void CShellBrowser::SetColumnText(UINT ColumnID,int ItemIndex,int ColumnIndex)
{
	LVITEM lvItem;
	lvItem.mask		= LVIF_PARAM;
	lvItem.iSubItem	= 0;
	lvItem.iItem	= ItemIndex;
	BOOL ItemRetrieved = ListView_GetItem(m_hListView,&lvItem);
	ItemRetrieved;

	assert(ItemRetrieved);

	std::wstring ColumnText = GetColumnText(ColumnID,static_cast<int>(lvItem.lParam));

	TCHAR ColumnTextTemp[1024];
	StringCchCopy(ColumnTextTemp,SIZEOF_ARRAY(ColumnTextTemp),ColumnText.c_str());
	ListView_SetItemText(m_hListView,ItemIndex,ColumnIndex,ColumnTextTemp);
}

std::wstring CShellBrowser::GetColumnText(UINT ColumnID,int InternalIndex) const
{
	switch(ColumnID)
	{
	case CM_NAME:
		return GetNameColumnText(InternalIndex);
		break;

	case CM_TYPE:
		return GetTypeColumnText(InternalIndex);
		break;
	case CM_SIZE:
		return GetSizeColumnText(InternalIndex);
		break;

	case CM_DATEMODIFIED:
		return GetTimeColumnText(InternalIndex,COLUMN_TIME_MODIFIED);
		break;
	case CM_CREATED:
		return GetTimeColumnText(InternalIndex,COLUMN_TIME_CREATED);
		break;
	case CM_ACCESSED:
		return GetTimeColumnText(InternalIndex,COLUMN_TIME_ACCESSED);
		break;

	case CM_ATTRIBUTES:
		return GetAttributeColumnText(InternalIndex);
		break;
	case CM_REALSIZE:
		return GetRealSizeColumnText(InternalIndex);
		break;
	case CM_SHORTNAME:
		return GetShortNameColumnText(InternalIndex);
		break;
	case CM_OWNER:
		return GetOwnerColumnText(InternalIndex);
		break;

	case CM_PRODUCTNAME:
		return GetVersionColumnText(InternalIndex,VERSION_INFO_PRODUCT_NAME);
		break;
	case CM_COMPANY:
		return GetVersionColumnText(InternalIndex,VERSION_INFO_COMPANY);
		break;
	case CM_DESCRIPTION:
		return GetVersionColumnText(InternalIndex,VERSION_INFO_DESCRIPTION);
		break;
	case CM_FILEVERSION:
		return GetVersionColumnText(InternalIndex,VERSION_INFO_FILE_VERSION);
		break;
	case CM_PRODUCTVERSION:
		return GetVersionColumnText(InternalIndex,VERSION_INFO_PRODUCT_VERSION);
		break;

	case CM_SHORTCUTTO:
		return GetShortcutToColumnText(InternalIndex);
		break;
	case CM_HARDLINKS:
		return GetHardLinksColumnText(InternalIndex);
		break;
	case CM_EXTENSION:
		return GetExtensionColumnText(InternalIndex);
		break;

	case CM_TITLE:
		return GetItemDetailsColumnText(InternalIndex, &SCID_TITLE);
		break;
	case CM_SUBJECT:
		return GetItemDetailsColumnText(InternalIndex, &SCID_SUBJECT);
		break;
	case CM_AUTHOR:
		return GetItemDetailsColumnText(InternalIndex, &SCID_AUTHOR);
		break;
	case CM_KEYWORDS:
		return GetItemDetailsColumnText(InternalIndex, &SCID_KEYWORDS);
		break;
	case CM_COMMENT:
		return GetItemDetailsColumnText(InternalIndex, &SCID_COMMENTS);
		break;

	case CM_CAMERAMODEL:
		return GetImageColumnText(InternalIndex,PropertyTagEquipModel);
		break;
	case CM_DATETAKEN:
		return GetImageColumnText(InternalIndex,PropertyTagDateTime);
		break;
	case CM_WIDTH:
		return GetImageColumnText(InternalIndex,PropertyTagImageWidth);
		break;
	case CM_HEIGHT:
		return GetImageColumnText(InternalIndex,PropertyTagImageHeight);
		break;

	case CM_VIRTUALCOMMENTS:
		return GetControlPanelCommentsColumnText(InternalIndex);
		break;

	case CM_TOTALSIZE:
		return GetDriveSpaceColumnText(InternalIndex,true);
		break;

	case CM_FREESPACE:
		return GetDriveSpaceColumnText(InternalIndex,false);
		break;

	case CM_FILESYSTEM:
		return GetFileSystemColumnText(InternalIndex);
		break;

	case CM_ORIGINALLOCATION:
		return GetItemDetailsColumnText(InternalIndex, &SCID_ORIGINAL_LOCATION);
		break;

	case CM_DATEDELETED:
		return GetItemDetailsColumnText(InternalIndex, &SCID_DATE_DELETED);
		break;

	case CM_NUMPRINTERDOCUMENTS:
		return GetPrinterColumnText(InternalIndex,PRINTER_INFORMATION_TYPE_NUM_JOBS);
		break;

	case CM_PRINTERSTATUS:
		return GetPrinterColumnText(InternalIndex,PRINTER_INFORMATION_TYPE_STATUS);
		break;

	case CM_PRINTERCOMMENTS:
		return GetPrinterColumnText(InternalIndex,PRINTER_INFORMATION_TYPE_COMMENTS);
		break;

	case CM_PRINTERLOCATION:
		return GetPrinterColumnText(InternalIndex,PRINTER_INFORMATION_TYPE_LOCATION);
		break;

	case CM_PRINTERMODEL:
		return GetPrinterColumnText(InternalIndex,PRINTER_INFORMATION_TYPE_MODEL);
		break;

	case CM_NETWORKADAPTER_STATUS:
		return GetNetworkAdapterColumnText(InternalIndex);
		break;

	case CM_MEDIA_BITRATE:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_BITRATE);
		break;
	case CM_MEDIA_COPYRIGHT:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_COPYRIGHT);
		break;
	case CM_MEDIA_DURATION:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_DURATION);
		break;
	case CM_MEDIA_PROTECTED:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_PROTECTED);
		break;
	case CM_MEDIA_RATING:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_RATING);
		break;
	case CM_MEDIA_ALBUMARTIST:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_ALBUM_ARTIST);
		break;
	case CM_MEDIA_ALBUM:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_ALBUM_TITLE);
		break;
	case CM_MEDIA_BEATSPERMINUTE:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_BEATS_PER_MINUTE);
		break;
	case CM_MEDIA_COMPOSER:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_COMPOSER);
		break;
	case CM_MEDIA_CONDUCTOR:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_CONDUCTOR);
		break;
	case CM_MEDIA_DIRECTOR:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_DIRECTOR);
		break;
	case CM_MEDIA_GENRE:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_GENRE);
		break;
	case CM_MEDIA_LANGUAGE:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_LANGUAGE);
		break;
	case CM_MEDIA_BROADCASTDATE:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_BROADCASTDATE);
		break;
	case CM_MEDIA_CHANNEL:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_CHANNEL);
		break;
	case CM_MEDIA_STATIONNAME:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_STATIONNAME);
		break;
	case CM_MEDIA_MOOD:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_MOOD);
		break;
	case CM_MEDIA_PARENTALRATING:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_PARENTALRATING);
		break;
	case CM_MEDIA_PARENTALRATINGREASON:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_PARENTALRATINGREASON);
		break;
	case CM_MEDIA_PERIOD:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_PERIOD);
		break;
	case CM_MEDIA_PRODUCER:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_PRODUCER);
		break;
	case CM_MEDIA_PUBLISHER:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_PUBLISHER);
		break;
	case CM_MEDIA_WRITER:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_WRITER);
		break;
	case CM_MEDIA_YEAR:
		return GetMediaMetadataColumnText(InternalIndex,MEDIAMETADATA_TYPE_YEAR);
		break;

	default:
		assert(false);
		break;
	}

	return EMPTY_STRING;
}

std::wstring CShellBrowser::GetNameColumnText(int InternalIndex) const
{
	return ProcessItemFileName(InternalIndex);
}

std::wstring CShellBrowser::GetTypeColumnText(int InternalIndex) const
{	
	LPITEMIDLIST pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[InternalIndex].pridl);

	SHFILEINFO shfi;
	DWORD_PTR Res = SHGetFileInfo(reinterpret_cast<LPTSTR>(pidlComplete),0,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_TYPENAME);

	CoTaskMemFree(pidlComplete);

	if(Res == 0)
	{
		return EMPTY_STRING;
	}

	return shfi.szTypeName;
}

std::wstring CShellBrowser::GetSizeColumnText(int InternalIndex) const
{
	if((m_pwfdFiles[InternalIndex].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		return EMPTY_STRING;
	}

	ULARGE_INTEGER FileSize = {m_pwfdFiles[InternalIndex].nFileSizeLow,m_pwfdFiles[InternalIndex].nFileSizeHigh};

	TCHAR FileSizeText[64];
	FormatSizeString(FileSize,FileSizeText,SIZEOF_ARRAY(FileSizeText),m_bForceSize,m_SizeDisplayFormat);

	return FileSizeText;
}

std::wstring CShellBrowser::GetTimeColumnText(int InternalIndex,TimeType_t TimeType) const
{
	TCHAR FileTime[64];
	BOOL bRet = FALSE;

	switch(TimeType)
	{
	case COLUMN_TIME_MODIFIED:
		bRet = CreateFileTimeString(&m_pwfdFiles[InternalIndex].ftLastWriteTime,
			FileTime,SIZEOF_ARRAY(FileTime),m_bShowFriendlyDates);
		break;

	case COLUMN_TIME_CREATED:
		bRet = CreateFileTimeString(&m_pwfdFiles[InternalIndex].ftCreationTime,
			FileTime,SIZEOF_ARRAY(FileTime),m_bShowFriendlyDates);
		break;

	case COLUMN_TIME_ACCESSED:
		bRet = CreateFileTimeString(&m_pwfdFiles[InternalIndex].ftLastAccessTime,
			FileTime,SIZEOF_ARRAY(FileTime),m_bShowFriendlyDates);
		break;

	default:
		assert(false);
		break;
	}

	if(!bRet)
	{
		return EMPTY_STRING;
	}

	return FileTime;
}

std::wstring CShellBrowser::GetAttributeColumnText(int InternalIndex) const
{
	TCHAR FullFileName[MAX_PATH];
	QueryFullItemNameInternal(InternalIndex,FullFileName,SIZEOF_ARRAY(FullFileName));

	TCHAR AttributeString[32];
	BuildFileAttributeString(FullFileName,AttributeString,SIZEOF_ARRAY(AttributeString));

	return AttributeString;
}

bool CShellBrowser::GetRealSizeColumnRawData(int InternalIndex,ULARGE_INTEGER &RealFileSize) const
{
	if((m_pwfdFiles[InternalIndex].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		return false;
	}

	TCHAR Root[MAX_PATH];
	StringCchCopy(Root,SIZEOF_ARRAY(Root),m_CurDir);
	PathStripToRoot(Root);

	DWORD dwClusterSize;
	BOOL bRet = GetClusterSize(Root, &dwClusterSize);

	if(!bRet)
	{
		return false;
	}

	ULARGE_INTEGER RealFileSizeTemp = {m_pwfdFiles[InternalIndex].nFileSizeLow,m_pwfdFiles[InternalIndex].nFileSizeHigh};

	if(RealFileSizeTemp.QuadPart != 0 && (RealFileSizeTemp.QuadPart % dwClusterSize) != 0)
	{
		RealFileSizeTemp.QuadPart += dwClusterSize - (RealFileSizeTemp.QuadPart % dwClusterSize);
	}

	RealFileSize = RealFileSizeTemp;

	return true;
}

std::wstring CShellBrowser::GetRealSizeColumnText(int InternalIndex) const
{
	ULARGE_INTEGER RealFileSize;
	bool Res = GetRealSizeColumnRawData(InternalIndex,RealFileSize);

	if(!Res)
	{
		return EMPTY_STRING;
	}

	TCHAR RealFileSizeText[32];
	FormatSizeString(RealFileSize,RealFileSizeText,SIZEOF_ARRAY(RealFileSizeText),
		m_bForceSize,m_SizeDisplayFormat);

	return RealFileSizeText;
}

std::wstring CShellBrowser::GetShortNameColumnText(int InternalIndex) const
{
	if(lstrlen(m_pwfdFiles[InternalIndex].cAlternateFileName) == 0)
	{
		return m_pwfdFiles[InternalIndex].cFileName;
	}

	return m_pwfdFiles[InternalIndex].cAlternateFileName;
}

std::wstring CShellBrowser::GetOwnerColumnText(int InternalIndex) const
{
	TCHAR FullFileName[MAX_PATH];
	QueryFullItemNameInternal(InternalIndex,FullFileName,SIZEOF_ARRAY(FullFileName));

	TCHAR Owner[512];
	BOOL ret = GetFileOwner(FullFileName,Owner,SIZEOF_ARRAY(Owner));

	if(!ret)
	{
		return EMPTY_STRING;
	}

	return Owner;
}

std::wstring CShellBrowser::GetVersionColumnText(int InternalIndex,VersionInfoType_t VersioninfoType) const
{
	std::wstring VersionInfoName;

	switch(VersioninfoType)
	{
	case VERSION_INFO_PRODUCT_NAME:
		VersionInfoName = L"ProductName";
		break;

	case VERSION_INFO_COMPANY:
		VersionInfoName = L"CompanyName";
		break;

	case VERSION_INFO_DESCRIPTION:
		VersionInfoName = L"FileDescription";
		break;

	case VERSION_INFO_FILE_VERSION:
		VersionInfoName = L"FileVersion";
		break;

	case VERSION_INFO_PRODUCT_VERSION:
		VersionInfoName = L"ProductVersion";
		break;

	default:
		assert(false);
		break;
	}

	TCHAR FullFileName[MAX_PATH];
	QueryFullItemNameInternal(InternalIndex,FullFileName,SIZEOF_ARRAY(FullFileName));

	TCHAR VersionInfo[512];
	BOOL VersionInfoObtained = GetVersionInfoString(FullFileName,VersionInfoName.c_str(),
		VersionInfo,SIZEOF_ARRAY(VersionInfo));

	if(!VersionInfoObtained)
	{
		return EMPTY_STRING;
	}

	return VersionInfo;
}

std::wstring CShellBrowser::GetShortcutToColumnText(int InternalIndex) const
{
	TCHAR FullFileName[MAX_PATH];
	QueryFullItemNameInternal(InternalIndex,FullFileName,SIZEOF_ARRAY(FullFileName));

	TCHAR ResolvedLinkPath[MAX_PATH];
	HRESULT hr = NFileOperations::ResolveLink(NULL,SLR_NO_UI,FullFileName,
		ResolvedLinkPath,SIZEOF_ARRAY(ResolvedLinkPath));

	if(FAILED(hr))
	{
		return EMPTY_STRING;	
	}

	return ResolvedLinkPath;
}

DWORD CShellBrowser::GetHardLinksColumnRawData(int InternalIndex) const
{
	TCHAR FullFileName[MAX_PATH];
	QueryFullItemNameInternal(InternalIndex,FullFileName,SIZEOF_ARRAY(FullFileName));

	return GetNumFileHardLinks(FullFileName);
}

std::wstring CShellBrowser::GetHardLinksColumnText(int InternalIndex) const
{
	DWORD NumHardLinks = GetHardLinksColumnRawData(InternalIndex);

	if(NumHardLinks == -1)
	{
		return EMPTY_STRING;
	}

	TCHAR NumHardLinksString[32];
	StringCchPrintf(NumHardLinksString,SIZEOF_ARRAY(NumHardLinksString),_T("%ld"),NumHardLinks);
	
	return NumHardLinksString;
}

std::wstring CShellBrowser::GetExtensionColumnText(int InternalIndex) const
{
	if((m_pwfdFiles[InternalIndex].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		return EMPTY_STRING;
	}

	TCHAR *Extension = PathFindExtension(m_pwfdFiles[InternalIndex].cFileName);

	if(*Extension != '.')
	{
		return EMPTY_STRING;
	}

	return Extension + 1;
}

HRESULT CShellBrowser::GetItemDetails(int InternalIndex, const SHCOLUMNID *pscid, TCHAR *szDetail, size_t cchMax) const
{
	VARIANT vt;
	HRESULT hr = GetItemDetailsRawData(InternalIndex, pscid, &vt);

	if (SUCCEEDED(hr))
	{
		hr = ConvertVariantToString(&vt, szDetail, cchMax, m_bShowFriendlyDates);
		VariantClear(&vt);
	}

	return hr;
}

HRESULT CShellBrowser::GetItemDetailsRawData(int InternalIndex, const SHCOLUMNID *pscid, VARIANT *vt) const
{
	IShellFolder2 *pShellFolder = NULL;
	HRESULT hr = BindToIdl(m_pidlDirectory, IID_PPV_ARGS(&pShellFolder));

	if (SUCCEEDED(hr))
	{
		hr = pShellFolder->GetDetailsEx(m_pExtraItemInfo[InternalIndex].pridl, pscid, vt);
		pShellFolder->Release();
	}

	return hr;
}

std::wstring CShellBrowser::GetItemDetailsColumnText(int InternalIndex, const SHCOLUMNID *pscid) const
{
	TCHAR szDetail[512];
	HRESULT hr = GetItemDetails(InternalIndex, pscid, szDetail, SIZEOF_ARRAY(szDetail));

	if(SUCCEEDED(hr))
	{
		return szDetail;
	}

	return EMPTY_STRING;
}

std::wstring CShellBrowser::GetImageColumnText(int InternalIndex,PROPID PropertyID) const
{
	TCHAR FullFileName[MAX_PATH];
	QueryFullItemNameInternal(InternalIndex,FullFileName,SIZEOF_ARRAY(FullFileName));

	TCHAR ImageProperty[512];
	BOOL Res = ReadImageProperty(FullFileName,PropertyID,ImageProperty,
		SIZEOF_ARRAY(ImageProperty));

	if(!Res)
	{
		return EMPTY_STRING;
	}

	return ImageProperty;
}

std::wstring CShellBrowser::GetFileSystemColumnText(int InternalIndex) const
{
	LPITEMIDLIST pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[InternalIndex].pridl);

	TCHAR FullFileName[MAX_PATH];
	GetDisplayName(pidlComplete,FullFileName,SIZEOF_ARRAY(FullFileName),SHGDN_FORPARSING);

	CoTaskMemFree(pidlComplete);

	BOOL IsRoot = PathIsRoot(FullFileName);

	if(!IsRoot)
	{
		return EMPTY_STRING;
	}

	TCHAR FileSystemName[MAX_PATH];
	BOOL Res = GetVolumeInformation(FullFileName,NULL,0,NULL,NULL,NULL,FileSystemName,
		SIZEOF_ARRAY(FileSystemName));

	if(!Res)
	{
		return EMPTY_STRING;
	}

	return FileSystemName;
}

BOOL CShellBrowser::GetDriveSpaceColumnRawData(int InternalIndex,bool TotalSize,ULARGE_INTEGER &DriveSpace) const
{
	LPITEMIDLIST pidlComplete = ILCombine(m_pidlDirectory,m_pExtraItemInfo[InternalIndex].pridl);

	TCHAR FullFileName[MAX_PATH];
	GetDisplayName(pidlComplete,FullFileName,SIZEOF_ARRAY(FullFileName),SHGDN_FORPARSING);

	CoTaskMemFree(pidlComplete);

	BOOL IsRoot = PathIsRoot(FullFileName);

	if(!IsRoot)
	{
		return FALSE;
	}

	ULARGE_INTEGER TotalBytes;
	ULARGE_INTEGER FreeBytes;
	BOOL Res = GetDiskFreeSpaceEx(FullFileName,NULL,&TotalBytes,&FreeBytes);

	if(TotalSize)
	{
		DriveSpace = TotalBytes;
	}
	else
	{
		DriveSpace = FreeBytes;
	}

	return Res;
}

std::wstring CShellBrowser::GetDriveSpaceColumnText(int InternalIndex,bool TotalSize) const
{
	ULARGE_INTEGER DriveSpace;
	BOOL Res = GetDriveSpaceColumnRawData(InternalIndex,TotalSize,DriveSpace);

	if(!Res)
	{
		return EMPTY_STRING;
	}

	TCHAR SizeText[32];
	FormatSizeString(DriveSpace,SizeText,SIZEOF_ARRAY(SizeText),m_bForceSize,m_SizeDisplayFormat);

	return SizeText;
}

std::wstring CShellBrowser::GetControlPanelCommentsColumnText(int InternalIndex) const
{
	TCHAR szFullFileName[MAX_PATH];
	QueryFullItemNameInternal(InternalIndex, szFullFileName, SIZEOF_ARRAY(szFullFileName));

	TCHAR InfoTip[512];
	HRESULT hr = GetItemInfoTip(szFullFileName, InfoTip, SIZEOF_ARRAY(InfoTip));

	if(FAILED(hr))
	{
		return EMPTY_STRING;
	}

	ReplaceCharacter(InfoTip,'\n',' ');

	return InfoTip;
}

std::wstring CShellBrowser::GetPrinterColumnText(int InternalIndex,PrinterInformationType_t PrinterInformationType) const
{
	TCHAR PrinterInformation[256] = EMPTY_STRING;
	TCHAR szStatus[256];

	HANDLE hPrinter;
	BOOL Res = OpenPrinter(m_pExtraItemInfo[InternalIndex].szDisplayName,&hPrinter,NULL);

	if(Res)
	{
		DWORD BytesNeeded;
		GetPrinter(hPrinter,2,NULL,0,&BytesNeeded);

		PRINTER_INFO_2 *PrinterInfo2 = reinterpret_cast<PRINTER_INFO_2 *>(new char[BytesNeeded]);
		Res = GetPrinter(hPrinter,2,reinterpret_cast<LPBYTE>(PrinterInfo2),BytesNeeded,&BytesNeeded);

		if(Res)
		{
			switch(PrinterInformationType)
			{
			case PRINTER_INFORMATION_TYPE_NUM_JOBS:
				StringCchPrintf(PrinterInformation,SIZEOF_ARRAY(PrinterInformation),
					_T("%d"),PrinterInfo2->cJobs);
				break;

			case PRINTER_INFORMATION_TYPE_STATUS:
				Res = GetPrinterStatusDescription(PrinterInfo2->Status, szStatus, SIZEOF_ARRAY(szStatus));

				if(Res)
				{
					StringCchCopyEx(PrinterInformation, SIZEOF_ARRAY(PrinterInformation),
						szStatus, NULL, NULL, STRSAFE_IGNORE_NULLS);
				}
				break;

			case PRINTER_INFORMATION_TYPE_COMMENTS:
				StringCchCopyEx(PrinterInformation,SIZEOF_ARRAY(PrinterInformation),
					PrinterInfo2->pComment,NULL,NULL,STRSAFE_IGNORE_NULLS);
				break;

			case PRINTER_INFORMATION_TYPE_LOCATION:
				StringCchCopyEx(PrinterInformation,SIZEOF_ARRAY(PrinterInformation),
					PrinterInfo2->pLocation,NULL,NULL,STRSAFE_IGNORE_NULLS);
				break;

			case PRINTER_INFORMATION_TYPE_MODEL:
				StringCchCopyEx(PrinterInformation,SIZEOF_ARRAY(PrinterInformation),
					PrinterInfo2->pDriverName,NULL,NULL,STRSAFE_IGNORE_NULLS);
				break;

			default:
				assert(false);
				break;
			}
		}

		delete[] PrinterInfo2;
		ClosePrinter(hPrinter);
	}

	return PrinterInformation;
}

BOOL GetPrinterStatusDescription(DWORD dwStatus, TCHAR *szStatus, size_t cchMax)
{
	BOOL bSuccess = TRUE;

	if(dwStatus == 0)
	{
		StringCchCopy(szStatus, cchMax, _T("Ready"));
	}
	else if(dwStatus & PRINTER_STATUS_BUSY)
	{
		StringCchCopy(szStatus, cchMax, _T("Busy"));
	}
	else if(dwStatus & PRINTER_STATUS_ERROR)
	{
		StringCchCopy(szStatus, cchMax, _T("Error"));
	}
	else if(dwStatus & PRINTER_STATUS_INITIALIZING)
	{
		StringCchCopy(szStatus, cchMax, _T("Initializing"));
	}
	else if(dwStatus & PRINTER_STATUS_IO_ACTIVE)
	{
		StringCchCopy(szStatus, cchMax, _T("Active"));
	}
	else if(dwStatus & PRINTER_STATUS_NOT_AVAILABLE)
	{
		StringCchCopy(szStatus, cchMax, _T("Unavailable"));
	}
	else if(dwStatus & PRINTER_STATUS_OFFLINE)
	{
		StringCchCopy(szStatus, cchMax, _T("Offline"));
	}
	else if(dwStatus & PRINTER_STATUS_OUT_OF_MEMORY)
	{
		StringCchCopy(szStatus, cchMax, _T("Out of memory"));
	}
	else if(dwStatus & PRINTER_STATUS_NO_TONER)
	{
		StringCchCopy(szStatus, cchMax, _T("Out of toner"));
	}
	else
	{
		bSuccess = FALSE;
	}

	return bSuccess;
}

std::wstring CShellBrowser::GetNetworkAdapterColumnText(int InternalIndex) const
{
	ULONG OutBufLen = 0;
	GetAdaptersAddresses(AF_UNSPEC,0,NULL,NULL,&OutBufLen);
	IP_ADAPTER_ADDRESSES *AdapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(new char[OutBufLen]);
	GetAdaptersAddresses(AF_UNSPEC,0,NULL,AdapterAddresses,&OutBufLen);

	IP_ADAPTER_ADDRESSES *AdapaterAddress = AdapterAddresses;

	while(AdapaterAddress != NULL &&
		lstrcmp(AdapaterAddress->FriendlyName,m_pwfdFiles[InternalIndex].cFileName) != 0)
	{
		AdapaterAddress = AdapaterAddress->Next;
	}

	std::wstring Status;

	/* TODO: These strings need to be setup correctly. */
	switch(AdapaterAddress->OperStatus)
	{
		case IfOperStatusUp:
			Status = L"Connected";
			break;

		case IfOperStatusDown:
			Status = L"Disconnected";
			break;

		case IfOperStatusTesting:
			Status = L"Testing";
			break;

		case IfOperStatusUnknown:
			Status = L"Unknown";
			break;

		case IfOperStatusDormant:
			Status = L"Dormant";
			break;

		case IfOperStatusNotPresent:
			Status = L"Not present";
			break;

		case IfOperStatusLowerLayerDown:
			Status = L"Lower layer non-operational";
			break;
	}

	delete[] AdapterAddresses;

	return Status;
}

std::wstring CShellBrowser::GetMediaMetadataColumnText(int InternalIndex,MediaMetadataType_t MediaMetaDataType) const
{
	TCHAR FullFileName[MAX_PATH];
	QueryFullItemNameInternal(InternalIndex,FullFileName,SIZEOF_ARRAY(FullFileName));

	const TCHAR *AttributeName = GetMediaMetadataAttributeName(MediaMetaDataType);

	BYTE *TempBuffer = NULL;
	HRESULT hr = GetMediaMetadata(FullFileName,AttributeName,&TempBuffer);

	if(!SUCCEEDED(hr))
	{
		return EMPTY_STRING;
	}

	TCHAR szOutput[512];

	switch(MediaMetaDataType)
	{
	case MEDIAMETADATA_TYPE_BITRATE:
		{
			DWORD BitRate = *(reinterpret_cast<DWORD *>(TempBuffer));

			if(BitRate > 1000)
			{
				StringCchPrintf(szOutput,SIZEOF_ARRAY(szOutput),_T("%d kbps"),BitRate / 1000);
			}
			else
			{
				StringCchPrintf(szOutput,SIZEOF_ARRAY(szOutput),_T("%d bps"),BitRate);
			}
		}
		break;

	case MEDIAMETADATA_TYPE_DURATION:
		{
			boost::posix_time::wtime_facet *Facet = new boost::posix_time::wtime_facet();
			Facet->time_duration_format(L"%H:%M:%S");

			std::wstringstream DateStream;
			DateStream.imbue(std::locale(DateStream.getloc(),Facet));

			/* Note that the duration itself is in 100-nanosecond units
			(see http://msdn.microsoft.com/en-us/library/windows/desktop/dd798053(v=vs.85).aspx). */
			boost::posix_time::time_duration Duration = boost::posix_time::microseconds(*(reinterpret_cast<QWORD *>(TempBuffer)) / 10);
			DateStream << Duration;

			StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),DateStream.str().c_str());
		}
		break;

	case MEDIAMETADATA_TYPE_PROTECTED:
		if(*(reinterpret_cast<BOOL *>(TempBuffer)))
		{
			StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),L"Yes");
		}
		else
		{
			StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),L"No");
		}
		break;

	case MEDIAMETADATA_TYPE_COPYRIGHT:
	case MEDIAMETADATA_TYPE_RATING:
	case MEDIAMETADATA_TYPE_ALBUM_ARTIST:
	case MEDIAMETADATA_TYPE_ALBUM_TITLE:
	case MEDIAMETADATA_TYPE_BEATS_PER_MINUTE:
	case MEDIAMETADATA_TYPE_COMPOSER:
	case MEDIAMETADATA_TYPE_CONDUCTOR:
	case MEDIAMETADATA_TYPE_DIRECTOR:
	case MEDIAMETADATA_TYPE_GENRE:
	case MEDIAMETADATA_TYPE_LANGUAGE:
	case MEDIAMETADATA_TYPE_BROADCASTDATE:
	case MEDIAMETADATA_TYPE_CHANNEL:
	case MEDIAMETADATA_TYPE_STATIONNAME:
	case MEDIAMETADATA_TYPE_MOOD:
	case MEDIAMETADATA_TYPE_PARENTALRATING:
	case MEDIAMETADATA_TYPE_PARENTALRATINGREASON:
	case MEDIAMETADATA_TYPE_PERIOD:
	case MEDIAMETADATA_TYPE_PRODUCER:
	case MEDIAMETADATA_TYPE_PUBLISHER:
	case MEDIAMETADATA_TYPE_WRITER:
	case MEDIAMETADATA_TYPE_YEAR:
	default:
		StringCchCopy(szOutput,SIZEOF_ARRAY(szOutput),reinterpret_cast<TCHAR *>(TempBuffer));
		break;
	}

	free(TempBuffer);

	return szOutput;
}

const TCHAR *CShellBrowser::GetMediaMetadataAttributeName(MediaMetadataType_t MediaMetaDataType) const
{
	switch(MediaMetaDataType)
	{
	case MEDIAMETADATA_TYPE_BITRATE:
		return g_wszWMBitrate;
		break;

	case MEDIAMETADATA_TYPE_COPYRIGHT:
		return g_wszWMCopyright;
		break;

	case MEDIAMETADATA_TYPE_DURATION:
		return g_wszWMDuration;
		break;

	case MEDIAMETADATA_TYPE_PROTECTED:
		return g_wszWMProtected;
		break;

	case MEDIAMETADATA_TYPE_RATING:
		return g_wszWMRating;
		break;

	case MEDIAMETADATA_TYPE_ALBUM_ARTIST:
		return g_wszWMAlbumArtist;
		break;

	case MEDIAMETADATA_TYPE_ALBUM_TITLE:
		return g_wszWMAlbumTitle;
		break;

	case MEDIAMETADATA_TYPE_BEATS_PER_MINUTE:
		return g_wszWMBeatsPerMinute;
		break;

	case MEDIAMETADATA_TYPE_COMPOSER:
		return g_wszWMComposer;
		break;

	case MEDIAMETADATA_TYPE_CONDUCTOR:
		return g_wszWMConductor;
		break;

	case MEDIAMETADATA_TYPE_DIRECTOR:
		return g_wszWMDirector;
		break;

	case MEDIAMETADATA_TYPE_GENRE:
		return g_wszWMGenre;
		break;

	case MEDIAMETADATA_TYPE_LANGUAGE:
		return g_wszWMLanguage;
		break;

	case MEDIAMETADATA_TYPE_BROADCASTDATE:
		return g_wszWMMediaOriginalBroadcastDateTime;
		break;

	case MEDIAMETADATA_TYPE_CHANNEL:
		return g_wszWMMediaOriginalChannel;
		break;

	case MEDIAMETADATA_TYPE_STATIONNAME:
		return g_wszWMMediaStationName;
		break;

	case MEDIAMETADATA_TYPE_MOOD:
		return g_wszWMMood;
		break;

	case MEDIAMETADATA_TYPE_PARENTALRATING:
		return g_wszWMParentalRating;
		break;

	case MEDIAMETADATA_TYPE_PARENTALRATINGREASON:
		return g_wszWMParentalRatingReason;
		break;

	case MEDIAMETADATA_TYPE_PERIOD:
		return g_wszWMPeriod;
		break;

	case MEDIAMETADATA_TYPE_PRODUCER:
		return g_wszWMProducer;
		break;

	case MEDIAMETADATA_TYPE_PUBLISHER:
		return g_wszWMPublisher;
		break;

	case MEDIAMETADATA_TYPE_WRITER:
		return g_wszWMWriter;
		break;

	case MEDIAMETADATA_TYPE_YEAR:
		return g_wszWMYear;
		break;

	default:
		assert(false);
		break;
	}

	return NULL;
}

void CShellBrowser::PlaceColumns(void)
{
	std::list<Column_t>::iterator	itr;
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

void CShellBrowser::InsertColumn(unsigned int ColumnId,int iColumnIndex,int iWidth)
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

	if(ColumnId == CM_SIZE || ColumnId == CM_REALSIZE || 
		ColumnId == CM_TOTALSIZE || ColumnId == CM_FREESPACE)
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

void CShellBrowser::SetActiveColumnSet(void)
{
	std::list<Column_t> *pActiveColumnList = NULL;

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

unsigned int CShellBrowser::DetermineColumnSortMode(int iColumnId) const
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

		case CM_MEDIA_BITRATE:
			return FSM_MEDIA_BITRATE;
			break;

		case CM_MEDIA_COPYRIGHT:
			return FSM_MEDIA_COPYRIGHT;
			break;

		case CM_MEDIA_DURATION:
			return FSM_MEDIA_DURATION;
			break;

		case CM_MEDIA_PROTECTED:
			return FSM_MEDIA_PROTECTED;
			break;

		case CM_MEDIA_RATING:
			return FSM_MEDIA_RATING;
			break;

		case CM_MEDIA_ALBUMARTIST:
			return FSM_MEDIA_ALBUMARTIST;
			break;

		case CM_MEDIA_ALBUM:
			return FSM_MEDIA_ALBUM;
			break;

		case CM_MEDIA_BEATSPERMINUTE:
			return FSM_MEDIA_BEATSPERMINUTE;
			break;

		case CM_MEDIA_COMPOSER:
			return FSM_MEDIA_COMPOSER;
			break;

		case CM_MEDIA_CONDUCTOR:
			return FSM_MEDIA_CONDUCTOR;
			break;

		case CM_MEDIA_DIRECTOR:
			return FSM_MEDIA_DIRECTOR;
			break;

		case CM_MEDIA_GENRE:
			return FSM_MEDIA_GENRE;
			break;

		case CM_MEDIA_LANGUAGE:
			return FSM_MEDIA_LANGUAGE;
			break;

		case CM_MEDIA_BROADCASTDATE:
			return FSM_MEDIA_BROADCASTDATE;
			break;

		case CM_MEDIA_CHANNEL:
			return FSM_MEDIA_CHANNEL;
			break;

		case CM_MEDIA_STATIONNAME:
			return FSM_MEDIA_STATIONNAME;
			break;

		case CM_MEDIA_MOOD:
			return FSM_MEDIA_MOOD;
			break;

		case CM_MEDIA_PARENTALRATING:
			return FSM_MEDIA_PARENTALRATING;
			break;

		case CM_MEDIA_PARENTALRATINGREASON:
			return FSM_MEDIA_PARENTALRATINGREASON;
			break;

		case CM_MEDIA_PERIOD:
			return FSM_MEDIA_PERIOD;
			break;

		case CM_MEDIA_PRODUCER:
			return FSM_MEDIA_PRODUCER;
			break;

		case CM_MEDIA_PUBLISHER:
			return FSM_MEDIA_PUBLISHER;
			break;

		case CM_MEDIA_WRITER:
			return FSM_MEDIA_WRITER;
			break;

		case CM_MEDIA_YEAR:
			return FSM_MEDIA_YEAR;
			break;

		default:
			assert(false);
			break;
	}

	return 0;
}

void CShellBrowser::ColumnClicked(int iClickedColumn)
{
	std::list<Column_t>::iterator itr;
	int iCurrentColumn = 0;
	UINT SortMode = 0;
	UINT iColumnId = 0;

	for(itr = m_pActiveColumnList->begin();itr != m_pActiveColumnList->end();itr++)
	{
		/* Only increment if this column is actually been shown. */
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

void CShellBrowser::ApplyHeaderSortArrow(void)
{
	HWND hHeader;
	HDITEM hdItem;
	std::list<Column_t>::iterator itr;
	BOOL previousColumnFound = FALSE;
	int iColumn = 0;
	int iPreviousSortedColumn = 0;
	int iColumnId = -1;

	hHeader = ListView_GetHeader(m_hListView);

	if (m_PreviousSortColumnExists)
	{
		/* Search through the currently active columns to find the column that previously
		had the up/down arrow. */
		for (itr = m_pActiveColumnList->begin(); itr != m_pActiveColumnList->end(); itr++)
		{
			/* Only increment if this column is actually been shown. */
			if (itr->bChecked)
			{
				if (m_iPreviousSortedColumnId == itr->id)
				{
					previousColumnFound = TRUE;
					break;
				}

				iPreviousSortedColumn++;
			}
		}

		if (previousColumnFound)
		{
			hdItem.mask = HDI_FORMAT;
			Header_GetItem(hHeader, iPreviousSortedColumn, &hdItem);

			if (hdItem.fmt & HDF_SORTUP)
			{
				hdItem.fmt &= ~HDF_SORTUP;
			}
			else if (hdItem.fmt & HDF_SORTDOWN)
			{
				hdItem.fmt &= ~HDF_SORTDOWN;
			}

			/* Remove the up/down arrow from the column by which
			results were previously sorted. */
			Header_SetItem(hHeader, iPreviousSortedColumn, &hdItem);
		}
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
	m_PreviousSortColumnExists = true;
}

size_t CShellBrowser::QueryNumActiveColumns(void) const
{
	return m_pActiveColumnList->size();
}

void CShellBrowser::ImportAllColumns(const ColumnExport_t *pce)
{
	m_ControlPanelColumnList = pce->ControlPanelColumnList;
	m_MyComputerColumnList = pce->MyComputerColumnList;
	m_MyNetworkPlacesColumnList = pce->MyNetworkPlacesColumnList;
	m_NetworkConnectionsColumnList = pce->NetworkConnectionsColumnList;
	m_PrintersColumnList = pce->PrintersColumnList;
	m_RealFolderColumnList = pce->RealFolderColumnList;
	m_RecycleBinColumnList = pce->RecycleBinColumnList;
}

void CShellBrowser::ExportAllColumns(ColumnExport_t *pce)
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

void CShellBrowser::SaveColumnWidths(void)
{
	std::list<Column_t> *pActiveColumnList = NULL;
	std::list<Column_t>::iterator itr;
	int iColumn = 0;

	if(CompareVirtualFolders(CSIDL_CONTROLS))
	{
		pActiveColumnList = &m_ControlPanelColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_DRIVES))
	{
		pActiveColumnList = &m_MyComputerColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_BITBUCKET))
	{
		pActiveColumnList = &m_RecycleBinColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_PRINTERS))
	{
		pActiveColumnList = &m_PrintersColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_CONNECTIONS))
	{
		pActiveColumnList = &m_NetworkConnectionsColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_NETWORK))
	{
		pActiveColumnList = &m_MyNetworkPlacesColumnList;
	}
	else
	{
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