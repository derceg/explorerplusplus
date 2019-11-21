// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser.h"
#include "Config.h"
#include "SortHelper.h"
#include "SortModes.h"
#include "ViewModes.h"
#include <propkey.h>
#include <cassert>

void CShellBrowser::SortFolder(SortMode sortMode)
{
	m_folderSettings.sortMode = sortMode;

	if(m_folderSettings.showInGroups)
	{
		ListView_EnableGroupView(m_hListView,FALSE);
		ListView_RemoveAllGroups(m_hListView);
		ListView_EnableGroupView(m_hListView,TRUE);

		SetShowInGroups(TRUE);
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

int CALLBACK CShellBrowser::SortStub(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
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
		case SortMode::Name:
			ComparisonResult = SortByName(basicItemInfo1, basicItemInfo2, m_config->globalFolderSettings);
			break;

		case SortMode::Type:
			ComparisonResult = SortByType(basicItemInfo1,basicItemInfo2);
			break;

		case SortMode::Size:
			ComparisonResult = SortBySize(basicItemInfo1,basicItemInfo2);
			break;

		case SortMode::DateModified:
			ComparisonResult = SortByDate(basicItemInfo1,basicItemInfo2,DATE_TYPE_MODIFIED);
			break;

		case SortMode::TotalSize:
			ComparisonResult = SortByTotalSize(basicItemInfo1,basicItemInfo2,TRUE);
			break;

		case SortMode::FreeSpace:
			ComparisonResult = SortByTotalSize(basicItemInfo1,basicItemInfo2,FALSE);
			break;

		case SortMode::DateDeleted:
			ComparisonResult = SortByItemDetails(basicItemInfo1, basicItemInfo2, &SCID_DATE_DELETED);
			break;

		case SortMode::OriginalLocation:
			ComparisonResult = SortByItemDetails(basicItemInfo1, basicItemInfo2, &SCID_ORIGINAL_LOCATION);
			break;

		case SortMode::Attributes:
			ComparisonResult = SortByAttributes(basicItemInfo1, basicItemInfo2);
			break;

		case SortMode::RealSize:
			ComparisonResult = SortByRealSize(basicItemInfo1, basicItemInfo2);
			break;

		case SortMode::ShortName:
			ComparisonResult = SortByShortName(basicItemInfo1, basicItemInfo2);
			break;

		case SortMode::Owner:
			ComparisonResult = SortByOwner(basicItemInfo1, basicItemInfo2);
			break;

		case SortMode::ProductName:
			ComparisonResult = SortByVersionInfo(basicItemInfo1, basicItemInfo2,VERSION_INFO_PRODUCT_NAME);
			break;

		case SortMode::Company:
			ComparisonResult = SortByVersionInfo(basicItemInfo1, basicItemInfo2,VERSION_INFO_COMPANY);
			break;

		case SortMode::Description:
			ComparisonResult = SortByVersionInfo(basicItemInfo1, basicItemInfo2,VERSION_INFO_DESCRIPTION);
			break;

		case SortMode::FileVersion:
			ComparisonResult = SortByVersionInfo(basicItemInfo1, basicItemInfo2,VERSION_INFO_FILE_VERSION);
			break;

		case SortMode::ProductVersion:
			ComparisonResult = SortByVersionInfo(basicItemInfo1, basicItemInfo2,VERSION_INFO_PRODUCT_VERSION);
			break;

		case SortMode::ShortcutTo:
			ComparisonResult = SortByShortcutTo(basicItemInfo1, basicItemInfo2);
			break;

		case SortMode::HardLinks:
			ComparisonResult = SortByHardlinks(basicItemInfo1, basicItemInfo2);
			break;

		case SortMode::Extension:
			ComparisonResult = SortByExtension(basicItemInfo1,basicItemInfo2);
			break;

		case SortMode::Created:
			ComparisonResult = SortByDate(basicItemInfo1,basicItemInfo2,DATE_TYPE_CREATED);
			break;

		case SortMode::Accessed:
			ComparisonResult = SortByDate(basicItemInfo1,basicItemInfo2,DATE_TYPE_ACCESSED);
			break;

		case SortMode::Title:
			ComparisonResult = SortByItemDetails(basicItemInfo1, basicItemInfo2,&PKEY_Title);
			break;

		case SortMode::Subject:
			ComparisonResult = SortByItemDetails(basicItemInfo1, basicItemInfo2,&PKEY_Subject);
			break;

		case SortMode::Authors:
			ComparisonResult = SortByItemDetails(basicItemInfo1, basicItemInfo2,&PKEY_Author);
			break;

		case SortMode::Keywords:
			ComparisonResult = SortByItemDetails(basicItemInfo1, basicItemInfo2,&PKEY_Keywords);
			break;

		case SortMode::Comments:
			ComparisonResult = SortByItemDetails(basicItemInfo1, basicItemInfo2,&PKEY_Comment);
			break;

		case SortMode::CameraModel:
			ComparisonResult = SortByImageProperty(basicItemInfo1, basicItemInfo2,PropertyTagEquipModel);
			break;

		case SortMode::DateTaken:
			ComparisonResult = SortByImageProperty(basicItemInfo1, basicItemInfo2,PropertyTagDateTime);
			break;

		case SortMode::Width:
			ComparisonResult = SortByImageProperty(basicItemInfo1, basicItemInfo2,PropertyTagImageWidth);
			break;

		case SortMode::Height:
			ComparisonResult = SortByImageProperty(basicItemInfo1, basicItemInfo2,PropertyTagImageHeight);
			break;

		case SortMode::VirtualComments:
			ComparisonResult = SortByVirtualComments(basicItemInfo1, basicItemInfo2);
			break;

		case SortMode::FileSystem:
			ComparisonResult = SortByFileSystem(basicItemInfo1,basicItemInfo2);
			break;

		case SortMode::NumPrinterDocuments:
			ComparisonResult = SortByPrinterProperty(basicItemInfo1,basicItemInfo2,PRINTER_INFORMATION_TYPE_NUM_JOBS);
			break;

		case SortMode::PrinterStatus:
			ComparisonResult = SortByPrinterProperty(basicItemInfo1,basicItemInfo2,PRINTER_INFORMATION_TYPE_STATUS);
			break;

		case SortMode::PrinterComments:
			ComparisonResult = SortByPrinterProperty(basicItemInfo1,basicItemInfo2,PRINTER_INFORMATION_TYPE_COMMENTS);
			break;

		case SortMode::PrinterLocation:
			ComparisonResult = SortByPrinterProperty(basicItemInfo1,basicItemInfo2,PRINTER_INFORMATION_TYPE_LOCATION);
			break;

		case SortMode::NetworkAdapterStatus:
			ComparisonResult = SortByNetworkAdapterStatus(basicItemInfo1,basicItemInfo2);
			break;

		case SortMode::MediaBitrate:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_BITRATE);
			break;

		case SortMode::MediaCopyright:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_COPYRIGHT);
			break;

		case SortMode::MediaDuration:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_DURATION);
			break;

		case SortMode::MediaProtected:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_PROTECTED);
			break;

		case SortMode::MediaRating:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_RATING);
			break;

		case SortMode::MediaAlbumArtist:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_ALBUM_ARTIST);
			break;

		case SortMode::MediaAlbum:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_ALBUM_TITLE);
			break;

		case SortMode::MediaBeatsPerMinute:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_BEATS_PER_MINUTE);
			break;

		case SortMode::MediaComposer:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_COMPOSER);
			break;

		case SortMode::MediaConductor:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_CONDUCTOR);
			break;

		case SortMode::MediaDirector:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_DIRECTOR);
			break;

		case SortMode::MediaGenre:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_GENRE);
			break;

		case SortMode::MediaLanguage:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_LANGUAGE);
			break;

		case SortMode::MediaBroadcastDate:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_BROADCASTDATE);
			break;

		case SortMode::MediaChannel:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_CHANNEL);
			break;

		case SortMode::MediaStationName:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_STATIONNAME);
			break;

		case SortMode::MediaMood:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_MOOD);
			break;

		case SortMode::MediaParentalRating:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_PARENTALRATING);
			break;

		case SortMode::MediaParentalRatingReason:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_PARENTALRATINGREASON);
			break;

		case SortMode::MediaPeriod:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_PERIOD);
			break;

		case SortMode::MediaProducer:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_PRODUCER);
			break;

		case SortMode::MediaPublisher:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_PUBLISHER);
			break;

		case SortMode::MediaWriter:
			ComparisonResult = SortByMediaMetadata(basicItemInfo1, basicItemInfo2,MEDIAMETADATA_TYPE_WRITER);
			break;

		case SortMode::MediaYear:
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
		ComparisonResult = StrCmpLogicalW(basicItemInfo1.szDisplayName,
			basicItemInfo2.szDisplayName);
	}

	if(!m_folderSettings.sortAscending)
	{
		ComparisonResult = -ComparisonResult;
	}

	return ComparisonResult;
}