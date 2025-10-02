// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BetterEnumsWrapper.h"

// These are used for both sorting and grouping. For example, it's possible to both sort a folder by
// name and group a folder by name.
// clang-format off
BETTER_ENUM(SortMode, int,
	Name = 1,
	DateModified = 2,
	Size = 3,
	Type = 4,
	TotalSize = 5,
	FreeSpace = 6,
	Comments = 7,
	DateDeleted = 8,
	OriginalLocation = 9,
	Attributes = 10,
	RealSize = 11,
	ShortName = 12,
	Owner = 13,

	ProductName = 14,
	Company = 15,
	Description = 16,
	FileVersion = 17,
	ProductVersion = 18,

	ShortcutTo = 19,
	HardLinks = 20,
	Extension = 21,
	Created = 22,
	Accessed = 23,

	Title = 24,
	Subject = 25,
	Authors = 26,
	Keywords = 27,

	CameraModel = 29,
	DateTaken = 30,
	Width = 31,
	Height = 32,

	VirtualComments = 33,

	FileSystem = 34,

	NumPrinterDocuments = 36,
	PrinterStatus = 37,
	PrinterComments = 38,
	PrinterLocation = 39,

	NetworkAdapterStatus = 40,

	MediaBitrate = 41,
	MediaCopyright = 42,
	MediaDuration = 43,
	MediaProtected = 44,
	MediaRating = 45,
	MediaAlbumArtist = 46,
	MediaAlbum = 47,
	MediaBeatsPerMinute = 48,
	MediaComposer = 49,
	MediaConductor = 50,
	MediaDirector = 51,
	MediaGenre = 52,
	MediaLanguage = 53,
	MediaBroadcastDate = 54,
	MediaChannel = 55,
	MediaStationName = 56,
	MediaMood = 57,
	MediaParentalRating = 58,
	MediaParentalRatingReason = 59,
	MediaPeriod = 60,
	MediaProducer = 61,
	MediaPublisher = 62,
	MediaWriter = 63,
	MediaYear = 64
)
// clang-format on
