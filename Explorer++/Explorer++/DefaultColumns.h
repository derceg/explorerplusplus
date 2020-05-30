// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/Columns.h"

static const int DEFAULT_COLUMN_WIDTH = 150;

static const Column_t REAL_FOLDER_DEFAULT_COLUMNS[] =
{{ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Type, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Size, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::DateModified, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Attributes, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::RealSize, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::ShortName, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Owner, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::ProductName, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Company, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Description, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::FileVersion, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::ProductVersion, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::ShortcutTo, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::HardLinks, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Extension, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Created, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Accessed, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Title, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Subject, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Authors, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Keywords, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Comment, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::CameraModel, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::DateTaken, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Width, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Height, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaBitrate, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaCopyright, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaDuration, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaProtected, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaRating, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaAlbumArtist, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaAlbum, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaBeatsPerMinute, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaComposer, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaConductor, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaDirector, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaGenre, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaLanguage, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaBroadcastDate, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaChannel, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaStationName, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaMood, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaParentalRating, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaParentalRatingReason, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaPeriod, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaProducer, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaPublisher, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaWriter, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::MediaYear, FALSE, DEFAULT_COLUMN_WIDTH}};

static const Column_t MY_COMPUTER_DEFAULT_COLUMNS[] =
{{ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Type, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::TotalSize, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::FreeSpace, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::VirtualComments, FALSE, DEFAULT_COLUMN_WIDTH},
{ColumnType::FileSystem, FALSE, DEFAULT_COLUMN_WIDTH}};

static const Column_t CONTROL_PANEL_DEFAULT_COLUMNS[] =
{{ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::VirtualComments, TRUE, DEFAULT_COLUMN_WIDTH}};

static const Column_t RECYCLE_BIN_DEFAULT_COLUMNS[] =
{{ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::OriginalLocation, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::DateDeleted, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Size, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Type, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::DateModified, TRUE, DEFAULT_COLUMN_WIDTH}};

static const Column_t PRINTERS_DEFAULT_COLUMNS[] =
{{ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::PrinterNumDocuments, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::PrinterStatus, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::PrinterComments, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::PrinterLocation, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::PrinterModel, TRUE, DEFAULT_COLUMN_WIDTH}};

static const Column_t NETWORK_CONNECTIONS_DEFAULT_COLUMNS[] =
{{ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Type, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::NetworkAdaptorStatus, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::Owner, TRUE, DEFAULT_COLUMN_WIDTH}};

static const Column_t MY_NETWORK_PLACES_DEFAULT_COLUMNS[] =
{{ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH},
{ColumnType::VirtualComments, TRUE, DEFAULT_COLUMN_WIDTH}};