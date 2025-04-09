// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ColumnHelper.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include <optional>

std::wstring GetColumnName(const ResourceLoader *resourceLoader, ColumnType columnType)
{
	std::optional<UINT> stringId;

	switch (columnType)
	{
	case ColumnType::Name:
		stringId = IDS_COLUMN_NAME_NAME;
		break;

	case ColumnType::Type:
		stringId = IDS_COLUMN_NAME_TYPE;
		break;

	case ColumnType::Size:
		stringId = IDS_COLUMN_NAME_SIZE;
		break;

	case ColumnType::DateModified:
		stringId = IDS_COLUMN_NAME_DATEMODIFIED;
		break;

	case ColumnType::Attributes:
		stringId = IDS_COLUMN_NAME_ATTRIBUTES;
		break;

	case ColumnType::RealSize:
		stringId = IDS_COLUMN_NAME_REALSIZE;
		break;

	case ColumnType::ShortName:
		stringId = IDS_COLUMN_NAME_SHORTNAME;
		break;

	case ColumnType::Owner:
		stringId = IDS_COLUMN_NAME_OWNER;
		break;

	case ColumnType::ProductName:
		stringId = IDS_COLUMN_NAME_PRODUCTNAME;
		break;

	case ColumnType::Company:
		stringId = IDS_COLUMN_NAME_COMPANY;
		break;

	case ColumnType::Description:
		stringId = IDS_COLUMN_NAME_DESCRIPTION;
		break;

	case ColumnType::FileVersion:
		stringId = IDS_COLUMN_NAME_FILEVERSION;
		break;

	case ColumnType::ProductVersion:
		stringId = IDS_COLUMN_NAME_PRODUCTVERSION;
		break;

	case ColumnType::ShortcutTo:
		stringId = IDS_COLUMN_NAME_SHORTCUTTO;
		break;

	case ColumnType::HardLinks:
		stringId = IDS_COLUMN_NAME_HARDLINKS;
		break;

	case ColumnType::Extension:
		stringId = IDS_COLUMN_NAME_EXTENSION;
		break;

	case ColumnType::Created:
		stringId = IDS_COLUMN_NAME_CREATED;
		break;

	case ColumnType::Accessed:
		stringId = IDS_COLUMN_NAME_ACCESSED;
		break;

	case ColumnType::Title:
		stringId = IDS_COLUMN_NAME_TITLE;
		break;

	case ColumnType::Subject:
		stringId = IDS_COLUMN_NAME_SUBJECT;
		break;

	case ColumnType::Authors:
		stringId = IDS_COLUMN_NAME_AUTHORS;
		break;

	case ColumnType::Keywords:
		stringId = IDS_COLUMN_NAME_KEYWORDS;
		break;

	case ColumnType::Comment:
		stringId = IDS_COLUMN_NAME_COMMENT;
		break;

	case ColumnType::CameraModel:
		stringId = IDS_COLUMN_NAME_CAMERAMODEL;
		break;

	case ColumnType::DateTaken:
		stringId = IDS_COLUMN_NAME_DATETAKEN;
		break;

	case ColumnType::Width:
		stringId = IDS_COLUMN_NAME_WIDTH;
		break;

	case ColumnType::Height:
		stringId = IDS_COLUMN_NAME_HEIGHT;
		break;

	case ColumnType::VirtualComments:
		stringId = IDS_COLUMN_NAME_VIRTUALCOMMENTS;
		break;

	case ColumnType::TotalSize:
		stringId = IDS_COLUMN_NAME_TOTALSIZE;
		break;

	case ColumnType::FreeSpace:
		stringId = IDS_COLUMN_NAME_FREESPACE;
		break;

	case ColumnType::FileSystem:
		stringId = IDS_COLUMN_NAME_FILESYSTEM;
		break;

	case ColumnType::OriginalLocation:
		stringId = IDS_COLUMN_NAME_ORIGINALLOCATION;
		break;

	case ColumnType::DateDeleted:
		stringId = IDS_COLUMN_NAME_DATEDELETED;
		break;

	case ColumnType::PrinterNumDocuments:
		stringId = IDS_COLUMN_NAME_NUMPRINTERDOCUMENTS;
		break;

	case ColumnType::PrinterStatus:
		stringId = IDS_COLUMN_NAME_PRINTERSTATUS;
		break;

	case ColumnType::PrinterComments:
		stringId = IDS_COLUMN_NAME_PRINTERCOMMENTS;
		break;

	case ColumnType::PrinterLocation:
		stringId = IDS_COLUMN_NAME_PRINTERLOCATION;
		break;

	case ColumnType::PrinterModel:
		stringId = IDS_COLUMN_NAME_PRINTERMODEL;
		break;

	case ColumnType::NetworkAdaptorStatus:
		stringId = IDS_COLUMN_NAME_NETWORKADAPTER_STATUS;
		break;

	case ColumnType::MediaBitrate:
		stringId = IDS_COLUMN_NAME_BITRATE;
		break;

	case ColumnType::MediaCopyright:
		stringId = IDS_COLUMN_NAME_COPYRIGHT;
		break;

	case ColumnType::MediaDuration:
		stringId = IDS_COLUMN_NAME_DURATION;
		break;

	case ColumnType::MediaProtected:
		stringId = IDS_COLUMN_NAME_PROTECTED;
		break;

	case ColumnType::MediaRating:
		stringId = IDS_COLUMN_NAME_RATING;
		break;

	case ColumnType::MediaAlbumArtist:
		stringId = IDS_COLUMN_NAME_ALBUMARTIST;
		break;

	case ColumnType::MediaAlbum:
		stringId = IDS_COLUMN_NAME_ALBUM;
		break;

	case ColumnType::MediaBeatsPerMinute:
		stringId = IDS_COLUMN_NAME_BEATSPERMINUTE;
		break;

	case ColumnType::MediaComposer:
		stringId = IDS_COLUMN_NAME_COMPOSER;
		break;

	case ColumnType::MediaConductor:
		stringId = IDS_COLUMN_NAME_CONDUCTOR;
		break;

	case ColumnType::MediaDirector:
		stringId = IDS_COLUMN_NAME_DIRECTOR;
		break;

	case ColumnType::MediaGenre:
		stringId = IDS_COLUMN_NAME_GENRE;
		break;

	case ColumnType::MediaLanguage:
		stringId = IDS_COLUMN_NAME_LANGUAGE;
		break;

	case ColumnType::MediaBroadcastDate:
		stringId = IDS_COLUMN_NAME_BROADCASTDATE;
		break;

	case ColumnType::MediaChannel:
		stringId = IDS_COLUMN_NAME_CHANNEL;
		break;

	case ColumnType::MediaStationName:
		stringId = IDS_COLUMN_NAME_STATIONNAME;
		break;

	case ColumnType::MediaMood:
		stringId = IDS_COLUMN_NAME_MOOD;
		break;

	case ColumnType::MediaParentalRating:
		stringId = IDS_COLUMN_NAME_PARENTALRATING;
		break;

	case ColumnType::MediaParentalRatingReason:
		stringId = IDS_COLUMN_NAME_PARENTALRATINGREASON;
		break;

	case ColumnType::MediaPeriod:
		stringId = IDS_COLUMN_NAME_PERIOD;
		break;

	case ColumnType::MediaProducer:
		stringId = IDS_COLUMN_NAME_PRODUCER;
		break;

	case ColumnType::MediaPublisher:
		stringId = IDS_COLUMN_NAME_PUBLISHER;
		break;

	case ColumnType::MediaWriter:
		stringId = IDS_COLUMN_NAME_WRITER;
		break;

	case ColumnType::MediaYear:
		stringId = IDS_COLUMN_NAME_YEAR;
		break;
	}

	CHECK(stringId);
	return resourceLoader->LoadString(*stringId);
}

