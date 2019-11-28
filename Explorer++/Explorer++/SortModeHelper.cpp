// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SortModeHelper.h"
#include "MainResource.h"

UINT GetSortMenuItemStringIndex(UINT uItemId)
{
	switch (uItemId)
	{
	case IDM_SORTBY_NAME:
		return IDS_SORTBY_NAME;
		break;

	case IDM_SORTBY_SIZE:
		return IDS_SORTBY_SIZE;
		break;

	case IDM_SORTBY_TYPE:
		return IDS_SORTBY_TYPE;
		break;

	case IDM_SORTBY_DATEMODIFIED:
		return IDS_SORTBY_DATEMODIFIED;
		break;

	case IDM_SORTBY_TOTALSIZE:
		return IDS_SORTBY_TOTALSIZE;
		break;

	case IDM_SORTBY_FREESPACE:
		return IDS_SORTBY_FREESPACE;
		break;

	case IDM_SORTBY_DATEDELETED:
		return IDS_SORTBY_DATEDELETED;
		break;

	case IDM_SORTBY_ORIGINALLOCATION:
		return IDS_SORTBY_ORIGINALLOCATION;
		break;

	case IDM_SORTBY_ATTRIBUTES:
		return IDS_SORTBY_ATTRIBUTES;
		break;

	case IDM_SORTBY_REALSIZE:
		return IDS_SORTBY_REALSIZE;
		break;

	case IDM_SORTBY_SHORTNAME:
		return IDS_SORTBY_SHORTNAME;
		break;

	case IDM_SORTBY_OWNER:
		return IDS_SORTBY_OWNER;
		break;

	case IDM_SORTBY_PRODUCTNAME:
		return IDS_SORTBY_PRODUCTNAME;
		break;

	case IDM_SORTBY_COMPANY:
		return IDS_SORTBY_COMPANY;
		break;

	case IDM_SORTBY_DESCRIPTION:
		return IDS_SORTBY_DESCRIPTION;
		break;

	case IDM_SORTBY_FILEVERSION:
		return IDS_SORTBY_FILEVERSION;
		break;

	case IDM_SORTBY_PRODUCTVERSION:
		return IDS_SORTBY_PRODUCTVERSION;
		break;

	case IDM_SORTBY_SHORTCUTTO:
		return IDS_SORTBY_SHORTCUTTO;
		break;

	case IDM_SORTBY_HARDLINKS:
		return IDS_SORTBY_HARDLINKS;
		break;

	case IDM_SORTBY_EXTENSION:
		return IDS_SORTBY_EXTENSION;
		break;

	case IDM_SORTBY_CREATED:
		return IDS_SORTBY_CREATED;
		break;

	case IDM_SORTBY_ACCESSED:
		return IDS_SORTBY_ACCESSED;
		break;

	case IDM_SORTBY_TITLE:
		return IDS_SORTBY_TITLE;
		break;

	case IDM_SORTBY_SUBJECT:
		return IDS_SORTBY_SUBJECT;
		break;

	case IDM_SORTBY_AUTHOR:
		return IDS_SORTBY_AUTHOR;
		break;

	case IDM_SORTBY_KEYWORDS:
		return IDS_SORTBY_KEYWORDS;
		break;

	case IDM_SORTBY_COMMENTS:
		return IDS_SORTBY_COMMENT;
		break;

	case IDM_SORTBY_CAMERAMODEL:
		return IDS_SORTBY_CAMERAMODEL;
		break;

	case IDM_SORTBY_DATETAKEN:
		return IDS_SORTBY_DATETAKEN;
		break;

	case IDM_SORTBY_WIDTH:
		return IDS_SORTBY_WIDTH;
		break;

	case IDM_SORTBY_HEIGHT:
		return IDS_SORTBY_HEIGHT;
		break;

	case IDM_SORTBY_VIRTUALCOMMENTS:
		return IDS_SORTBY_COMMENT;
		break;

	case IDM_SORTBY_FILESYSTEM:
		return IDS_SORTBY_FILESYSTEM;
		break;

	case IDM_SORTBY_NUMPRINTERDOCUMENTS:
		return IDS_SORTBY_NUMPRINTERDOCUMENTS;
		break;

	case IDM_SORTBY_PRINTERSTATUS:
		return IDS_SORTBY_PRINTERSTATUS;
		break;

	case IDM_SORTBY_PRINTERCOMMENTS:
		return IDS_SORTBY_COMMENT;
		break;

	case IDM_SORTBY_PRINTERLOCATION:
		return IDS_SORTBY_PRINTERLOCATION;
		break;

	case IDM_SORTBY_NETWORKADAPTER_STATUS:
		return IDS_SORTBY_NETWORKADAPTERSTATUS;
		break;

	case IDM_SORTBY_MEDIA_BITRATE:
		return IDS_COLUMN_NAME_BITRATE;
		break;

	case IDM_SORTBY_MEDIA_COPYRIGHT:
		return IDS_COLUMN_NAME_COPYRIGHT;
		break;

	case IDM_SORTBY_MEDIA_DURATION:
		return IDS_COLUMN_NAME_DURATION;
		break;

	case IDM_SORTBY_MEDIA_PROTECTED:
		return IDS_COLUMN_NAME_PROTECTED;
		break;

	case IDM_SORTBY_MEDIA_RATING:
		return IDS_COLUMN_NAME_RATING;
		break;

	case IDM_SORTBY_MEDIA_ALBUM_ARTIST:
		return IDS_COLUMN_NAME_ALBUMARTIST;
		break;

	case IDM_SORTBY_MEDIA_ALBUM:
		return IDS_COLUMN_NAME_ALBUM;
		break;

	case IDM_SORTBY_MEDIA_BEATS_PER_MINUTE:
		return IDS_COLUMN_NAME_BEATSPERMINUTE;
		break;

	case IDM_SORTBY_MEDIA_COMPOSER:
		return IDS_COLUMN_NAME_COMPOSER;
		break;

	case IDM_SORTBY_MEDIA_CONDUCTOR:
		return IDS_COLUMN_NAME_CONDUCTOR;
		break;

	case IDM_SORTBY_MEDIA_DIRECTOR:
		return IDS_COLUMN_NAME_DIRECTOR;
		break;

	case IDM_SORTBY_MEDIA_GENRE:
		return IDS_COLUMN_NAME_GENRE;
		break;

	case IDM_SORTBY_MEDIA_LANGUAGE:
		return IDS_COLUMN_NAME_LANGUAGE;
		break;

	case IDM_SORTBY_MEDIA_BROADCAST_DATE:
		return IDS_COLUMN_NAME_BROADCASTDATE;
		break;

	case IDM_SORTBY_MEDIA_CHANNEL:
		return IDS_COLUMN_NAME_CHANNEL;
		break;

	case IDM_SORTBY_MEDIA_STATION_NAME:
		return IDS_COLUMN_NAME_STATIONNAME;
		break;

	case IDM_SORTBY_MEDIA_MOOD:
		return IDS_COLUMN_NAME_MOOD;
		break;

	case IDM_SORTBY_MEDIA_PARENTAL_RATING:
		return IDS_COLUMN_NAME_PARENTALRATING;
		break;

	case IDM_SORTBY_MEDIA_PARENTAL_RATNG_REASON:
		return IDS_COLUMN_NAME_PARENTALRATINGREASON;
		break;

	case IDM_SORTBY_MEDIA_PERIOD:
		return IDS_COLUMN_NAME_PERIOD;
		break;

	case IDM_SORTBY_MEDIA_PRODUCER:
		return IDS_COLUMN_NAME_PRODUCER;
		break;

	case IDM_SORTBY_MEDIA_PUBLISHER:
		return IDS_COLUMN_NAME_PUBLISHER;
		break;

	case IDM_SORTBY_MEDIA_WRITER:
		return IDS_COLUMN_NAME_WRITER;
		break;

	case IDM_SORTBY_MEDIA_YEAR:
		return IDS_COLUMN_NAME_YEAR;
		break;

	default:
		assert(false);
		break;
	}

	return 0;
}

