// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BetterEnumsWrapper.h"

// clang-format off
BETTER_ENUM(ColumnType, unsigned int,
	Name = 1,
	Type = 2,
	Size = 3,
	DateModified = 4,
	Attributes = 5,
	RealSize = 6,
	ShortName = 7,
	Owner = 8,

	/*File version information.*/
	ProductName = 9,
	Company = 10,
	Description = 11,
	FileVersion = 12,
	ProductVersion = 13,

	ShortcutTo = 14,
	HardLinks = 15,
	Extension = 16,
	Created = 17,
	Accessed = 18,

	/* File summary information. */
	Title = 19,
	Subject = 20,
	Authors = 21,
	Keywords = 22,
	Comment = 23,

	/* Photo data. */
	CameraModel = 24,
	DateTaken = 25,
	Width = 26,
	Height = 27,

	/* Control panel. */
	VirtualComments = 28,

	/* My Computer. */
	TotalSize = 29,
	FreeSpace = 30,
	FileSystem = 31,

	/* Recycle Bin. */
	OriginalLocation = 33,
	DateDeleted = 34,

	/* Printer columns. */
	PrinterNumDocuments = 35,
	PrinterStatus = 36,
	PrinterComments = 37,
	PrinterLocation = 38,

	/* Network connections columns. */
	NetworkAdaptorStatus = 39,

	/* Media metadata. */
	MediaBitrate = 40,
	MediaCopyright = 41,
	MediaDuration = 42,
	MediaProtected = 43,
	MediaRating = 44,
	MediaAlbumArtist = 45,
	MediaAlbum = 46,
	MediaBeatsPerMinute = 47,
	MediaComposer = 48,
	MediaConductor = 49,
	MediaDirector = 50,
	MediaGenre = 51,
	MediaLanguage = 52,
	MediaBroadcastDate = 53,
	MediaChannel = 54,
	MediaStationName = 55,
	MediaMood = 56,
	MediaParentalRating = 57,
	MediaParentalRatingReason = 58,
	MediaPeriod = 59,
	MediaProducer = 60,
	MediaPublisher = 61,
	MediaWriter = 62,
	MediaYear = 63,

	/* Printer columns. */
	PrinterModel = 64
)
// clang-format on

struct Column_t
{
	ColumnType type;
	BOOL checked;
	int width;

	// This is only used in tests.
	bool operator==(const Column_t &) const = default;
};
