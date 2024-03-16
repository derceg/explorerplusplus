// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ColumnDataRetrieval.h"
#include "Columns.h"
#include "FolderSettings.h"
#include "ItemData.h"
#include "../Helper/DriveInfo.h"
#include "../Helper/FileOperations.h"
#include "../Helper/FolderSize.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/StringHelper.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <wil/com.h>
#include <IPHlpApi.h>
#include <propkey.h>
#include <filesystem>

BOOL GetPrinterStatusDescription(DWORD dwStatus, TCHAR *szStatus, size_t cchMax);

std::wstring GetColumnText(ColumnType columnType, const BasicItemInfo_t &basicItemInfo,
	const GlobalFolderSettings &globalFolderSettings)
{
	switch (columnType)
	{
	case ColumnType::Name:
		return GetNameColumnText(basicItemInfo, globalFolderSettings);

	case ColumnType::Type:
		return GetTypeColumnText(basicItemInfo);
	case ColumnType::Size:
		return GetSizeColumnText(basicItemInfo, globalFolderSettings);

	case ColumnType::DateModified:
		return GetTimeColumnText(basicItemInfo, TimeType::Modified, globalFolderSettings);
	case ColumnType::Created:
		return GetTimeColumnText(basicItemInfo, TimeType::Created, globalFolderSettings);
	case ColumnType::Accessed:
		return GetTimeColumnText(basicItemInfo, TimeType::Accessed, globalFolderSettings);

	case ColumnType::Attributes:
		return GetAttributeColumnText(basicItemInfo);
	case ColumnType::RealSize:
		return GetRealSizeColumnText(basicItemInfo, globalFolderSettings);
	case ColumnType::ShortName:
		return GetShortNameColumnText(basicItemInfo);
	case ColumnType::Owner:
		return GetOwnerColumnText(basicItemInfo);

	case ColumnType::ProductName:
		return GetVersionColumnText(basicItemInfo, VersionInfoType::ProductName);
	case ColumnType::Company:
		return GetVersionColumnText(basicItemInfo, VersionInfoType::Company);
	case ColumnType::Description:
		return GetVersionColumnText(basicItemInfo, VersionInfoType::Description);
	case ColumnType::FileVersion:
		return GetVersionColumnText(basicItemInfo, VersionInfoType::FileVersion);
	case ColumnType::ProductVersion:
		return GetVersionColumnText(basicItemInfo, VersionInfoType::ProductVersion);

	case ColumnType::ShortcutTo:
		return GetShortcutToColumnText(basicItemInfo);
	case ColumnType::HardLinks:
		return GetHardLinksColumnText(basicItemInfo);
	case ColumnType::Extension:
		return GetExtensionColumnText(basicItemInfo);

	case ColumnType::Title:
		return GetItemDetailsColumnText(basicItemInfo, &PKEY_Title, globalFolderSettings);
	case ColumnType::Subject:
		return GetItemDetailsColumnText(basicItemInfo, &PKEY_Subject, globalFolderSettings);
	case ColumnType::Authors:
		return GetItemDetailsColumnText(basicItemInfo, &PKEY_Author, globalFolderSettings);
	case ColumnType::Keywords:
		return GetItemDetailsColumnText(basicItemInfo, &PKEY_Keywords, globalFolderSettings);
	case ColumnType::Comment:
		return GetItemDetailsColumnText(basicItemInfo, &PKEY_Comment, globalFolderSettings);

	case ColumnType::CameraModel:
		return GetImageColumnText(basicItemInfo, PropertyTagEquipModel);
	case ColumnType::DateTaken:
		return GetImageColumnText(basicItemInfo, PropertyTagDateTime);
	case ColumnType::Width:
		return GetImageColumnText(basicItemInfo, PropertyTagImageWidth);
	case ColumnType::Height:
		return GetImageColumnText(basicItemInfo, PropertyTagImageHeight);

	case ColumnType::VirtualComments:
		return GetControlPanelCommentsColumnText(basicItemInfo);

	case ColumnType::TotalSize:
		return GetDriveSpaceColumnText(basicItemInfo, true, globalFolderSettings);

	case ColumnType::FreeSpace:
		return GetDriveSpaceColumnText(basicItemInfo, false, globalFolderSettings);

	case ColumnType::FileSystem:
		return GetFileSystemColumnText(basicItemInfo);

	case ColumnType::OriginalLocation:
		return GetItemDetailsColumnText(basicItemInfo, &SCID_ORIGINAL_LOCATION,
			globalFolderSettings);

	case ColumnType::DateDeleted:
		return GetItemDetailsColumnText(basicItemInfo, &SCID_DATE_DELETED, globalFolderSettings);

	case ColumnType::PrinterNumDocuments:
		return GetPrinterColumnText(basicItemInfo, PrinterInformationType::NumJobs);

	case ColumnType::PrinterStatus:
		return GetPrinterColumnText(basicItemInfo, PrinterInformationType::Status);

	case ColumnType::PrinterComments:
		return GetPrinterColumnText(basicItemInfo, PrinterInformationType::Comments);

	case ColumnType::PrinterLocation:
		return GetPrinterColumnText(basicItemInfo, PrinterInformationType::Location);

	case ColumnType::PrinterModel:
		return GetPrinterColumnText(basicItemInfo, PrinterInformationType::Model);

	case ColumnType::NetworkAdaptorStatus:
		return GetNetworkAdapterColumnText(basicItemInfo);

	case ColumnType::MediaBitrate:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Bitrate);
	case ColumnType::MediaCopyright:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Copyright);
	case ColumnType::MediaDuration:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Duration);
	case ColumnType::MediaProtected:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Protected);
	case ColumnType::MediaRating:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Rating);
	case ColumnType::MediaAlbumArtist:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::AlbumArtist);
	case ColumnType::MediaAlbum:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::AlbumTitle);
	case ColumnType::MediaBeatsPerMinute:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::BeatsPerMinute);
	case ColumnType::MediaComposer:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Composer);
	case ColumnType::MediaConductor:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Conductor);
	case ColumnType::MediaDirector:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Director);
	case ColumnType::MediaGenre:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Genre);
	case ColumnType::MediaLanguage:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Language);
	case ColumnType::MediaBroadcastDate:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::BroadcastDate);
	case ColumnType::MediaChannel:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Channel);
	case ColumnType::MediaStationName:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::StationName);
	case ColumnType::MediaMood:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Mood);
	case ColumnType::MediaParentalRating:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::ParentalRating);
	case ColumnType::MediaParentalRatingReason:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::ParentalRatingReason);
	case ColumnType::MediaPeriod:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Period);
	case ColumnType::MediaProducer:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Producer);
	case ColumnType::MediaPublisher:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Publisher);
	case ColumnType::MediaWriter:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Writer);
	case ColumnType::MediaYear:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Year);

	default:
		assert(false);
		break;
	}

	return EMPTY_STRING;
}

