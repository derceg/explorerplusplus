// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include <list>
#include <cassert>
#include <propkey.h>
#include <propvarutil.h>
#include "ColumnDataRetrieval.h"
#include "IShellView.h"
#include "iShellBrowser_internal.h"
#include "SortModes.h"
#include "ViewModes.h"
#include "../Helper/Controls.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/FileOperations.h"
#include "../Helper/FolderSize.h"
#include "../Helper/Macros.h"


void CShellBrowser::SortFolder(SortMode sortMode)
{
	m_folderSettings.sortMode = sortMode;

	if(m_folderSettings.showInGroups)
	{
		ListView_EnableGroupView(m_hListView,FALSE);
		ListView_RemoveAllGroups(m_hListView);
		ListView_EnableGroupView(m_hListView,TRUE);

		SetGrouping(TRUE);
	}

	SendMessage(m_hListView,LVM_SORTITEMS,reinterpret_cast<WPARAM>(this),reinterpret_cast<LPARAM>(SortStub));

	/* If in details view, the column sort
	arrow will need to be changed to reflect
	the new sorting mode. */
	if(m_folderSettings.viewMode == +ViewMode::Details)
	{
		ApplyHeaderSortArrow();
	}
}

int CALLBACK SortStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CShellBrowser *pShellBrowser = reinterpret_cast<CShellBrowser *>(lParamSort);
	return pShellBrowser->Sort(static_cast<int>(lParam1),static_cast<int>(lParam2));
}