int DetermineSortModeMenuId(SortMode sortMode)
{
	switch (sortMode)
	{
	case SortMode::Name:
		return IDM_SORTBY_NAME;
		break;

	case SortMode::DateModified:
		return IDM_SORTBY_DATEMODIFIED;
		break;

	case SortMode::Size:
		return IDM_SORTBY_SIZE;
		break;

	case SortMode::Type:
		return IDM_SORTBY_TYPE;
		break;

	case SortMode::TotalSize:
		return IDM_SORTBY_TOTALSIZE;
		break;

	case SortMode::FreeSpace:
		return IDM_SORTBY_FREESPACE;
		break;

	case SortMode::Comments:
		return IDM_SORTBY_COMMENTS;
		break;

	case SortMode::DateDeleted:
		return IDM_SORTBY_DATEDELETED;
		break;

	case SortMode::OriginalLocation:
		return IDM_SORTBY_ORIGINALLOCATION;
		break;

	case SortMode::Attributes:
		return IDM_SORTBY_ATTRIBUTES;
		break;

	case SortMode::RealSize:
		return IDM_SORTBY_REALSIZE;
		break;

	case SortMode::ShortName:
		return IDM_SORTBY_SHORTNAME;
		break;

	case SortMode::Owner:
		return IDM_SORTBY_OWNER;
		break;

	case SortMode::ProductName:
		return IDM_SORTBY_PRODUCTNAME;
		break;

	case SortMode::Company:
		return IDM_SORTBY_COMPANY;
		break;

	case SortMode::Description:
		return IDM_SORTBY_DESCRIPTION;
		break;

	case SortMode::FileVersion:
		return IDM_SORTBY_FILEVERSION;
		break;

	case SortMode::ProductVersion:
		return IDM_SORTBY_PRODUCTVERSION;
		break;

	case SortMode::ShortcutTo:
		return IDM_SORTBY_SHORTCUTTO;
		break;

	case SortMode::HardLinks:
		return IDM_SORTBY_HARDLINKS;
		break;

	case SortMode::Extension:
		return IDM_SORTBY_EXTENSION;
		break;

	case SortMode::Created:
		return IDM_SORTBY_CREATED;
		break;

	case SortMode::Accessed:
		return IDM_SORTBY_ACCESSED;
		break;

	case SortMode::Title:
		return IDM_SORTBY_TITLE;
		break;

	case SortMode::Subject:
		return IDM_SORTBY_SUBJECT;
		break;

	case SortMode::Authors:
		return IDM_SORTBY_AUTHOR;
		break;

	case SortMode::Keywords:
		return IDM_SORTBY_KEYWORDS;
		break;

	case SortMode::CameraModel:
		return IDM_SORTBY_CAMERAMODEL;
		break;

	case SortMode::DateTaken:
		return IDM_SORTBY_DATETAKEN;
		break;

	case SortMode::Width:
		return IDM_SORTBY_WIDTH;
		break;

	case SortMode::Height:
		return IDM_SORTBY_HEIGHT;
		break;

	case SortMode::VirtualComments:
		return IDM_SORTBY_VIRTUALCOMMENTS;
		break;

	case SortMode::FileSystem:
		return IDM_SORTBY_FILESYSTEM;
		break;

	case SortMode::NumPrinterDocuments:
		return IDM_SORTBY_NUMPRINTERDOCUMENTS;
		break;

	case SortMode::PrinterStatus:
		return IDM_SORTBY_PRINTERSTATUS;
		break;

	case SortMode::PrinterComments:
		return IDM_SORTBY_PRINTERCOMMENTS;
		break;

	case SortMode::PrinterLocation:
		return IDM_SORTBY_PRINTERLOCATION;
		break;

	case SortMode::NetworkAdapterStatus:
		return IDM_SORTBY_NETWORKADAPTER_STATUS;
		break;

	case SortMode::MediaBitrate:
		return IDM_SORTBY_MEDIA_BITRATE;
		break;

	case SortMode::MediaCopyright:
		return IDM_SORTBY_MEDIA_COPYRIGHT;
		break;

	case SortMode::MediaDuration:
		return IDM_SORTBY_MEDIA_DURATION;
		break;

	case SortMode::MediaProtected:
		return IDM_SORTBY_MEDIA_PROTECTED;
		break;

	case SortMode::MediaRating:
		return IDM_SORTBY_MEDIA_RATING;
		break;

	case SortMode::MediaAlbumArtist:
		return IDM_SORTBY_MEDIA_ALBUM_ARTIST;
		break;

	case SortMode::MediaAlbum:
		return IDM_SORTBY_MEDIA_ALBUM;
		break;

	case SortMode::MediaBeatsPerMinute:
		return IDM_SORTBY_MEDIA_BEATS_PER_MINUTE;
		break;

	case SortMode::MediaComposer:
		return IDM_SORTBY_MEDIA_COMPOSER;
		break;

	case SortMode::MediaConductor:
		return IDM_SORTBY_MEDIA_CONDUCTOR;
		break;

	case SortMode::MediaDirector:
		return IDM_SORTBY_MEDIA_DIRECTOR;
		break;

	case SortMode::MediaGenre:
		return IDM_SORTBY_MEDIA_GENRE;
		break;

	case SortMode::MediaLanguage:
		return IDM_SORTBY_MEDIA_LANGUAGE;
		break;

	case SortMode::MediaBroadcastDate:
		return IDM_SORTBY_MEDIA_BROADCAST_DATE;
		break;

	case SortMode::MediaChannel:
		return IDM_SORTBY_MEDIA_CHANNEL;
		break;

	case SortMode::MediaStationName:
		return IDM_SORTBY_MEDIA_STATION_NAME;
		break;

	case SortMode::MediaMood:
		return IDM_SORTBY_MEDIA_MOOD;
		break;

	case SortMode::MediaParentalRating:
		return IDM_SORTBY_MEDIA_PARENTAL_RATING;
		break;

	case SortMode::MediaParentalRatingReason:
		return IDM_SORTBY_MEDIA_PARENTAL_RATNG_REASON;
		break;

	case SortMode::MediaPeriod:
		return IDM_SORTBY_MEDIA_PERIOD;
		break;

	case SortMode::MediaProducer:
		return IDM_SORTBY_MEDIA_PRODUCER;
		break;

	case SortMode::MediaPublisher:
		return IDM_SORTBY_MEDIA_PUBLISHER;
		break;

	case SortMode::MediaWriter:
		return IDM_SORTBY_MEDIA_WRITER;
		break;

	case SortMode::MediaYear:
		return IDM_SORTBY_MEDIA_YEAR;
		break;

	default:
		assert(false);
		break;
	}

	return -1;
}

