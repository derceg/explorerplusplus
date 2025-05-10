// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SortMenuBuilder.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "Tab.h"
#include "../Helper/MenuHelper.h"

const int SORT_MENU_RESOURCE_BLOCK_SIZE = 1000;

SortMenuBuilder::SortMenuBuilder(const ResourceLoader *resourceLoader) :
	m_resourceLoader(resourceLoader)
{
}

SortMenuBuilder::SortMenus SortMenuBuilder::BuildMenus(const Tab &tab)
{
	auto sortByMenu = CreateDefaultMenu(IDM_SORT_ASCENDING, IDM_SORT_DESCENDING);
	auto groupByMenu = CreateDefaultMenu(IDM_GROUP_SORT_ASCENDING, IDM_GROUP_SORT_DESCENDING);

	auto sortModes = tab.GetShellBrowserImpl()->GetAvailableSortModes();
	int position = 0;

	for (SortMode sortMode : sortModes)
	{
		int sortById = DetermineSortModeMenuId(sortMode);
		int groupById = DetermineGroupModeMenuId(sortMode);

		UINT stringIndex = GetSortMenuItemStringIndex(sortById);
		std::wstring menuText = m_resourceLoader->LoadString(stringIndex);

		MenuHelper::AddStringItem(sortByMenu.get(), sortById, menuText, position, TRUE);
		MenuHelper::AddStringItem(groupByMenu.get(), groupById, menuText, position, TRUE);

		position++;
	}

	if (tab.GetShellBrowserImpl()->GetShowInGroups())
	{
		std::wstring groupByNoneText = m_resourceLoader->LoadString(IDS_GROUP_BY_NONE);
		MenuHelper::AddStringItem(groupByMenu.get(), IDM_GROUP_BY_NONE, groupByNoneText, position++,
			true);
	}

	SetMenuItemStates(sortByMenu.get(), groupByMenu.get(), tab);

	return { std::move(sortByMenu), std::move(groupByMenu) };
}

wil::unique_hmenu SortMenuBuilder::CreateDefaultMenu(UINT ascendingMenuItemId,
	UINT descendingMenuItemId)
{
	wil::unique_hmenu menu(CreatePopupMenu());

	MenuHelper::AddSeparator(menu.get());

	std::wstring sortAscending = m_resourceLoader->LoadString(IDS_MENU_SORT_ASCENDING);
	MenuHelper::AddStringItem(menu.get(), ascendingMenuItemId, sortAscending);

	std::wstring sortDescending = m_resourceLoader->LoadString(IDS_MENU_SORT_DESCENDING);
	MenuHelper::AddStringItem(menu.get(), descendingMenuItemId, sortDescending);

	MenuHelper::AddSeparator(menu.get());

	std::wstring sortByMore = m_resourceLoader->LoadString(IDS_MENU_SORT_MORE);
	MenuHelper::AddStringItem(menu.get(), IDM_SORTBY_MORE, sortByMore);

	return menu;
}

void SortMenuBuilder::SetMenuItemStates(HMENU sortByMenu, HMENU groupByMenu, const Tab &tab)
{
	SortMode sortMode = tab.GetShellBrowserImpl()->GetSortMode();
	CheckMenuRadioItem(sortByMenu, IDM_SORTBY_NAME,
		IDM_SORTBY_NAME + (SORT_MENU_RESOURCE_BLOCK_SIZE - 1), DetermineSortModeMenuId(sortMode),
		MF_BYCOMMAND);

	CheckMenuRadioItem(sortByMenu, IDM_SORT_ASCENDING, IDM_SORT_DESCENDING,
		tab.GetShellBrowserImpl()->GetSortDirection() == +SortDirection::Ascending
			? IDM_SORT_ASCENDING
			: IDM_SORT_DESCENDING,
		MF_BYCOMMAND);

	BOOL showInGroups = tab.GetShellBrowserImpl()->GetShowInGroups();

	if (showInGroups)
	{
		SortMode groupMode = tab.GetShellBrowserImpl()->GetGroupMode();
		CheckMenuRadioItem(groupByMenu, IDM_GROUPBY_NAME,
			IDM_GROUPBY_NAME + (SORT_MENU_RESOURCE_BLOCK_SIZE - 1),
			DetermineGroupModeMenuId(groupMode), MF_BYCOMMAND);

		CheckMenuRadioItem(groupByMenu, IDM_GROUP_SORT_ASCENDING, IDM_GROUP_SORT_DESCENDING,
			tab.GetShellBrowserImpl()->GetGroupSortDirection() == +SortDirection::Ascending
				? IDM_GROUP_SORT_ASCENDING
				: IDM_GROUP_SORT_DESCENDING,
			MF_BYCOMMAND);
	}
	else
	{
		MenuHelper::EnableItem(groupByMenu, IDM_GROUP_SORT_ASCENDING, FALSE);
		MenuHelper::EnableItem(groupByMenu, IDM_GROUP_SORT_DESCENDING, FALSE);
	}
}

