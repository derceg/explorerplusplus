// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SortHelper.h"
#include "ItemData.h"
#include <wil/common.h>
#include <propvarutil.h>

int SortByName(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2,
	const GlobalFolderSettings &globalFolderSettings)
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
		if (globalFolderSettings.useNaturalSortOrder)
		{
			return StrCmpLogicalW(itemInfo1.getFullPath().c_str(), itemInfo2.getFullPath().c_str());
		}
		else
		{
			return StrCmpIW(itemInfo1.getFullPath().c_str(), itemInfo2.getFullPath().c_str());
		}
	}

	std::wstring name1 = GetNameColumnText(itemInfo1, globalFolderSettings);
	std::wstring name2 = GetNameColumnText(itemInfo2, globalFolderSettings);

	if (globalFolderSettings.useNaturalSortOrder)
	{
		return StrCmpLogicalW(name1.c_str(), name2.c_str());
	}
	else
	{
		return StrCmpIW(name1.c_str(), name2.c_str());
	}
}

int SortBySize(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	if (!itemInfo1.isFindDataValid && itemInfo2.isFindDataValid)
	{
		return -1;
	}
	else if (itemInfo1.isFindDataValid && !itemInfo2.isFindDataValid)
	{
		return 1;
	}
	else if (!itemInfo1.isFindDataValid && !itemInfo2.isFindDataValid)
	{
		return 0;
	}

	bool isFolder1 = WI_IsFlagSet(itemInfo1.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
	bool isFolder2 = WI_IsFlagSet(itemInfo2.wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);

	ULONGLONG size1;
	ULONGLONG size2;

	if (isFolder1 && isFolder2)
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
		ULARGE_INTEGER fileSize1 = { { itemInfo1.wfd.nFileSizeLow, itemInfo1.wfd.nFileSizeHigh } };
		ULARGE_INTEGER fileSize2 = { { itemInfo2.wfd.nFileSizeLow, itemInfo2.wfd.nFileSizeHigh } };

		size1 = fileSize1.QuadPart;
		size2 = fileSize2.QuadPart;
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

	std::wstring type1 = GetTypeColumnText(itemInfo1);
	std::wstring type2 = GetTypeColumnText(itemInfo2);

	return StrCmpLogicalW(type1.c_str(), type2.c_str());
}

int SortByDate(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2,
	DateType dateType)
{
	if (!itemInfo1.isFindDataValid && itemInfo2.isFindDataValid)
	{
		return -1;
	}
	else if (itemInfo1.isFindDataValid && !itemInfo2.isFindDataValid)
	{
		return 1;
	}
	else if (!itemInfo1.isFindDataValid && !itemInfo2.isFindDataValid)
	{
		return 0;
	}

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

int SortByTotalSize(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2,
	bool TotalSize)
{
	ULARGE_INTEGER driveSpace1;
	BOOL res1 = GetDriveSpaceColumnRawData(itemInfo1, TotalSize, driveSpace1);

	ULARGE_INTEGER driveSpace2;
	BOOL res2 = GetDriveSpaceColumnRawData(itemInfo2, TotalSize, driveSpace2);

	if (res1 && !res2)
	{
		return 1;
	}
	else if (!res1 && res2)
	{
		return -1;
	}
	else if (!res1 && !res2)
	{
		return 0;
	}

	if (driveSpace1.QuadPart > driveSpace2.QuadPart)
	{
		return 1;
	}
	else if (driveSpace1.QuadPart < driveSpace2.QuadPart)
	{
		return -1;
	}

	return 0;
}

int SortByAttributes(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring attributeString1 = GetAttributeColumnText(itemInfo1);
	std::wstring attributeString2 = GetAttributeColumnText(itemInfo2);

	return StrCmpLogicalW(attributeString1.c_str(), attributeString2.c_str());
}

int SortByRealSize(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	ULARGE_INTEGER realFileSize1;
	bool res1 = GetRealSizeColumnRawData(itemInfo1, realFileSize1);

	ULARGE_INTEGER realFileSize2;
	bool res2 = GetRealSizeColumnRawData(itemInfo2, realFileSize2);

	if (res1 && !res2)
	{
		return 1;
	}
	else if (!res1 && res2)
	{
		return -1;
	}
	else if (!res1 && !res2)
	{
		return 0;
	}

	if (realFileSize1.QuadPart > realFileSize2.QuadPart)
	{
		return 1;
	}
	else if (realFileSize1.QuadPart < realFileSize2.QuadPart)
	{
		return -1;
	}

	return 0;
}

int SortByShortName(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring shortName1 = GetShortNameColumnText(itemInfo1);
	std::wstring shortName2 = GetShortNameColumnText(itemInfo2);

	return StrCmpLogicalW(shortName1.c_str(), shortName2.c_str());
}

int SortByOwner(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring owner1 = GetOwnerColumnText(itemInfo1);
	std::wstring owner2 = GetOwnerColumnText(itemInfo2);

	return StrCmpLogicalW(owner1.c_str(), owner2.c_str());
}

int SortByVersionInfo(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2,
	VersionInfoType versionInfoType)
{
	std::wstring versionInfo1 = GetVersionColumnText(itemInfo1, versionInfoType);
	std::wstring versionInfo2 = GetVersionColumnText(itemInfo2, versionInfoType);

	return StrCmpLogicalW(versionInfo1.c_str(), versionInfo2.c_str());
}

int SortByShortcutTo(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring resolvedLinkPath1 = GetShortcutToColumnText(itemInfo1);
	std::wstring resolvedLinkPath2 = GetShortcutToColumnText(itemInfo2);

	return StrCmpLogicalW(resolvedLinkPath1.c_str(), resolvedLinkPath2.c_str());
}

int SortByHardlinks(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	auto numHardLinks1 = GetHardLinksColumnRawData(itemInfo1);
	auto numHardLinks2 = GetHardLinksColumnRawData(itemInfo2);

	if (!numHardLinks1 && !numHardLinks2)
	{
		return 0;
	}
	else if (!numHardLinks1 && numHardLinks2)
	{
		return -1;
	}
	else if (numHardLinks1 && !numHardLinks2)
	{
		return 1;
	}
	else
	{
		return *numHardLinks1 - *numHardLinks2;
	}
}

int SortByExtension(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring extension1 = GetExtensionColumnText(itemInfo1);
	std::wstring extension2 = GetExtensionColumnText(itemInfo2);

	return StrCmpLogicalW(extension1.c_str(), extension2.c_str());
}

int SortByItemDetails(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2,
	const SHCOLUMNID *pscid)
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

int SortByImageProperty(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2,
	PROPID PropertyId)
{
	std::wstring imageProperty1 = GetImageColumnText(itemInfo1, PropertyId);
	std::wstring imageProperty2 = GetImageColumnText(itemInfo2, PropertyId);

	return StrCmpLogicalW(imageProperty1.c_str(), imageProperty2.c_str());
}

int SortByVirtualComments(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring comments1 = GetControlPanelCommentsColumnText(itemInfo1);
	std::wstring comments2 = GetControlPanelCommentsColumnText(itemInfo2);

	return StrCmpLogicalW(comments1.c_str(), comments2.c_str());
}

int SortByFileSystem(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring fileSystemName1 = GetFileSystemColumnText(itemInfo1);
	std::wstring fileSystemName2 = GetFileSystemColumnText(itemInfo2);

	return StrCmpLogicalW(fileSystemName1.c_str(), fileSystemName2.c_str());
}

int SortByPrinterProperty(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2,
	PrinterInformationType printerInformationType)
{
	std::wstring printerInformation1 = GetPrinterColumnText(itemInfo1, printerInformationType);
	std::wstring printerInformation2 = GetPrinterColumnText(itemInfo2, printerInformationType);

	return StrCmpLogicalW(printerInformation1.c_str(), printerInformation2.c_str());
}

int SortByNetworkAdapterStatus(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2)
{
	std::wstring status1 = GetNetworkAdapterColumnText(itemInfo1);
	std::wstring status2 = GetNetworkAdapterColumnText(itemInfo2);

	return StrCmpLogicalW(status1.c_str(), status2.c_str());
}

int SortByMediaMetadata(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2,
	MediaMetadataType mediaMetadataType)
{
	std::wstring mediaMetadata1 = GetMediaMetadataColumnText(itemInfo1, mediaMetadataType);
	std::wstring mediaMetadata2 = GetMediaMetadataColumnText(itemInfo2, mediaMetadataType);

	return StrCmpLogicalW(mediaMetadata1.c_str(), mediaMetadata2.c_str());
}