std::wstring GetNameColumnText(const BasicItemInfo_t &itemInfo,
	const GlobalFolderSettings &globalFolderSettings)
{
	return ProcessItemFileName(itemInfo, globalFolderSettings);
}

/* Processes an items filename. Essentially checks
if the extension (if any) needs to be removed, and
removes it if it does. */
std::wstring ProcessItemFileName(const BasicItemInfo_t &itemInfo,
	const GlobalFolderSettings &globalFolderSettings)
{
	BOOL bHideExtension = FALSE;
	TCHAR *pExt = nullptr;

	if (globalFolderSettings.hideLinkExtension
		&& ((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY))
	{
		pExt = PathFindExtension(itemInfo.szDisplayName);

		if (*pExt != '\0')
		{
			if (lstrcmpi(pExt, _T(".lnk")) == 0)
			{
				bHideExtension = TRUE;
			}
		}
	}

	/* We'll hide the extension, provided it is meant
	to be hidden, and the filename does not begin with
	a period, and the item is not a directory. */
	if ((!globalFolderSettings.showExtensions || bHideExtension) && itemInfo.szDisplayName[0] != '.'
		&& (itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
	{
		TCHAR szDisplayName[MAX_PATH];

		StringCchCopy(szDisplayName, SIZEOF_ARRAY(szDisplayName), itemInfo.szDisplayName);

		/* Strip the extension. */
		PathRemoveExtension(szDisplayName);

		return szDisplayName;
	}
	else
	{
		return itemInfo.szDisplayName;
	}
}

std::wstring GetTypeColumnText(const BasicItemInfo_t &itemInfo)
{
	SHFILEINFO shfi;
	DWORD_PTR res = SHGetFileInfo(reinterpret_cast<LPTSTR>(itemInfo.pidlComplete.get()), 0, &shfi,
		sizeof(shfi), SHGFI_PIDL | SHGFI_TYPENAME);

	if (res == 0)
	{
		return EMPTY_STRING;
	}

	return shfi.szTypeName;
}

std::wstring GetSizeColumnText(const BasicItemInfo_t &itemInfo,
	const GlobalFolderSettings &globalFolderSettings)
{
	if (!itemInfo.isFindDataValid)
	{
		return L"";
	}

	if ((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		TCHAR drive[MAX_PATH];
		StringCchCopy(drive, SIZEOF_ARRAY(drive), itemInfo.getFullPath().c_str());
		PathStripToRoot(drive);

		bool bNetworkRemovable = false;

		if (GetDriveType(drive) == DRIVE_REMOVABLE || GetDriveType(drive) == DRIVE_REMOTE)
		{
			bNetworkRemovable = true;
		}

		if (globalFolderSettings.showFolderSizes
			&& !(globalFolderSettings.disableFolderSizesNetworkRemovable && bNetworkRemovable))
		{
			return GetFolderSizeColumnText(itemInfo, globalFolderSettings);
		}
		else
		{
			return EMPTY_STRING;
		}
	}

	ULARGE_INTEGER fileSize = { itemInfo.wfd.nFileSizeLow, itemInfo.wfd.nFileSizeHigh };
	auto displayFormat = globalFolderSettings.forceSize ? globalFolderSettings.sizeDisplayFormat
														: +SizeDisplayFormat::None;
	return FormatSizeString(fileSize.QuadPart, displayFormat);
}

std::wstring GetFolderSizeColumnText(const BasicItemInfo_t &itemInfo,
	const GlobalFolderSettings &globalFolderSettings)
{
	auto folderInfo = GetFolderInfo(itemInfo.getFullPath());

	/* TODO: This should
	be done some other way.
	Shouldn't depend on
	the internal index. */
	// m_cachedFolderSizes.insert({internalIndex, totalFolderSize.QuadPart});

	auto displayFormat = globalFolderSettings.forceSize ? globalFolderSettings.sizeDisplayFormat
														: +SizeDisplayFormat::None;
	return FormatSizeString(folderInfo.size, displayFormat);
}

std::wstring GetTimeColumnText(const BasicItemInfo_t &itemInfo, TimeType timeType,
	const GlobalFolderSettings &globalFolderSettings)
{
	if (!itemInfo.isFindDataValid)
	{
		return L"";
	}

	TCHAR fileTime[64];
	BOOL bRet = FALSE;

	switch (timeType)
	{
	case TimeType::Modified:
		bRet = CreateFileTimeString(&itemInfo.wfd.ftLastWriteTime, fileTime, SIZEOF_ARRAY(fileTime),
			globalFolderSettings.showFriendlyDates);
		break;

	case TimeType::Created:
		bRet = CreateFileTimeString(&itemInfo.wfd.ftCreationTime, fileTime, SIZEOF_ARRAY(fileTime),
			globalFolderSettings.showFriendlyDates);
		break;

	case TimeType::Accessed:
		bRet = CreateFileTimeString(&itemInfo.wfd.ftLastAccessTime, fileTime,
			SIZEOF_ARRAY(fileTime), globalFolderSettings.showFriendlyDates);
		break;

	default:
		assert(false);
		break;
	}

	if (!bRet)
	{
		return EMPTY_STRING;
	}

	return fileTime;
}

std::wstring GetRealSizeColumnText(const BasicItemInfo_t &itemInfo,
	const GlobalFolderSettings &globalFolderSettings)
{
	ULARGE_INTEGER realFileSize;
	bool res = GetRealSizeColumnRawData(itemInfo, realFileSize);

	if (!res)
	{
		return EMPTY_STRING;
	}

	auto displayFormat = globalFolderSettings.forceSize ? globalFolderSettings.sizeDisplayFormat
														: +SizeDisplayFormat::None;
	return FormatSizeString(realFileSize.QuadPart, displayFormat);
}

bool GetRealSizeColumnRawData(const BasicItemInfo_t &itemInfo, ULARGE_INTEGER &RealFileSize)
{
	if ((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		return false;
	}

	TCHAR root[MAX_PATH];
	StringCchCopy(root, SIZEOF_ARRAY(root), itemInfo.getFullPath().c_str());
	PathStripToRoot(root);

	DWORD dwClusterSize;
	BOOL bRet = GetClusterSize(root, &dwClusterSize);

	if (!bRet)
	{
		return false;
	}

	ULARGE_INTEGER realFileSizeTemp = { itemInfo.wfd.nFileSizeLow, itemInfo.wfd.nFileSizeHigh };

	if (realFileSizeTemp.QuadPart != 0 && (realFileSizeTemp.QuadPart % dwClusterSize) != 0)
	{
		realFileSizeTemp.QuadPart += dwClusterSize - (realFileSizeTemp.QuadPart % dwClusterSize);
	}

	RealFileSize = realFileSizeTemp;

	return true;
}

std::wstring GetAttributeColumnText(const BasicItemInfo_t &itemInfo)
{
	if (!itemInfo.isFindDataValid)
	{
		return {};
	}

	return BuildFileAttributesString(itemInfo.wfd.dwFileAttributes);
}

std::wstring GetShortNameColumnText(const BasicItemInfo_t &itemInfo)
{
	DWORD length = GetShortPathName(itemInfo.getFullPath().c_str(), nullptr, 0);

	if (length == 0)
	{
		return {};
	}

	std::wstring shortPath;
	shortPath.resize(length);

	length = GetShortPathName(itemInfo.getFullPath().c_str(), shortPath.data(),
		static_cast<DWORD>(shortPath.capacity()));

	if (length == 0)
	{
		return {};
	}

	std::filesystem::path path(shortPath);
	return path.filename();
}

std::wstring GetOwnerColumnText(const BasicItemInfo_t &itemInfo)
{
	TCHAR owner[512];
	BOOL ret = GetFileOwner(itemInfo.getFullPath().c_str(), owner, SIZEOF_ARRAY(owner));

	if (!ret)
	{
		return EMPTY_STRING;
	}

	return owner;
}

std::wstring GetItemDetailsColumnText(const BasicItemInfo_t &itemInfo, const SHCOLUMNID *pscid,
	const GlobalFolderSettings &globalFolderSettings)
{
	TCHAR szDetail[512];
	HRESULT hr =
		GetItemDetails(itemInfo, pscid, szDetail, SIZEOF_ARRAY(szDetail), globalFolderSettings);

	if (SUCCEEDED(hr))
	{
		return szDetail;
	}

	return EMPTY_STRING;
}

HRESULT GetItemDetails(const BasicItemInfo_t &itemInfo, const SHCOLUMNID *pscid, TCHAR *szDetail,
	size_t cchMax, const GlobalFolderSettings &globalFolderSettings)
{
	VARIANT vt;
	HRESULT hr = GetItemDetailsRawData(itemInfo, pscid, &vt);

	if (SUCCEEDED(hr))
	{
		hr = ConvertVariantToString(&vt, szDetail, cchMax, globalFolderSettings.showFriendlyDates);
		VariantClear(&vt);
	}

	return hr;
}

HRESULT GetItemDetailsRawData(const BasicItemInfo_t &itemInfo, const SHCOLUMNID *pscid, VARIANT *vt)
{
	wil::com_ptr_nothrow<IShellFolder2> pShellFolder;
	HRESULT hr = SHBindToParent(itemInfo.pidlComplete.get(), IID_PPV_ARGS(&pShellFolder), nullptr);

	if (SUCCEEDED(hr))
	{
		hr = pShellFolder->GetDetailsEx(itemInfo.pridl.get(), pscid, vt);
	}

	return hr;
}

std::wstring GetVersionColumnText(const BasicItemInfo_t &itemInfo, VersionInfoType versioninfoType)
{
	std::wstring versionInfoName;

	switch (versioninfoType)
	{
	case VersionInfoType::ProductName:
		versionInfoName = L"ProductName";
		break;

	case VersionInfoType::Company:
		versionInfoName = L"CompanyName";
		break;

	case VersionInfoType::Description:
		versionInfoName = L"FileDescription";
		break;

	case VersionInfoType::FileVersion:
		versionInfoName = L"FileVersion";
		break;

	case VersionInfoType::ProductVersion:
		versionInfoName = L"ProductVersion";
		break;

	default:
		assert(false);
		break;
	}

	TCHAR versionInfo[512];
	BOOL versionInfoObtained = GetVersionInfoString(itemInfo.getFullPath().c_str(),
		versionInfoName.c_str(), versionInfo, SIZEOF_ARRAY(versionInfo));

	if (!versionInfoObtained)
	{
		return EMPTY_STRING;
	}

	return versionInfo;
}

std::wstring GetShortcutToColumnText(const BasicItemInfo_t &itemInfo)
{
	TCHAR resolvedLinkPath[MAX_PATH];
	HRESULT hr = NFileOperations::ResolveLink(nullptr, SLR_NO_UI, itemInfo.getFullPath().c_str(),
		resolvedLinkPath, SIZEOF_ARRAY(resolvedLinkPath));

	if (FAILED(hr))
	{
		return EMPTY_STRING;
	}

	return resolvedLinkPath;
}

std::wstring GetHardLinksColumnText(const BasicItemInfo_t &itemInfo)
{
	DWORD numHardLinks = GetHardLinksColumnRawData(itemInfo);

	if (numHardLinks == -1)
	{
		return EMPTY_STRING;
	}

	TCHAR numHardLinksString[32];
	StringCchPrintf(numHardLinksString, SIZEOF_ARRAY(numHardLinksString), _T("%ld"), numHardLinks);

	return numHardLinksString;
}

DWORD GetHardLinksColumnRawData(const BasicItemInfo_t &itemInfo)
{
	return GetNumFileHardLinks(itemInfo.getFullPath().c_str());
}

std::wstring GetExtensionColumnText(const BasicItemInfo_t &itemInfo)
{
	if ((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		return EMPTY_STRING;
	}

	TCHAR *extension = PathFindExtension(itemInfo.wfd.cFileName);

	if (*extension != '.')
	{
		return EMPTY_STRING;
	}

	return extension + 1;
}

std::wstring GetImageColumnText(const BasicItemInfo_t &itemInfo, PROPID PropertyID)
{
	TCHAR imageProperty[512];
	BOOL res = ReadImageProperty(itemInfo.getFullPath().c_str(), PropertyID, imageProperty,
		SIZEOF_ARRAY(imageProperty));

	if (!res)
	{
		return EMPTY_STRING;
	}

	return imageProperty;
}

std::wstring GetFileSystemColumnText(const BasicItemInfo_t &itemInfo)
{
	std::wstring fullFileName;
	GetDisplayName(itemInfo.pidlComplete.get(), SHGDN_FORPARSING, fullFileName);

	BOOL isRoot = PathIsRoot(fullFileName.c_str());

	if (!isRoot)
	{
		return EMPTY_STRING;
	}

	TCHAR fileSystemName[MAX_PATH];
	BOOL res = GetVolumeInformation(fullFileName.c_str(), nullptr, 0, nullptr, nullptr, nullptr,
		fileSystemName, SIZEOF_ARRAY(fileSystemName));

	if (!res)
	{
		return EMPTY_STRING;
	}

	return fileSystemName;
}

std::wstring GetControlPanelCommentsColumnText(const BasicItemInfo_t &itemInfo)
{
	std::wstring infoTip;
	HRESULT hr = GetItemInfoTip(itemInfo.getFullPath(), infoTip);

	if (FAILED(hr))
	{
		return {};
	}

	return infoTip;
}

std::wstring GetPrinterColumnText(const BasicItemInfo_t &itemInfo,
	PrinterInformationType printerInformationType)
{
	TCHAR printerInformation[256] = EMPTY_STRING;
	TCHAR szStatus[256];

	TCHAR itemDisplayName[MAX_PATH];
	StringCchCopy(itemDisplayName, SIZEOF_ARRAY(itemDisplayName), itemInfo.szDisplayName);

	HANDLE hPrinter;
	BOOL res = OpenPrinter(itemDisplayName, &hPrinter, nullptr);

	if (res)
	{
		DWORD bytesNeeded;
		GetPrinter(hPrinter, 2, nullptr, 0, &bytesNeeded);

		auto *printerInfo2 = reinterpret_cast<PRINTER_INFO_2 *>(new char[bytesNeeded]);
		res = GetPrinter(hPrinter, 2, reinterpret_cast<LPBYTE>(printerInfo2), bytesNeeded,
			&bytesNeeded);

		if (res)
		{
			switch (printerInformationType)
			{
			case PrinterInformationType::NumJobs:
				StringCchPrintf(printerInformation, SIZEOF_ARRAY(printerInformation), _T("%d"),
					printerInfo2->cJobs);
				break;

			case PrinterInformationType::Status:
				res = GetPrinterStatusDescription(printerInfo2->Status, szStatus,
					SIZEOF_ARRAY(szStatus));

				if (res)
				{
					StringCchCopyEx(printerInformation, SIZEOF_ARRAY(printerInformation), szStatus,
						nullptr, nullptr, STRSAFE_IGNORE_NULLS);
				}
				break;

			case PrinterInformationType::Comments:
				StringCchCopyEx(printerInformation, SIZEOF_ARRAY(printerInformation),
					printerInfo2->pComment, nullptr, nullptr, STRSAFE_IGNORE_NULLS);
				break;

			case PrinterInformationType::Location:
				StringCchCopyEx(printerInformation, SIZEOF_ARRAY(printerInformation),
					printerInfo2->pLocation, nullptr, nullptr, STRSAFE_IGNORE_NULLS);
				break;

			case PrinterInformationType::Model:
				StringCchCopyEx(printerInformation, SIZEOF_ARRAY(printerInformation),
					printerInfo2->pDriverName, nullptr, nullptr, STRSAFE_IGNORE_NULLS);
				break;

			default:
				assert(false);
				break;
			}
		}

		delete[] printerInfo2;
		ClosePrinter(hPrinter);
	}

	return printerInformation;
}

BOOL GetPrinterStatusDescription(DWORD dwStatus, TCHAR *szStatus, size_t cchMax)
{
	BOOL bSuccess = TRUE;

	if (dwStatus == 0)
	{
		StringCchCopy(szStatus, cchMax, _T("Ready"));
	}
	else if (dwStatus & PRINTER_STATUS_BUSY)
	{
		StringCchCopy(szStatus, cchMax, _T("Busy"));
	}
	else if (dwStatus & PRINTER_STATUS_ERROR)
	{
		StringCchCopy(szStatus, cchMax, _T("Error"));
	}
	else if (dwStatus & PRINTER_STATUS_INITIALIZING)
	{
		StringCchCopy(szStatus, cchMax, _T("Initializing"));
	}
	else if (dwStatus & PRINTER_STATUS_IO_ACTIVE)
	{
		StringCchCopy(szStatus, cchMax, _T("Active"));
	}
	else if (dwStatus & PRINTER_STATUS_NOT_AVAILABLE)
	{
		StringCchCopy(szStatus, cchMax, _T("Unavailable"));
	}
	else if (dwStatus & PRINTER_STATUS_OFFLINE)
	{
		StringCchCopy(szStatus, cchMax, _T("Offline"));
	}
	else if (dwStatus & PRINTER_STATUS_OUT_OF_MEMORY)
	{
		StringCchCopy(szStatus, cchMax, _T("Out of memory"));
	}
	else if (dwStatus & PRINTER_STATUS_NO_TONER)
	{
		StringCchCopy(szStatus, cchMax, _T("Out of toner"));
	}
	else
	{
		bSuccess = FALSE;
	}

	return bSuccess;
}

std::wstring GetNetworkAdapterColumnText(const BasicItemInfo_t &itemInfo)
{
	ULONG outBufLen = 0;
	GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, nullptr, &outBufLen);
	auto *adapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(new char[outBufLen]);
	GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, adapterAddresses, &outBufLen);

	IP_ADAPTER_ADDRESSES *adapaterAddress = adapterAddresses;

	while (adapaterAddress != nullptr
		&& lstrcmp(adapaterAddress->FriendlyName, itemInfo.wfd.cFileName) != 0)
	{
		adapaterAddress = adapaterAddress->Next;
	}

	std::wstring status;

	/* TODO: These strings need to be setup correctly. */
	switch (adapaterAddress->OperStatus)
	{
	case IfOperStatusUp:
		status = L"Connected";
		break;

	case IfOperStatusDown:
		status = L"Disconnected";
		break;

	case IfOperStatusTesting:
		status = L"Testing";
		break;

	case IfOperStatusUnknown:
		status = L"Unknown";
		break;

	case IfOperStatusDormant:
		status = L"Dormant";
		break;

	case IfOperStatusNotPresent:
		status = L"Not present";
		break;

	case IfOperStatusLowerLayerDown:
		status = L"Lower layer non-operational";
		break;
	}

	delete[] adapterAddresses;

	return status;
}

std::wstring GetMediaMetadataColumnText(const BasicItemInfo_t &itemInfo,
	MediaMetadataType mediaMetadataType)
{
	const TCHAR *attributeName = GetMediaMetadataAttributeName(mediaMetadataType);

	BYTE *tempBuffer = nullptr;
	HRESULT hr = GetMediaMetadata(itemInfo.getFullPath().c_str(), attributeName, &tempBuffer);

	if (!SUCCEEDED(hr))
	{
		return EMPTY_STRING;
	}

	TCHAR szOutput[512];

	switch (mediaMetadataType)
	{
	case MediaMetadataType::Bitrate:
	{
		DWORD bitRate = *(reinterpret_cast<DWORD *>(tempBuffer));

		if (bitRate > 1000)
		{
			StringCchPrintf(szOutput, SIZEOF_ARRAY(szOutput), _T("%d kbps"), bitRate / 1000);
		}
		else
		{
			StringCchPrintf(szOutput, SIZEOF_ARRAY(szOutput), _T("%d bps"), bitRate);
		}
	}
	break;

	case MediaMetadataType::Duration:
	{
		auto *facet = new boost::posix_time::wtime_facet();
		facet->time_duration_format(L"%H:%M:%S");

		std::wstringstream dateStream;
		dateStream.imbue(std::locale(dateStream.getloc(), facet));

		/* Note that the duration itself is in 100-nanosecond units
		(see http://msdn.microsoft.com/en-us/library/windows/desktop/dd798053(v=vs.85).aspx). */
		boost::posix_time::time_duration duration =
			boost::posix_time::microseconds(*(reinterpret_cast<QWORD *>(tempBuffer)) / 10);
		dateStream << duration;

		StringCchCopy(szOutput, SIZEOF_ARRAY(szOutput), dateStream.str().c_str());
	}
	break;

	case MediaMetadataType::Protected:
		if (*(reinterpret_cast<BOOL *>(tempBuffer)))
		{
			StringCchCopy(szOutput, SIZEOF_ARRAY(szOutput), L"Yes");
		}
		else
		{
			StringCchCopy(szOutput, SIZEOF_ARRAY(szOutput), L"No");
		}
		break;

	case MediaMetadataType::Copyright:
	case MediaMetadataType::Rating:
	case MediaMetadataType::AlbumArtist:
	case MediaMetadataType::AlbumTitle:
	case MediaMetadataType::BeatsPerMinute:
	case MediaMetadataType::Composer:
	case MediaMetadataType::Conductor:
	case MediaMetadataType::Director:
	case MediaMetadataType::Genre:
	case MediaMetadataType::Language:
	case MediaMetadataType::BroadcastDate:
	case MediaMetadataType::Channel:
	case MediaMetadataType::StationName:
	case MediaMetadataType::Mood:
	case MediaMetadataType::ParentalRating:
	case MediaMetadataType::ParentalRatingReason:
	case MediaMetadataType::Period:
	case MediaMetadataType::Producer:
	case MediaMetadataType::Publisher:
	case MediaMetadataType::Writer:
	case MediaMetadataType::Year:
	default:
		StringCchCopy(szOutput, SIZEOF_ARRAY(szOutput), reinterpret_cast<TCHAR *>(tempBuffer));
		break;
	}

	free(tempBuffer);

	return szOutput;
}

const TCHAR *GetMediaMetadataAttributeName(MediaMetadataType mediaMetadataType)
{
	switch (mediaMetadataType)
	{
	case MediaMetadataType::Bitrate:
		return g_wszWMBitrate;

	case MediaMetadataType::Copyright:
		return g_wszWMCopyright;

	case MediaMetadataType::Duration:
		return g_wszWMDuration;

	case MediaMetadataType::Protected:
		return g_wszWMProtected;

	case MediaMetadataType::Rating:
		return g_wszWMRating;

	case MediaMetadataType::AlbumArtist:
		return g_wszWMAlbumArtist;

	case MediaMetadataType::AlbumTitle:
		return g_wszWMAlbumTitle;

	case MediaMetadataType::BeatsPerMinute:
		return g_wszWMBeatsPerMinute;

	case MediaMetadataType::Composer:
		return g_wszWMComposer;

	case MediaMetadataType::Conductor:
		return g_wszWMConductor;

	case MediaMetadataType::Director:
		return g_wszWMDirector;

	case MediaMetadataType::Genre:
		return g_wszWMGenre;

	case MediaMetadataType::Language:
		return g_wszWMLanguage;

	case MediaMetadataType::BroadcastDate:
		return g_wszWMMediaOriginalBroadcastDateTime;

	case MediaMetadataType::Channel:
		return g_wszWMMediaOriginalChannel;

	case MediaMetadataType::StationName:
		return g_wszWMMediaStationName;

	case MediaMetadataType::Mood:
		return g_wszWMMood;

	case MediaMetadataType::ParentalRating:
		return g_wszWMParentalRating;

	case MediaMetadataType::ParentalRatingReason:
		return g_wszWMParentalRatingReason;

	case MediaMetadataType::Period:
		return g_wszWMPeriod;

	case MediaMetadataType::Producer:
		return g_wszWMProducer;

	case MediaMetadataType::Publisher:
		return g_wszWMPublisher;

	case MediaMetadataType::Writer:
		return g_wszWMWriter;

	case MediaMetadataType::Year:
		return g_wszWMYear;

	default:
		assert(false);
		break;
	}

	return nullptr;
}

std::wstring GetDriveSpaceColumnText(const BasicItemInfo_t &itemInfo, bool TotalSize,
	const GlobalFolderSettings &globalFolderSettings)
{
	ULARGE_INTEGER driveSpace;
	BOOL res = GetDriveSpaceColumnRawData(itemInfo, TotalSize, driveSpace);

	if (!res)
	{
		return EMPTY_STRING;
	}

	auto displayFormat = globalFolderSettings.forceSize ? globalFolderSettings.sizeDisplayFormat
														: +SizeDisplayFormat::None;
	return FormatSizeString(driveSpace.QuadPart, displayFormat);
}

BOOL GetDriveSpaceColumnRawData(const BasicItemInfo_t &itemInfo, bool TotalSize,
	ULARGE_INTEGER &DriveSpace)
{
	std::wstring fullFileName;
	GetDisplayName(itemInfo.pidlComplete.get(), SHGDN_FORPARSING, fullFileName);

	BOOL isRoot = PathIsRoot(fullFileName.c_str());

	if (!isRoot)
	{
		return FALSE;
	}

	ULARGE_INTEGER totalBytes;
	ULARGE_INTEGER freeBytes;
	BOOL res = GetDiskFreeSpaceEx(fullFileName.c_str(), nullptr, &totalBytes, &freeBytes);

	if (TotalSize)
	{
		DriveSpace = totalBytes;
	}
	else
	{
		DriveSpace = freeBytes;
	}

	return res;
}