UINT SortMenuBuilder::GetSortMenuItemStringIndex(UINT uItemId)
{
	switch (uItemId)
	{
	case IDM_SORTBY_NAME:
		return IDS_SORTBY_NAME;

	case IDM_SORTBY_SIZE:
		return IDS_SORTBY_SIZE;

	case IDM_SORTBY_TYPE:
		return IDS_SORTBY_TYPE;

	case IDM_SORTBY_DATEMODIFIED:
		return IDS_SORTBY_DATEMODIFIED;

	case IDM_SORTBY_TOTALSIZE:
		return IDS_SORTBY_TOTALSIZE;

	case IDM_SORTBY_FREESPACE:
		return IDS_SORTBY_FREESPACE;

	case IDM_SORTBY_DATEDELETED:
		return IDS_SORTBY_DATEDELETED;

	case IDM_SORTBY_ORIGINALLOCATION:
		return IDS_SORTBY_ORIGINALLOCATION;

	case IDM_SORTBY_ATTRIBUTES:
		return IDS_SORTBY_ATTRIBUTES;

	case IDM_SORTBY_REALSIZE:
		return IDS_SORTBY_REALSIZE;

	case IDM_SORTBY_SHORTNAME:
		return IDS_SORTBY_SHORTNAME;

	case IDM_SORTBY_OWNER:
		return IDS_SORTBY_OWNER;

	case IDM_SORTBY_PRODUCTNAME:
		return IDS_SORTBY_PRODUCTNAME;

	case IDM_SORTBY_COMPANY:
		return IDS_SORTBY_COMPANY;

	case IDM_SORTBY_DESCRIPTION:
		return IDS_SORTBY_DESCRIPTION;

	case IDM_SORTBY_FILEVERSION:
		return IDS_SORTBY_FILEVERSION;

	case IDM_SORTBY_PRODUCTVERSION:
		return IDS_SORTBY_PRODUCTVERSION;

	case IDM_SORTBY_SHORTCUTTO:
		return IDS_SORTBY_SHORTCUTTO;

	case IDM_SORTBY_HARDLINKS:
		return IDS_SORTBY_HARDLINKS;

	case IDM_SORTBY_EXTENSION:
		return IDS_SORTBY_EXTENSION;

	case IDM_SORTBY_CREATED:
		return IDS_SORTBY_CREATED;

	case IDM_SORTBY_ACCESSED:
		return IDS_SORTBY_ACCESSED;

	case IDM_SORTBY_TITLE:
		return IDS_SORTBY_TITLE;

	case IDM_SORTBY_SUBJECT:
		return IDS_SORTBY_SUBJECT;

	case IDM_SORTBY_AUTHOR:
		return IDS_SORTBY_AUTHOR;

	case IDM_SORTBY_KEYWORDS:
		return IDS_SORTBY_KEYWORDS;

	case IDM_SORTBY_COMMENTS:
		return IDS_SORTBY_COMMENT;

	case IDM_SORTBY_CAMERAMODEL:
		return IDS_SORTBY_CAMERAMODEL;

	case IDM_SORTBY_DATETAKEN:
		return IDS_SORTBY_DATETAKEN;

	case IDM_SORTBY_WIDTH:
		return IDS_SORTBY_WIDTH;

	case IDM_SORTBY_HEIGHT:
		return IDS_SORTBY_HEIGHT;

	case IDM_SORTBY_VIRTUALCOMMENTS:
		return IDS_SORTBY_COMMENT;

	case IDM_SORTBY_FILESYSTEM:
		return IDS_SORTBY_FILESYSTEM;

	case IDM_SORTBY_NUMPRINTERDOCUMENTS:
		return IDS_SORTBY_NUMPRINTERDOCUMENTS;

	case IDM_SORTBY_PRINTERSTATUS:
		return IDS_SORTBY_PRINTERSTATUS;

	case IDM_SORTBY_PRINTERCOMMENTS:
		return IDS_SORTBY_COMMENT;

	case IDM_SORTBY_PRINTERLOCATION:
		return IDS_SORTBY_PRINTERLOCATION;

	case IDM_SORTBY_NETWORKADAPTER_STATUS:
		return IDS_SORTBY_NETWORKADAPTERSTATUS;

	case IDM_SORTBY_MEDIA_BITRATE:
		return IDS_COLUMN_NAME_BITRATE;

	case IDM_SORTBY_MEDIA_COPYRIGHT:
		return IDS_COLUMN_NAME_COPYRIGHT;

	case IDM_SORTBY_MEDIA_DURATION:
		return IDS_COLUMN_NAME_DURATION;

	case IDM_SORTBY_MEDIA_PROTECTED:
		return IDS_COLUMN_NAME_PROTECTED;

	case IDM_SORTBY_MEDIA_RATING:
		return IDS_COLUMN_NAME_RATING;

	case IDM_SORTBY_MEDIA_ALBUM_ARTIST:
		return IDS_COLUMN_NAME_ALBUMARTIST;

	case IDM_SORTBY_MEDIA_ALBUM:
		return IDS_COLUMN_NAME_ALBUM;

	case IDM_SORTBY_MEDIA_BEATS_PER_MINUTE:
		return IDS_COLUMN_NAME_BEATSPERMINUTE;

	case IDM_SORTBY_MEDIA_COMPOSER:
		return IDS_COLUMN_NAME_COMPOSER;

	case IDM_SORTBY_MEDIA_CONDUCTOR:
		return IDS_COLUMN_NAME_CONDUCTOR;

	case IDM_SORTBY_MEDIA_DIRECTOR:
		return IDS_COLUMN_NAME_DIRECTOR;

	case IDM_SORTBY_MEDIA_GENRE:
		return IDS_COLUMN_NAME_GENRE;

	case IDM_SORTBY_MEDIA_LANGUAGE:
		return IDS_COLUMN_NAME_LANGUAGE;

	case IDM_SORTBY_MEDIA_BROADCAST_DATE:
		return IDS_COLUMN_NAME_BROADCASTDATE;

	case IDM_SORTBY_MEDIA_CHANNEL:
		return IDS_COLUMN_NAME_CHANNEL;

	case IDM_SORTBY_MEDIA_STATION_NAME:
		return IDS_COLUMN_NAME_STATIONNAME;

	case IDM_SORTBY_MEDIA_MOOD:
		return IDS_COLUMN_NAME_MOOD;

	case IDM_SORTBY_MEDIA_PARENTAL_RATING:
		return IDS_COLUMN_NAME_PARENTALRATING;

	case IDM_SORTBY_MEDIA_PARENTAL_RATNG_REASON:
		return IDS_COLUMN_NAME_PARENTALRATINGREASON;

	case IDM_SORTBY_MEDIA_PERIOD:
		return IDS_COLUMN_NAME_PERIOD;

	case IDM_SORTBY_MEDIA_PRODUCER:
		return IDS_COLUMN_NAME_PRODUCER;

	case IDM_SORTBY_MEDIA_PUBLISHER:
		return IDS_COLUMN_NAME_PUBLISHER;

	case IDM_SORTBY_MEDIA_WRITER:
		return IDS_COLUMN_NAME_WRITER;

	case IDM_SORTBY_MEDIA_YEAR:
		return IDS_COLUMN_NAME_YEAR;

	default:
		assert(false);
		break;
	}

	return 0;
}

