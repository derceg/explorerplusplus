// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct Column_t
{
	unsigned int id;
	BOOL bChecked;
	int iWidth;
};

struct ColumnOld_t
{
	unsigned int id;
	BOOL bChecked;
};

enum COLUMNS
{
	CM_NAME = 1,
	CM_TYPE = 2,
	CM_SIZE = 3,
	CM_DATEMODIFIED = 4,
	CM_ATTRIBUTES = 5,
	CM_REALSIZE = 6,
	CM_SHORTNAME = 7,
	CM_OWNER = 8,

	/*File version information.*/
	CM_PRODUCTNAME = 9,
	CM_COMPANY = 10,
	CM_DESCRIPTION = 11,
	CM_FILEVERSION = 12,
	CM_PRODUCTVERSION = 13,

	CM_SHORTCUTTO = 14,
	CM_HARDLINKS = 15,
	CM_EXTENSION = 16,
	CM_CREATED = 17,
	CM_ACCESSED = 18,

	/* File summary information. */
	CM_TITLE = 19,
	CM_SUBJECT = 20,
	CM_AUTHORS = 21,
	CM_KEYWORDS = 22,
	CM_COMMENT = 23,

	/* Photo data. */
	CM_CAMERAMODEL = 24,
	CM_DATETAKEN = 25,
	CM_WIDTH = 26,
	CM_HEIGHT = 27,

	/* Control panel. */
	CM_VIRTUALCOMMENTS = 28,

	/* My Computer. */
	CM_TOTALSIZE = 29,
	CM_FREESPACE = 30,
	CM_FILESYSTEM = 31,

	/* Recycle Bin. */
	CM_ORIGINALLOCATION = 33,
	CM_DATEDELETED = 34,

	/* Printer columns. */
	CM_NUMPRINTERDOCUMENTS = 35,
	CM_PRINTERSTATUS = 36,
	CM_PRINTERCOMMENTS = 37,
	CM_PRINTERLOCATION = 38,

	/* Network connections columns. */
	CM_NETWORKADAPTER_STATUS = 39,

	/* Media metadata. */
	CM_MEDIA_BITRATE = 40,
	CM_MEDIA_COPYRIGHT = 41,
	CM_MEDIA_DURATION = 42,
	CM_MEDIA_PROTECTED = 43,
	CM_MEDIA_RATING = 44,
	CM_MEDIA_ALBUMARTIST = 45,
	CM_MEDIA_ALBUM = 46,
	CM_MEDIA_BEATSPERMINUTE = 47,
	CM_MEDIA_COMPOSER = 48,
	CM_MEDIA_CONDUCTOR = 49,
	CM_MEDIA_DIRECTOR = 50,
	CM_MEDIA_GENRE = 51,
	CM_MEDIA_LANGUAGE = 52,
	CM_MEDIA_BROADCASTDATE = 53,
	CM_MEDIA_CHANNEL = 54,
	CM_MEDIA_STATIONNAME = 55,
	CM_MEDIA_MOOD = 56,
	CM_MEDIA_PARENTALRATING = 57,
	CM_MEDIA_PARENTALRATINGREASON = 58,
	CM_MEDIA_PERIOD = 59,
	CM_MEDIA_PRODUCER = 60,
	CM_MEDIA_PUBLISHER = 61,
	CM_MEDIA_WRITER = 62,
	CM_MEDIA_YEAR = 63,

	/* Printer columns. */
	CM_PRINTERMODEL = 64
};