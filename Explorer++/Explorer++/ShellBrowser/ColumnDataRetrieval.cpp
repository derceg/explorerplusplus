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
#include <boost\date_time\posix_time\posix_time.hpp>
#include <wil/com.h>
#include <IPHlpApi.h>
#include <propkey.h>

BOOL GetPrinterStatusDescription(DWORD dwStatus, TCHAR *szStatus, size_t cchMax);

std::wstring GetColumnText(UINT ColumnID, const BasicItemInfo_t &basicItemInfo, const GlobalFolderSettings &globalFolderSettings)
{
	switch (ColumnID)
	{
	case CM_NAME:
		return GetNameColumnText(basicItemInfo, globalFolderSettings);

	case CM_TYPE:
		return GetTypeColumnText(basicItemInfo);
	case CM_SIZE:
		return GetSizeColumnText(basicItemInfo, globalFolderSettings);

	case CM_DATEMODIFIED:
		return GetTimeColumnText(basicItemInfo, TimeType::Modified, globalFolderSettings);
	case CM_CREATED:
		return GetTimeColumnText(basicItemInfo, TimeType::Created, globalFolderSettings);
	case CM_ACCESSED:
		return GetTimeColumnText(basicItemInfo, TimeType::Accessed, globalFolderSettings);

	case CM_ATTRIBUTES:
		return GetAttributeColumnText(basicItemInfo);
	case CM_REALSIZE:
		return GetRealSizeColumnText(basicItemInfo, globalFolderSettings);
	case CM_SHORTNAME:
		return GetShortNameColumnText(basicItemInfo);
	case CM_OWNER:
		return GetOwnerColumnText(basicItemInfo);

	case CM_PRODUCTNAME:
		return GetVersionColumnText(basicItemInfo, VersionInfoType::ProductName);
	case CM_COMPANY:
		return GetVersionColumnText(basicItemInfo, VersionInfoType::Company);
	case CM_DESCRIPTION:
		return GetVersionColumnText(basicItemInfo, VersionInfoType::Description);
	case CM_FILEVERSION:
		return GetVersionColumnText(basicItemInfo, VersionInfoType::FileVersion);
	case CM_PRODUCTVERSION:
		return GetVersionColumnText(basicItemInfo, VersionInfoType::ProductVersion);

	case CM_SHORTCUTTO:
		return GetShortcutToColumnText(basicItemInfo);
	case CM_HARDLINKS:
		return GetHardLinksColumnText(basicItemInfo);
	case CM_EXTENSION:
		return GetExtensionColumnText(basicItemInfo);

	case CM_TITLE:
		return GetItemDetailsColumnText(basicItemInfo, &PKEY_Title, globalFolderSettings);
	case CM_SUBJECT:
		return GetItemDetailsColumnText(basicItemInfo, &PKEY_Subject, globalFolderSettings);
	case CM_AUTHORS:
		return GetItemDetailsColumnText(basicItemInfo, &PKEY_Author, globalFolderSettings);
	case CM_KEYWORDS:
		return GetItemDetailsColumnText(basicItemInfo, &PKEY_Keywords, globalFolderSettings);
	case CM_COMMENT:
		return GetItemDetailsColumnText(basicItemInfo, &PKEY_Comment, globalFolderSettings);

	case CM_CAMERAMODEL:
		return GetImageColumnText(basicItemInfo, PropertyTagEquipModel);
	case CM_DATETAKEN:
		return GetImageColumnText(basicItemInfo, PropertyTagDateTime);
	case CM_WIDTH:
		return GetImageColumnText(basicItemInfo, PropertyTagImageWidth);
	case CM_HEIGHT:
		return GetImageColumnText(basicItemInfo, PropertyTagImageHeight);

	case CM_VIRTUALCOMMENTS:
		return GetControlPanelCommentsColumnText(basicItemInfo);

	case CM_TOTALSIZE:
		return GetDriveSpaceColumnText(basicItemInfo, true, globalFolderSettings);

	case CM_FREESPACE:
		return GetDriveSpaceColumnText(basicItemInfo, false, globalFolderSettings);

	case CM_FILESYSTEM:
		return GetFileSystemColumnText(basicItemInfo);

	case CM_ORIGINALLOCATION:
		return GetItemDetailsColumnText(basicItemInfo, &SCID_ORIGINAL_LOCATION, globalFolderSettings);

	case CM_DATEDELETED:
		return GetItemDetailsColumnText(basicItemInfo, &SCID_DATE_DELETED, globalFolderSettings);

	case CM_NUMPRINTERDOCUMENTS:
		return GetPrinterColumnText(basicItemInfo, PrinterInformationType::NumJobs);

	case CM_PRINTERSTATUS:
		return GetPrinterColumnText(basicItemInfo, PrinterInformationType::Status);

	case CM_PRINTERCOMMENTS:
		return GetPrinterColumnText(basicItemInfo, PrinterInformationType::Comments);

	case CM_PRINTERLOCATION:
		return GetPrinterColumnText(basicItemInfo, PrinterInformationType::Location);

	case CM_PRINTERMODEL:
		return GetPrinterColumnText(basicItemInfo, PrinterInformationType::Model);

	case CM_NETWORKADAPTER_STATUS:
		return GetNetworkAdapterColumnText(basicItemInfo);

	case CM_MEDIA_BITRATE:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Bitrate);
	case CM_MEDIA_COPYRIGHT:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Copyright);
	case CM_MEDIA_DURATION:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Duration);
	case CM_MEDIA_PROTECTED:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Protected);
	case CM_MEDIA_RATING:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Rating);
	case CM_MEDIA_ALBUMARTIST:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::AlbumArtist);
	case CM_MEDIA_ALBUM:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::AlbumTitle);
	case CM_MEDIA_BEATSPERMINUTE:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::BeatsPerMinute);
	case CM_MEDIA_COMPOSER:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Composer);
	case CM_MEDIA_CONDUCTOR:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Conductor);
	case CM_MEDIA_DIRECTOR:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Director);
	case CM_MEDIA_GENRE:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Genre);
	case CM_MEDIA_LANGUAGE:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Language);
	case CM_MEDIA_BROADCASTDATE:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::BroadcastDate);
	case CM_MEDIA_CHANNEL:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Channel);
	case CM_MEDIA_STATIONNAME:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::StationName);
	case CM_MEDIA_MOOD:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Mood);
	case CM_MEDIA_PARENTALRATING:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::ParentalRating);
	case CM_MEDIA_PARENTALRATINGREASON:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::ParentalRatingReason);
	case CM_MEDIA_PERIOD:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Period);
	case CM_MEDIA_PRODUCER:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Producer);
	case CM_MEDIA_PUBLISHER:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Publisher);
	case CM_MEDIA_WRITER:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Writer);
	case CM_MEDIA_YEAR:
		return GetMediaMetadataColumnText(basicItemInfo, MediaMetadataType::Year);

	default:
		assert(false);
		break;
	}

	return EMPTY_STRING;
}