/* Also see NBookmarkHelper::Sort. */
int CALLBACK CShellBrowser::Sort(int InternalIndex1,int InternalIndex2) const
{
	int ComparisonResult = 0;

	BasicItemInfo_t basicItemInfo1 = getBasicItemInfo(InternalIndex1);
	BasicItemInfo_t basicItemInfo2 = getBasicItemInfo(InternalIndex2);

	bool IsFolder1 = ((basicItemInfo1.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) ? true : false;
	bool IsFolder2 = ((basicItemInfo2.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) ? true : false;
	
	/* Folders will always be sorted separately from files,
	except in the recycle bin. */
	if(IsFolder1 && !IsFolder2 && !CompareVirtualFolders(CSIDL_BITBUCKET))
	{
		ComparisonResult = -1;
	}
	else if(!IsFolder1 && IsFolder2 && !CompareVirtualFolders(CSIDL_BITBUCKET))
	{
		ComparisonResult = 1;
	}
	else
	{
		switch(m_folderSettings.sortMode)
		{
		case SortMode::FSM_NAME:
			ComparisonResult = SortByName(basicItemInfo1, basicItemInfo2, *m_globalFolderSettings);
			break;

		case SortMode::FSM_TYPE:
			ComparisonResult = SortByType(basicItemInfo1,basicItemInfo2);
			break;

		case SortMode::FSM_SIZE:
			ComparisonResult = SortBySize(InternalIndex1,InternalIndex2);
			break;

		case SortMode::FSM_DATEMODIFIED:
			ComparisonResult = SortByDate(InternalIndex1,InternalIndex2,DATE_TYPE_MODIFIED);
			break;

		case SortMode::FSM_TOTALSIZE:
			ComparisonResult = SortByTotalSize(basicItemInfo1,basicItemInfo2,TRUE);
			break;

		case SortMode::FSM_FREESPACE:
			ComparisonResult = SortByTotalSize(basicItemInfo1,basicItemInfo2,FALSE);
			break;

		case SortMode::FSM_DATEDELETED:
			ComparisonResult = SortByItemDetails(basicItemInfo1, basicItemInfo2, &SCID_DATE_DELETED);
			break;

		case SortMode::FSM_ORIGINALLOCATION:
			ComparisonResult = SortByItemDetails(basicItemInfo1, basicItemInfo2, &SCID_ORIGINAL_LOCATION);
			break;

		case SortMode::FSM_ATTRIBUTES:
			ComparisonResult = SortByAttributes(basicItemInfo1, basicItemInfo2);
			break;

		case SortMode::FSM_REALSIZE:
			ComparisonResult = SortByRealSize(basicItemInfo1, basicItemInfo2);
			break;

		case SortMode::FSM_SHORTNAME:
			ComparisonResult = SortByShortName(basicItemInfo1, basicItemInfo2);
			break;

		case SortMode::FSM_OWNER:
			ComparisonResult = SortByOwner(basicItemInfo1, basicItemInfo2);
			break;

		case SortMode::FSM_PRODUCTNAME:
			ComparisonResult = SortByVersionInfo(basicItemInfo1, basicItemInfo2,VERSION_INFO_PRODUCT_NAME);
			break;

		case SortMode::FSM_COMPANY:
			ComparisonResult = SortByVersionInfo(basicItemInfo1, basicItemInfo2,VERSION_INFO_COMPANY);
			break;

		case SortMode::FSM_DESCRIPTION:
			ComparisonResult = SortByVersionInfo(basicItemInfo1, basicItemInfo2,VERSION_INFO_DESCRIPTION);
			break;

		case SortMode::FSM_FILEVERSION:
			ComparisonResult = SortByVersionInfo(basicItemInfo1, basicItemInfo2,VERSION_INFO_FILE_VERSION);
			break;

		case SortMode::FSM_PRODUCTVERSION:
			ComparisonResult = SortByVersionInfo(basicItemInfo1, basicItemInfo2,VERSION_INFO_PRODUCT_VERSION);
			break;

		case SortMode::FSM_SHORTCUTTO:
			ComparisonResult = SortByShortcutTo(basicItemInfo1, basicItemInfo2);
			break;

		case SortMode::FSM_HARDLINKS:
			ComparisonResult = SortByHardlinks(basicItemInfo1, basicItemInfo2);
			break;

		case SortMode::FSM_EXTENSION:
			ComparisonResult = SortByExtension(basicItemInfo1,basicItemInfo2);
			break;

		case SortMode::FSM_CREATED:
			ComparisonResult = SortByDate(InternalIndex1,InternalIndex2,DATE_TYPE_CREATED);
			break;

		case SortMode::FSM_ACCESSED:
			ComparisonResult = SortByDate(InternalIndex1,InternalIndex2,DATE_TYPE_ACCESSED);
			break;

		case SortMode::FSM_TITLE:
			ComparisonResult = SortByItemDetails(basicItemInfo1, basicItemInfo2,&PKEY_Title);
			break;

		case SortMode::FSM_SUBJECT:
			ComparisonResult = SortByItemDetails(basicItemInfo1, basicItemInfo2,&PKEY_Subject);
			break;

		case SortMode::FSM_AUTHORS:
			ComparisonResult = SortByItemDetails(basicItemInfo1, basicItemInfo2,&PKEY_Author);
			break;

		case SortMode::FSM_KEYWORDS:
			ComparisonResult = SortByItemDetails(basicItemInfo1, basicItemInfo2,&PKEY_Keywords);
			break;

		case SortMode::FSM_COMMENTS:
			ComparisonResult = SortByItemDetails(basicItemInfo1, basicItemInfo2,&PKEY_Comment);
			break;

		case SortMode::FSM_CAMERAMODEL:
			ComparisonResult = SortByImageProperty(basicItemInfo1, basicItemInfo2,PropertyTagEquipModel);
			break;

		case SortMode::FSM_DATETAKEN:
			ComparisonResult = SortByImageProperty(basicItemInfo1, basicItemInfo2,PropertyTagDateTime);
			break;

		case SortMode::FSM_WIDTH:
			ComparisonResult = SortByImageProperty(basicItemInfo1, basicItemInfo2,PropertyTagImageWidth);
			break;

		case SortMode::FSM_HEIGHT:
			ComparisonResult = SortByImageProperty(basicItemInfo1, basicItemInfo2,PropertyTagImageHeight);
			break;

		case SortMode::FSM_VIRTUALCOMMENTS:
			ComparisonResult = SortByVirtualComments(basicItemInfo1, basicItemInfo2);
			break;

		case SortMode::FSM_FILESYSTEM:
			ComparisonResult = SortByFileSystem(basicItemInfo1,basicItemInfo2);
			break;

		case SortMode::FSM_NUMPRINTERDOCUMENTS:
			ComparisonResult = SortByPrinterProperty(basicItemInfo1,basicItemInfo2,PRINTER_INFORMATION_TYPE_NUM_JOBS);
			break;

		case SortMode::FSM_PRINTERSTATUS:
			ComparisonResult = SortByPrinterProperty(basicItemInfo1,basicItemInfo2,PRINTER_INFORMATION_TYPE_STATUS);
			break;

		case SortMode::FSM_PRINTERCOMMENTS:
			ComparisonResult = SortByPrinterProperty(basicItemInfo1,basicItemInfo2,PRINTER_INFORMATION_TYPE_COMMENTS);
			break;

		case SortMode::FSM_PRINTERLOCATION:
			ComparisonResult = SortByPrinterProperty(basicItemInfo1,basicItemInfo2,PRINTER_INFORMATION_TYPE_LOCATION);
			break;

		case SortMode::FSM_NETWORKADAPTER_STATUS:
			ComparisonResult = SortByNetworkAdapterStatus(basicItemInfo1,basicItemInfo2);
			break;

		case SortMode::FSM_MEDIA_BITRATE:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_BITRATE);
			break;

		case SortMode::FSM_MEDIA_COPYRIGHT:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_COPYRIGHT);
			break;

		case SortMode::FSM_MEDIA_DURATION:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_DURATION);
			break;

		case SortMode::FSM_MEDIA_PROTECTED:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_PROTECTED);
			break;

		case SortMode::FSM_MEDIA_RATING:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_RATING);
			break;

		case SortMode::FSM_MEDIA_ALBUMARTIST:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_ALBUM_ARTIST);
			break;

		case SortMode::FSM_MEDIA_ALBUM:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_ALBUM_TITLE);
			break;

		case SortMode::FSM_MEDIA_BEATSPERMINUTE:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_BEATS_PER_MINUTE);
			break;

		case SortMode::FSM_MEDIA_COMPOSER:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_COMPOSER);
			break;

		case SortMode::FSM_MEDIA_CONDUCTOR:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_CONDUCTOR);
			break;

		case SortMode::FSM_MEDIA_DIRECTOR:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_DIRECTOR);
			break;

		case SortMode::FSM_MEDIA_GENRE:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_GENRE);
			break;

		case SortMode::FSM_MEDIA_LANGUAGE:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_LANGUAGE);
			break;

		case SortMode::FSM_MEDIA_BROADCASTDATE:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_BROADCASTDATE);
			break;

		case SortMode::FSM_MEDIA_CHANNEL:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_CHANNEL);
			break;

		case SortMode::FSM_MEDIA_STATIONNAME:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_STATIONNAME);
			break;

		case SortMode::FSM_MEDIA_MOOD:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_MOOD);
			break;

		case SortMode::FSM_MEDIA_PARENTALRATING:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_PARENTALRATING);
			break;

		case SortMode::FSM_MEDIA_PARENTALRATINGREASON:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_PARENTALRATINGREASON);
			break;

		case SortMode::FSM_MEDIA_PERIOD:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_PERIOD);
			break;

		case SortMode::FSM_MEDIA_PRODUCER:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_PRODUCER);
			break;

		case SortMode::FSM_MEDIA_PUBLISHER:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_PUBLISHER);
			break;

		case SortMode::FSM_MEDIA_WRITER:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_WRITER);
			break;

		case SortMode::FSM_MEDIA_YEAR:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_YEAR);
			break;

		default:
			assert(false);
			break;
		}
	}

	if(ComparisonResult == 0)
	{
		/* By default, items that are equal will be sub-sorted
		by their display names. */
		ComparisonResult = StrCmpLogicalW(m_itemInfoMap.at(InternalIndex1).szDisplayName,
			m_itemInfoMap.at(InternalIndex2).szDisplayName);
	}

	if(!m_folderSettings.sortAscending)
	{
		ComparisonResult = -ComparisonResult;
	}

	return ComparisonResult;
}