int SortMenuBuilder::DetermineSortModeMenuId(SortMode sortMode)
{
	switch (sortMode)
	{
	case SortMode::Name:
		return IDM_SORTBY_NAME;

	case SortMode::DateModified:
		return IDM_SORTBY_DATEMODIFIED;

	case SortMode::Size:
		return IDM_SORTBY_SIZE;

	case SortMode::Type:
		return IDM_SORTBY_TYPE;

	case SortMode::TotalSize:
		return IDM_SORTBY_TOTALSIZE;

	case SortMode::FreeSpace:
		return IDM_SORTBY_FREESPACE;

	case SortMode::Comments:
		return IDM_SORTBY_COMMENTS;

	case SortMode::DateDeleted:
		return IDM_SORTBY_DATEDELETED;

	case SortMode::OriginalLocation:
		return IDM_SORTBY_ORIGINALLOCATION;

	case SortMode::Attributes:
		return IDM_SORTBY_ATTRIBUTES;

	case SortMode::RealSize:
		return IDM_SORTBY_REALSIZE;

	case SortMode::ShortName:
		return IDM_SORTBY_SHORTNAME;

	case SortMode::Owner:
		return IDM_SORTBY_OWNER;

	case SortMode::ProductName:
		return IDM_SORTBY_PRODUCTNAME;

	case SortMode::Company:
		return IDM_SORTBY_COMPANY;

	case SortMode::Description:
		return IDM_SORTBY_DESCRIPTION;

	case SortMode::FileVersion:
		return IDM_SORTBY_FILEVERSION;

	case SortMode::ProductVersion:
		return IDM_SORTBY_PRODUCTVERSION;

	case SortMode::ShortcutTo:
		return IDM_SORTBY_SHORTCUTTO;

	case SortMode::HardLinks:
		return IDM_SORTBY_HARDLINKS;

	case SortMode::Extension:
		return IDM_SORTBY_EXTENSION;

	case SortMode::Created:
		return IDM_SORTBY_CREATED;

	case SortMode::Accessed:
		return IDM_SORTBY_ACCESSED;

	case SortMode::Title:
		return IDM_SORTBY_TITLE;

	case SortMode::Subject:
		return IDM_SORTBY_SUBJECT;

	case SortMode::Authors:
		return IDM_SORTBY_AUTHOR;

	case SortMode::Keywords:
		return IDM_SORTBY_KEYWORDS;

	case SortMode::CameraModel:
		return IDM_SORTBY_CAMERAMODEL;

	case SortMode::DateTaken:
		return IDM_SORTBY_DATETAKEN;

	case SortMode::Width:
		return IDM_SORTBY_WIDTH;

	case SortMode::Height:
		return IDM_SORTBY_HEIGHT;

	case SortMode::VirtualComments:
		return IDM_SORTBY_VIRTUALCOMMENTS;

	case SortMode::FileSystem:
		return IDM_SORTBY_FILESYSTEM;

	case SortMode::NumPrinterDocuments:
		return IDM_SORTBY_NUMPRINTERDOCUMENTS;

	case SortMode::PrinterStatus:
		return IDM_SORTBY_PRINTERSTATUS;

	case SortMode::PrinterComments:
		return IDM_SORTBY_PRINTERCOMMENTS;

	case SortMode::PrinterLocation:
		return IDM_SORTBY_PRINTERLOCATION;

	case SortMode::NetworkAdapterStatus:
		return IDM_SORTBY_NETWORKADAPTER_STATUS;

	case SortMode::MediaBitrate:
		return IDM_SORTBY_MEDIA_BITRATE;

	case SortMode::MediaCopyright:
		return IDM_SORTBY_MEDIA_COPYRIGHT;

	case SortMode::MediaDuration:
		return IDM_SORTBY_MEDIA_DURATION;

	case SortMode::MediaProtected:
		return IDM_SORTBY_MEDIA_PROTECTED;

	case SortMode::MediaRating:
		return IDM_SORTBY_MEDIA_RATING;

	case SortMode::MediaAlbumArtist:
		return IDM_SORTBY_MEDIA_ALBUM_ARTIST;

	case SortMode::MediaAlbum:
		return IDM_SORTBY_MEDIA_ALBUM;

	case SortMode::MediaBeatsPerMinute:
		return IDM_SORTBY_MEDIA_BEATS_PER_MINUTE;

	case SortMode::MediaComposer:
		return IDM_SORTBY_MEDIA_COMPOSER;

	case SortMode::MediaConductor:
		return IDM_SORTBY_MEDIA_CONDUCTOR;

	case SortMode::MediaDirector:
		return IDM_SORTBY_MEDIA_DIRECTOR;

	case SortMode::MediaGenre:
		return IDM_SORTBY_MEDIA_GENRE;

	case SortMode::MediaLanguage:
		return IDM_SORTBY_MEDIA_LANGUAGE;

	case SortMode::MediaBroadcastDate:
		return IDM_SORTBY_MEDIA_BROADCAST_DATE;

	case SortMode::MediaChannel:
		return IDM_SORTBY_MEDIA_CHANNEL;

	case SortMode::MediaStationName:
		return IDM_SORTBY_MEDIA_STATION_NAME;

	case SortMode::MediaMood:
		return IDM_SORTBY_MEDIA_MOOD;

	case SortMode::MediaParentalRating:
		return IDM_SORTBY_MEDIA_PARENTAL_RATING;

	case SortMode::MediaParentalRatingReason:
		return IDM_SORTBY_MEDIA_PARENTAL_RATNG_REASON;

	case SortMode::MediaPeriod:
		return IDM_SORTBY_MEDIA_PERIOD;

	case SortMode::MediaProducer:
		return IDM_SORTBY_MEDIA_PRODUCER;

	case SortMode::MediaPublisher:
		return IDM_SORTBY_MEDIA_PUBLISHER;

	case SortMode::MediaWriter:
		return IDM_SORTBY_MEDIA_WRITER;

	case SortMode::MediaYear:
		return IDM_SORTBY_MEDIA_YEAR;

	default:
		assert(false);
		break;
	}

	return -1;
}

