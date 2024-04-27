// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ColumnStorageTestHelper.h"
#include "ShellBrowser/FolderSettings.h"

FolderColumns BuildFolderColumnsLoadSaveReference()
{
	FolderColumns folderColumns;

	// clang-format off
	folderColumns.realFolderColumns = {
		{ ColumnType::Name, true, 423 },
		{ ColumnType::Type, true, 150 },
		{ ColumnType::Size, true, 300 },
		{ ColumnType::DateModified, false, 150 },
		{ ColumnType::Attributes, false, 150 },
		{ ColumnType::RealSize, false, 150 },
		{ ColumnType::ShortName, false, 150 },
		{ ColumnType::Owner, false, 150 },
		{ ColumnType::ProductName, false, 150 },
		{ ColumnType::Company, true, 150 },
		{ ColumnType::Description, false, 150 },
		{ ColumnType::FileVersion, false, 150 },
		{ ColumnType::ProductVersion, false, 150 },
		{ ColumnType::ShortcutTo, false, 150 },
		{ ColumnType::HardLinks, true, 150 },
		{ ColumnType::Extension, false, 150 },
		{ ColumnType::Created, false, 150 },
		{ ColumnType::Accessed, false, 150 },
		{ ColumnType::Title, false, 150 },
		{ ColumnType::Subject, false, 150 },
		{ ColumnType::Authors, false, 150 },
		{ ColumnType::Keywords, false, 150 },
		{ ColumnType::Comment, false, 150 },
		{ ColumnType::CameraModel, false, 150 },
		{ ColumnType::DateTaken, false, 150 },
		{ ColumnType::Width, false, 150 },
		{ ColumnType::Height, false, 150 },
		{ ColumnType::MediaBitrate, false, 150 },
		{ ColumnType::MediaCopyright, false, 150 },
		{ ColumnType::MediaDuration, false, 150 },
		{ ColumnType::MediaProtected, false, 150 },
		{ ColumnType::MediaRating, false, 150 },
		{ ColumnType::MediaAlbumArtist, false, 150 },
		{ ColumnType::MediaAlbum, false, 150 },
		{ ColumnType::MediaBeatsPerMinute, false, 150 },
		{ ColumnType::MediaComposer, false, 150 },
		{ ColumnType::MediaConductor, false, 150 },
		{ ColumnType::MediaDirector, false, 150 },
		{ ColumnType::MediaGenre, false, 539 },
		{ ColumnType::MediaLanguage, false, 150 },
		{ ColumnType::MediaBroadcastDate, false, 150 },
		{ ColumnType::MediaChannel, false, 150 },
		{ ColumnType::MediaStationName, false, 150 },
		{ ColumnType::MediaMood, false, 150 },
		{ ColumnType::MediaParentalRating, false, 827 },
		{ ColumnType::MediaParentalRatingReason, false, 150 },
		{ ColumnType::MediaPeriod, false, 150 },
		{ ColumnType::MediaProducer, false, 150 },
		{ ColumnType::MediaPublisher, false, 150 },
		{ ColumnType::MediaWriter, false, 150 },
		{ ColumnType::MediaYear, false, 150 }
	};

	folderColumns.myComputerColumns = {
		{ ColumnType::Name, true, 150 },
		{ ColumnType::Type, false, 150 },
		{ ColumnType::TotalSize, true, 150 },
		{ ColumnType::FreeSpace, true, 150 },
		{ ColumnType::FileSystem, true, 80 },
		{ ColumnType::VirtualComments, true, 111 }
	};

	folderColumns.controlPanelColumns = {
		{ ColumnType::Name, true, 502 },
		{ ColumnType::VirtualComments, false, 150 }
	};

	folderColumns.recycleBinColumns = {
		{ ColumnType::Name, true, 150 },
		{ ColumnType::DateDeleted, true, 150 },
		{ ColumnType::OriginalLocation, true, 687 },
		{ ColumnType::Size, true, 150 },
		{ ColumnType::Type, false, 144 },
		{ ColumnType::DateModified, false, 150 }
	};

	folderColumns.printersColumns = {
		{ ColumnType::Name, true, 150 },
		{ ColumnType::PrinterNumDocuments, false, 150 },
		{ ColumnType::PrinterComments, true, 216 },
		{ ColumnType::PrinterStatus, true, 150 },
		{ ColumnType::PrinterLocation, true, 384 },
		{ ColumnType::PrinterModel, true, 150 }
	};

	folderColumns.networkConnectionsColumns = {
		{ ColumnType::Name, true, 718 },
		{ ColumnType::Type, true, 150 },
		{ ColumnType::NetworkAdaptorStatus, false, 206 },
		{ ColumnType::Owner, true, 150 }
	};

	folderColumns.myNetworkPlacesColumns = {
		{ ColumnType::VirtualComments, true, 88 },
		{ ColumnType::Name, true, 912 }
	};
	// clang-format on

	return folderColumns;
}