int CALLBACK CShellBrowser::SortByName(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, const GlobalFolderSettings &globalFolderSettings) const
{
	if(m_bVirtualFolder)
	{
		BOOL IsRoot1 = PathIsRoot(itemInfo1.getFullPath().c_str());
		BOOL IsRoot2 = PathIsRoot(itemInfo2.getFullPath().c_str());

		if(IsRoot1 && !IsRoot2)
		{
			return -1;
		}
		else if(!IsRoot1 && IsRoot2)
		{
			return 1;
		}
		else if(IsRoot1 && IsRoot2)
		{
			/* If the items been compared are both drives,
			sort by drive letter, rather than display name. */
			return StrCmpLogicalW(itemInfo1.getFullPath().c_str(), itemInfo2.getFullPath().c_str());
		}
	}

	std::wstring Name1 = GetNameColumnText(itemInfo1, globalFolderSettings);
	std::wstring Name2 = GetNameColumnText(itemInfo2, globalFolderSettings);

	return StrCmpLogicalW(Name1.c_str(),Name2.c_str());
}

int CALLBACK CShellBrowser::SortBySize(int InternalIndex1,int InternalIndex2) const
{
	bool IsFolder1 = ((m_itemInfoMap.at(InternalIndex1).wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) ? true : false;
	bool IsFolder2 = ((m_itemInfoMap.at(InternalIndex2).wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) ? true : false;

	ULONGLONG size1;
	ULONGLONG size2;
	
	if(IsFolder1 && IsFolder2)
	{
		auto itr1 = m_cachedFolderSizes.find(InternalIndex1);
		auto itr2 = m_cachedFolderSizes.find(InternalIndex2);

		if (itr1 != m_cachedFolderSizes.end() && itr2 == m_cachedFolderSizes.end())
		{
			return 1;
		}
		else if (itr1 == m_cachedFolderSizes.end() && itr2 != m_cachedFolderSizes.end())
		{
			return -1;
		}

		size1 = itr1->second;
		size2 = itr2->second;
	}
	else
	{
		// Both items are files (as opposed to folders).
		ULARGE_INTEGER FileSize1 = { m_itemInfoMap.at(InternalIndex1).wfd.nFileSizeLow,m_itemInfoMap.at(InternalIndex1).wfd.nFileSizeHigh };
		ULARGE_INTEGER FileSize2 = { m_itemInfoMap.at(InternalIndex2).wfd.nFileSizeLow,m_itemInfoMap.at(InternalIndex2).wfd.nFileSizeHigh };

		size1 = FileSize1.QuadPart;
		size2 = FileSize2.QuadPart;
	}

	if(size1 > size2)
	{
		return 1;
	}
	else if(size1 < size2)
	{
		return -1;
	}

	return 0;
}

int CALLBACK CShellBrowser::SortByType(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const
{
	if(m_bVirtualFolder)
	{
		TCHAR FullFileName1[MAX_PATH];
		GetDisplayName(itemInfo1.pidlComplete.get(),FullFileName1,SIZEOF_ARRAY(FullFileName1),SHGDN_FORPARSING);

		TCHAR FullFileName2[MAX_PATH];
		GetDisplayName(itemInfo2.pidlComplete.get(),FullFileName2,SIZEOF_ARRAY(FullFileName2),SHGDN_FORPARSING);

		BOOL IsRoot1 = PathIsRoot(FullFileName1);
		BOOL IsRoot2 = PathIsRoot(FullFileName2);

		if(IsRoot1 && !IsRoot2)
		{
			return -1;
		}
		else if(!IsRoot1 && IsRoot2)
		{
			return 1;
		}
	}

	std::wstring Type1 = GetTypeColumnText(itemInfo1);
	std::wstring Type2 = GetTypeColumnText(itemInfo2);

	return StrCmpLogicalW(Type1.c_str(),Type2.c_str());
}

int CALLBACK CShellBrowser::SortByDate(int InternalIndex1,int InternalIndex2,DateType_t DateType) const
{
	switch(DateType)
	{
	case DATE_TYPE_CREATED:
		return CompareFileTime(&m_itemInfoMap.at(InternalIndex1).wfd.ftCreationTime,&m_itemInfoMap.at(InternalIndex2).wfd.ftCreationTime);
		break;

	case DATE_TYPE_MODIFIED:
		return CompareFileTime(&m_itemInfoMap.at(InternalIndex1).wfd.ftLastWriteTime,&m_itemInfoMap.at(InternalIndex2).wfd.ftLastWriteTime);
		break;

	case DATE_TYPE_ACCESSED:
		return CompareFileTime(&m_itemInfoMap.at(InternalIndex1).wfd.ftLastAccessTime,&m_itemInfoMap.at(InternalIndex2).wfd.ftLastAccessTime);
		break;

	default:
		assert(false);
		break;
	}

	return 0;
}

int CALLBACK CShellBrowser::SortByTotalSize(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, bool TotalSize) const
{
	ULARGE_INTEGER DriveSpace1;
	BOOL Res1 = GetDriveSpaceColumnRawData(itemInfo1,TotalSize,DriveSpace1);

	ULARGE_INTEGER DriveSpace2;
	BOOL Res2 = GetDriveSpaceColumnRawData(itemInfo2,TotalSize,DriveSpace2);

	if(Res1 && !Res2)
	{
		return 1;
	}
	else if(!Res1 && Res2)
	{
		return -1;
	}
	else if(!Res1 && !Res2)
	{
		return 0;
	}

	if(DriveSpace1.QuadPart > DriveSpace2.QuadPart)
	{
		return 1;
	}
	else if(DriveSpace1.QuadPart < DriveSpace2.QuadPart)
	{
		return -1;
	}

	return 0;
}

int CALLBACK CShellBrowser::SortByAttributes(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const
{
	std::wstring AttributeString1 = GetAttributeColumnText(itemInfo1);
	std::wstring AttributeString2 = GetAttributeColumnText(itemInfo2);

	return StrCmpLogicalW(AttributeString1.c_str(),AttributeString2.c_str());
}

int CALLBACK CShellBrowser::SortByRealSize(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const
{
	ULARGE_INTEGER RealFileSize1;
	bool Res1 = GetRealSizeColumnRawData(itemInfo1,RealFileSize1);

	ULARGE_INTEGER RealFileSize2;
	bool Res2 = GetRealSizeColumnRawData(itemInfo2,RealFileSize2);

	if(Res1 && !Res2)
	{
		return 1;
	}
	else if(!Res1 && Res2)
	{
		return -1;
	}
	else if(!Res1 && !Res2)
	{
		return 0;
	}

	if(RealFileSize1.QuadPart > RealFileSize2.QuadPart)
	{
		return 1;
	}
	else if(RealFileSize1.QuadPart < RealFileSize2.QuadPart)
	{
		return -1;
	}

	return 0;
}

int CALLBACK CShellBrowser::SortByShortName(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const
{
	std::wstring ShortName1 = GetShortNameColumnText(itemInfo1);
	std::wstring ShortName2 = GetShortNameColumnText(itemInfo2);

	return StrCmpLogicalW(ShortName1.c_str(),ShortName2.c_str());
}

int CALLBACK CShellBrowser::SortByOwner(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const
{
	std::wstring Owner1 = GetOwnerColumnText(itemInfo1);
	std::wstring Owner2 = GetOwnerColumnText(itemInfo2);

	return StrCmpLogicalW(Owner1.c_str(),Owner2.c_str());
}

int CALLBACK CShellBrowser::SortByVersionInfo(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, VersionInfoType_t VersioninfoType) const
{
	std::wstring VersionInfo1 = GetVersionColumnText(itemInfo1,VersioninfoType);
	std::wstring VersionInfo2 = GetVersionColumnText(itemInfo2,VersioninfoType);

	return StrCmpLogicalW(VersionInfo1.c_str(),VersionInfo2.c_str());
}

int CALLBACK CShellBrowser::SortByShortcutTo(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const
{
	std::wstring ResolvedLinkPath1 = GetShortcutToColumnText(itemInfo1);
	std::wstring ResolvedLinkPath2 = GetShortcutToColumnText(itemInfo2);

	return StrCmpLogicalW(ResolvedLinkPath1.c_str(),ResolvedLinkPath2.c_str());
}

int CALLBACK CShellBrowser::SortByHardlinks(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const
{
	DWORD NumHardLinks1 = GetHardLinksColumnRawData(itemInfo1);
	DWORD NumHardLinks2 = GetHardLinksColumnRawData(itemInfo2);

	return NumHardLinks1 - NumHardLinks2;
}

int CALLBACK CShellBrowser::SortByExtension(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const
{
	std::wstring Extension1 = GetExtensionColumnText(itemInfo1);
	std::wstring Extension2 = GetExtensionColumnText(itemInfo2);

	return StrCmpLogicalW(Extension1.c_str(),Extension2.c_str());
}

int CALLBACK CShellBrowser::SortByItemDetails(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, const SHCOLUMNID *pscid) const
{
	VARIANT vt1;
	HRESULT hr1 = GetItemDetailsRawData(itemInfo1, pscid, &vt1);

	VARIANT vt2;
	HRESULT hr2 = GetItemDetailsRawData(itemInfo2, pscid, &vt2);

	int ret = 0;

	if (SUCCEEDED(hr1) && SUCCEEDED(hr2) && vt1.vt == vt2.vt)
	{
		ret = VariantCompare(vt1, vt2);
	}

	if (SUCCEEDED(hr1))
	{
		VariantClear(&vt1);
	}

	if (SUCCEEDED(hr2))
	{
		VariantClear(&vt2);
	}

	return ret;
}

int CALLBACK CShellBrowser::SortByImageProperty(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, PROPID PropertyId) const
{
	std::wstring ImageProperty1 = GetImageColumnText(itemInfo1,PropertyId);
	std::wstring ImageProperty2 = GetImageColumnText(itemInfo2,PropertyId);

	return StrCmpLogicalW(ImageProperty1.c_str(),ImageProperty2.c_str());
}

int CALLBACK CShellBrowser::SortByVirtualComments(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const
{
	std::wstring Comments1 = GetControlPanelCommentsColumnText(itemInfo1);
	std::wstring Comments2 = GetControlPanelCommentsColumnText(itemInfo2);

	return StrCmpLogicalW(Comments1.c_str(),Comments2.c_str());
}

int CALLBACK CShellBrowser::SortByFileSystem(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const
{
	std::wstring FileSystemName1 = GetFileSystemColumnText(itemInfo1);
	std::wstring FileSystemName2 = GetFileSystemColumnText(itemInfo2);

	return StrCmpLogicalW(FileSystemName1.c_str(),FileSystemName2.c_str());
}

int CALLBACK CShellBrowser::SortByPrinterProperty(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, PrinterInformationType_t PrinterInformationType) const
{
	std::wstring PrinterInformation1 = GetPrinterColumnText(itemInfo1,PrinterInformationType);
	std::wstring PrinterInformation2 = GetPrinterColumnText(itemInfo2,PrinterInformationType);

	return StrCmpLogicalW(PrinterInformation1.c_str(),PrinterInformation2.c_str());
}

int CALLBACK CShellBrowser::SortByNetworkAdapterStatus(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2) const
{
	std::wstring Status1 = GetNetworkAdapterColumnText(itemInfo1);
	std::wstring Status2 = GetNetworkAdapterColumnText(itemInfo2);

	return StrCmpLogicalW(Status1.c_str(),Status2.c_str());
}

int CALLBACK CShellBrowser::SortByMediaMetadata(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, MediaMetadataType_t MediaMetaDataType) const
{
	std::wstring MediaMetadata1 = GetMediaMetadataColumnText(itemInfo1,MediaMetaDataType);
	std::wstring MediaMetadata2 = GetMediaMetadataColumnText(itemInfo2,MediaMetaDataType);

	return StrCmpLogicalW(MediaMetadata1.c_str(),MediaMetadata2.c_str());
}