std::wstring GetNameColumnText(const BasicItemInfo_t &itemInfo, const GlobalFolderSettings &globalFolderSettings)
{
	return ProcessItemFileName(itemInfo, globalFolderSettings);
}

/* Processes an items filename. Essentially checks
if the extension (if any) needs to be removed, and
removes it if it does. */
std::wstring ProcessItemFileName(const BasicItemInfo_t &itemInfo, const GlobalFolderSettings &globalFolderSettings)
{
	BOOL bHideExtension = FALSE;
	TCHAR *pExt = nullptr;

	if (globalFolderSettings.hideLinkExtension &&
		((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY))
	{
		pExt = PathFindExtension(itemInfo.szDisplayName);

		if (*pExt != '\0')
		{
			if (lstrcmpi(pExt, _T(".lnk")) == 0)
				bHideExtension = TRUE;
		}
	}

	/* We'll hide the extension, provided it is meant
	to be hidden, and the filename does not begin with
	a period, and the item is not a directory. */
	if ((!globalFolderSettings.showExtensions || bHideExtension) &&
		itemInfo.szDisplayName[0] != '.' &&
		(itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
	{
		static TCHAR szDisplayName[MAX_PATH];

		StringCchCopy(szDisplayName, SIZEOF_ARRAY(szDisplayName),
			itemInfo.szDisplayName);

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
	DWORD_PTR Res = SHGetFileInfo(reinterpret_cast<LPTSTR>(itemInfo.pidlComplete.get()),
		0, &shfi, sizeof(shfi), SHGFI_PIDL | SHGFI_TYPENAME);

	if (Res == 0)
	{
		return EMPTY_STRING;
	}

	return shfi.szTypeName;
}

std::wstring GetSizeColumnText(const BasicItemInfo_t &itemInfo, const GlobalFolderSettings &globalFolderSettings)
{
	if ((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		TCHAR drive[MAX_PATH];
		StringCchCopy(drive, SIZEOF_ARRAY(drive), itemInfo.getFullPath().c_str());
		PathStripToRoot(drive);

		bool bNetworkRemovable = false;

		if (GetDriveType(drive) == DRIVE_REMOVABLE ||
			GetDriveType(drive) == DRIVE_REMOTE)
		{
			bNetworkRemovable = true;
		}

		if (globalFolderSettings.showFolderSizes && !(globalFolderSettings.disableFolderSizesNetworkRemovable && bNetworkRemovable))
		{
			return GetFolderSizeColumnText(itemInfo, globalFolderSettings);
		}
		else
		{
			return EMPTY_STRING;
		}
	}

	ULARGE_INTEGER FileSize = { itemInfo.wfd.nFileSizeLow,itemInfo.wfd.nFileSizeHigh };

	TCHAR FileSizeText[64];
	FormatSizeString(FileSize, FileSizeText, SIZEOF_ARRAY(FileSizeText), globalFolderSettings.forceSize, globalFolderSettings.sizeDisplayFormat);

	return FileSizeText;
}

std::wstring GetFolderSizeColumnText(const BasicItemInfo_t &itemInfo, const GlobalFolderSettings &globalFolderSettings)
{
	int numFolders;
	int numFiles;
	ULARGE_INTEGER totalFolderSize;
	CalculateFolderSize(itemInfo.getFullPath().c_str(), &numFolders, &numFiles, &totalFolderSize);

	/* TODO: This should
	be done some other way.
	Shouldn't depend on
	the internal index. */
	//m_cachedFolderSizes.insert({internalIndex, totalFolderSize.QuadPart});

	TCHAR fileSizeText[64];
	FormatSizeString(totalFolderSize, fileSizeText, SIZEOF_ARRAY(fileSizeText),
		globalFolderSettings.forceSize, globalFolderSettings.sizeDisplayFormat);

	return fileSizeText;
}

std::wstring GetTimeColumnText(const BasicItemInfo_t &itemInfo, TimeType timeType, const GlobalFolderSettings &globalFolderSettings)
{
	TCHAR FileTime[64];
	BOOL bRet = FALSE;

	switch (timeType)
	{
	case TimeType::Modified:
		bRet = CreateFileTimeString(&itemInfo.wfd.ftLastWriteTime,
			FileTime, SIZEOF_ARRAY(FileTime), globalFolderSettings.showFriendlyDates);
		break;

	case TimeType::Created:
		bRet = CreateFileTimeString(&itemInfo.wfd.ftCreationTime,
			FileTime, SIZEOF_ARRAY(FileTime), globalFolderSettings.showFriendlyDates);
		break;

	case TimeType::Accessed:
		bRet = CreateFileTimeString(&itemInfo.wfd.ftLastAccessTime,
			FileTime, SIZEOF_ARRAY(FileTime), globalFolderSettings.showFriendlyDates);
		break;

	default:
		assert(false);
		break;
	}

	if (!bRet)
	{
		return EMPTY_STRING;
	}

	return FileTime;
}

std::wstring GetRealSizeColumnText(const BasicItemInfo_t &itemInfo, const GlobalFolderSettings &globalFolderSettings)
{
	ULARGE_INTEGER RealFileSize;
	bool Res = GetRealSizeColumnRawData(itemInfo, RealFileSize);

	if (!Res)
	{
		return EMPTY_STRING;
	}

	TCHAR RealFileSizeText[32];
	FormatSizeString(RealFileSize, RealFileSizeText, SIZEOF_ARRAY(RealFileSizeText),
		globalFolderSettings.forceSize, globalFolderSettings.sizeDisplayFormat);

	return RealFileSizeText;
}

bool GetRealSizeColumnRawData(const BasicItemInfo_t &itemInfo, ULARGE_INTEGER &RealFileSize)
{
	if ((itemInfo.wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		return false;
	}

	TCHAR Root[MAX_PATH];
	StringCchCopy(Root, SIZEOF_ARRAY(Root), itemInfo.getFullPath().c_str());
	PathStripToRoot(Root);

	DWORD dwClusterSize;
	BOOL bRet = GetClusterSize(Root, &dwClusterSize);

	if (!bRet)
	{
		return false;
	}

	ULARGE_INTEGER RealFileSizeTemp = { itemInfo.wfd.nFileSizeLow,itemInfo.wfd.nFileSizeHigh };

	if (RealFileSizeTemp.QuadPart != 0 && (RealFileSizeTemp.QuadPart % dwClusterSize) != 0)
	{
		RealFileSizeTemp.QuadPart += dwClusterSize - (RealFileSizeTemp.QuadPart % dwClusterSize);
	}

	RealFileSize = RealFileSizeTemp;

	return true;
}

std::wstring GetAttributeColumnText(const BasicItemInfo_t &itemInfo)
{
	TCHAR AttributeString[32];
	BuildFileAttributeString(itemInfo.getFullPath().c_str(), AttributeString, SIZEOF_ARRAY(AttributeString));

	return AttributeString;
}

std::wstring GetShortNameColumnText(const BasicItemInfo_t &itemInfo)
{
	if (lstrlen(itemInfo.wfd.cAlternateFileName) == 0)
	{
		return itemInfo.wfd.cFileName;
	}

	return itemInfo.wfd.cAlternateFileName;
}

std::wstring GetOwnerColumnText(const BasicItemInfo_t &itemInfo)
{
	TCHAR Owner[512];
	BOOL ret = GetFileOwner(itemInfo.getFullPath().c_str(), Owner, SIZEOF_ARRAY(Owner));

	if (!ret)
	{
		return EMPTY_STRING;
	}

	return Owner;
}

std::wstring GetItemDetailsColumnText(const BasicItemInfo_t &itemInfo, const SHCOLUMNID *pscid, const GlobalFolderSettings &globalFolderSettings)
{
	TCHAR szDetail[512];
	HRESULT hr = GetItemDetails(itemInfo, pscid, szDetail, SIZEOF_ARRAY(szDetail), globalFolderSettings);

	if (SUCCEEDED(hr))
	{
		return szDetail;
	}

	return EMPTY_STRING;
}

HRESULT GetItemDetails(const BasicItemInfo_t &itemInfo, const SHCOLUMNID *pscid, TCHAR *szDetail, size_t cchMax, const GlobalFolderSettings &globalFolderSettings)
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
	wil::com_ptr<IShellFolder2> pShellFolder;
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

	TCHAR VersionInfo[512];
	BOOL VersionInfoObtained = GetVersionInfoString(itemInfo.getFullPath().c_str(), versionInfoName.c_str(),
		VersionInfo, SIZEOF_ARRAY(VersionInfo));

	if (!VersionInfoObtained)
	{
		return EMPTY_STRING;
	}

	return VersionInfo;
}

std::wstring GetShortcutToColumnText(const BasicItemInfo_t &itemInfo)
{
	TCHAR ResolvedLinkPath[MAX_PATH];
	HRESULT hr = NFileOperations::ResolveLink(nullptr, SLR_NO_UI, itemInfo.getFullPath().c_str(),
		ResolvedLinkPath, SIZEOF_ARRAY(ResolvedLinkPath));

	if (FAILED(hr))
	{
		return EMPTY_STRING;
	}

	return ResolvedLinkPath;
}

std::wstring GetHardLinksColumnText(const BasicItemInfo_t &itemInfo)
{
	DWORD NumHardLinks = GetHardLinksColumnRawData(itemInfo);

	if (NumHardLinks == -1)
	{
		return EMPTY_STRING;
	}

	TCHAR NumHardLinksString[32];
	StringCchPrintf(NumHardLinksString, SIZEOF_ARRAY(NumHardLinksString), _T("%ld"), NumHardLinks);

	return NumHardLinksString;
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

	TCHAR *Extension = PathFindExtension(itemInfo.wfd.cFileName);

	if (*Extension != '.')
	{
		return EMPTY_STRING;
	}

	return Extension + 1;
}

std::wstring GetImageColumnText(const BasicItemInfo_t &itemInfo, PROPID PropertyID)
{
	TCHAR ImageProperty[512];
	BOOL Res = ReadImageProperty(itemInfo.getFullPath().c_str(), PropertyID, ImageProperty,
		SIZEOF_ARRAY(ImageProperty));

	if (!Res)
	{
		return EMPTY_STRING;
	}

	return ImageProperty;
}

std::wstring GetFileSystemColumnText(const BasicItemInfo_t &itemInfo)
{
	TCHAR FullFileName[MAX_PATH];
	GetDisplayName(itemInfo.pidlComplete.get(), FullFileName, SIZEOF_ARRAY(FullFileName), SHGDN_FORPARSING);

	BOOL IsRoot = PathIsRoot(FullFileName);

	if (!IsRoot)
	{
		return EMPTY_STRING;
	}

	TCHAR FileSystemName[MAX_PATH];
	BOOL Res = GetVolumeInformation(FullFileName, nullptr, 0, nullptr, nullptr, nullptr, FileSystemName,
		SIZEOF_ARRAY(FileSystemName));

	if (!Res)
	{
		return EMPTY_STRING;
	}

	return FileSystemName;
}

std::wstring GetControlPanelCommentsColumnText(const BasicItemInfo_t &itemInfo)
{
	TCHAR InfoTip[512];
	HRESULT hr = GetItemInfoTip(itemInfo.getFullPath().c_str(), InfoTip, SIZEOF_ARRAY(InfoTip));

	if (FAILED(hr))
	{
		return EMPTY_STRING;
	}

	ReplaceCharacter(InfoTip, '\n', ' ');

	return InfoTip;
}

std::wstring GetPrinterColumnText(const BasicItemInfo_t &itemInfo, PrinterInformationType printerInformationType)
{
	TCHAR PrinterInformation[256] = EMPTY_STRING;
	TCHAR szStatus[256];

	TCHAR itemDisplayName[MAX_PATH];
	StringCchCopy(itemDisplayName, SIZEOF_ARRAY(itemDisplayName), itemInfo.szDisplayName);

	HANDLE hPrinter;
	BOOL Res = OpenPrinter(itemDisplayName, &hPrinter, nullptr);

	if (Res)
	{
		DWORD BytesNeeded;
		GetPrinter(hPrinter, 2, nullptr, 0, &BytesNeeded);

		PRINTER_INFO_2 *PrinterInfo2 = reinterpret_cast<PRINTER_INFO_2 *>(new char[BytesNeeded]);
		Res = GetPrinter(hPrinter, 2, reinterpret_cast<LPBYTE>(PrinterInfo2), BytesNeeded, &BytesNeeded);

		if (Res)
		{
			switch (printerInformationType)
			{
			case PrinterInformationType::NumJobs:
				StringCchPrintf(PrinterInformation, SIZEOF_ARRAY(PrinterInformation),
					_T("%d"), PrinterInfo2->cJobs);
				break;

			case PrinterInformationType::Status:
				Res = GetPrinterStatusDescription(PrinterInfo2->Status, szStatus, SIZEOF_ARRAY(szStatus));

				if (Res)
				{
					StringCchCopyEx(PrinterInformation, SIZEOF_ARRAY(PrinterInformation),
						szStatus, nullptr, nullptr, STRSAFE_IGNORE_NULLS);
				}
				break;

			case PrinterInformationType::Comments:
				StringCchCopyEx(PrinterInformation, SIZEOF_ARRAY(PrinterInformation),
					PrinterInfo2->pComment, nullptr, nullptr, STRSAFE_IGNORE_NULLS);
				break;

			case PrinterInformationType::Location:
				StringCchCopyEx(PrinterInformation, SIZEOF_ARRAY(PrinterInformation),
					PrinterInfo2->pLocation, nullptr, nullptr, STRSAFE_IGNORE_NULLS);
				break;

			case PrinterInformationType::Model:
				StringCchCopyEx(PrinterInformation, SIZEOF_ARRAY(PrinterInformation),
					PrinterInfo2->pDriverName, nullptr, nullptr, STRSAFE_IGNORE_NULLS);
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
	ULONG OutBufLen = 0;
	GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, nullptr, &OutBufLen);
	IP_ADAPTER_ADDRESSES *AdapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(new char[OutBufLen]);
	GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, AdapterAddresses, &OutBufLen);

	IP_ADAPTER_ADDRESSES *AdapaterAddress = AdapterAddresses;

	while (AdapaterAddress != nullptr &&
		lstrcmp(AdapaterAddress->FriendlyName, itemInfo.wfd.cFileName) != 0)
	{
		AdapaterAddress = AdapaterAddress->Next;
	}

	std::wstring Status;

	/* TODO: These strings need to be setup correctly. */
	switch (AdapaterAddress->OperStatus)
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

std::wstring GetMediaMetadataColumnText(const BasicItemInfo_t &itemInfo, MediaMetadataType mediaMetadataType)
{
	const TCHAR *AttributeName = GetMediaMetadataAttributeName(mediaMetadataType);

	BYTE *TempBuffer = nullptr;
	HRESULT hr = GetMediaMetadata(itemInfo.getFullPath().c_str(), AttributeName, &TempBuffer);

	if (!SUCCEEDED(hr))
	{
		return EMPTY_STRING;
	}

	TCHAR szOutput[512];

	switch (mediaMetadataType)
	{
	case MediaMetadataType::Bitrate:
	{
		DWORD BitRate = *(reinterpret_cast<DWORD *>(TempBuffer));

		if (BitRate > 1000)
		{
			StringCchPrintf(szOutput, SIZEOF_ARRAY(szOutput), _T("%d kbps"), BitRate / 1000);
		}
		else
		{
			StringCchPrintf(szOutput, SIZEOF_ARRAY(szOutput), _T("%d bps"), BitRate);
		}
	}
	break;

	case MediaMetadataType::Duration:
	{
		boost::posix_time::wtime_facet *Facet = new boost::posix_time::wtime_facet();
		Facet->time_duration_format(L"%H:%M:%S");

		std::wstringstream DateStream;
		DateStream.imbue(std::locale(DateStream.getloc(), Facet));

		/* Note that the duration itself is in 100-nanosecond units
		(see http://msdn.microsoft.com/en-us/library/windows/desktop/dd798053(v=vs.85).aspx). */
		boost::posix_time::time_duration Duration = boost::posix_time::microseconds(*(reinterpret_cast<QWORD *>(TempBuffer)) / 10);
		DateStream << Duration;

		StringCchCopy(szOutput, SIZEOF_ARRAY(szOutput), DateStream.str().c_str());
	}
	break;

	case MediaMetadataType::Protected:
		if (*(reinterpret_cast<BOOL *>(TempBuffer)))
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
		StringCchCopy(szOutput, SIZEOF_ARRAY(szOutput), reinterpret_cast<TCHAR *>(TempBuffer));
		break;
	}

	free(TempBuffer);

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

std::wstring GetDriveSpaceColumnText(const BasicItemInfo_t &itemInfo, bool TotalSize, const GlobalFolderSettings &globalFolderSettings)
{
	ULARGE_INTEGER DriveSpace;
	BOOL Res = GetDriveSpaceColumnRawData(itemInfo, TotalSize, DriveSpace);

	if (!Res)
	{
		return EMPTY_STRING;
	}

	TCHAR SizeText[32];
	FormatSizeString(DriveSpace, SizeText, SIZEOF_ARRAY(SizeText),
		globalFolderSettings.forceSize, globalFolderSettings.sizeDisplayFormat);

	return SizeText;
}

BOOL GetDriveSpaceColumnRawData(const BasicItemInfo_t &itemInfo, bool TotalSize, ULARGE_INTEGER &DriveSpace)
{
	TCHAR FullFileName[MAX_PATH];
	GetDisplayName(itemInfo.pidlComplete.get(), FullFileName,
		SIZEOF_ARRAY(FullFileName), SHGDN_FORPARSING);

	BOOL IsRoot = PathIsRoot(FullFileName);

	if (!IsRoot)
	{
		return FALSE;
	}

	ULARGE_INTEGER TotalBytes;
	ULARGE_INTEGER FreeBytes;
	BOOL Res = GetDiskFreeSpaceEx(FullFileName, nullptr, &TotalBytes, &FreeBytes);

	if (TotalSize)
	{
		DriveSpace = TotalBytes;
	}
	else
	{
		DriveSpace = FreeBytes;
	}

	return Res;
}