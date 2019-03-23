// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ViewModes.h"
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
	BOOL oneClickActivate;
	UINT oneClickActivateHoverTime;
};

struct FolderSettings
{
	ViewMode viewMode;
	BOOL autoArrange;
	BOOL sortAscending;
	BOOL showInGroups;
	BOOL showHidden;

	BOOL applyFilter;
	BOOL filterCaseSensitive;
	std::wstring filter;
};