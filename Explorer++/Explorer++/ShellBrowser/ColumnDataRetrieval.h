#pragma once

#include "ItemData.h"
#include "TabPreferences.h"
#include <string>

enum TimeType_t
{
	COLUMN_TIME_MODIFIED,
	COLUMN_TIME_CREATED,
	COLUMN_TIME_ACCESSED
};

enum VersionInfoType_t
{
	VERSION_INFO_PRODUCT_NAME,
	VERSION_INFO_COMPANY,
	VERSION_INFO_DESCRIPTION,
	VERSION_INFO_FILE_VERSION,
	VERSION_INFO_PRODUCT_VERSION
};

enum PrinterInformationType_t
{
	PRINTER_INFORMATION_TYPE_NUM_JOBS,
	PRINTER_INFORMATION_TYPE_STATUS,
	PRINTER_INFORMATION_TYPE_COMMENTS,
	PRINTER_INFORMATION_TYPE_LOCATION,
	PRINTER_INFORMATION_TYPE_MODEL
};

enum MediaMetadataType_t
{
	MEDIAMETADATA_TYPE_BITRATE,
	MEDIAMETADATA_TYPE_COPYRIGHT,
	MEDIAMETADATA_TYPE_DURATION,
	MEDIAMETADATA_TYPE_PROTECTED,
	MEDIAMETADATA_TYPE_RATING,
	MEDIAMETADATA_TYPE_ALBUM_ARTIST,
	MEDIAMETADATA_TYPE_ALBUM_TITLE,
	MEDIAMETADATA_TYPE_BEATS_PER_MINUTE,
	MEDIAMETADATA_TYPE_COMPOSER,
	MEDIAMETADATA_TYPE_CONDUCTOR,
	MEDIAMETADATA_TYPE_DIRECTOR,
	MEDIAMETADATA_TYPE_GENRE,
	MEDIAMETADATA_TYPE_LANGUAGE,
	MEDIAMETADATA_TYPE_BROADCASTDATE,
	MEDIAMETADATA_TYPE_CHANNEL,
	MEDIAMETADATA_TYPE_STATIONNAME,
	MEDIAMETADATA_TYPE_MOOD,
	MEDIAMETADATA_TYPE_PARENTALRATING,
	MEDIAMETADATA_TYPE_PARENTALRATINGREASON,
	MEDIAMETADATA_TYPE_PERIOD,
	MEDIAMETADATA_TYPE_PRODUCER,
	MEDIAMETADATA_TYPE_PUBLISHER,
	MEDIAMETADATA_TYPE_WRITER,
	MEDIAMETADATA_TYPE_YEAR
};

std::wstring GetNameColumnText(const ItemInfo_t &itemInfo, const Preferences_t &preferences);
std::wstring ProcessItemFileName(const ItemInfo_t &itemInfo, const Preferences_t &preferences);
std::wstring GetTypeColumnText(const ItemInfo_t &itemInfo);
std::wstring GetTimeColumnText(const ItemInfo_t &itemInfo, TimeType_t TimeType, const Preferences_t &preferences);
std::wstring GetRealSizeColumnText(const ItemInfo_t &itemInfo, const Preferences_t &preferences);
bool GetRealSizeColumnRawData(const ItemInfo_t &itemInfo, ULARGE_INTEGER &RealFileSize);
std::wstring GetAttributeColumnText(const ItemInfo_t &itemInfo);
std::wstring GetShortNameColumnText(const ItemInfo_t &itemInfo);
std::wstring GetOwnerColumnText(const ItemInfo_t &itemInfo);
std::wstring GetItemDetailsColumnText(const ItemInfo_t &itemInfo, const SHCOLUMNID *pscid, const Preferences_t &preferences);
HRESULT GetItemDetails(const ItemInfo_t &itemInfo, const SHCOLUMNID *pscid, TCHAR *szDetail, size_t cchMax, const Preferences_t &preferences);
HRESULT GetItemDetailsRawData(const ItemInfo_t &itemInfo, const SHCOLUMNID *pscid, VARIANT *vt);
std::wstring GetVersionColumnText(const ItemInfo_t &itemInfo, VersionInfoType_t VersioninfoType);
std::wstring GetShortcutToColumnText(const ItemInfo_t &itemInfo);
std::wstring GetHardLinksColumnText(const ItemInfo_t &itemInfo);
DWORD GetHardLinksColumnRawData(const ItemInfo_t &itemInfo);
std::wstring GetExtensionColumnText(const ItemInfo_t &itemInfo);
std::wstring GetImageColumnText(const ItemInfo_t &itemInfo, PROPID PropertyID);
std::wstring GetFileSystemColumnText(const ItemInfo_t &itemInfo);
std::wstring GetControlPanelCommentsColumnText(const ItemInfo_t &itemInfo);
std::wstring GetPrinterColumnText(const ItemInfo_t &itemInfo, PrinterInformationType_t PrinterInformationType);
std::wstring GetNetworkAdapterColumnText(const ItemInfo_t &itemInfo);
std::wstring GetMediaMetadataColumnText(const ItemInfo_t &itemInfo, MediaMetadataType_t MediaMetaDataType);
const TCHAR *GetMediaMetadataAttributeName(MediaMetadataType_t MediaMetaDataType);
std::wstring GetDriveSpaceColumnText(const ItemInfo_t &itemInfo, bool TotalSize, const Preferences_t &preferences);
BOOL GetDriveSpaceColumnRawData(const ItemInfo_t &itemInfo, bool TotalSize, ULARGE_INTEGER &DriveSpace);
std::wstring GetSizeColumnText(const ItemInfo_t &itemInfo, const Preferences_t &preferences);
std::wstring GetFolderSizeColumnText(const ItemInfo_t &itemInfo, const Preferences_t &preferences);