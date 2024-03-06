// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ColumnStorage.h"
#include "ShellBrowser/FolderSettings.h"
#include <span>

void ValidateColumns(FolderColumns &folderColumns)
{
	ValidateSingleColumnSet(ColumnValidationType::RealFolder, folderColumns.realFolderColumns);
	ValidateSingleColumnSet(ColumnValidationType::ControlPanel, folderColumns.controlPanelColumns);
	ValidateSingleColumnSet(ColumnValidationType::MyComputer, folderColumns.myComputerColumns);
	ValidateSingleColumnSet(ColumnValidationType::RecycleBin, folderColumns.recycleBinColumns);
	ValidateSingleColumnSet(ColumnValidationType::Printers, folderColumns.printersColumns);
	ValidateSingleColumnSet(ColumnValidationType::NetworkConnections,
		folderColumns.networkConnectionsColumns);
	ValidateSingleColumnSet(ColumnValidationType::MyNetworkPlaces,
		folderColumns.myNetworkPlacesColumns);
}

void ValidateSingleColumnSet(ColumnValidationType columnValidationType,
	std::vector<Column_t> &columns)
{
	std::span<const Column_t> defaultColumns;

	switch (columnValidationType)
	{
	case ColumnValidationType::RealFolder:
		defaultColumns = std::span(REAL_FOLDER_DEFAULT_COLUMNS);
		break;

	case ColumnValidationType::ControlPanel:
		defaultColumns = std::span(CONTROL_PANEL_DEFAULT_COLUMNS);
		break;

	case ColumnValidationType::MyComputer:
		defaultColumns = std::span(MY_COMPUTER_DEFAULT_COLUMNS);
		break;

	case ColumnValidationType::RecycleBin:
		defaultColumns = std::span(RECYCLE_BIN_DEFAULT_COLUMNS);
		break;

	case ColumnValidationType::Printers:
		defaultColumns = std::span(PRINTERS_DEFAULT_COLUMNS);
		break;

	case ColumnValidationType::NetworkConnections:
		defaultColumns = std::span(NETWORK_CONNECTIONS_DEFAULT_COLUMNS);
		break;

	case ColumnValidationType::MyNetworkPlaces:
		defaultColumns = std::span(MY_NETWORK_PLACES_DEFAULT_COLUMNS);
		break;
	}

	auto doesColumnExist =
		[]<class ColumnsContainer>(const ColumnsContainer &container, ColumnType columnType)
	{
		auto itr = std::find_if(container.begin(), container.end(),
			[columnType](const auto &column) { return column.type == columnType; });
		return itr != container.end();
	};

	// Add any columns that are missing.
	std::copy_if(defaultColumns.begin(), defaultColumns.end(), std::back_inserter(columns),
		[doesColumnExist, &columns](const auto &column)
		{ return !doesColumnExist(columns, column.type); });

	// Remove any unknown columns. Each column set has its own individual columns and not all
	// columns appear in all sets.
	std::erase_if(columns,
		[doesColumnExist, &defaultColumns](const auto &column)
		{ return !doesColumnExist(defaultColumns, column.type); });
}
