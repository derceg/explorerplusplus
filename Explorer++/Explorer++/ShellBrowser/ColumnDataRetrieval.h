// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Columns.h"
#include <optional>
#include <string>

struct BasicItemInfo_t;
struct GlobalFolderSettings;

enum class TimeType
{
	Modified,
	Created,
	Accessed
};

enum class VersionInfoType
{
	ProductName,
	Company,
	Description,
	FileVersion,
	ProductVersion
};

enum class PrinterInformationType
{
	NumJobs,
	Status,
	Comments,
	Location,
	Model
};

enum class MediaMetadataType
{
	Bitrate,
	Copyright,
	Duration,
	Protected,
	Rating,
	AlbumArtist,
	AlbumTitle,
	BeatsPerMinute,
	Composer,
	Conductor,
	Director,
	Genre,
	Language,
	BroadcastDate,
	Channel,
	StationName,
	Mood,
	ParentalRating,
	ParentalRatingReason,
	Period,
	Producer,
	Publisher,
	Writer,
	Year
};

std::wstring GetColumnText(ColumnType columnType, const BasicItemInfo_t &basicItemInfo,
	const GlobalFolderSettings &globalFolderSettings);
std::wstring GetNameColumnText(const BasicItemInfo_t &itemInfo,
	const GlobalFolderSettings &globalFolderSettings);
std::wstring ProcessItemFileName(const BasicItemInfo_t &itemInfo,
	const GlobalFolderSettings &globalFolderSettings);
std::wstring GetTypeColumnText(const BasicItemInfo_t &itemInfo);
std::wstring GetTimeColumnText(const BasicItemInfo_t &itemInfo, TimeType timeType,
	const GlobalFolderSettings &globalFolderSettings);
std::wstring GetRealSizeColumnText(const BasicItemInfo_t &itemInfo,
	const GlobalFolderSettings &globalFolderSettings);
bool GetRealSizeColumnRawData(const BasicItemInfo_t &itemInfo, ULARGE_INTEGER &RealFileSize);
std::wstring GetAttributeColumnText(const BasicItemInfo_t &itemInfo);
std::wstring GetShortNameColumnText(const BasicItemInfo_t &itemInfo);
std::wstring GetOwnerColumnText(const BasicItemInfo_t &itemInfo);
std::wstring GetItemDetailsColumnText(const BasicItemInfo_t &itemInfo, const SHCOLUMNID *pscid,
	const GlobalFolderSettings &globalFolderSettings);
HRESULT GetItemDetails(const BasicItemInfo_t &itemInfo, const SHCOLUMNID *pscid, TCHAR *szDetail,
	size_t cchMax, const GlobalFolderSettings &globalFolderSettings);
HRESULT GetItemDetailsRawData(const BasicItemInfo_t &itemInfo, const SHCOLUMNID *pscid,
	VARIANT *vt);
std::wstring GetVersionColumnText(const BasicItemInfo_t &itemInfo, VersionInfoType versioninfoType);
std::wstring GetShortcutToColumnText(const BasicItemInfo_t &itemInfo);
std::wstring GetHardLinksColumnText(const BasicItemInfo_t &itemInfo);
std::optional<DWORD> GetHardLinksColumnRawData(const BasicItemInfo_t &itemInfo);
std::wstring GetExtensionColumnText(const BasicItemInfo_t &itemInfo);
std::wstring GetImageColumnText(const BasicItemInfo_t &itemInfo, PROPID PropertyID);
std::wstring GetFileSystemColumnText(const BasicItemInfo_t &itemInfo);
std::wstring GetControlPanelCommentsColumnText(const BasicItemInfo_t &itemInfo);
std::wstring GetPrinterColumnText(const BasicItemInfo_t &itemInfo,
	PrinterInformationType printerInformationType);
std::wstring GetNetworkAdapterColumnText(const BasicItemInfo_t &itemInfo);
std::wstring GetMediaMetadataColumnText(const BasicItemInfo_t &itemInfo,
	MediaMetadataType mediaMetadataType);
const TCHAR *GetMediaMetadataAttributeName(MediaMetadataType mediaMetadataType);
std::wstring GetDriveSpaceColumnText(const BasicItemInfo_t &itemInfo, bool TotalSize,
	const GlobalFolderSettings &globalFolderSettings);
BOOL GetDriveSpaceColumnRawData(const BasicItemInfo_t &itemInfo, bool TotalSize,
	ULARGE_INTEGER &DriveSpace);
std::wstring GetSizeColumnText(const BasicItemInfo_t &itemInfo,
	const GlobalFolderSettings &globalFolderSettings);
std::wstring GetFolderSizeColumnText(const BasicItemInfo_t &itemInfo,
	const GlobalFolderSettings &globalFolderSettings);
