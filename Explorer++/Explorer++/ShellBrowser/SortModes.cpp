// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "SortModes.h"
#include "MainResource.h"

UINT GetSortModeMenuStringId(SortMode sortMode)
{
	switch (sortMode)
	{
	case SortMode::Name:
		return IDS_SORTBY_NAME;

	case SortMode::DateModified:
		return IDS_SORTBY_DATEMODIFIED;

	case SortMode::Size:
		return IDS_SORTBY_SIZE;

	case SortMode::Type:
		return IDS_SORTBY_TYPE;

	case SortMode::TotalSize:
		return IDS_SORTBY_TOTALSIZE;

	case SortMode::FreeSpace:
		return IDS_SORTBY_FREESPACE;

	case SortMode::Comments:
		return IDS_SORTBY_COMMENT;

	case SortMode::DateDeleted:
		return IDS_SORTBY_DATEDELETED;

	case SortMode::OriginalLocation:
		return IDS_SORTBY_ORIGINALLOCATION;

	case SortMode::Attributes:
		return IDS_SORTBY_ATTRIBUTES;

	case SortMode::RealSize:
		return IDS_SORTBY_REALSIZE;

	case SortMode::ShortName:
		return IDS_SORTBY_SHORTNAME;

	case SortMode::Owner:
		return IDS_SORTBY_OWNER;

	case SortMode::ProductName:
		return IDS_SORTBY_PRODUCTNAME;

	case SortMode::Company:
		return IDS_SORTBY_COMPANY;

	case SortMode::Description:
		return IDS_SORTBY_DESCRIPTION;

	case SortMode::FileVersion:
		return IDS_SORTBY_FILEVERSION;

	case SortMode::ProductVersion:
		return IDS_SORTBY_PRODUCTVERSION;

	case SortMode::ShortcutTo:
		return IDS_SORTBY_SHORTCUTTO;

	case SortMode::HardLinks:
		return IDS_SORTBY_HARDLINKS;

	case SortMode::Extension:
		return IDS_SORTBY_EXTENSION;

	case SortMode::Created:
		return IDS_SORTBY_CREATED;

	case SortMode::Accessed:
		return IDS_SORTBY_ACCESSED;

	case SortMode::Title:
		return IDS_SORTBY_TITLE;

	case SortMode::Subject:
		return IDS_SORTBY_SUBJECT;

	case SortMode::Authors:
		return IDS_SORTBY_AUTHORS;

	case SortMode::Keywords:
		return IDS_SORTBY_KEYWORDS;

	case SortMode::CameraModel:
		return IDS_SORTBY_CAMERAMODEL;

	case SortMode::DateTaken:
		return IDS_SORTBY_DATETAKEN;

	case SortMode::Width:
		return IDS_SORTBY_WIDTH;

	case SortMode::Height:
		return IDS_SORTBY_HEIGHT;

	case SortMode::VirtualComments:
		return IDS_SORTBY_COMMENT;

	case SortMode::FileSystem:
		return IDS_SORTBY_FILESYSTEM;

	case SortMode::NumPrinterDocuments:
		return IDS_SORTBY_NUMPRINTERDOCUMENTS;

	case SortMode::PrinterStatus:
		return IDS_SORTBY_PRINTERSTATUS;

	case SortMode::PrinterComments:
		return IDS_SORTBY_COMMENT;

	case SortMode::PrinterLocation:
		return IDS_SORTBY_PRINTERLOCATION;

	case SortMode::NetworkAdapterStatus:
		return IDS_SORTBY_NETWORKADAPTERSTATUS;

	case SortMode::MediaBitrate:
		return IDS_COLUMN_NAME_BITRATE;

	case SortMode::MediaCopyright:
		return IDS_COLUMN_NAME_COPYRIGHT;

	case SortMode::MediaDuration:
		return IDS_COLUMN_NAME_DURATION;

	case SortMode::MediaProtected:
		return IDS_COLUMN_NAME_PROTECTED;

	case SortMode::MediaRating:
		return IDS_COLUMN_NAME_RATING;

	case SortMode::MediaAlbumArtist:
		return IDS_COLUMN_NAME_ALBUMARTIST;

	case SortMode::MediaAlbum:
		return IDS_COLUMN_NAME_ALBUM;

	case SortMode::MediaBeatsPerMinute:
		return IDS_COLUMN_NAME_BEATSPERMINUTE;

	case SortMode::MediaComposer:
		return IDS_COLUMN_NAME_COMPOSER;

	case SortMode::MediaConductor:
		return IDS_COLUMN_NAME_CONDUCTOR;

	case SortMode::MediaDirector:
		return IDS_COLUMN_NAME_DIRECTOR;

	case SortMode::MediaGenre:
		return IDS_COLUMN_NAME_GENRE;

	case SortMode::MediaLanguage:
		return IDS_COLUMN_NAME_LANGUAGE;

	case SortMode::MediaBroadcastDate:
		return IDS_COLUMN_NAME_BROADCASTDATE;

	case SortMode::MediaChannel:
		return IDS_COLUMN_NAME_CHANNEL;

	case SortMode::MediaStationName:
		return IDS_COLUMN_NAME_STATIONNAME;

	case SortMode::MediaMood:
		return IDS_COLUMN_NAME_MOOD;

	case SortMode::MediaParentalRating:
		return IDS_COLUMN_NAME_PARENTALRATING;

	case SortMode::MediaParentalRatingReason:
		return IDS_COLUMN_NAME_PARENTALRATINGREASON;

	case SortMode::MediaPeriod:
		return IDS_COLUMN_NAME_PERIOD;

	case SortMode::MediaProducer:
		return IDS_COLUMN_NAME_PRODUCER;

	case SortMode::MediaPublisher:
		return IDS_COLUMN_NAME_PUBLISHER;

	case SortMode::MediaWriter:
		return IDS_COLUMN_NAME_WRITER;

	case SortMode::MediaYear:
		return IDS_COLUMN_NAME_YEAR;
	}

	LOG(FATAL) << "Invalid SortMode value";
}

