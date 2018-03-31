/******************************************************************
 *
 * Project: ShellBrowser
 * File: SortManager.cpp
 * License: GPL - See LICENSE in the top level directory
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
#include <cassert>
#include <propvarutil.h>
#include "IShellView.h"
#include "iShellBrowser_internal.h"
#include "../Helper/Controls.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/FileOperations.h"
#include "../Helper/FolderSize.h"
#include "../Helper/Macros.h"


void CShellBrowser::SortFolder(UINT SortMode)
{
	m_SortMode = SortMode;

	if(m_bShowInGroups)
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
	if(m_ViewMode == VM_DETAILS)
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

	bool IsFolder1 = ((m_pwfdFiles[InternalIndex1].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) ? true : false;
	bool IsFolder2 = ((m_pwfdFiles[InternalIndex2].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) ? true : false;
	
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
		switch(m_SortMode)
		{
		case FSM_NAME:
			ComparisonResult = SortByName(InternalIndex1,InternalIndex2);
			break;

		case FSM_TYPE:
			ComparisonResult = SortByType(InternalIndex1,InternalIndex2);
			break;

		case FSM_SIZE:
			ComparisonResult = SortBySize(InternalIndex1,InternalIndex2);
			break;

		case FSM_DATEMODIFIED:
			ComparisonResult = SortByDate(InternalIndex1,InternalIndex2,DATE_TYPE_MODIFIED);
			break;

		case FSM_TOTALSIZE:
			ComparisonResult = SortByTotalSize(InternalIndex1,InternalIndex2,TRUE);
			break;

		case FSM_FREESPACE:
			ComparisonResult = SortByTotalSize(InternalIndex1,InternalIndex2,FALSE);
			break;

		case FSM_DATEDELETED:
			ComparisonResult = SortByItemDetails(InternalIndex1, InternalIndex2, &SCID_DATE_DELETED);
			break;

		case FSM_ORIGINALLOCATION:
			ComparisonResult = SortByItemDetails(InternalIndex1, InternalIndex2, &SCID_ORIGINAL_LOCATION);
			break;

		case FSM_ATTRIBUTES:
			ComparisonResult = SortByAttributes(InternalIndex1,InternalIndex2);
			break;

		case FSM_REALSIZE:
			ComparisonResult = SortByRealSize(InternalIndex1,InternalIndex2);
			break;

		case FSM_SHORTNAME:
			ComparisonResult = SortByShortName(InternalIndex1,InternalIndex2);
			break;

		case FSM_OWNER:
			ComparisonResult = SortByOwner(InternalIndex1,InternalIndex2);
			break;

		case FSM_PRODUCTNAME:
			ComparisonResult = SortByVersionInfo(InternalIndex1,InternalIndex2,VERSION_INFO_PRODUCT_NAME);
			break;

		case FSM_COMPANY:
			ComparisonResult = SortByVersionInfo(InternalIndex1,InternalIndex2,VERSION_INFO_COMPANY);
			break;

		case FSM_DESCRIPTION:
			ComparisonResult = SortByVersionInfo(InternalIndex1,InternalIndex2,VERSION_INFO_DESCRIPTION);
			break;

		case FSM_FILEVERSION:
			ComparisonResult = SortByVersionInfo(InternalIndex1,InternalIndex2,VERSION_INFO_FILE_VERSION);
			break;

		case FSM_PRODUCTVERSION:
			ComparisonResult = SortByVersionInfo(InternalIndex1,InternalIndex2,VERSION_INFO_PRODUCT_VERSION);
			break;

		case FSM_SHORTCUTTO:
			ComparisonResult = SortByShortcutTo(InternalIndex1,InternalIndex2);
			break;

		case FSM_HARDLINKS:
			ComparisonResult = SortByHardlinks(InternalIndex1,InternalIndex2);
			break;

		case FSM_EXTENSION:
			ComparisonResult = SortByExtension(InternalIndex1,InternalIndex2);
			break;

		case FSM_CREATED:
			ComparisonResult = SortByDate(InternalIndex1,InternalIndex2,DATE_TYPE_CREATED);
			break;

		case FSM_ACCESSED:
			ComparisonResult = SortByDate(InternalIndex1,InternalIndex2,DATE_TYPE_ACCESSED);
			break;

		case FSM_TITLE:
			ComparisonResult = SortByItemDetails(InternalIndex1,InternalIndex2,&SCID_TITLE);
			break;

		case FSM_SUBJECT:
			ComparisonResult = SortByItemDetails(InternalIndex1,InternalIndex2,&SCID_SUBJECT);
			break;

		case FSM_AUTHOR:
			ComparisonResult = SortByItemDetails(InternalIndex1,InternalIndex2,&SCID_AUTHOR);
			break;

		case FSM_KEYWORDS:
			ComparisonResult = SortByItemDetails(InternalIndex1,InternalIndex2,&SCID_KEYWORDS);
			break;

		case FSM_COMMENTS:
			ComparisonResult = SortByItemDetails(InternalIndex1,InternalIndex2,&SCID_COMMENTS);
			break;

		case FSM_CAMERAMODEL:
			ComparisonResult = SortByImageProperty(InternalIndex1,InternalIndex2,PropertyTagEquipModel);
			break;

		case FSM_DATETAKEN:
			ComparisonResult = SortByImageProperty(InternalIndex1,InternalIndex2,PropertyTagDateTime);
			break;

		case FSM_WIDTH:
			ComparisonResult = SortByImageProperty(InternalIndex1,InternalIndex2,PropertyTagImageWidth);
			break;

		case FSM_HEIGHT:
			ComparisonResult = SortByImageProperty(InternalIndex1,InternalIndex2,PropertyTagImageHeight);
			break;

		case FSM_VIRTUALCOMMENTS:
			ComparisonResult = SortByVirtualComments(InternalIndex1,InternalIndex2);
			break;

		case FSM_FILESYSTEM:
			ComparisonResult = SortByFileSystem(InternalIndex1,InternalIndex2);
			break;

		case FSM_NUMPRINTERDOCUMENTS:
			ComparisonResult = SortByPrinterProperty(InternalIndex1,InternalIndex2,PRINTER_INFORMATION_TYPE_NUM_JOBS);
			break;

		case FSM_PRINTERSTATUS:
			ComparisonResult = SortByPrinterProperty(InternalIndex1,InternalIndex2,PRINTER_INFORMATION_TYPE_STATUS);
			break;

		case FSM_PRINTERCOMMENTS:
			ComparisonResult = SortByPrinterProperty(InternalIndex1,InternalIndex2,PRINTER_INFORMATION_TYPE_COMMENTS);
			break;

		case FSM_PRINTERLOCATION:
			ComparisonResult = SortByPrinterProperty(InternalIndex1,InternalIndex2,PRINTER_INFORMATION_TYPE_LOCATION);
			break;

		case FSM_NETWORKADAPTER_STATUS:
			ComparisonResult = SortByNetworkAdapterStatus(InternalIndex1,InternalIndex2);
			break;

		case FSM_MEDIA_BITRATE:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_BITRATE);
			break;

		case FSM_MEDIA_COPYRIGHT:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_COPYRIGHT);
			break;

		case FSM_MEDIA_DURATION:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_DURATION);
			break;

		case FSM_MEDIA_PROTECTED:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_PROTECTED);
			break;

		case FSM_MEDIA_RATING:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_RATING);
			break;

		case FSM_MEDIA_ALBUMARTIST:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_ALBUM_ARTIST);
			break;

		case FSM_MEDIA_ALBUM:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_ALBUM_TITLE);
			break;

		case FSM_MEDIA_BEATSPERMINUTE:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_BEATS_PER_MINUTE);
			break;

		case FSM_MEDIA_COMPOSER:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_COMPOSER);
			break;

		case FSM_MEDIA_CONDUCTOR:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_CONDUCTOR);
			break;

		case FSM_MEDIA_DIRECTOR:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_DIRECTOR);
			break;

		case FSM_MEDIA_GENRE:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_GENRE);
			break;

		case FSM_MEDIA_LANGUAGE:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_LANGUAGE);
			break;

		case FSM_MEDIA_BROADCASTDATE:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_BROADCASTDATE);
			break;

		case FSM_MEDIA_CHANNEL:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_CHANNEL);
			break;

		case FSM_MEDIA_STATIONNAME:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_STATIONNAME);
			break;

		case FSM_MEDIA_MOOD:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_MOOD);
			break;

		case FSM_MEDIA_PARENTALRATING:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_PARENTALRATING);
			break;

		case FSM_MEDIA_PARENTALRATINGREASON:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_PARENTALRATINGREASON);
			break;

		case FSM_MEDIA_PERIOD:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_PERIOD);
			break;

		case FSM_MEDIA_PRODUCER:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_PRODUCER);
			break;

		case FSM_MEDIA_PUBLISHER:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_PUBLISHER);
			break;

		case FSM_MEDIA_WRITER:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_WRITER);
			break;

		case FSM_MEDIA_YEAR:
			ComparisonResult = SortByMediaMetadata(InternalIndex1,InternalIndex2,MEDIAMETADATA_TYPE_YEAR);
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
		ComparisonResult = StrCmpLogicalW(m_pExtraItemInfo[InternalIndex1].szDisplayName,
			m_pExtraItemInfo[InternalIndex2].szDisplayName);
	}

	if(!m_bSortAscending)
	{
		ComparisonResult = -ComparisonResult;
	}

	return ComparisonResult;
}

int CALLBACK CShellBrowser::SortByName(int InternalIndex1,int InternalIndex2) const
{
	if(m_bVirtualFolder)
	{
		TCHAR FullFileName1[MAX_PATH];
		LPITEMIDLIST pidlComplete1 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[InternalIndex1].pridl);
		GetDisplayName(pidlComplete1,FullFileName1,SIZEOF_ARRAY(FullFileName1),SHGDN_FORPARSING);
		CoTaskMemFree(pidlComplete1);

		TCHAR FullFileName2[MAX_PATH];
		LPITEMIDLIST pidlComplete2 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[InternalIndex2].pridl);
		GetDisplayName(pidlComplete2,FullFileName2,SIZEOF_ARRAY(FullFileName2),SHGDN_FORPARSING);
		CoTaskMemFree(pidlComplete2);

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
		else if(IsRoot1 && IsRoot2)
		{
			/* If the items been compared are both drives,
			sort by drive letter, rather than display name. */
			return StrCmpLogicalW(FullFileName1,FullFileName2);
		}
	}

	std::wstring Name1 = GetNameColumnText(InternalIndex1);
	std::wstring Name2 = GetNameColumnText(InternalIndex2);

	return StrCmpLogicalW(Name1.c_str(),Name2.c_str());
}

