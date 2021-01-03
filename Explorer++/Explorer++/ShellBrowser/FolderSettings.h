// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Columns.h"
#include "SortModes.h"
#include "ViewModes.h"
#include "../Helper/StringHelper.h"

struct FolderColumns
{
	std::vector<Column_t> realFolderColumns;
	std::vector<Column_t> myComputerColumns;
	std::vector<Column_t> controlPanelColumns;
	std::vector<Column_t> recycleBinColumns;
	std::vector<Column_t> printersColumns;
	std::vector<Column_t> networkConnectionsColumns;
	std::vector<Column_t> myNetworkPlacesColumns;
};

struct GlobalFolderSettings
{
	BOOL showExtensions;
	BOOL showFriendlyDates;
	BOOL showFolderSizes;
	BOOL disableFolderSizesNetworkRemovable;
	BOOL hideSystemFiles;
	BOOL hideLinkExtension;
	BOOL insertSorted;
	BOOL showGridlines;
	BOOL forceSize;
	SizeDisplayFormat sizeDisplayFormat;
	BOOL oneClickActivate;
	UINT oneClickActivateHoverTime;
	BOOL displayMixedFilesAndFolders;
	BOOL useNaturalSortOrder;

	FolderColumns folderColumns;
};

struct FolderSettings
{
	SortMode sortMode;
	ViewMode viewMode;
	BOOL autoArrange;
	BOOL sortAscending;
	BOOL showInGroups;
	BOOL showHidden;

	BOOL applyFilter;
	BOOL filterCaseSensitive;
	std::wstring filter;
};