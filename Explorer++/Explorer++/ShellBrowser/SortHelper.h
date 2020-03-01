// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ColumnDataRetrieval.h"
#include "FolderSettings.h"
#include "ItemData.h"

enum class DateType
{
	Created,
	Modified,
	Accessed
};

int SortByName(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, const GlobalFolderSettings &globalFolderSettings);
int SortBySize(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2);
int SortByType(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2);
int SortByDate(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, DateType dateType);
int SortByTotalSize(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, bool TotalSize);
int SortByAttributes(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2);
int SortByRealSize(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2);
int SortByShortName(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2);
int SortByOwner(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2);
int SortByVersionInfo(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, VersionInfoType versionInfoType);
int SortByShortcutTo(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2);
int SortByHardlinks(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2);
int SortByExtension(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2);
int SortByItemDetails(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, const SHCOLUMNID *pscid);
int SortByImageProperty(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, PROPID PropertyId);
int SortByVirtualComments(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2);
int SortByFileSystem(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2);
int SortByPrinterProperty(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, PrinterInformationType printerInformationType);
int SortByNetworkAdapterStatus(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2);
int SortByMediaMetadata(const BasicItemInfo_t &itemInfo1, const BasicItemInfo_t &itemInfo2, MediaMetadataType mediaMetadataType);