// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellBrowser/FolderSettings.h"
#include "../Helper/Macros.h"

namespace
{

enum class ColumnValidationType
{
	RealFolder,
	ControlPanel,
	MyComputer,
	RecycleBin,
	Printers,
	NetworkConnections,
	MyNetworkPlaces
};

void ValidateSingleColumnSet(ColumnValidationType columnValidationType,
	std::vector<Column_t> &columns);

}

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

namespace
{

void ValidateSingleColumnSet(ColumnValidationType columnValidationType,
	std::vector<Column_t> &columns)
{
	Column_t column;
	BOOL bFound = FALSE;
	const Column_t *pColumns = nullptr;
	unsigned int iTotalColumnSize = 0;
	unsigned int i = 0;

	switch (columnValidationType)
	{
	case ColumnValidationType::RealFolder:
		iTotalColumnSize = SIZEOF_ARRAY(REAL_FOLDER_DEFAULT_COLUMNS);
		pColumns = REAL_FOLDER_DEFAULT_COLUMNS;
		break;

	case ColumnValidationType::ControlPanel:
		iTotalColumnSize = SIZEOF_ARRAY(CONTROL_PANEL_DEFAULT_COLUMNS);
		pColumns = CONTROL_PANEL_DEFAULT_COLUMNS;
		break;

	case ColumnValidationType::MyComputer:
		iTotalColumnSize = SIZEOF_ARRAY(MY_COMPUTER_DEFAULT_COLUMNS);
		pColumns = MY_COMPUTER_DEFAULT_COLUMNS;
		break;

	case ColumnValidationType::RecycleBin:
		iTotalColumnSize = SIZEOF_ARRAY(RECYCLE_BIN_DEFAULT_COLUMNS);
		pColumns = RECYCLE_BIN_DEFAULT_COLUMNS;
		break;

	case ColumnValidationType::Printers:
		iTotalColumnSize = SIZEOF_ARRAY(PRINTERS_DEFAULT_COLUMNS);
		pColumns = PRINTERS_DEFAULT_COLUMNS;
		break;

	case ColumnValidationType::NetworkConnections:
		iTotalColumnSize = SIZEOF_ARRAY(NETWORK_CONNECTIONS_DEFAULT_COLUMNS);
		pColumns = NETWORK_CONNECTIONS_DEFAULT_COLUMNS;
		break;

	case ColumnValidationType::MyNetworkPlaces:
		iTotalColumnSize = SIZEOF_ARRAY(MY_NETWORK_PLACES_DEFAULT_COLUMNS);
		pColumns = MY_NETWORK_PLACES_DEFAULT_COLUMNS;
		break;
	}

	/* Check that every column that is supposed to appear
	is in the column list. */
	for (i = 0; i < iTotalColumnSize; i++)
	{
		bFound = FALSE;

		for (auto itr = columns.begin(); itr != columns.end(); itr++)
		{
			if (itr->type == pColumns[i].type)
			{
				bFound = TRUE;
				break;
			}
		}

		/* The column is not currently in the set. Add it in. */
		if (!bFound)
		{
			column.type = pColumns[i].type;
			column.checked = pColumns[i].checked;
			column.width = DEFAULT_COLUMN_WIDTH;
			columns.push_back(column);
		}
	}

	/* Check that no unknown column types appear in the column list. */
	for (auto itr = columns.cbegin(); itr != columns.cend();)
	{
		bFound = FALSE;

		for (i = 0; i < iTotalColumnSize; i++)
		{
			if (itr->type == pColumns[i].type)
			{
				bFound = TRUE;
				break;
			}
		}

		if (!bFound)
		{
			/* The column is not recognized in the set. Remove it. */
			itr = columns.erase(itr);
		}
		else
		{
			++itr;
		}
	}
}

}
