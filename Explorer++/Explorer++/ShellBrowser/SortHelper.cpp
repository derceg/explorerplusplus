// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SortHelper.h"
#include <wil/common.h>
#include <propvarutil.h>

int SortByName(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, const GlobalFolderSettings &globalFolderSettings)
{
	if (itemInfo1.isRoot && !itemInfo2.isRoot)
	{
		return -1;
	}
	else if (!itemInfo1.isRoot && itemInfo2.isRoot)
	{
		return 1;
	}
	else if (itemInfo1.isRoot && itemInfo2.isRoot)
	{
		/* If the items been compared are both drives,
		sort by drive letter, rather than display name. */
		return StrCmpLogicalW(itemInfo1.getFullPath().c_str(), itemInfo2.getFullPath().c_str());
	}

	std::wstring Name1 = GetNameColumnText(itemInfo1, globalFolderSettings);
	std::wstring Name2 = GetNameColumnText(itemInfo2, globalFolderSettings);

	return StrCmpLogicalW(Name1.c_str(), Name2.c_str());
}

int SortBySize(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	bool IsFolder1 = WI_IsFlagSet(itemInfo1.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
	bool IsFolder2 = WI_IsFlagSet(itemInfo2.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);

	ULONGLONG size1;
	ULONGLONG size2;

	if (IsFolder1 && IsFolder2)
	{
		// Temporarily disabled.
		/*auto itr1 = m_cachedFolderSizes.find(InternalIndex1);
		auto itr2 = m_cachedFolderSizes.find(InternalIndex2);

		if (itr1 == m_cachedFolderSizes.end() && itr2 == m_cachedFolderSizes.end())
		{
			return 0;
		}
		else if (itr1 != m_cachedFolderSizes.end() && itr2 == m_cachedFolderSizes.end())
		{
			return 1;
		}
		else if (itr1 == m_cachedFolderSizes.end() && itr2 != m_cachedFolderSizes.end())
		{
			return -1;
		}

		size1 = itr1->second;
		size2 = itr2->second;*/

		size1 = 0;
		size2 = 0;
	}
	else
	{
		// Both items are files (as opposed to folders).
		ULARGE_INTEGER FileSize1 = { itemInfo1.wfd.nFileSizeLow, itemInfo1.wfd.nFileSizeHigh };
		ULARGE_INTEGER FileSize2 = { itemInfo2.wfd.nFileSizeLow, itemInfo2.wfd.nFileSizeHigh };

		size1 = FileSize1.QuadPart;
		size2 = FileSize2.QuadPart;
	}

	if (size1 > size2)
	{
		return 1;
	}
	else if (size1 < size2)
	{
		return -1;
	}

	return 0;
}

int SortByType(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	if (itemInfo1.isRoot && !itemInfo2.isRoot)
	{
		return -1;
	}
	else if (!itemInfo1.isRoot && itemInfo2.isRoot)
	{
		return 1;
	}

	std::wstring Type1 = GetTypeColumnText(itemInfo1);
	std::wstring Type2 = GetTypeColumnText(itemInfo2);

	return StrCmpLogicalW(Type1.c_str(), Type2.c_str());
}

int SortByDate(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, DateType dateType)
{
	switch (dateType)
	{
	case DateType::Created:
		return CompareFileTime(&itemInfo1.wfd.ftCreationTime, &itemInfo2.wfd.ftCreationTime);

	case DateType::Modified:
		return CompareFileTime(&itemInfo1.wfd.ftLastWriteTime, &itemInfo2.wfd.ftLastWriteTime);

	case DateType::Accessed:
		return CompareFileTime(&itemInfo1.wfd.ftLastAccessTime, &itemInfo2.wfd.ftLastAccessTime);

	default:
		assert(false);
		break;
	}

	return 0;
}

int SortByTotalSize(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, bool TotalSize)
{
	ULARGE_INTEGER DriveSpace1;
	BOOL Res1 = GetDriveSpaceColumnRawData(itemInfo1, TotalSize, DriveSpace1);

	ULARGE_INTEGER DriveSpace2;
	BOOL Res2 = GetDriveSpaceColumnRawData(itemInfo2, TotalSize, DriveSpace2);

	if (Res1 && !Res2)
	{
		return 1;
	}
	else if (!Res1 && Res2)
	{
		return -1;
	}
	else if (!Res1 && !Res2)
	{
		return 0;
	}

	if (DriveSpace1.QuadPart > DriveSpace2.QuadPart)
	{
		return 1;
	}
	else if (DriveSpace1.QuadPart < DriveSpace2.QuadPart)
	{
		return -1;
	}

	return 0;
}

int SortByAttributes(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring AttributeString1 = GetAttributeColumnText(itemInfo1);
	std::wstring AttributeString2 = GetAttributeColumnText(itemInfo2);

	return StrCmpLogicalW(AttributeString1.c_str(), AttributeString2.c_str());
}

int SortByRealSize(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	ULARGE_INTEGER RealFileSize1;
	bool Res1 = GetRealSizeColumnRawData(itemInfo1, RealFileSize1);

	ULARGE_INTEGER RealFileSize2;
	bool Res2 = GetRealSizeColumnRawData(itemInfo2, RealFileSize2);

	if (Res1 && !Res2)
	{
		return 1;
	}
	else if (!Res1 && Res2)
	{
		return -1;
	}
	else if (!Res1 && !Res2)
	{
		return 0;
	}

	if (RealFileSize1.QuadPart > RealFileSize2.QuadPart)
	{
		return 1;
	}
	else if (RealFileSize1.QuadPart < RealFileSize2.QuadPart)
	{
		return -1;
	}

	return 0;
}

int SortByShortName(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring ShortName1 = GetShortNameColumnText(itemInfo1);
	std::wstring ShortName2 = GetShortNameColumnText(itemInfo2);

	return StrCmpLogicalW(ShortName1.c_str(), ShortName2.c_str());
}

int SortByOwner(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring Owner1 = GetOwnerColumnText(itemInfo1);
	std::wstring Owner2 = GetOwnerColumnText(itemInfo2);

	return StrCmpLogicalW(Owner1.c_str(), Owner2.c_str());
}

int SortByVersionInfo(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, VersionInfoType versionInfoType)
{
	std::wstring VersionInfo1 = GetVersionColumnText(itemInfo1, versionInfoType);
	std::wstring VersionInfo2 = GetVersionColumnText(itemInfo2, versionInfoType);

	return StrCmpLogicalW(VersionInfo1.c_str(), VersionInfo2.c_str());
}

int SortByShortcutTo(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring ResolvedLinkPath1 = GetShortcutToColumnText(itemInfo1);
	std::wstring ResolvedLinkPath2 = GetShortcutToColumnText(itemInfo2);

	return StrCmpLogicalW(ResolvedLinkPath1.c_str(), ResolvedLinkPath2.c_str());
}

int SortByHardlinks(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	DWORD NumHardLinks1 = GetHardLinksColumnRawData(itemInfo1);
	DWORD NumHardLinks2 = GetHardLinksColumnRawData(itemInfo2);

	return NumHardLinks1 - NumHardLinks2;
}

int SortByExtension(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring Extension1 = GetExtensionColumnText(itemInfo1);
	std::wstring Extension2 = GetExtensionColumnText(itemInfo2);

	return StrCmpLogicalW(Extension1.c_str(), Extension2.c_str());
}

int SortByItemDetails(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, const SHCOLUMNID *pscid)
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

int SortByImageProperty(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, PROPID PropertyId)
{
	std::wstring ImageProperty1 = GetImageColumnText(itemInfo1, PropertyId);
	std::wstring ImageProperty2 = GetImageColumnText(itemInfo2, PropertyId);

	return StrCmpLogicalW(ImageProperty1.c_str(), ImageProperty2.c_str());
}

int SortByVirtualComments(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring Comments1 = GetControlPanelCommentsColumnText(itemInfo1);
	std::wstring Comments2 = GetControlPanelCommentsColumnText(itemInfo2);

	return StrCmpLogicalW(Comments1.c_str(), Comments2.c_str());
}

int SortByFileSystem(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring FileSystemName1 = GetFileSystemColumnText(itemInfo1);
	std::wstring FileSystemName2 = GetFileSystemColumnText(itemInfo2);

	return StrCmpLogicalW(FileSystemName1.c_str(), FileSystemName2.c_str());
}

int SortByPrinterProperty(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, PrinterInformationType printerInformationType)
{
	std::wstring PrinterInformation1 = GetPrinterColumnText(itemInfo1, printerInformationType);
	std::wstring PrinterInformation2 = GetPrinterColumnText(itemInfo2, printerInformationType);

	return StrCmpLogicalW(PrinterInformation1.c_str(), PrinterInformation2.c_str());
}

int SortByNetworkAdapterStatus(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring Status1 = GetNetworkAdapterColumnText(itemInfo1);
	std::wstring Status2 = GetNetworkAdapterColumnText(itemInfo2);

	return StrCmpLogicalW(Status1.c_str(), Status2.c_str());
}

int SortByMediaMetadata(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, MediaMetadataType mediaMetadataType)
{
	std::wstring MediaMetadata1 = GetMediaMetadataColumnText(itemInfo1, mediaMetadataType);
	std::wstring MediaMetadata2 = GetMediaMetadataColumnText(itemInfo2, mediaMetadataType);

	return StrCmpLogicalW(MediaMetadata1.c_str(), MediaMetadata2.c_str());
}