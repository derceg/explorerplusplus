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
#include <boost\date_time\posix_time\posix_time.hpp>
#include <iphlpapi.h>
#include <propkey.h>
#include "Columns.h"
#include "IShellView.h"
#include "iShellBrowser_internal.h"
#include "MainResource.h"
#include "SortModes.h"
#include "ViewModes.h"
#include "../Helper/Helper.h"
#include "../Helper/DriveInfo.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/FolderSize.h"
#include "../Helper/Macros.h"


BOOL GetPrinterStatusDescription(DWORD dwStatus, TCHAR *szStatus, size_t cchMax);

void CShellBrowser::QueueColumnTask(int itemInternalIndex, int columnIndex)
{
	auto columnID = GetColumnIdByIndex(columnIndex);

	if (!columnID)
	{
		return;
	}

	int columnResultID = m_columnResultIDCounter++;

	auto result = m_columnThreadPool.push([this, columnResultID, columnID, itemInternalIndex](int id) {
		UNREFERENCED_PARAMETER(id);

		return this->GetColumnTextAsync(columnResultID, *columnID, itemInternalIndex);
	});

	// The function call above might finish before this line runs,
	// but that doesn't matter, as the results won't be processed
	// until a message posted to the main thread has been handled
	// (which can only occur after this function has returned).
	m_columnResults.insert({ columnResultID, std::move(result) });
}

CShellBrowser::ColumnResult_t CShellBrowser::GetColumnTextAsync(int columnResultId, unsigned int ColumnID, int InternalIndex) const
{
	std::wstring columnText = GetColumnText(ColumnID, InternalIndex);

	// This message may be delivered before this function has returned.
	// That doesn't actually matter, since the message handler will
	// simply wait for the result to be returned.
	PostMessage(m_hListView, WM_APP_COLUMN_RESULT_READY, columnResultId, 0);

	ColumnResult_t result;
	result.itemInternalIndex = InternalIndex;
	result.columnID = ColumnID;
	result.columnText = columnText;

	return result;
}

void CShellBrowser::ProcessColumnResult(int columnResultId)
{
	auto itr = m_columnResults.find(columnResultId);

	if (itr == m_columnResults.end())
	{
		// This result is for a previous folder. It can be ignored.
		return;
	}

	if (m_ViewMode != VM_DETAILS)
	{
		return;
	}

	auto result = itr->second.get();

	auto index = LocateItemByInternalIndex(result.itemInternalIndex);

	if (!index)
	{
		// This is a valid state. The item may simply have been deleted.
		return;
	}

	auto columnIndex = GetColumnIndexById(result.columnID);

	if (!columnIndex)
	{
		// This is also a valid state. The column may have been removed.
		return;
	}

	auto columnText = std::make_unique<TCHAR[]>(result.columnText.size() + 1);
	StringCchCopy(columnText.get(), result.columnText.size() + 1, result.columnText.c_str());
	ListView_SetItemText(m_hListView, *index, *columnIndex, columnText.get());

	m_columnResults.erase(itr);
}

boost::optional<int> CShellBrowser::GetColumnIndexById(unsigned int id) const
{
	HWND header = ListView_GetHeader(m_hListView);

	int numItems = Header_GetItemCount(header);

	for (int i = 0; i < numItems; i++)
	{
		HDITEM hdItem;
		hdItem.mask = HDI_LPARAM;
		BOOL res = Header_GetItem(header, i, &hdItem);

		if (!res)
		{
			continue;
		}

		if (hdItem.lParam == id)
		{
			return i;
		}
	}

	return boost::none;
}