std::optional<UINT> MaybeGetSortModeMenuHelpStringId(SortMode sortMode)
{
	switch (sortMode)
	{
	case SortMode::Name:
		return IDS_SORTBY_NAME_HELP_TEXT;

	case SortMode::DateModified:
		return IDS_SORTBY_DATEMODIFIED_HELP_TEXT;

	case SortMode::Size:
		return IDS_SORTBY_SIZE_HELP_TEXT;

	case SortMode::Type:
		return IDS_SORTBY_TYPE_HELP_TEXT;

	case SortMode::TotalSize:
		return IDS_SORTBY_TOTALSIZE_HELP_TEXT;

	case SortMode::FreeSpace:
		return IDS_SORTBY_FREESPACE_HELP_TEXT;

	case SortMode::Comments:
		return IDS_SORTBY_COMMENTS_HELP_TEXT;

	case SortMode::DateDeleted:
		return IDS_SORTBY_DATEDELETED_HELP_TEXT;

	case SortMode::OriginalLocation:
		return IDS_SORTBY_ORIGINALLOCATION_HELP_TEXT;

	case SortMode::Attributes:
		return IDS_SORTBY_ATTRIBUTES_HELP_TEXT;

	case SortMode::RealSize:
		return IDS_SORTBY_REALSIZE_HELP_TEXT;

	case SortMode::ShortName:
		return IDS_SORTBY_SHORTNAME_HELP_TEXT;

	case SortMode::Owner:
		return IDS_SORTBY_OWNER_HELP_TEXT;

	case SortMode::ProductName:
		return IDS_SORTBY_PRODUCTNAME_HELP_TEXT;

	case SortMode::Company:
		return IDS_SORTBY_COMPANY_HELP_TEXT;

	case SortMode::Description:
		return IDS_SORTBY_DESCRIPTION_HELP_TEXT;

	case SortMode::FileVersion:
		return IDS_SORTBY_FILEVERSION_HELP_TEXT;

	case SortMode::ProductVersion:
		return IDS_SORTBY_PRODUCTVERSION_HELP_TEXT;

	case SortMode::ShortcutTo:
		return IDS_SORTBY_SHORTCUTTO_HELP_TEXT;

	case SortMode::HardLinks:
		return IDS_SORTBY_HARDLINKS_HELP_TEXT;

	case SortMode::Extension:
		return IDS_SORTBY_EXTENSION_HELP_TEXT;

	case SortMode::Created:
		return IDS_SORTBY_CREATED_HELP_TEXT;

	case SortMode::Accessed:
		return IDS_SORTBY_ACCESSED_HELP_TEXT;

	case SortMode::Title:
		return IDS_SORTBY_TITLE_HELP_TEXT;

	case SortMode::Subject:
		return IDS_SORTBY_SUBJECT_HELP_TEXT;

	case SortMode::Authors:
		return IDS_SORTBY_AUTHORS_HELP_TEXT;

	case SortMode::Keywords:
		return IDS_SORTBY_KEYWORDS_HELP_TEXT;

	case SortMode::CameraModel:
		return IDS_SORTBY_CAMERAMODEL_HELP_TEXT;

	case SortMode::DateTaken:
		return IDS_SORTBY_DATETAKEN_HELP_TEXT;

	case SortMode::Width:
		return IDS_SORTBY_WIDTH_HELP_TEXT;

	case SortMode::Height:
		return IDS_SORTBY_HEIGHT_HELP_TEXT;

	case SortMode::VirtualComments:
		return IDS_SORTBY_VIRTUALCOMMENTS_HELP_TEXT;

	case SortMode::FileSystem:
		return IDS_SORTBY_FILESYSTEM_HELP_TEXT;

	case SortMode::NumPrinterDocuments:
		return IDS_SORTBY_NUMPRINTERDOCUMENTS_HELP_TEXT;

	case SortMode::PrinterStatus:
		return IDS_SORTBY_PRINTERSTATUS_HELP_TEXT;

	case SortMode::PrinterComments:
		return IDS_SORTBY_PRINTERCOMMENTS_HELP_TEXT;

	case SortMode::PrinterLocation:
		return IDS_SORTBY_PRINTERLOCATION_HELP_TEXT;

	case SortMode::NetworkAdapterStatus:
		return IDS_SORTBY_NETWORKADAPTER_STATUS_HELP_TEXT;

	// These items don't currently have any help text.
	case SortMode::MediaBitrate:
	case SortMode::MediaCopyright:
	case SortMode::MediaDuration:
	case SortMode::MediaProtected:
	case SortMode::MediaRating:
	case SortMode::MediaAlbumArtist:
	case SortMode::MediaAlbum:
	case SortMode::MediaBeatsPerMinute:
	case SortMode::MediaComposer:
	case SortMode::MediaConductor:
	case SortMode::MediaDirector:
	case SortMode::MediaGenre:
	case SortMode::MediaLanguage:
	case SortMode::MediaBroadcastDate:
	case SortMode::MediaChannel:
	case SortMode::MediaStationName:
	case SortMode::MediaMood:
	case SortMode::MediaParentalRating:
	case SortMode::MediaParentalRatingReason:
	case SortMode::MediaPeriod:
	case SortMode::MediaProducer:
	case SortMode::MediaPublisher:
	case SortMode::MediaWriter:
	case SortMode::MediaYear:
		return std::nullopt;
	}

	LOG(FATAL) << "Invalid SortMode value";
}