int SortMenuBuilder::DetermineGroupModeMenuId(SortMode sortMode)
{
	switch (sortMode)
	{
	case SortMode::Name:
		return IDM_GROUPBY_NAME;

	case SortMode::DateModified:
		return IDM_GROUPBY_DATEMODIFIED;

	case SortMode::Size:
		return IDM_GROUPBY_SIZE;

	case SortMode::Type:
		return IDM_GROUPBY_TYPE;

	case SortMode::TotalSize:
		return IDM_GROUPBY_TOTALSIZE;

	case SortMode::FreeSpace:
		return IDM_GROUPBY_FREESPACE;

	case SortMode::Comments:
		return IDM_GROUPBY_COMMENTS;

	case SortMode::DateDeleted:
		return IDM_GROUPBY_DATEDELETED;

	case SortMode::OriginalLocation:
		return IDM_GROUPBY_ORIGINALLOCATION;

	case SortMode::Attributes:
		return IDM_GROUPBY_ATTRIBUTES;

	case SortMode::RealSize:
		return IDM_GROUPBY_REALSIZE;

	case SortMode::ShortName:
		return IDM_GROUPBY_SHORTNAME;

	case SortMode::Owner:
		return IDM_GROUPBY_OWNER;

	case SortMode::ProductName:
		return IDM_GROUPBY_PRODUCTNAME;

	case SortMode::Company:
		return IDM_GROUPBY_COMPANY;

	case SortMode::Description:
		return IDM_GROUPBY_DESCRIPTION;

	case SortMode::FileVersion:
		return IDM_GROUPBY_FILEVERSION;

	case SortMode::ProductVersion:
		return IDM_GROUPBY_PRODUCTVERSION;

	case SortMode::ShortcutTo:
		return IDM_GROUPBY_SHORTCUTTO;

	case SortMode::HardLinks:
		return IDM_GROUPBY_HARDLINKS;

	case SortMode::Extension:
		return IDM_GROUPBY_EXTENSION;

	case SortMode::Created:
		return IDM_GROUPBY_CREATED;

	case SortMode::Accessed:
		return IDM_GROUPBY_ACCESSED;

	case SortMode::Title:
		return IDM_GROUPBY_TITLE;

	case SortMode::Subject:
		return IDM_GROUPBY_SUBJECT;

	case SortMode::Authors:
		return IDM_GROUPBY_AUTHOR;

	case SortMode::Keywords:
		return IDM_GROUPBY_KEYWORDS;

	case SortMode::CameraModel:
		return IDM_GROUPBY_CAMERAMODEL;

	case SortMode::DateTaken:
		return IDM_GROUPBY_DATETAKEN;

	case SortMode::Width:
		return IDM_GROUPBY_WIDTH;

	case SortMode::Height:
		return IDM_GROUPBY_HEIGHT;

	case SortMode::VirtualComments:
		return IDM_GROUPBY_VIRTUALCOMMENTS;

	case SortMode::FileSystem:
		return IDM_GROUPBY_FILESYSTEM;

	case SortMode::NumPrinterDocuments:
		return IDM_GROUPBY_NUMPRINTERDOCUMENTS;

	case SortMode::PrinterStatus:
		return IDM_GROUPBY_PRINTERSTATUS;

	case SortMode::PrinterComments:
		return IDM_GROUPBY_PRINTERCOMMENTS;

	case SortMode::PrinterLocation:
		return IDM_GROUPBY_PRINTERLOCATION;

	case SortMode::NetworkAdapterStatus:
		return IDM_GROUPBY_NETWORKADAPTER_STATUS;

	case SortMode::MediaBitrate:
		return IDM_GROUPBY_MEDIA_BITRATE;

	case SortMode::MediaCopyright:
		return IDM_GROUPBY_MEDIA_COPYRIGHT;

	case SortMode::MediaDuration:
		return IDM_GROUPBY_MEDIA_DURATION;

	case SortMode::MediaProtected:
		return IDM_GROUPBY_MEDIA_PROTECTED;

	case SortMode::MediaRating:
		return IDM_GROUPBY_MEDIA_RATING;

	case SortMode::MediaAlbumArtist:
		return IDM_GROUPBY_MEDIA_ALBUM_ARTIST;

	case SortMode::MediaAlbum:
		return IDM_GROUPBY_MEDIA_ALBUM;

	case SortMode::MediaBeatsPerMinute:
		return IDM_GROUPBY_MEDIA_BEATS_PER_MINUTE;

	case SortMode::MediaComposer:
		return IDM_GROUPBY_MEDIA_COMPOSER;

	case SortMode::MediaConductor:
		return IDM_GROUPBY_MEDIA_CONDUCTOR;

	case SortMode::MediaDirector:
		return IDM_GROUPBY_MEDIA_DIRECTOR;

	case SortMode::MediaGenre:
		return IDM_GROUPBY_MEDIA_GENRE;

	case SortMode::MediaLanguage:
		return IDM_GROUPBY_MEDIA_LANGUAGE;

	case SortMode::MediaBroadcastDate:
		return IDM_GROUPBY_MEDIA_BROADCAST_DATE;

	case SortMode::MediaChannel:
		return IDM_GROUPBY_MEDIA_CHANNEL;

	case SortMode::MediaStationName:
		return IDM_GROUPBY_MEDIA_STATION_NAME;

	case SortMode::MediaMood:
		return IDM_GROUPBY_MEDIA_MOOD;

	case SortMode::MediaParentalRating:
		return IDM_GROUPBY_MEDIA_PARENTAL_RATING;

	case SortMode::MediaParentalRatingReason:
		return IDM_GROUPBY_MEDIA_PARENTAL_RATING_REASON;

	case SortMode::MediaPeriod:
		return IDM_GROUPBY_MEDIA_PERIOD;

	case SortMode::MediaProducer:
		return IDM_GROUPBY_MEDIA_PRODUCER;

	case SortMode::MediaPublisher:
		return IDM_GROUPBY_MEDIA_PUBLISHER;

	case SortMode::MediaWriter:
		return IDM_GROUPBY_MEDIA_WRITER;

	case SortMode::MediaYear:
		return IDM_GROUPBY_MEDIA_YEAR;

	default:
		assert(false);
		break;
	}

	return -1;
}