boost::optional<unsigned int> CShellBrowser::GetColumnIdByIndex(int index) const
{
	HWND hHeader = ListView_GetHeader(m_hListView);

	HDITEM hdItem;
	hdItem.mask = HDI_LPARAM;
	BOOL res = Header_GetItem(hHeader, index, &hdItem);

	if (!res)
	{
		return boost::none;
	}

	return hdItem.lParam;
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
	const ItemInfo_t &itemInfo = m_itemInfoMap.at(InternalIndex);

	switch(ColumnID)
	{
	case CM_NAME:
		return GetNameColumnText(itemInfo);
		break;

	case CM_TYPE:
		return GetTypeColumnText(itemInfo);
		break;
	case CM_SIZE:
		return GetSizeColumnText(InternalIndex);
		break;

	case CM_DATEMODIFIED:
		return GetTimeColumnText(itemInfo,COLUMN_TIME_MODIFIED);
		break;
	case CM_CREATED:
		return GetTimeColumnText(itemInfo,COLUMN_TIME_CREATED);
		break;
	case CM_ACCESSED:
		return GetTimeColumnText(itemInfo,COLUMN_TIME_ACCESSED);
		break;

	case CM_ATTRIBUTES:
		return GetAttributeColumnText(itemInfo);
		break;
	case CM_REALSIZE:
		return GetRealSizeColumnText(itemInfo);
		break;
	case CM_SHORTNAME:
		return GetShortNameColumnText(itemInfo);
		break;
	case CM_OWNER:
		return GetOwnerColumnText(itemInfo);
		break;

	case CM_PRODUCTNAME:
		return GetVersionColumnText(itemInfo,VERSION_INFO_PRODUCT_NAME);
		break;
	case CM_COMPANY:
		return GetVersionColumnText(itemInfo,VERSION_INFO_COMPANY);
		break;
	case CM_DESCRIPTION:
		return GetVersionColumnText(itemInfo,VERSION_INFO_DESCRIPTION);
		break;
	case CM_FILEVERSION:
		return GetVersionColumnText(itemInfo,VERSION_INFO_FILE_VERSION);
		break;
	case CM_PRODUCTVERSION:
		return GetVersionColumnText(itemInfo,VERSION_INFO_PRODUCT_VERSION);
		break;

	case CM_SHORTCUTTO:
		return GetShortcutToColumnText(itemInfo);
		break;
	case CM_HARDLINKS:
		return GetHardLinksColumnText(itemInfo);
		break;
	case CM_EXTENSION:
		return GetExtensionColumnText(itemInfo);
		break;

	case CM_TITLE:
		return GetItemDetailsColumnText(itemInfo, &PKEY_Title);
		break;
	case CM_SUBJECT:
		return GetItemDetailsColumnText(itemInfo, &PKEY_Subject);
		break;
	case CM_AUTHORS:
		return GetItemDetailsColumnText(itemInfo, &PKEY_Author);
		break;
	case CM_KEYWORDS:
		return GetItemDetailsColumnText(itemInfo, &PKEY_Keywords);
		break;
	case CM_COMMENT:
		return GetItemDetailsColumnText(itemInfo, &PKEY_Comment);
		break;

	case CM_CAMERAMODEL:
		return GetImageColumnText(itemInfo,PropertyTagEquipModel);
		break;
	case CM_DATETAKEN:
		return GetImageColumnText(itemInfo,PropertyTagDateTime);
		break;
	case CM_WIDTH:
		return GetImageColumnText(itemInfo,PropertyTagImageWidth);
		break;
	case CM_HEIGHT:
		return GetImageColumnText(itemInfo,PropertyTagImageHeight);
		break;

	case CM_VIRTUALCOMMENTS:
		return GetControlPanelCommentsColumnText(itemInfo);
		break;

	case CM_TOTALSIZE:
		return GetDriveSpaceColumnText(itemInfo,true);
		break;

	case CM_FREESPACE:
		return GetDriveSpaceColumnText(itemInfo,false);
		break;

	case CM_FILESYSTEM:
		return GetFileSystemColumnText(itemInfo);
		break;

	case CM_ORIGINALLOCATION:
		return GetItemDetailsColumnText(itemInfo, &SCID_ORIGINAL_LOCATION);
		break;

	case CM_DATEDELETED:
		return GetItemDetailsColumnText(itemInfo, &SCID_DATE_DELETED);
		break;

	case CM_NUMPRINTERDOCUMENTS:
		return GetPrinterColumnText(itemInfo,PRINTER_INFORMATION_TYPE_NUM_JOBS);
		break;

	case CM_PRINTERSTATUS:
		return GetPrinterColumnText(itemInfo,PRINTER_INFORMATION_TYPE_STATUS);
		break;

	case CM_PRINTERCOMMENTS:
		return GetPrinterColumnText(itemInfo,PRINTER_INFORMATION_TYPE_COMMENTS);
		break;

	case CM_PRINTERLOCATION:
		return GetPrinterColumnText(itemInfo,PRINTER_INFORMATION_TYPE_LOCATION);
		break;

	case CM_PRINTERMODEL:
		return GetPrinterColumnText(itemInfo,PRINTER_INFORMATION_TYPE_MODEL);
		break;

	case CM_NETWORKADAPTER_STATUS:
		return GetNetworkAdapterColumnText(itemInfo);
		break;

	case CM_MEDIA_BITRATE:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_BITRATE);
		break;
	case CM_MEDIA_COPYRIGHT:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_COPYRIGHT);
		break;
	case CM_MEDIA_DURATION:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_DURATION);
		break;
	case CM_MEDIA_PROTECTED:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_PROTECTED);
		break;
	case CM_MEDIA_RATING:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_RATING);
		break;
	case CM_MEDIA_ALBUMARTIST:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_ALBUM_ARTIST);
		break;
	case CM_MEDIA_ALBUM:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_ALBUM_TITLE);
		break;
	case CM_MEDIA_BEATSPERMINUTE:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_BEATS_PER_MINUTE);
		break;
	case CM_MEDIA_COMPOSER:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_COMPOSER);
		break;
	case CM_MEDIA_CONDUCTOR:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_CONDUCTOR);
		break;
	case CM_MEDIA_DIRECTOR:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_DIRECTOR);
		break;
	case CM_MEDIA_GENRE:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_GENRE);
		break;
	case CM_MEDIA_LANGUAGE:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_LANGUAGE);
		break;
	case CM_MEDIA_BROADCASTDATE:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_BROADCASTDATE);
		break;
	case CM_MEDIA_CHANNEL:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_CHANNEL);
		break;
	case CM_MEDIA_STATIONNAME:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_STATIONNAME);
		break;
	case CM_MEDIA_MOOD:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_MOOD);
		break;
	case CM_MEDIA_PARENTALRATING:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_PARENTALRATING);
		break;
	case CM_MEDIA_PARENTALRATINGREASON:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_PARENTALRATINGREASON);
		break;
	case CM_MEDIA_PERIOD:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_PERIOD);
		break;
	case CM_MEDIA_PRODUCER:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_PRODUCER);
		break;
	case CM_MEDIA_PUBLISHER:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_PUBLISHER);
		break;
	case CM_MEDIA_WRITER:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_WRITER);
		break;
	case CM_MEDIA_YEAR:
		return GetMediaMetadataColumnText(itemInfo,MEDIAMETADATA_TYPE_YEAR);
		break;

	default:
		assert(false);
		break;
	}

	return EMPTY_STRING;
}

