// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/StringHelper.h"

struct GlobalFolderSettings
{
	BOOL showExtensions;
	BOOL showFriendlyDates;
	BOOL showFolderSizes;
	BOOL disableFolderSizesNetworkRemovable;
	BOOL hideSystemFiles;
	BOOL hideLinkExtension;
	BOOL showGridlines;
	BOOL forceSize;
	SizeDisplayFormat_t sizeDisplayFormat;
};

struct FolderSettings
{
	BOOL autoArrange;
	BOOL sortAscending;
	BOOL showInGroups;
	BOOL showHidden;
};