int DetermineGroupModeMenuId(SortMode sortMode)
{
	switch (sortMode)
	{
	case SortMode::Name:
		return IDM_GROUPBY_NAME;
		break;

	case SortMode::DateModified:
		return IDM_GROUPBY_DATEMODIFIED;
		break;

	case SortMode::Size:
		return IDM_GROUPBY_SIZE;
		break;

	case SortMode::Type:
		return IDM_GROUPBY_TYPE;
		break;

	case SortMode::TotalSize:
		return IDM_GROUPBY_TOTALSIZE;
		break;

	case SortMode::FreeSpace:
		return IDM_GROUPBY_FREESPACE;
		break;

	case SortMode::Comments:
		return IDM_GROUPBY_COMMENTS;
		break;

	case SortMode::DateDeleted:
		return IDM_GROUPBY_DATEDELETED;
		break;

	case SortMode::OriginalLocation:
		return IDM_GROUPBY_ORIGINALLOCATION;
		break;

	case SortMode::Attributes:
		return IDM_GROUPBY_ATTRIBUTES;
		break;

	case SortMode::RealSize:
		return IDM_GROUPBY_REALSIZE;
		break;

	case SortMode::ShortName:
		return IDM_GROUPBY_SHORTNAME;
		break;

	case SortMode::Owner:
		return IDM_GROUPBY_OWNER;
		break;

	case SortMode::ProductName:
		return IDM_GROUPBY_PRODUCTNAME;
		break;

	case SortMode::Company:
		return IDM_GROUPBY_COMPANY;
		break;

	case SortMode::Description:
		return IDM_GROUPBY_DESCRIPTION;
		break;

	case SortMode::FileVersion:
		return IDM_GROUPBY_FILEVERSION;
		break;

	case SortMode::ProductVersion:
		return IDM_GROUPBY_PRODUCTVERSION;
		break;

	case SortMode::ShortcutTo:
		return IDM_GROUPBY_SHORTCUTTO;
		break;

	case SortMode::HardLinks:
		return IDM_GROUPBY_HARDLINKS;
		break;

	case SortMode::Extension:
		return IDM_GROUPBY_EXTENSION;
		break;

	case SortMode::Created:
		return IDM_GROUPBY_CREATED;
		break;

	case SortMode::Accessed:
		return IDM_GROUPBY_ACCESSED;
		break;

	case SortMode::Title:
		return IDM_GROUPBY_TITLE;
		break;

	case SortMode::Subject:
		return IDM_GROUPBY_SUBJECT;
		break;

	case SortMode::Authors:
		return IDM_GROUPBY_AUTHOR;
		break;

	case SortMode::Keywords:
		return IDM_GROUPBY_KEYWORDS;
		break;

	case SortMode::CameraModel:
		return IDM_GROUPBY_CAMERAMODEL;
		break;

	case SortMode::DateTaken:
		return IDM_GROUPBY_DATETAKEN;
		break;

	case SortMode::Width:
		return IDM_GROUPBY_WIDTH;
		break;

	case SortMode::Height:
		return IDM_GROUPBY_HEIGHT;
		break;

	case SortMode::VirtualComments:
		return IDM_GROUPBY_VIRTUALCOMMENTS;
		break;

	case SortMode::FileSystem:
		return IDM_GROUPBY_FILESYSTEM;
		break;

	case SortMode::NumPrinterDocuments:
		return IDM_GROUPBY_NUMPRINTERDOCUMENTS;
		break;

	case SortMode::PrinterStatus:
		return IDM_GROUPBY_PRINTERSTATUS;
		break;

	case SortMode::PrinterComments:
		return IDM_GROUPBY_PRINTERCOMMENTS;
		break;

	case SortMode::PrinterLocation:
		return IDM_GROUPBY_PRINTERLOCATION;
		break;

	case SortMode::NetworkAdapterStatus:
		return IDM_GROUPBY_NETWORKADAPTER_STATUS;
		break;

	case SortMode::MediaBitrate:
		return IDM_GROUPBY_MEDIA_BITRATE;
		break;

	case SortMode::MediaCopyright:
		return IDM_GROUPBY_MEDIA_COPYRIGHT;
		break;

	case SortMode::MediaDuration:
		return IDM_GROUPBY_MEDIA_DURATION;
		break;

	case SortMode::MediaProtected:
		return IDM_GROUPBY_MEDIA_PROTECTED;
		break;

	case SortMode::MediaRating:
		return IDM_GROUPBY_MEDIA_RATING;
		break;

	case SortMode::MediaAlbumArtist:
		return IDM_GROUPBY_MEDIA_ALBUM_ARTIST;
		break;

	case SortMode::MediaAlbum:
		return IDM_GROUPBY_MEDIA_ALBUM;
		break;

	case SortMode::MediaBeatsPerMinute:
		return IDM_GROUPBY_MEDIA_BEATS_PER_MINUTE;
		break;

	case SortMode::MediaComposer:
		return IDM_GROUPBY_MEDIA_COMPOSER;
		break;

	case SortMode::MediaConductor:
		return IDM_GROUPBY_MEDIA_CONDUCTOR;
		break;

	case SortMode::MediaDirector:
		return IDM_GROUPBY_MEDIA_DIRECTOR;
		break;

	case SortMode::MediaGenre:
		return IDM_GROUPBY_MEDIA_GENRE;
		break;

	case SortMode::MediaLanguage:
		return IDM_GROUPBY_MEDIA_LANGUAGE;
		break;

	case SortMode::MediaBroadcastDate:
		return IDM_GROUPBY_MEDIA_BROADCAST_DATE;
		break;

	case SortMode::MediaChannel:
		return IDM_GROUPBY_MEDIA_CHANNEL;
		break;

	case SortMode::MediaStationName:
		return IDM_GROUPBY_MEDIA_STATION_NAME;
		break;

	case SortMode::MediaMood:
		return IDM_GROUPBY_MEDIA_MOOD;
		break;

	case SortMode::MediaParentalRating:
		return IDM_GROUPBY_MEDIA_PARENTAL_RATING;
		break;

	case SortMode::MediaParentalRatingReason:
		return IDM_GROUPBY_MEDIA_PARENTAL_RATING_REASON;
		break;

	case SortMode::MediaPeriod:
		return IDM_GROUPBY_MEDIA_PERIOD;
		break;

	case SortMode::MediaProducer:
		return IDM_GROUPBY_MEDIA_PRODUCER;
		break;

	case SortMode::MediaPublisher:
		return IDM_GROUPBY_MEDIA_PUBLISHER;
		break;

	case SortMode::MediaWriter:
		return IDM_GROUPBY_MEDIA_WRITER;
		break;

	case SortMode::MediaYear:
		return IDM_GROUPBY_MEDIA_YEAR;
		break;

	default:
		assert(false);
		break;
	}

	return -1;
}