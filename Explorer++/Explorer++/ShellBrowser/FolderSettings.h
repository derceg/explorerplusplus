// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Columns.h"
#include "DefaultColumns.h"
#include "SortModes.h"
#include "ValueWrapper.h"
#include "ViewModes.h"
#include "../Helper/StringHelper.h"

static const int DEFAULT_LISTVIEW_HOVER_TIME = 500;

struct FolderColumns
{
	std::vector<Column_t> realFolderColumns = std::vector<Column_t>(
		std::begin(REAL_FOLDER_DEFAULT_COLUMNS), std::end(REAL_FOLDER_DEFAULT_COLUMNS));
	std::vector<Column_t> myComputerColumns = std::vector<Column_t>(
		std::begin(MY_COMPUTER_DEFAULT_COLUMNS), std::end(MY_COMPUTER_DEFAULT_COLUMNS));
	std::vector<Column_t> controlPanelColumns = std::vector<Column_t>(
		std::begin(CONTROL_PANEL_DEFAULT_COLUMNS), std::end(CONTROL_PANEL_DEFAULT_COLUMNS));
	std::vector<Column_t> recycleBinColumns = std::vector<Column_t>(
		std::begin(RECYCLE_BIN_DEFAULT_COLUMNS), std::end(RECYCLE_BIN_DEFAULT_COLUMNS));
	std::vector<Column_t> printersColumns = std::vector<Column_t>(
		std::begin(PRINTERS_DEFAULT_COLUMNS), std::end(PRINTERS_DEFAULT_COLUMNS));
	std::vector<Column_t> networkConnectionsColumns =
		std::vector<Column_t>(std::begin(NETWORK_CONNECTIONS_DEFAULT_COLUMNS),
			std::end(NETWORK_CONNECTIONS_DEFAULT_COLUMNS));
	std::vector<Column_t> myNetworkPlacesColumns = std::vector<Column_t>(
		std::begin(MY_NETWORK_PLACES_DEFAULT_COLUMNS), std::end(MY_NETWORK_PLACES_DEFAULT_COLUMNS));

	// This is only used in tests.
	bool operator==(const FolderColumns &) const = default;
};

struct GlobalFolderSettings
{
	bool showExtensions = true;
	bool showFriendlyDates = true;
	bool showFolderSizes = false;
	bool disableFolderSizesNetworkRemovable = false;
	bool hideSystemFiles = false;
	bool hideLinkExtension = false;
	bool insertSorted = true;
	ValueWrapper<bool> showGridlines = true;
	bool forceSize = false;
	SizeDisplayFormat sizeDisplayFormat = SizeDisplayFormat::Bytes;
	ValueWrapper<bool> oneClickActivate = false;
	ValueWrapper<UINT> oneClickActivateHoverTime = DEFAULT_LISTVIEW_HOVER_TIME;
	bool displayMixedFilesAndFolders = false;
	bool useNaturalSortOrder = true;

	FolderColumns folderColumns;

	// This is only used in tests.
	bool operator==(const GlobalFolderSettings &) const = default;
};

struct FolderSettings
{
	SortMode sortMode = SortMode::Name;
	SortMode groupMode = SortMode::Name;
	ViewMode viewMode = ViewMode::Icons;
	bool autoArrange = true;
	SortDirection sortDirection = SortDirection::Ascending;
	SortDirection groupSortDirection = SortDirection::Ascending;
	bool showInGroups = false;
	bool showHidden = true;

	bool applyFilter = false;
	bool filterCaseSensitive = false;
	std::wstring filter;

	// This is only used in tests.
	bool operator==(const FolderSettings &) const = default;
};
