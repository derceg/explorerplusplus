#pragma once

#include "ItemData.h"
#include <string>

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

std::wstring GetTypeColumnText(const ItemInfo_t &itemInfo);
std::wstring GetAttributeColumnText(const ItemInfo_t &itemInfo);
std::wstring GetShortNameColumnText(const ItemInfo_t &itemInfo);
std::wstring GetOwnerColumnText(const ItemInfo_t &itemInfo);
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