std::wstring GetColumnDescription(const ResourceLoader *resourceLoader, ColumnType columnType)
{
	std::optional<UINT> stringId;

	switch (columnType)
	{
	case ColumnType::Name:
		stringId = IDS_COLUMN_DESCRIPTION_NAME;
		break;

	case ColumnType::Type:
		stringId = IDS_COLUMN_DESCRIPTION_TYPE;
		break;

	case ColumnType::Size:
		stringId = IDS_COLUMN_DESCRIPTION_SIZE;
		break;

	case ColumnType::DateModified:
		stringId = IDS_COLUMN_DESCRIPTION_MODIFIED;
		break;

	case ColumnType::Attributes:
		stringId = IDS_COLUMN_DESCRIPTION_ATTRIBUTES;
		break;

	case ColumnType::RealSize:
		stringId = IDS_COLUMN_DESCRIPTION_REALSIZE;
		break;

	case ColumnType::ShortName:
		stringId = IDS_COLUMN_DESCRIPTION_SHORTNAME;
		break;

	case ColumnType::Owner:
		stringId = IDS_COLUMN_DESCRIPTION_OWNER;
		break;

	case ColumnType::ProductName:
		stringId = IDS_COLUMN_DESCRIPTION_PRODUCTNAME;
		break;

	case ColumnType::Company:
		stringId = IDS_COLUMN_DESCRIPTION_COMPANY;
		break;

	case ColumnType::Description:
		stringId = IDS_COLUMN_DESCRIPTION_DESCRIPTION;
		break;

	case ColumnType::FileVersion:
		stringId = IDS_COLUMN_DESCRIPTION_FILEVERSION;
		break;

	case ColumnType::ProductVersion:
		stringId = IDS_COLUMN_DESCRIPTION_PRODUCTVERSION;
		break;

	case ColumnType::ShortcutTo:
		stringId = IDS_COLUMN_DESCRIPTION_SHORTCUTTO;
		break;

	case ColumnType::HardLinks:
		stringId = IDS_COLUMN_DESCRIPTION_HARDLINKS;
		break;

	case ColumnType::Extension:
		stringId = IDS_COLUMN_DESCRIPTION_EXTENSION;
		break;

	case ColumnType::Created:
		stringId = IDS_COLUMN_DESCRIPTION_CREATED;
		break;

	case ColumnType::Accessed:
		stringId = IDS_COLUMN_DESCRIPTION_ACCESSED;
		break;

	case ColumnType::Title:
		stringId = IDS_COLUMN_DESCRIPTION_TITLE;
		break;

	case ColumnType::Subject:
		stringId = IDS_COLUMN_DESCRIPTION_SUBJECT;
		break;

	case ColumnType::Authors:
		stringId = IDS_COLUMN_DESCRIPTION_AUTHORS;
		break;

	case ColumnType::Keywords:
		stringId = IDS_COLUMN_DESCRIPTION_KEYWORDS;
		break;

	case ColumnType::Comment:
		stringId = IDS_COLUMN_DESCRIPTION_COMMENT;
		break;

	case ColumnType::CameraModel:
		stringId = IDS_COLUMN_DESCRIPTION_CAMERAMODEL;
		break;

	case ColumnType::DateTaken:
		stringId = IDS_COLUMN_DESCRIPTION_DATETAKEN;
		break;

	case ColumnType::Width:
		stringId = IDS_COLUMN_DESCRIPTION_WIDTH;
		break;

	case ColumnType::Height:
		stringId = IDS_COLUMN_DESCRIPTION_HEIGHT;
		break;

	case ColumnType::VirtualComments:
		stringId = IDS_COLUMN_DESCRIPTION_COMMENT;
		break;

	case ColumnType::TotalSize:
		stringId = IDS_COLUMN_DESCRIPTION_TOTALSIZE;
		break;

	case ColumnType::FreeSpace:
		stringId = IDS_COLUMN_DESCRIPTION_FREESPACE;
		break;

	case ColumnType::FileSystem:
		stringId = IDS_COLUMN_DESCRIPTION_FILESYSTEM;
		break;

	case ColumnType::PrinterNumDocuments:
		stringId = IDS_COLUMN_DESCRIPTION_NUMPRINTERDOCUMENTS;
		break;

	case ColumnType::PrinterComments:
		stringId = IDS_COLUMN_DESCRIPTION_PRINTERCOMMENTS;
		break;

	case ColumnType::PrinterLocation:
		stringId = IDS_COLUMN_DESCRIPTION_PRINTERLOCATION;
		break;

	case ColumnType::NetworkAdaptorStatus:
		stringId = IDS_COLUMN_DESCRIPTION_NETWORKADAPTER_STATUS;
		break;

	case ColumnType::MediaBitrate:
		stringId = IDS_COLUMN_DESCRIPTION_BITRATE;
		break;
	}

	if (!stringId)
	{
		return L"";
	}

	return resourceLoader->LoadString(*stringId);
}