std::wstring CShellBrowser::GetNameColumnText(const ItemInfo_t &itemInfo) const
{
	return ProcessItemFileName(itemInfo);
}

std::wstring CShellBrowser::GetTypeColumnText(const ItemInfo_t &itemInfo) const
{
	SHFILEINFO shfi;
	DWORD_PTR Res = SHGetFileInfo(reinterpret_cast<LPTSTR>(itemInfo.pidlComplete.get()),
		0,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_TYPENAME);

	if(Res == 0)
	{
		return EMPTY_STRING;
	}

	return shfi.szTypeName;
}

std::wstring CShellBrowser::GetSizeColumnText(int InternalIndex) const
{
	if((m_itemInfoMap.at(InternalIndex).wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		TCHAR fullItemPath[MAX_PATH];
		QueryFullItemNameInternal(InternalIndex, fullItemPath, SIZEOF_ARRAY(fullItemPath));

		TCHAR drive[MAX_PATH];
		StringCchCopy(drive, SIZEOF_ARRAY(drive), fullItemPath);
		PathStripToRoot(drive);

		bool bNetworkRemovable = false;

		if (GetDriveType(drive) == DRIVE_REMOVABLE ||
			GetDriveType(drive) == DRIVE_REMOTE)
		{
			bNetworkRemovable = true;
		}

		if (m_bShowFolderSizes && !(m_bDisableFolderSizesNetworkRemovable && bNetworkRemovable))
		{
			return GetFolderSizeColumnText(InternalIndex);
		}
		else
		{
			return EMPTY_STRING;
		}
	}

	ULARGE_INTEGER FileSize = {m_itemInfoMap.at(InternalIndex).wfd.nFileSizeLow,m_itemInfoMap.at(InternalIndex).wfd.nFileSizeHigh};

	TCHAR FileSizeText[64];
	FormatSizeString(FileSize,FileSizeText,SIZEOF_ARRAY(FileSizeText),m_bForceSize,m_SizeDisplayFormat);

	return FileSizeText;
}

std::wstring CShellBrowser::GetFolderSizeColumnText(int internalIndex) const
{
	TCHAR fullItemPath[MAX_PATH];
	QueryFullItemNameInternal(internalIndex, fullItemPath, SIZEOF_ARRAY(fullItemPath));

	int numFolders;
	int numFiles;
	ULARGE_INTEGER totalFolderSize;
	CalculateFolderSize(fullItemPath, &numFolders, &numFiles, &totalFolderSize);

	m_cachedFolderSizes.insert({internalIndex, totalFolderSize.QuadPart});

	TCHAR fileSizeText[64];
	FormatSizeString(totalFolderSize, fileSizeText, SIZEOF_ARRAY(fileSizeText),
		m_bForceSize, m_SizeDisplayFormat);

	return fileSizeText;
}

std::wstring CShellBrowser::GetTimeColumnText(const ItemInfo_t &itemInfo,TimeType_t TimeType) const
{
	TCHAR FileTime[64];
	BOOL bRet = FALSE;

	switch(TimeType)
	{
	case COLUMN_TIME_MODIFIED:
		bRet = CreateFileTimeString(&itemInfo.wfd.ftLastWriteTime,
			FileTime,SIZEOF_ARRAY(FileTime),m_bShowFriendlyDates);
		break;

	case COLUMN_TIME_CREATED:
		bRet = CreateFileTimeString(&itemInfo.wfd.ftCreationTime,
			FileTime,SIZEOF_ARRAY(FileTime),m_bShowFriendlyDates);
		break;

	case COLUMN_TIME_ACCESSED:
		bRet = CreateFileTimeString(&itemInfo.wfd.ftLastAccessTime,
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

std::wstring CShellBrowser::GetAttributeColumnText(const ItemInfo_t &itemInfo) const
{
	TCHAR AttributeString[32];
	BuildFileAttributeString(itemInfo.getFullPath().c_str(),AttributeString,SIZEOF_ARRAY(AttributeString));

	return AttributeString;
}

bool CShellBrowser::GetRealSizeColumnRawData(const ItemInfo_t &itemInfo,ULARGE_INTEGER &RealFileSize) const
{
	if((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
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

	ULARGE_INTEGER RealFileSizeTemp = {itemInfo.wfd.nFileSizeLow,itemInfo.wfd.nFileSizeHigh};

	if(RealFileSizeTemp.QuadPart != 0 && (RealFileSizeTemp.QuadPart % dwClusterSize) != 0)
	{
		RealFileSizeTemp.QuadPart += dwClusterSize - (RealFileSizeTemp.QuadPart % dwClusterSize);
	}

	RealFileSize = RealFileSizeTemp;

	return true;
}

std::wstring CShellBrowser::GetRealSizeColumnText(const ItemInfo_t &itemInfo) const
{
	ULARGE_INTEGER RealFileSize;
	bool Res = GetRealSizeColumnRawData(itemInfo,RealFileSize);

	if(!Res)
	{
		return EMPTY_STRING;
	}

	TCHAR RealFileSizeText[32];
	FormatSizeString(RealFileSize,RealFileSizeText,SIZEOF_ARRAY(RealFileSizeText),
		m_bForceSize,m_SizeDisplayFormat);

	return RealFileSizeText;
}

std::wstring CShellBrowser::GetShortNameColumnText(const ItemInfo_t &itemInfo) const
{
	if(lstrlen(itemInfo.wfd.cAlternateFileName) == 0)
	{
		return itemInfo.wfd.cFileName;
	}

	return itemInfo.wfd.cAlternateFileName;
}

std::wstring CShellBrowser::GetOwnerColumnText(const ItemInfo_t &itemInfo) const
{
	TCHAR Owner[512];
	BOOL ret = GetFileOwner(itemInfo.getFullPath().c_str(),Owner,SIZEOF_ARRAY(Owner));

	if(!ret)
	{
		return EMPTY_STRING;
	}

	return Owner;
}

std::wstring CShellBrowser::GetVersionColumnText(const ItemInfo_t &itemInfo, VersionInfoType_t VersioninfoType) const
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

	TCHAR VersionInfo[512];
	BOOL VersionInfoObtained = GetVersionInfoString(itemInfo.getFullPath().c_str(),VersionInfoName.c_str(),
		VersionInfo,SIZEOF_ARRAY(VersionInfo));

	if(!VersionInfoObtained)
	{
		return EMPTY_STRING;
	}

	return VersionInfo;
}

std::wstring CShellBrowser::GetShortcutToColumnText(const ItemInfo_t &itemInfo) const
{
	TCHAR ResolvedLinkPath[MAX_PATH];
	HRESULT hr = NFileOperations::ResolveLink(NULL,SLR_NO_UI,itemInfo.getFullPath().c_str(),
		ResolvedLinkPath,SIZEOF_ARRAY(ResolvedLinkPath));

	if(FAILED(hr))
	{
		return EMPTY_STRING;	
	}

	return ResolvedLinkPath;
}

DWORD CShellBrowser::GetHardLinksColumnRawData(const ItemInfo_t &itemInfo) const
{
	return GetNumFileHardLinks(itemInfo.getFullPath().c_str());
}

std::wstring CShellBrowser::GetHardLinksColumnText(const ItemInfo_t &itemInfo) const
{
	DWORD NumHardLinks = GetHardLinksColumnRawData(itemInfo);

	if(NumHardLinks == -1)
	{
		return EMPTY_STRING;
	}

	TCHAR NumHardLinksString[32];
	StringCchPrintf(NumHardLinksString,SIZEOF_ARRAY(NumHardLinksString),_T("%ld"),NumHardLinks);
	
	return NumHardLinksString;
}

std::wstring CShellBrowser::GetExtensionColumnText(const ItemInfo_t &itemInfo) const
{
	if((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		return EMPTY_STRING;
	}

	TCHAR *Extension = PathFindExtension(itemInfo.wfd.cFileName);

	if(*Extension != '.')
	{
		return EMPTY_STRING;
	}

	return Extension + 1;
}

std::wstring CShellBrowser::GetItemDetailsColumnText(const ItemInfo_t &itemInfo, const SHCOLUMNID *pscid) const
{
	TCHAR szDetail[512];
	HRESULT hr = GetItemDetails(itemInfo, pscid, szDetail, SIZEOF_ARRAY(szDetail));

	if(SUCCEEDED(hr))
	{
		return szDetail;
	}

	return EMPTY_STRING;
}

HRESULT CShellBrowser::GetItemDetails(const ItemInfo_t &itemInfo, const SHCOLUMNID *pscid, TCHAR *szDetail, size_t cchMax) const
{
	VARIANT vt;
	HRESULT hr = GetItemDetailsRawData(itemInfo, pscid, &vt);

	if (SUCCEEDED(hr))
	{
		hr = ConvertVariantToString(&vt, szDetail, cchMax, m_bShowFriendlyDates);
		VariantClear(&vt);
	}

	return hr;
}

HRESULT CShellBrowser::GetItemDetailsRawData(const ItemInfo_t &itemInfo, const SHCOLUMNID *pscid, VARIANT *vt) const
{
	IShellFolder2 *pShellFolder = NULL;
	HRESULT hr = BindToIdl(m_pidlDirectory, IID_PPV_ARGS(&pShellFolder));

	if (SUCCEEDED(hr))
	{
		hr = pShellFolder->GetDetailsEx(itemInfo.pridl.get(), pscid, vt);
		pShellFolder->Release();
	}

	return hr;
}

std::wstring CShellBrowser::GetImageColumnText(const ItemInfo_t &itemInfo, PROPID PropertyID) const
{
	TCHAR ImageProperty[512];
	BOOL Res = ReadImageProperty(itemInfo.getFullPath().c_str(),PropertyID,ImageProperty,
		SIZEOF_ARRAY(ImageProperty));

	if(!Res)
	{
		return EMPTY_STRING;
	}

	return ImageProperty;
}

std::wstring CShellBrowser::GetFileSystemColumnText(const ItemInfo_t &itemInfo) const
{
	TCHAR FullFileName[MAX_PATH];
	GetDisplayName(itemInfo.pidlComplete.get(),FullFileName,SIZEOF_ARRAY(FullFileName),SHGDN_FORPARSING);

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

BOOL CShellBrowser::GetDriveSpaceColumnRawData(const ItemInfo_t &itemInfo,bool TotalSize,ULARGE_INTEGER &DriveSpace) const
{
	TCHAR FullFileName[MAX_PATH];
	GetDisplayName(itemInfo.pidlComplete.get(),FullFileName,
		SIZEOF_ARRAY(FullFileName),SHGDN_FORPARSING);

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

std::wstring CShellBrowser::GetDriveSpaceColumnText(const ItemInfo_t &itemInfo,bool TotalSize) const
{
	ULARGE_INTEGER DriveSpace;
	BOOL Res = GetDriveSpaceColumnRawData(itemInfo,TotalSize,DriveSpace);

	if(!Res)
	{
		return EMPTY_STRING;
	}

	TCHAR SizeText[32];
	FormatSizeString(DriveSpace,SizeText,SIZEOF_ARRAY(SizeText),m_bForceSize,m_SizeDisplayFormat);

	return SizeText;
}

std::wstring CShellBrowser::GetControlPanelCommentsColumnText(const ItemInfo_t &itemInfo) const
{
	TCHAR InfoTip[512];
	HRESULT hr = GetItemInfoTip(itemInfo.getFullPath().c_str(), InfoTip, SIZEOF_ARRAY(InfoTip));

	if(FAILED(hr))
	{
		return EMPTY_STRING;
	}

	ReplaceCharacter(InfoTip,'\n',' ');

	return InfoTip;
}

std::wstring CShellBrowser::GetPrinterColumnText(const ItemInfo_t &itemInfo,PrinterInformationType_t PrinterInformationType) const
{
	TCHAR PrinterInformation[256] = EMPTY_STRING;
	TCHAR szStatus[256];

	TCHAR itemDisplayName[MAX_PATH];
	StringCchCopy(itemDisplayName, SIZEOF_ARRAY(itemDisplayName), itemInfo.szDisplayName);

	HANDLE hPrinter;
	BOOL Res = OpenPrinter(itemDisplayName,&hPrinter,NULL);

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

std::wstring CShellBrowser::GetNetworkAdapterColumnText(const ItemInfo_t &itemInfo) const
{
	ULONG OutBufLen = 0;
	GetAdaptersAddresses(AF_UNSPEC,0,NULL,NULL,&OutBufLen);
	IP_ADAPTER_ADDRESSES *AdapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(new char[OutBufLen]);
	GetAdaptersAddresses(AF_UNSPEC,0,NULL,AdapterAddresses,&OutBufLen);

	IP_ADAPTER_ADDRESSES *AdapaterAddress = AdapterAddresses;

	while(AdapaterAddress != NULL &&
		lstrcmp(AdapaterAddress->FriendlyName,itemInfo.wfd.cFileName) != 0)
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

std::wstring CShellBrowser::GetMediaMetadataColumnText(const ItemInfo_t &itemInfo, MediaMetadataType_t MediaMetaDataType) const
{
	const TCHAR *AttributeName = GetMediaMetadataAttributeName(MediaMetaDataType);

	BYTE *TempBuffer = NULL;
	HRESULT hr = GetMediaMetadata(itemInfo.getFullPath().c_str(),AttributeName,&TempBuffer);

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

	iStringIndex = LookupColumnNameStringIndex(ColumnId);

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

unsigned int CShellBrowser::DetermineColumnSortMode(int iColumnId)
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

		case CM_AUTHORS:
			return FSM_AUTHORS;
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

int CShellBrowser::LookupColumnNameStringIndex(int iColumnId)
{
	switch (iColumnId)
	{
	case CM_NAME:
		return IDS_COLUMN_NAME_NAME;
		break;

	case CM_TYPE:
		return IDS_COLUMN_NAME_TYPE;
		break;

	case CM_SIZE:
		return IDS_COLUMN_NAME_SIZE;
		break;

	case CM_DATEMODIFIED:
		return IDS_COLUMN_NAME_DATEMODIFIED;
		break;

	case CM_ATTRIBUTES:
		return IDS_COLUMN_NAME_ATTRIBUTES;
		break;

	case CM_REALSIZE:
		return IDS_COLUMN_NAME_REALSIZE;
		break;

	case CM_SHORTNAME:
		return IDS_COLUMN_NAME_SHORTNAME;
		break;

	case CM_OWNER:
		return IDS_COLUMN_NAME_OWNER;
		break;

	case CM_PRODUCTNAME:
		return IDS_COLUMN_NAME_PRODUCTNAME;
		break;

	case CM_COMPANY:
		return IDS_COLUMN_NAME_COMPANY;
		break;

	case CM_DESCRIPTION:
		return IDS_COLUMN_NAME_DESCRIPTION;
		break;

	case CM_FILEVERSION:
		return IDS_COLUMN_NAME_FILEVERSION;
		break;

	case CM_PRODUCTVERSION:
		return IDS_COLUMN_NAME_PRODUCTVERSION;
		break;

	case CM_SHORTCUTTO:
		return IDS_COLUMN_NAME_SHORTCUTTO;
		break;

	case CM_HARDLINKS:
		return IDS_COLUMN_NAME_HARDLINKS;
		break;

	case CM_EXTENSION:
		return IDS_COLUMN_NAME_EXTENSION;
		break;

	case CM_CREATED:
		return IDS_COLUMN_NAME_CREATED;
		break;

	case CM_ACCESSED:
		return IDS_COLUMN_NAME_ACCESSED;
		break;

	case CM_TITLE:
		return IDS_COLUMN_NAME_TITLE;
		break;

	case CM_SUBJECT:
		return IDS_COLUMN_NAME_SUBJECT;
		break;

	case CM_AUTHORS:
		return IDS_COLUMN_NAME_AUTHORS;
		break;

	case CM_KEYWORDS:
		return IDS_COLUMN_NAME_KEYWORDS;
		break;

	case CM_COMMENT:
		return IDS_COLUMN_NAME_COMMENT;
		break;

	case CM_CAMERAMODEL:
		return IDS_COLUMN_NAME_CAMERAMODEL;
		break;

	case CM_DATETAKEN:
		return IDS_COLUMN_NAME_DATETAKEN;
		break;

	case CM_WIDTH:
		return IDS_COLUMN_NAME_WIDTH;
		break;

	case CM_HEIGHT:
		return IDS_COLUMN_NAME_HEIGHT;
		break;

	case CM_VIRTUALCOMMENTS:
		return IDS_COLUMN_NAME_VIRTUALCOMMENTS;
		break;

	case CM_TOTALSIZE:
		return IDS_COLUMN_NAME_TOTALSIZE;
		break;

	case CM_FREESPACE:
		return IDS_COLUMN_NAME_FREESPACE;
		break;

	case CM_FILESYSTEM:
		return IDS_COLUMN_NAME_FILESYSTEM;
		break;

	case CM_ORIGINALLOCATION:
		return IDS_COLUMN_NAME_ORIGINALLOCATION;
		break;

	case CM_DATEDELETED:
		return IDS_COLUMN_NAME_DATEDELETED;
		break;

	case CM_NUMPRINTERDOCUMENTS:
		return IDS_COLUMN_NAME_NUMPRINTERDOCUMENTS;
		break;

	case CM_PRINTERSTATUS:
		return IDS_COLUMN_NAME_PRINTERSTATUS;
		break;

	case CM_PRINTERCOMMENTS:
		return IDS_COLUMN_NAME_PRINTERCOMMENTS;
		break;

	case CM_PRINTERLOCATION:
		return IDS_COLUMN_NAME_PRINTERLOCATION;
		break;

	case CM_PRINTERMODEL:
		return IDS_COLUMN_NAME_PRINTERMODEL;
		break;

	case CM_NETWORKADAPTER_STATUS:
		return IDS_COLUMN_NAME_NETWORKADAPTER_STATUS;
		break;

	case CM_MEDIA_BITRATE:
		return IDS_COLUMN_NAME_BITRATE;
		break;

	case CM_MEDIA_COPYRIGHT:
		return IDS_COLUMN_NAME_COPYRIGHT;
		break;

	case CM_MEDIA_DURATION:
		return IDS_COLUMN_NAME_DURATION;
		break;

	case CM_MEDIA_PROTECTED:
		return IDS_COLUMN_NAME_PROTECTED;
		break;

	case CM_MEDIA_RATING:
		return IDS_COLUMN_NAME_RATING;
		break;

	case CM_MEDIA_ALBUMARTIST:
		return IDS_COLUMN_NAME_ALBUMARTIST;
		break;

	case CM_MEDIA_ALBUM:
		return IDS_COLUMN_NAME_ALBUM;
		break;

	case CM_MEDIA_BEATSPERMINUTE:
		return IDS_COLUMN_NAME_BEATSPERMINUTE;
		break;

	case CM_MEDIA_COMPOSER:
		return IDS_COLUMN_NAME_COMPOSER;
		break;

	case CM_MEDIA_CONDUCTOR:
		return IDS_COLUMN_NAME_CONDUCTOR;
		break;

	case CM_MEDIA_DIRECTOR:
		return IDS_COLUMN_NAME_DIRECTOR;
		break;

	case CM_MEDIA_GENRE:
		return IDS_COLUMN_NAME_GENRE;
		break;

	case CM_MEDIA_LANGUAGE:
		return IDS_COLUMN_NAME_LANGUAGE;
		break;

	case CM_MEDIA_BROADCASTDATE:
		return IDS_COLUMN_NAME_BROADCASTDATE;
		break;

	case CM_MEDIA_CHANNEL:
		return IDS_COLUMN_NAME_CHANNEL;
		break;

	case CM_MEDIA_STATIONNAME:
		return IDS_COLUMN_NAME_STATIONNAME;
		break;

	case CM_MEDIA_MOOD:
		return IDS_COLUMN_NAME_MOOD;
		break;

	case CM_MEDIA_PARENTALRATING:
		return IDS_COLUMN_NAME_PARENTALRATING;
		break;

	case CM_MEDIA_PARENTALRATINGREASON:
		return IDS_COLUMN_NAME_PARENTALRATINGREASON;
		break;

	case CM_MEDIA_PERIOD:
		return IDS_COLUMN_NAME_PERIOD;
		break;

	case CM_MEDIA_PRODUCER:
		return IDS_COLUMN_NAME_PRODUCER;
		break;

	case CM_MEDIA_PUBLISHER:
		return IDS_COLUMN_NAME_PUBLISHER;
		break;

	case CM_MEDIA_WRITER:
		return IDS_COLUMN_NAME_WRITER;
		break;

	case CM_MEDIA_YEAR:
		return IDS_COLUMN_NAME_YEAR;
		break;

	default:
		assert(false);
		break;
	}

	return 0;
}

int CShellBrowser::LookupColumnDescriptionStringIndex(int iColumnId)
{
	switch (iColumnId)
	{
	case CM_NAME:
		return IDS_COLUMN_DESCRIPTION_NAME;
		break;

	case CM_TYPE:
		return IDS_COLUMN_DESCRIPTION_TYPE;
		break;

	case CM_SIZE:
		return IDS_COLUMN_DESCRIPTION_SIZE;
		break;

	case CM_DATEMODIFIED:
		return IDS_COLUMN_DESCRIPTION_MODIFIED;
		break;

	case CM_ATTRIBUTES:
		return IDS_COLUMN_DESCRIPTION_ATTRIBUTES;
		break;

	case CM_REALSIZE:
		return IDS_COLUMN_DESCRIPTION_REALSIZE;
		break;

	case CM_SHORTNAME:
		return IDS_COLUMN_DESCRIPTION_SHORTNAME;
		break;

	case CM_OWNER:
		return IDS_COLUMN_DESCRIPTION_OWNER;
		break;

	case CM_PRODUCTNAME:
		return IDS_COLUMN_DESCRIPTION_PRODUCTNAME;
		break;

	case CM_COMPANY:
		return IDS_COLUMN_DESCRIPTION_COMPANY;
		break;

	case CM_DESCRIPTION:
		return IDS_COLUMN_DESCRIPTION_DESCRIPTION;
		break;

	case CM_FILEVERSION:
		return IDS_COLUMN_DESCRIPTION_FILEVERSION;
		break;

	case CM_PRODUCTVERSION:
		return IDS_COLUMN_DESCRIPTION_PRODUCTVERSION;
		break;

	case CM_SHORTCUTTO:
		return IDS_COLUMN_DESCRIPTION_SHORTCUTTO;
		break;

	case CM_HARDLINKS:
		return IDS_COLUMN_DESCRIPTION_HARDLINKS;
		break;

	case CM_EXTENSION:
		return IDS_COLUMN_DESCRIPTION_EXTENSION;
		break;

	case CM_CREATED:
		return IDS_COLUMN_DESCRIPTION_CREATED;
		break;

	case CM_ACCESSED:
		return IDS_COLUMN_DESCRIPTION_ACCESSED;
		break;

	case CM_TITLE:
		return IDS_COLUMN_DESCRIPTION_TITLE;
		break;

	case CM_SUBJECT:
		return IDS_COLUMN_DESCRIPTION_SUBJECT;
		break;

	case CM_AUTHORS:
		return IDS_COLUMN_DESCRIPTION_AUTHORS;
		break;

	case CM_KEYWORDS:
		return IDS_COLUMN_DESCRIPTION_KEYWORDS;
		break;

	case CM_COMMENT:
		return IDS_COLUMN_DESCRIPTION_COMMENT;
		break;

	case CM_CAMERAMODEL:
		return IDS_COLUMN_DESCRIPTION_CAMERAMODEL;
		break;

	case CM_DATETAKEN:
		return IDS_COLUMN_DESCRIPTION_DATETAKEN;
		break;

	case CM_WIDTH:
		return IDS_COLUMN_DESCRIPTION_WIDTH;
		break;

	case CM_HEIGHT:
		return IDS_COLUMN_DESCRIPTION_HEIGHT;
		break;

	case CM_VIRTUALCOMMENTS:
		return IDS_COLUMN_DESCRIPTION_COMMENT;
		break;

	case CM_TOTALSIZE:
		return IDS_COLUMN_DESCRIPTION_TOTALSIZE;
		break;

	case CM_FREESPACE:
		return IDS_COLUMN_DESCRIPTION_FREESPACE;
		break;

	case CM_FILESYSTEM:
		return IDS_COLUMN_DESCRIPTION_FILESYSTEM;
		break;

	case CM_NUMPRINTERDOCUMENTS:
		return IDS_COLUMN_DESCRIPTION_NUMPRINTERDOCUMENTS;
		break;

	case CM_PRINTERCOMMENTS:
		return IDS_COLUMN_DESCRIPTION_PRINTERCOMMENTS;
		break;

	case CM_PRINTERLOCATION:
		return IDS_COLUMN_DESCRIPTION_PRINTERLOCATION;
		break;

	case CM_NETWORKADAPTER_STATUS:
		return IDS_COLUMN_DESCRIPTION_NETWORKADAPTER_STATUS;
		break;

	case CM_MEDIA_BITRATE:
		return IDS_COLUMN_DESCRIPTION_BITRATE;
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