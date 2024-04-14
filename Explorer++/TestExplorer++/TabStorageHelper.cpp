// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "TabStorageHelper.h"
#include "ShellHelper.h"
#include "TabStorage.h"

bool operator==(const TabStorageData &first, const TabStorageData &second)
{
	bool areDirectoriesEquivalent = false;

	if (first.pidl.HasValue() && second.pidl.HasValue())
	{
		areDirectoriesEquivalent = (first.pidl == second.pidl);
	}
	else
	{
		areDirectoriesEquivalent = (first.directory == second.directory);
	}

	return areDirectoriesEquivalent && first.tabSettings == second.tabSettings
		&& first.folderSettings == second.folderSettings && first.columns == second.columns;
}

void BuildTabStorageLoadSaveReference(std::vector<TabStorageData> &outputTabs)
{
	TabStorageData tab1;

	tab1.directory = L"C:\\";
	tab1.pidl = CreateSimplePidlForTest(tab1.directory);

	tab1.tabSettings.name = L"C drive";

	tab1.folderSettings.sortMode = SortMode::Size;
	tab1.folderSettings.groupMode = SortMode::Name;
	tab1.folderSettings.viewMode = ViewMode::Icons;
	tab1.folderSettings.autoArrange = true;
	tab1.folderSettings.sortDirection = SortDirection::Descending;
	tab1.folderSettings.groupSortDirection = SortDirection::Ascending;
	tab1.folderSettings.showInGroups = false;
	tab1.folderSettings.showHidden = true;
	tab1.folderSettings.applyFilter = false;
	tab1.folderSettings.filterCaseSensitive = false;

	tab1.columns.realFolderColumns = { { ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::Type, FALSE, 200 } };
	tab1.columns.myComputerColumns = { { ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::TotalSize, TRUE, 800 } };
	tab1.columns.controlPanelColumns = { { ColumnType::VirtualComments, TRUE, 685 },
		{ ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH } };
	tab1.columns.recycleBinColumns = { { ColumnType::Size, FALSE, 363 },
		{ ColumnType::DateModified, TRUE, 212 } };
	tab1.columns.printersColumns = { { ColumnType::PrinterComments, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::PrinterLocation, FALSE, DEFAULT_COLUMN_WIDTH } };
	tab1.columns.networkConnectionsColumns = { { ColumnType::NetworkAdaptorStatus, TRUE, 88 },
		{ ColumnType::Type, TRUE, DEFAULT_COLUMN_WIDTH } };
	tab1.columns.myNetworkPlacesColumns = { { ColumnType::Name, TRUE, DEFAULT_COLUMN_WIDTH },
		{ ColumnType::VirtualComments, FALSE, DEFAULT_COLUMN_WIDTH } };

	outputTabs.push_back(tab1);

	// Note that since no columns appear in the saved data for this tab and the third tab, there's
	// no need to set any columns here. If there are no columns in the saved data, the default
	// columns will be used.
	TabStorageData tab2;

	tab2.directory = L"C:\\Users";
	tab2.pidl = CreateSimplePidlForTest(tab2.directory);

	tab2.tabSettings.name = L"";
	tab2.tabSettings.lockState = Tab::LockState::Locked;

	tab2.folderSettings.sortMode = SortMode::DateModified;
	tab2.folderSettings.groupMode = SortMode::Size;
	tab2.folderSettings.viewMode = ViewMode::SmallIcons;
	tab2.folderSettings.autoArrange = false;
	tab2.folderSettings.sortDirection = SortDirection::Ascending;
	tab2.folderSettings.groupSortDirection = SortDirection::Descending;
	tab2.folderSettings.showInGroups = true;
	tab2.folderSettings.showHidden = false;
	tab2.folderSettings.applyFilter = true;
	tab2.folderSettings.filterCaseSensitive = false;
	tab2.folderSettings.filter = L"*.exe";

	outputTabs.push_back(tab2);

	TabStorageData tab3;

	tab3.directory = L"C:\\Users\\Default";
	tab3.pidl = CreateSimplePidlForTest(tab3.directory);

	tab3.tabSettings.name = L"";
	tab3.tabSettings.lockState = Tab::LockState::AddressLocked;

	tab3.folderSettings.sortMode = SortMode::Created;
	tab3.folderSettings.groupMode = SortMode::Name;
	tab3.folderSettings.viewMode = ViewMode::Details;
	tab3.folderSettings.autoArrange = true;
	tab3.folderSettings.sortDirection = SortDirection::Descending;
	tab3.folderSettings.groupSortDirection = SortDirection::Ascending;
	tab3.folderSettings.showInGroups = false;
	tab3.folderSettings.showHidden = true;
	tab3.folderSettings.applyFilter = true;
	tab3.folderSettings.filterCaseSensitive = true;
	tab3.folderSettings.filter = L"*.txt";

	outputTabs.push_back(tab3);
}