int CALLBACK CShellBrowser::SortBySize(int InternalIndex1,int InternalIndex2) const
{
	bool IsFolder1 = ((m_pwfdFiles[InternalIndex1].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) ? true : false;
	bool IsFolder2 = ((m_pwfdFiles[InternalIndex2].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) ? true : false;
	
	if(IsFolder1 && IsFolder2)
	{
		if(m_pExtraItemInfo[InternalIndex1].bFolderSizeRetrieved && !m_pExtraItemInfo[InternalIndex2].bFolderSizeRetrieved)
		{
			return 1;
		}
		else if(!m_pExtraItemInfo[InternalIndex1].bFolderSizeRetrieved && m_pExtraItemInfo[InternalIndex2].bFolderSizeRetrieved)
		{
			return -1;
		}
	}

	ULARGE_INTEGER FileSize1 = {m_pwfdFiles[InternalIndex1].nFileSizeLow,m_pwfdFiles[InternalIndex1].nFileSizeHigh};
	ULARGE_INTEGER FileSize2 = {m_pwfdFiles[InternalIndex2].nFileSizeLow,m_pwfdFiles[InternalIndex2].nFileSizeHigh};

	if(FileSize1.QuadPart > FileSize2.QuadPart)
	{
		return 1;
	}
	else if(FileSize1.QuadPart < FileSize2.QuadPart)
	{
		return -1;
	}

	return 0;
}

int CALLBACK CShellBrowser::SortByType(int InternalIndex1,int InternalIndex2) const
{
	if(m_bVirtualFolder)
	{
		TCHAR FullFileName1[MAX_PATH];
		LPITEMIDLIST pidlComplete1 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[InternalIndex1].pridl);
		GetDisplayName(pidlComplete1,FullFileName1,SIZEOF_ARRAY(FullFileName1),SHGDN_FORPARSING);
		CoTaskMemFree(pidlComplete1);

		TCHAR FullFileName2[MAX_PATH];
		LPITEMIDLIST pidlComplete2 = ILCombine(m_pidlDirectory,m_pExtraItemInfo[InternalIndex2].pridl);
		GetDisplayName(pidlComplete2,FullFileName2,SIZEOF_ARRAY(FullFileName2),SHGDN_FORPARSING);
		CoTaskMemFree(pidlComplete2);

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

	std::wstring Type1 = GetTypeColumnText(InternalIndex1);
	std::wstring Type2 = GetTypeColumnText(InternalIndex2);

	return StrCmpLogicalW(Type1.c_str(),Type2.c_str());
}

int CALLBACK CShellBrowser::SortByDate(int InternalIndex1,int InternalIndex2,DateType_t DateType) const
{
	switch(DateType)
	{
	case DATE_TYPE_CREATED:
		return CompareFileTime(&m_pwfdFiles[InternalIndex1].ftCreationTime,&m_pwfdFiles[InternalIndex2].ftCreationTime);
		break;

	case DATE_TYPE_MODIFIED:
		return CompareFileTime(&m_pwfdFiles[InternalIndex1].ftLastWriteTime,&m_pwfdFiles[InternalIndex2].ftLastWriteTime);
		break;

	case DATE_TYPE_ACCESSED:
		return CompareFileTime(&m_pwfdFiles[InternalIndex1].ftLastAccessTime,&m_pwfdFiles[InternalIndex2].ftLastAccessTime);
		break;

	default:
		assert(false);
		break;
	}

	return 0;
}

int CALLBACK CShellBrowser::SortByTotalSize(int InternalIndex1,int InternalIndex2,bool TotalSize) const
{
	ULARGE_INTEGER DriveSpace1;
	BOOL Res1 = GetDriveSpaceColumnRawData(InternalIndex1,TotalSize,DriveSpace1);

	ULARGE_INTEGER DriveSpace2;
	BOOL Res2 = GetDriveSpaceColumnRawData(InternalIndex2,TotalSize,DriveSpace2);

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

int CALLBACK CShellBrowser::SortByAttributes(int InternalIndex1,int InternalIndex2) const
{
	std::wstring AttributeString1 = GetAttributeColumnText(InternalIndex1);
	std::wstring AttributeString2 = GetAttributeColumnText(InternalIndex2);

	return StrCmpLogicalW(AttributeString1.c_str(),AttributeString2.c_str());
}

int CALLBACK CShellBrowser::SortByRealSize(int InternalIndex1,int InternalIndex2) const
{
	ULARGE_INTEGER RealFileSize1;
	bool Res1 = GetRealSizeColumnRawData(InternalIndex1,RealFileSize1);

	ULARGE_INTEGER RealFileSize2;
	bool Res2 = GetRealSizeColumnRawData(InternalIndex2,RealFileSize2);

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

int CALLBACK CShellBrowser::SortByShortName(int InternalIndex1,int InternalIndex2) const
{
	std::wstring ShortName1 = GetShortNameColumnText(InternalIndex1);
	std::wstring ShortName2 = GetShortNameColumnText(InternalIndex2);

	return StrCmpLogicalW(ShortName1.c_str(),ShortName2.c_str());
}

int CALLBACK CShellBrowser::SortByOwner(int InternalIndex1,int InternalIndex2) const
{
	std::wstring Owner1 = GetOwnerColumnText(InternalIndex1);
	std::wstring Owner2 = GetOwnerColumnText(InternalIndex2);

	return StrCmpLogicalW(Owner1.c_str(),Owner2.c_str());
}

int CALLBACK CShellBrowser::SortByVersionInfo(int InternalIndex1,int InternalIndex2,VersionInfoType_t VersioninfoType) const
{
	std::wstring VersionInfo1 = GetVersionColumnText(InternalIndex1,VersioninfoType);
	std::wstring VersionInfo2 = GetVersionColumnText(InternalIndex2,VersioninfoType);

	return StrCmpLogicalW(VersionInfo1.c_str(),VersionInfo2.c_str());
}

int CALLBACK CShellBrowser::SortByShortcutTo(int InternalIndex1,int InternalIndex2) const
{
	std::wstring ResolvedLinkPath1 = GetShortcutToColumnText(InternalIndex1);
	std::wstring ResolvedLinkPath2 = GetShortcutToColumnText(InternalIndex2);

	return StrCmpLogicalW(ResolvedLinkPath1.c_str(),ResolvedLinkPath2.c_str());
}

int CALLBACK CShellBrowser::SortByHardlinks(int InternalIndex1,int InternalIndex2) const
{
	DWORD NumHardLinks1 = GetHardLinksColumnRawData(InternalIndex1);
	DWORD NumHardLinks2 = GetHardLinksColumnRawData(InternalIndex2);

	return NumHardLinks1 - NumHardLinks2;
}

int CALLBACK CShellBrowser::SortByExtension(int InternalIndex1,int InternalIndex2) const
{
	std::wstring Extension1 = GetExtensionColumnText(InternalIndex1);
	std::wstring Extension2 = GetExtensionColumnText(InternalIndex2);

	return StrCmpLogicalW(Extension1.c_str(),Extension2.c_str());
}

int CALLBACK CShellBrowser::SortByItemDetails(int InternalIndex1, int InternalIndex2, const SHCOLUMNID *pscid) const
{
	VARIANT vt1;
	HRESULT hr1 = GetItemDetailsRawData(InternalIndex1, pscid, &vt1);

	VARIANT vt2;
	HRESULT hr2 = GetItemDetailsRawData(InternalIndex2, pscid, &vt2);

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

int CALLBACK CShellBrowser::SortByImageProperty(int InternalIndex1,int InternalIndex2,PROPID PropertyId) const
{
	std::wstring ImageProperty1 = GetImageColumnText(InternalIndex1,PropertyId);
	std::wstring ImageProperty2 = GetImageColumnText(InternalIndex2,PropertyId);

	return StrCmpLogicalW(ImageProperty1.c_str(),ImageProperty2.c_str());
}

int CALLBACK CShellBrowser::SortByVirtualComments(int InternalIndex1,int InternalIndex2) const
{
	std::wstring Comments1 = GetControlPanelCommentsColumnText(InternalIndex1);
	std::wstring Comments2 = GetControlPanelCommentsColumnText(InternalIndex2);

	return StrCmpLogicalW(Comments1.c_str(),Comments2.c_str());
}

int CALLBACK CShellBrowser::SortByFileSystem(int InternalIndex1,int InternalIndex2) const
{
	std::wstring FileSystemName1 = GetFileSystemColumnText(InternalIndex1);
	std::wstring FileSystemName2 = GetFileSystemColumnText(InternalIndex2);

	return StrCmpLogicalW(FileSystemName1.c_str(),FileSystemName2.c_str());
}

int CALLBACK CShellBrowser::SortByPrinterProperty(int InternalIndex1,int InternalIndex2,PrinterInformationType_t PrinterInformationType) const
{
	std::wstring PrinterInformation1 = GetPrinterColumnText(InternalIndex1,PrinterInformationType);
	std::wstring PrinterInformation2 = GetPrinterColumnText(InternalIndex2,PrinterInformationType);

	return StrCmpLogicalW(PrinterInformation1.c_str(),PrinterInformation2.c_str());
}

int CALLBACK CShellBrowser::SortByNetworkAdapterStatus(int InternalIndex1,int InternalIndex2) const
{
	std::wstring Status1 = GetNetworkAdapterColumnText(InternalIndex1);
	std::wstring Status2 = GetNetworkAdapterColumnText(InternalIndex2);

	return StrCmpLogicalW(Status1.c_str(),Status2.c_str());
}

int CALLBACK CShellBrowser::SortByMediaMetadata(int InternalIndex1,int InternalIndex2,MediaMetadataType_t MediaMetaDataType) const
{
	std::wstring MediaMetadata1 = GetMediaMetadataColumnText(InternalIndex1,MediaMetaDataType);
	std::wstring MediaMetadata2 = GetMediaMetadataColumnText(InternalIndex2,MediaMetaDataType);

	return StrCmpLogicalW(MediaMetadata1.c_str(),MediaMetadata2.c_str());
}